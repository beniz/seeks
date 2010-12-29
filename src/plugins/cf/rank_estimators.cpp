/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "rank_estimators.h"
#include "query_recommender.h"
#include "cf.h"
#include "cf_configuration.h"
#include "qprocess.h"
#include "query_capture.h"
#include "uri_capture.h"
#include "db_uri_record.h"
#include "mrf.h"
#include "urlmatch.h"
#include "miscutil.h"
#include "errlog.h"

#include <assert.h>
#include <math.h>
#include <iostream>

using lsh::qprocess;
using lsh::mrf;
using lsh::str_chain;
using lsh::stopwordlist;
using sp::urlmatch;
using sp::miscutil;
using sp::errlog;

namespace seeks_plugins
{

  /*- rank_estimator -*/
  void rank_estimator::fetch_user_db_record(const std::string &query,
					    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> &records)
  {
    static std::string qc_str = "query-capture";

    // strip query.
    std::string q = query_capture_element::no_command_query(query);

    // generate query fragments.
    hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
    qprocess::generate_query_hashes(q,0,5,features); //TODO: from configuration (5).

    // fetch records from the user DB.
    hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
    while (hit!=features.end())
      {
        std::string key_str = (*hit).second.to_rstring();
        db_record *dbr = seeks_proxy::_user_db->find_dbr(key_str,qc_str);
        if (dbr)
          records.insert(std::pair<const DHTKey*,db_record*>(new DHTKey((*hit).second),dbr));
	++hit;
      }
  }

  void rank_estimator::extract_queries(const std::string &query,
				       const std::string &lang,
				       const hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> &records,
				       hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata)
  {
    static std::string qc_str = "query-capture";
    
    str_chain strc_query(query_capture_element::no_command_query(query),0,true);
    strc_query = strc_query.rank_alpha();
    stopwordlist *swl = seeks_proxy::_lsh_config->get_wordlist(lang);
    
    // iterate records and gather queries and data.
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit;
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey>::const_iterator vit = records.begin();
    while (vit!=records.end())
      {
        db_query_record *dbqr = static_cast<db_query_record*>((*vit).second);
        hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator qit
        = dbqr->_related_queries.begin();
        while (qit!=dbqr->_related_queries.end())
          {
            query_data *qd = (*qit).second;
	    if (!query_recommender::select_query(strc_query,qd->_query,swl))
	      {
		++qit;
		continue;
	      }
	    
            if ((hit=qdata.find(qd->_query.c_str()))==qdata.end())
              {
                if (qd->_radius == 0) // contains the data.
		  {
		    query_data *nqd = new query_data(qd);
		    nqd->_record_key = new DHTKey(*(*vit).first); // mark data with record key.
		    qdata.insert(std::pair<const char*,query_data*>(qd->_query.c_str(),
								    nqd));
		  }
                else
                  {
                    // data are in lower radius records.
                    hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
                    qprocess::generate_query_hashes(qd->_query,0,0,features);

                    // XXX: when the query contains > 8 words there are many features generated
                    // for the same radius. The original query data can be fetched from any
                    // of the generated features, so we take the first one.
		    if (features.empty()) // this should never happen.
		      {
			++qit;
			continue;
		      }
		      
		    std::string key_str = (*features.begin()).second.to_rstring();
                    db_record *dbr_data = seeks_proxy::_user_db->find_dbr(key_str,qc_str);
		    if (!dbr_data) // this should never happen.
		      {
			++qit;
			continue;
		      }

                    db_query_record *dbqr_data = static_cast<db_query_record*>(dbr_data);
                    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator qit2
                    = dbqr_data->_related_queries.begin();
                    while (qit2!=dbqr_data->_related_queries.end())
                      {
                        if ((*qit2).second->_radius == 0
                            && (*qit2).second->_query == qd->_query)
                          {
                            query_data *dbqrc = new query_data((*qit2).second);
			    dbqrc->_radius = qd->_radius; // update radius relatively to original query.
			    dbqrc->_record_key = new DHTKey((*features.begin()).second); // mark the data with the record key.
			    qdata.insert(std::pair<const char*,query_data*>(dbqrc->_query.c_str(),
                                         dbqrc));
			    break;
                          }
                        ++qit2;
                      }
                    delete dbqr_data;
                  }
              }
            ++qit;
          }
        ++vit;
      }
  }

