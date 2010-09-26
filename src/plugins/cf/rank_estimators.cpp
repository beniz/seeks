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
#include "cf_configuration.h"
#include "qprocess.h"
#include "query_capture.h"
#include "urlmatch.h"

#include <assert.h>
#include <iostream>

using lsh::qprocess;
using sp::urlmatch;

namespace seeks_plugins
{
   
   /*- rank_estimator -*/
   void rank_estimator::fetch_user_db_record(const std::string &query,
					     std::vector<db_record*> &records)
     {
	static std::string qc_str = "query-capture";
	
	// strip query.
	std::string q = query_capture_element::no_command_query(query);
	
	// generate query fragments.
	hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
	qprocess::generate_query_hashes(q,0,5,features); //TODO: from configuration (5).
	
	// fetch records from the user DB.
	hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
	while(hit!=features.end())
	  {
	     std::string key_str = (*hit).second.to_rstring();
	     db_record *dbr = seeks_proxy::_user_db->find_dbr(key_str,qc_str);
	     if (dbr)
	       records.push_back(dbr);
	     ++hit;
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
   
   void simple_re::extract_queries(const std::vector<db_record*> &records,
				   hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata)
     {
	static std::string qc_str = "query-capture";
	
	// iterate records and gather queries and data.
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit;
	std::vector<db_record*>::const_iterator vit = records.begin();
	while(vit!=records.end())
	  {
	     db_query_record *dbqr = static_cast<db_query_record*>((*vit));
	     hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator qit
	       = dbqr->_related_queries.begin();
	     while(qit!=dbqr->_related_queries.end())
	       {
		  query_data *qd = (*qit).second;
		  if ((hit=qdata.find(qd->_query.c_str()))==qdata.end())
		    {
		       if (qd->_radius == 0) // contains the data.
			 qdata.insert(std::pair<const char*,query_data*>(qd->_query.c_str(),
									 new query_data(qd)));
		       else
			 {
			    // data are in lower radius records.
			    hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
			    qprocess::generate_query_hashes(qd->_query,0,0,features);
			    
			    // XXX: when the query contains > 8 words there are many features generated
			    // for the same radius. The original query data can be fetched from any
			    // of the generated features, so we take the first one.
			    assert(features.size()>=1);
			    std::string key_str = (*features.begin()).second.to_rstring();
			    db_record *dbr_data = seeks_proxy::_user_db->find_dbr(key_str,qc_str);
			    assert(dbr_data != NULL); // beware.
			    db_query_record *dbqr_data = static_cast<db_query_record*>(dbr_data);
			    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator qit2
			      = dbqr_data->_related_queries.begin();
			    while(qit2!=dbqr_data->_related_queries.end())
			      {
				 if ((*qit2).second->_radius == 0
				     && (*qit2).second->_query == qd->_query)
				   {
				      query_data *dbqrc = new query_data((*qit2).second);
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
   
   void simple_re::estimate_ranks(const std::string &query,
				  std::vector<search_snippet*> &snippets)
     {
	// fetch records from user DB.
	std::vector<db_record*> records;
	fetch_user_db_record(query,records);
	
	std::cerr << "[estimate_ranks]: number of fetched records: " << records.size() << std::endl;
	
	// extract queries.
	hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
	extract_queries(records,qdata);
	
	std::cerr << "[estimate_ranks]: number of extracted queries: " << qdata.size() << std::endl;
	
	// destroy records.
	std::vector<db_record*>::iterator rit = records.begin();
	while(rit!=records.end())
	  {
	     db_record *dbr = (*rit);
	     rit = records.erase(rit);
	     delete dbr;
	  }
		
	// gather normalizing values.
	int i = 0;
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
	  = qdata.begin();
	float q_vurl_hits[qdata.size()];
	while(hit!=qdata.end())
	  {
	     q_vurl_hits[i++] = (*hit).second->vurls_total_hits();
	     ++hit;
	  }
	float sum_se_ranks = 0.0;
	std::vector<search_snippet*>::iterator vit = snippets.begin();
	while(vit!=snippets.end())
	  {
	     sum_se_ranks += (*vit)->_seeks_rank;
	     ++vit;
	  }
	
	// estimate each URL's rank.
	int j = 0;
	size_t ns = snippets.size();
	float posteriors[ns];
	float sum_posteriors = 0.0;
	vit = snippets.begin();
	while(vit!=snippets.end())
	  {
	     i = 0;
	     posteriors[j] = 0.0;
	     hit = qdata.begin();
	     while(hit!=qdata.end())
	       {
		  float qpost = estimate_rank((*vit),ns,(*hit).second,q_vurl_hits[i++]);
		  //qpost *= (*vit)->_seeks_rank / sum_se_ranks; // account for URL rank in results from search engines.
		  qpost *= 1.0/static_cast<float>(((*hit).second->_radius + 1.0)); // account for distance to original query.
		  posteriors[j] += qpost; // boosting over similar queries.
		  ++hit;
	       
		  //std::cerr << "url: " << (*vit)->_url << " -- qpost: " << qpost << std::endl;
	       }
	     //std::cerr << "url: " << (*vit)->_url << " -- posterior: " << posteriors[j] << std::endl;
	     sum_posteriors += posteriors[j++];
	     ++vit;
	  }
	
	// wrapup.
	if (sum_posteriors > 0.0)
	  {
	     for (size_t k=0;k<ns;k++)
	       {
		  posteriors[k] /= sum_posteriors; // normalize.
		  snippets.at(k)->_seeks_rank = posteriors[k];
	       
		  std::cerr << "url: " << snippets.at(k)->_url << " -- seeks_rank: " << snippets.at(k)->_seeks_rank << std::endl;
	       }
	  }
			
	// destroy query data.
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit2;
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator chit;
	hit2 = qdata.begin();
	while(hit2!=qdata.end())
	  {
	     query_data *qd = (*hit2).second;
	     chit = hit2;
	     ++hit2;
	     delete qd;
	  }
     }
   
   float simple_re::estimate_rank(search_snippet *s, const int &ns,
				  const query_data *qd, 
				  const float &total_hits)
     {
	float posterior = 0.0;
	
	// URL.
	std::string url = s->_url;
	std::transform(url.begin(),url.end(),url.begin(),tolower);
	std::string surl = urlmatch::strip_url(url);
	vurl_data *vd = qd->find_vurl(surl);
	if (!vd)
	  posterior =  1.0 / static_cast<float>(ns); // XXX: may replace ns with a less discriminative value.
	else posterior = (vd->_hits + 1.0) / (total_hits + ns);
	
	// host.
	std::string host, path;
	urlmatch::parse_url_host_and_path(url,host,path);
	host = urlmatch::strip_url(host);
	vd = qd->find_vurl(host);
	if (!vd)
	  posterior *= cf_configuration::_config->_domain_name_weight 
	  / static_cast<float>(ns); // XXX: may replace ns with a less discriminative value.
	else posterior *= cf_configuration::_config->_domain_name_weight*(vd->_hits + 1.0) 
	  / (total_hits + ns); // 0.7 is domain-name weight factor.
	
	//std::cerr << "posterior: " << posterior << std::endl;
	
	return posterior;
     }
      
} /* end of namespace. */
  