  /*- simple_re -*/
  simple_re::simple_re()
    :rank_estimator()
  {
  }

  simple_re::~simple_re()
  {
  }

  void simple_re::estimate_ranks(const std::string &query,
                                 const std::string &lang,
				 std::vector<search_snippet*> &snippets)
  {
    // fetch records from user DB.
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
    rank_estimator::fetch_user_db_record(query,records);

    //std::cerr << "[estimate_ranks]: number of fetched records: " << records.size() << std::endl;

    // extract queries.
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    rank_estimator::extract_queries(query,lang,records,qdata);

    //std::cerr << "[estimate_ranks]: number of extracted queries: " << qdata.size() << std::endl;

    // destroy records.
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey>::iterator rit = records.begin();
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey>::iterator crit;
    while (rit!=records.end())
      {
        db_record *dbr = (*rit).second;
	crit = rit;
        ++rit;
        delete dbr;
	delete (*crit).first;
      }

    // gather normalizing values.
    int i = 0;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
    = qdata.begin();
    float q_vurl_hits[qdata.size()];
    while (hit!=qdata.end())
      {
        q_vurl_hits[i++] = (*hit).second->vurls_total_hits();
        ++hit;
      }
    float sum_se_ranks = 0.0;
    std::vector<search_snippet*>::iterator vit = snippets.begin();
    while (vit!=snippets.end())
      {
        sum_se_ranks += (*vit)->_seeks_rank;
        ++vit;
      }

    // get number of captured URIs.
    uint64_t nuri = 0;
    if (cf::_uc_plugin)
      nuri = static_cast<uri_capture*>(cf::_uc_plugin)->_nr;

    // build up the filter.
    hash_map<const char*,const char*,hash<const char*>,eqstr> filter;
    hit = qdata.begin();
    while(hit!=qdata.end())
      {
	query_data *qd = (*hit).second;
	if (qd->_radius == 0)
	  {
	    if (qd->_visited_urls)
	      {
		hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::const_iterator vit
		  = qd->_visited_urls->begin();
		while(vit!=qd->_visited_urls->end())
		  {
		    vurl_data *vd = (*vit).second;
		    if (vd->_hits < 0)
		      {
			const char *furl = strdup(vd->_url.c_str());
			filter.insert(std::pair<const char*,const char*>(furl,furl));
		      }
		    ++vit;
		  }
	      }
	    break;
	  }
	++hit;
      }
    
    // estimate each URL's rank.
    int j = 0;
    size_t ns = snippets.size();
    float posteriors[ns];
    float sum_posteriors = 0.0;
    vit = snippets.begin();
    while (vit!=snippets.end())
      {
        std::string url = (*vit)->_url;
        	
	std::string host, path;
	query_capture::process_url(url,host,path);

        i = 0;
        posteriors[j] = 0.0;
        hit = qdata.begin();
        while (hit!=qdata.end())
          {
            float qpost = estimate_rank((*vit),filter.empty() ? NULL:&filter,ns,(*hit).second,
					q_vurl_hits[i++],url,host);
	    //qpost *= (*vit)->_seeks_rank / sum_se_ranks; // account for URL rank in results from search engines.
            qpost *= 1.0/static_cast<float>(log((*hit).second->_radius + 1.0) + 1.0); // account for distance to original query.
	    posteriors[j] += qpost; // boosting over similar queries.

            //std::cerr << "url: " << (*vit)->_url << " -- qpost: " << qpost << std::endl;
	    ++hit;
          }

        // estimate the url prior.
        float prior = 1.0;
	if (nuri != 0 && (*vit)->_doc_type != VIDEO_THUMB 
	    && (*vit)->_doc_type != TWEET && (*vit)->_doc_type != IMAGE) // not empty or type with not enought competition on domains.
          prior = estimate_prior((*vit),filter.empty() ? NULL:&filter,url,host,nuri);
	posteriors[j] *= prior;

        //std::cerr << "url: " << (*vit)->_url << " -- prior: " << prior << " -- posterior: " << posteriors[j] << std::endl;

        sum_posteriors += posteriors[j++];
        ++vit;
      }

    // wrapup.
    if (sum_posteriors > 0.0)
      {
        for (size_t k=0; k<ns; k++)
          {
            posteriors[k] /= sum_posteriors; // normalize.
            snippets.at(k)->_seeks_rank = posteriors[k];
            //std::cerr << "url: " << snippets.at(k)->_url << " -- seeks_rank: " << snippets.at(k)->_seeks_rank << std::endl;
          }
      }
    
    // destroy filter.
    hash_map<const char*,const char*,hash<const char*>,eqstr>::iterator fit, fit2;
    fit = filter.begin();
    while(fit!=filter.end())
      {
	fit2 = fit;
	++fit;
	free((char*)(*fit2).second);
      }

    // destroy query data.
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit2;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator chit;
    hit2 = qdata.begin();
    while (hit2!=qdata.end())
      {
        query_data *qd = (*hit2).second;
        chit = hit2;
        ++hit2;
        delete qd;
      }
  }

  float simple_re::estimate_rank(search_snippet *s, 
				 const hash_map<const char*,const char*,hash<const char*>,eqstr> *filter,
				 const int &ns,
                                 const query_data *qd,
                                 const float &total_hits,
                                 const std::string &surl,
                                 const std::string &host)
  {
    // URL and host.
    vurl_data *vd_url = qd->find_vurl(surl);
    vurl_data *vd_host = qd->find_vurl(host);
    
    // compute rank.
    return estimate_rank(s,filter,ns,vd_url,vd_host,total_hits);
  }

  float simple_re::estimate_rank(search_snippet *s, 
				 const hash_map<const char*,const char*,hash<const char*>,eqstr> *filter,
				 const int &ns,
				 const vurl_data *vd_url,
				 const vurl_data *vd_host,
				 const float &total_hits)
  {
    float posterior = 0.0;
    bool filtered = false;
    
    if (vd_url && filter)
      {
	hash_map<const char*,const char*,hash<const char*>,eqstr>::const_iterator hit
	  = filter->find(vd_url->_url.c_str());
	if (hit!=filter->end())
	  filtered = true;
      }
  
    if (!vd_url || vd_url->_hits < 0 || filtered)
      posterior = 1.0 / (log(total_hits + 1.0) + ns); // XXX: may replace ns with a less discriminative value.
    else
      {
        posterior = (log(vd_url->_hits + 1.0) + 1.0)/ (log(total_hits + 1.0) + ns);
        if (s)
	  {
	    s->_personalized = true;
	    s->_engine |= SE_SEEKS;
	    s->_meta_rank++;
	  }
      }
    
    // host.
    if (!vd_host || vd_host->_hits < 0 || !s || s->_doc_type == VIDEO_THUMB || s->_doc_type == TWEET
        || s->_doc_type == IMAGE // empty or type with not enough competition on domains.
	|| (filter && (filtered || filter->find(vd_host->_url.c_str())!=filter->end()))) // filter domain out if url was filtered out.
      filtered = true;
    else filtered = false;
    if (filtered)
      posterior *= cf_configuration::_config->_domain_name_weight
	           / (log(total_hits + 1.0) + ns); // XXX: may replace ns with a less discriminative value.
    else
      {
        posterior *= cf_configuration::_config->_domain_name_weight
                     * (log(vd_host->_hits + 1.0) + 1.0)
                     / (log(total_hits + 1.0) + ns); // with domain-name weight factor.
        if (s && (!vd_url || vd_url->_hits > 0))
	  s->_personalized = true;
      }
    //std::cerr << "posterior: " << posterior << std::endl;

    return posterior;
  }

  float simple_re::estimate_prior(search_snippet *s,
				  const hash_map<const char*,const char*,hash<const char*>,eqstr> *filter,
				  const std::string &surl,
                                  const std::string &host,
                                  const uint64_t &nuri)
  {
    static std::string uc_str = "uri-capture";
    bool filtered = false;
    float prior = 0.0;
    float furi = static_cast<float>(nuri);
    
    if (filter)
      {
	hash_map<const char*,const char*,hash<const char*>,eqstr>::const_iterator hit
	  = filter->find(surl.c_str());
	if (hit!=filter->end())
	  filtered = true;
      }
    db_record *dbr = seeks_proxy::_user_db->find_dbr(surl,uc_str);
    if (!dbr || filtered)
      prior = 1.0 / (log(furi + 1.0) + 1.0);
    else
      {
        db_uri_record *uc_dbr = static_cast<db_uri_record*>(dbr);
        prior = (log(uc_dbr->_hits + 1.0) + 1.0)/ (log(furi + 1.0) + 1.0);
        delete uc_dbr;
	if (s)
	  {
	    s->_personalized = true;
	    s->_engine |= SE_SEEKS;
	    s->_meta_rank++;
	  }
      }
    dbr = seeks_proxy::_user_db->find_dbr(host,uc_str);
    
    // XXX: code below is too aggressive and pushes other results too quickly
    //      below all the personalized results.
    /*if (!dbr)
      prior *= cf_configuration::_config->_domain_name_weight / (log(furi + 1.0) + 1.0);
    else
      {
        db_uri_record *uc_dbr = static_cast<db_uri_record*>(dbr);
        prior *= cf_configuration::_config->_domain_name_weight
	  * (log(uc_dbr->_hits + 1.0) + 1.0) / (log(furi + 1.0) + 1.0);
	delete uc_dbr;
	if (s)
	 s->_personalized = true;
	 }*/
    
    // code below treats domain recommendation as another search engines:
    // the meta rank of the snippet is incremented. This allows search engine
    // results to compete with the personalized results.
    // filter out domain if url was filtered out.
    if (dbr && (!filter || (!filtered && filter->find(host.c_str())==filter->end())))
      {
	if (s)
	  {
	    s->_meta_rank++;
	    s->_personalized = true;
	  }
	delete dbr;
      }
    //std::cerr << "prior: " << prior << std::endl;
    return prior;
  }

  void simple_re::recommend_urls(const std::string &query,
				 const std::string &lang,
				 hash_map<uint32_t,search_snippet*,id_hash_uint> &snippets)
  {
    // fetch records from user DB.
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
    rank_estimator::fetch_user_db_record(query,records);

    //std::cerr << "[recommend_urls]: number of fetched records: " << records.size() << std::endl;

    // extract queries.
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    rank_estimator::extract_queries(query,lang,records,qdata);

    //std::cerr << "[recommend_urls]: number of extracted queries: " << qdata.size() << std::endl;

    // destroy records.
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey>::iterator rit = records.begin();
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey>::iterator crit;
    while (rit!=records.end())
      {
        db_record *dbr = (*rit).second;
	crit = rit;
        ++rit;
        delete dbr;
	delete (*crit).first;
      }
    
    // gather normalizing values.
    int nvurls = 1.0;
    int i = 0;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator chit;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
      = qdata.begin();
    float q_vurl_hits[qdata.size()];
    while (hit!=qdata.end())
      {
	int vhits = (*hit).second->vurls_total_hits();
	q_vurl_hits[i++] = vhits;
	if (vhits > 0)
	  nvurls += (*hit).second->_visited_urls->size();
	++hit;
      }
    
    // get number of captured URIs.
    uint64_t nuri = 0;
    if (cf::_uc_plugin)
      nuri = static_cast<uri_capture*>(cf::_uc_plugin)->_nr;
    
    // look for URLs to recommend.
    hit = qdata.begin();
    while(hit!=qdata.end())
      {
	query_data *qd = (*hit).second;
	if (!qd->_visited_urls)
	  {
	    ++hit;
	    continue;
	  }

	// rank all URLs for this query.
	int i = 0;
	hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator sit;
	hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::const_iterator vit
	  = qd->_visited_urls->begin();
	while(vit!=qd->_visited_urls->end())
	  {
	    float posterior = 0.0;
	    vurl_data *vd = (*vit).second;
	    
	    // we do not recommend hosts.
	    if (miscutil::strncmpic(vd->_url.c_str(),"http://",7) == 0) // we do not consider https URLs for now, also avoids pure hosts.
	      {
		posterior = estimate_rank(NULL,NULL,nvurls,vd,NULL,q_vurl_hits[i]); // Note: no filter yet.
		
		// level them down according to query radius. 
		posterior *= 1.0 / static_cast<float>(log(qd->_radius + 1.0) + 1.0); // account for distance to original query.
		
		// update or create snippet.
		std::string surl = urlmatch::strip_url(vd->_url);
		uint32_t sid = mrf::mrf_single_feature(surl,""); //TODO: generic id generator.
		if ((sit = snippets.find(sid))!=snippets.end())
		  (*sit).second->_seeks_rank = posterior; // update.
		else
		  {
		    search_snippet *sp = new search_snippet();
		    sp->set_url(vd->_url);
		    sp->_meta_rank = 1;
		    sp->_seeks_rank = posterior;
		    snippets.insert(std::pair<uint32_t,search_snippet*>(sp->_id,sp));
		  }
	      }
	    
	    ++vit;
	  }
	++i;
	++hit;
      }
    
    // destroy query data.
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit2;
    hit2 = qdata.begin();
    while (hit2!=qdata.end())
      {
        query_data *qd = (*hit2).second;
        chit = hit2;
        ++hit2;
        delete qd;
      }
  }

  void simple_re::thumb_down_url(const std::string &query,
				 const std::string &lang,
				 const std::string &url)
  {
    static std::string qc_string = "query-capture";

    // fetch records from user DB.
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
    rank_estimator::fetch_user_db_record(query,records);
    
    // extract queries.
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    rank_estimator::extract_queries(query,lang,records,qdata);
    
    // destroy records.
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey>::iterator rit = records.begin();
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey>::iterator crit;
    while (rit!=records.end())
      {
        db_record *dbr = (*rit).second;
	crit = rit;
        ++rit;
        delete dbr;
	delete (*crit).first;
      }

    // preprocess url.
    std::string host, path;
    std::string purl = url;
    query_capture::process_url(purl,host,path);
    
    // get radius 0 record.
    std::string query_clean = query_capture_element::no_command_query(query);
    DHTKey *rkey = NULL;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=qdata.find(query_clean.c_str()))!=qdata.end())
      {
	query_data *qd = (*hit).second;
	rkey = qd->_record_key;
	
	hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator vit;
	if (qd->_visited_urls 
	    && (vit = qd->_visited_urls->find(url.c_str()))!=qd->_visited_urls->end())
	  {
	    // update url & domain count.
	    vurl_data *vd = (*vit).second;
	    
	    if (rkey)
	      {
		query_capture_element::remove_url(*rkey,query_clean,purl,host,
						  vd->_hits,qd->_radius,
						  qc_string);
	      }
	  }
	else // mark url down and downgrade / mark down domain.
	  {
	    if (rkey)
	      {
		query_capture_element::remove_url(*rkey,query_clean,purl,host,
                                                  1,qd->_radius,
                                                  qc_string);
	      }
	  }
      }
    else // we should never reach here.
      {
	errlog::log_error(LOG_LEVEL_ERROR,"thumb_down_url %s failed: cannot find query %s in records",
			  purl.c_str(),query_clean.c_str());
	
	// destroy query data.
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator chit;
	hit = qdata.begin();
	while (hit!=qdata.end())
	  {
	    query_data *qd = (*hit).second;
	    chit = hit;
	    ++hit;
	    delete qd;
	  }
	return;
      }	 
        
    // locate url and host in captured uris and remove / lower counters accordingly.
    plugin *ucpl = plugin_manager::get_plugin("uri-capture");
    if (ucpl)
      {
	uri_capture *uc = static_cast<uri_capture*>(ucpl);
	static_cast<uri_capture_element*>(uc->_interceptor_plugin)->remove_uri(purl,host);
      }

    // destroy query data.
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator chit;
    hit = qdata.begin();
    while (hit!=qdata.end())
      {
        query_data *qd = (*hit).second;
        chit = hit;
        ++hit;
        delete qd;
      }
  }

} /* end of namespace. */

