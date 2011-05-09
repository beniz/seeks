/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010, 2011 Emmanuel Benazera, <ebenazer@seeks-project.info>
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
  cr_store rank_estimator::_store;
  sp_mutex_t rank_estimator::_est_mutex;

  rank_estimator::rank_estimator()
  {
    mutex_init(&_est_mutex);
  }

  // threaded call to personalization.
  void rank_estimator::peers_personalize(const std::string &query,
                                         const std::string &lang,
                                         const uint32_t &expansion,
                                         uint32_t &npeers,
                                         std::vector<search_snippet*> &snippets,
                                         std::multimap<double,std::string,std::less<double> > &related_queries,
                                         hash_map<uint32_t,search_snippet*,id_hash_uint> &reco_snippets)
  {
    std::vector<pthread_t> perso_threads;
    std::vector<perso_thread_arg*> perso_args;

    bool have_peers = npeers > 0 ? true : false;

    // thread for local db.
    threaded_personalize(perso_args,perso_threads,
                         query,lang,expansion,&snippets,&related_queries,&reco_snippets);

    // one thread per remote peer, to handle the IO.
    hash_map<const char*,peer*,hash<const char*>,eqstr>::const_iterator hit
    = cf_configuration::_config->_pl._peers.begin();
    while(hit!=cf_configuration::_config->_pl._peers.end())
      {
        threaded_personalize(perso_args,perso_threads,
                             query,lang,expansion,&snippets,&related_queries,&reco_snippets,
                             (*hit).second);
        ++hit;
      }

    // join.
    for (size_t i=0; i<perso_threads.size(); i++)
      {
        if (perso_threads.at(i)!=0)
          pthread_join(perso_threads.at(i),NULL);
      }

    // clear.
    for (size_t i=0; i<perso_args.size(); i++)
      {
        if (perso_args.at(i))
          {
            if (!have_peers && perso_args.at(i)->_err == SP_ERR_OK)
              npeers++;
            delete perso_args.at(i);
          }
      }
  }

  void rank_estimator::threaded_personalize(std::vector<perso_thread_arg*> &perso_args,
      std::vector<pthread_t> &perso_threads,
      const std::string &query,
      const std::string &lang,
      const uint32_t &expansion,
      std::vector<search_snippet*> *snippets,
      std::multimap<double,std::string,std::less<double> > *related_queries,
      hash_map<uint32_t,search_snippet*,id_hash_uint> *reco_snippets,
      peer *pe)
  {
    perso_thread_arg *args  = new perso_thread_arg();
    args->_query = query;
    args->_lang = lang;
    args->_snippets = snippets;
    args->_related_queries = related_queries;
    args->_reco_snippets = reco_snippets;
    args->_estimator = this;
    args->_pe = pe;
    args->_expansion = expansion;
    args->_err = SP_ERR_OK;

    pthread_t p_thread;
    int err = pthread_create(&p_thread, NULL, // default attribute is PTHREAD_CREATE_JOINABLE
                             (void * (*)(void*))rank_estimator::personalize_cb,args);
    if (err!=0)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Error creating personalization thread.");
        perso_threads.push_back(0);
        delete args;
        perso_args.push_back(NULL);
        return;
      }
    perso_args.push_back(args);
    perso_threads.push_back(p_thread);
  }

  void rank_estimator::personalize_cb(perso_thread_arg *args)
  {
    // call + catch exception here.
    try
      {
        if (!args->_pe)
          args->_estimator->personalize(args->_query,args->_lang,
                                        args->_expansion,
                                        *args->_snippets,
                                        *args->_related_queries,
                                        *args->_reco_snippets);
        else
          args->_estimator->personalize(args->_query,args->_lang,
                                        args->_expansion,
                                        *args->_snippets,
                                        *args->_related_queries,
                                        *args->_reco_snippets,
                                        args->_pe->_host,
                                        args->_pe->_port,
                                        args->_pe->_path,
                                        args->_pe->_rsc);
      }
    catch(sp_exception &e)
      {
        args->_err = e.code();
      }
  }

  void rank_estimator::fetch_query_data(const std::string &query,
                                        const std::string &lang,
                                        const uint32_t &expansion,
                                        hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata,
                                        const std::string &host,
                                        const int &port,
                                        const std::string &path,
                                        const std::string &rsc) throw (sp_exception)
  {
    user_db *udb = NULL;
    if (host.empty()) // local user database.
      udb = seeks_proxy::_user_db;
    else if (rsc == "sn")
      {
        udb = new user_db(false,host,port,path,rsc);
      }
    else if (rsc == "tt")
      {
        udb = new user_db(false,host,port); // remote db.
        int err = udb->open_db();
        if (err != SP_ERR_OK)
          {
            delete udb;
            std::string msg = "cannot open remote db " + host + ":" + miscutil::to_string(port);
            throw sp_exception(err,msg);
          }
      }
    else
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Wrong user db resource %s for fetching data",
                          rsc.c_str());
        return;
      }

    // fetch records from user DB.
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
    rank_estimator::fetch_user_db_record(query,udb,records);

    // extract queries.
    rank_estimator::extract_queries(query,lang,expansion,udb,records,qdata);

    errlog::log_error(LOG_LEVEL_DEBUG,"%s%s: fetched %d queries",
                      host.c_str(),path.c_str(),qdata.size());

    // close db.
    if (udb != seeks_proxy::_user_db) // not the local db.
      delete udb;

    // destroy records.
    if (cf_configuration::_config->_record_cache_timeout == 0)
      rank_estimator::destroy_records(records);
  }

  void rank_estimator::fetch_user_db_record(const std::string &query,
      user_db *udb,
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
        db_record *dbr = rank_estimator::find_dbr(udb,key_str,qc_str);
        if (dbr)
          {
            records.insert(std::pair<const DHTKey*,db_record*>(new DHTKey((*hit).second),dbr));
          }
        ++hit;
      }
  }

  void rank_estimator::extract_queries(const std::string &query,
                                       const std::string &lang,
                                       const uint32_t &expansion,
                                       user_db *udb,
                                       const hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> &records,
                                       hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata)
  {
    static std::string qc_str = "query-capture";

    str_chain strc_query(query_capture_element::no_command_query(query),0,true);
    strc_query = strc_query.rank_alpha();

    mutex_lock(&_est_mutex);
    stopwordlist *swl = seeks_proxy::_lsh_config->get_wordlist(lang);
    mutex_unlock(&_est_mutex);

    /*
     * Iterate records and gather queries and data.
     */
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

            if (swl && !query_recommender::select_query(strc_query,qd->_query,swl))
              {
                ++qit;
                continue;
              }

            if ((hit=qdata.find(qd->_query.c_str()))==qdata.end())
              {
                str_chain strc_rquery(qd->_query,0,true);
                int nradius = std::max(strc_rquery.size(),strc_query.size())
                              - strc_query.intersect_size(strc_rquery);

                if (qd->_radius == 0) // contains the data.
                  {
                    query_data *nqd = new query_data(qd);
                    nqd->_radius = nradius;
                    nqd->_record_key = new DHTKey(*(*vit).first); // mark data with record key.
                    qdata.insert(std::pair<const char*,query_data*>(nqd->_query.c_str(),nqd));
                  }
                else if (nradius <= (expansion==0 ? 0 : expansion-1)) //TODO: check on max radius here.
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
                    db_record *dbr_data = rank_estimator::find_dbr(udb,key_str,qc_str);
                    if (!dbr_data) // this should never happen.
                      {
                        errlog::log_error(LOG_LEVEL_ERROR, "cannot find query data for key %s in user db",
                                          key_str.c_str());
                        ++qit;
                        continue;
                      }

                    db_query_record *dbqr_data = dynamic_cast<db_query_record*>(dbr_data);
                    if (dbqr_data)
                      {
                        hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator qit2
                        = dbqr_data->_related_queries.begin();
                        while (qit2!=dbqr_data->_related_queries.end())
                          {
                            if ((*qit2).second->_radius == 0
                                && (*qit2).second->_query == qd->_query)
                              {
                                query_data *dbqrc = new query_data((*qit2).second);
                                dbqrc->_radius = nradius;
                                dbqrc->_record_key = new DHTKey((*features.begin()).second); // mark the data with the record key.
                                qdata.insert(std::pair<const char*,query_data*>(dbqrc->_query.c_str(),
                                             dbqrc));
                                break;
                              }
                            ++qit2;
                          }
                        if (cf_configuration::_config->_record_cache_timeout == 0)
                          delete dbqr_data;
                      }
                  }
                else // radius is below requested expansion, keep queries for recommendation.
                  {
                    query_data *dbqrc = new query_data(*qd);
                    str_chain strc_rquery(qd->_query,0,true);
                    dbqrc->_radius = std::max(strc_query.size(),strc_rquery.size())
                                     - strc_query.intersect_size(strc_rquery); // update radius relatively to original query.
                    qdata.insert(std::pair<const char*,query_data*>(dbqrc->_query.c_str(),
                                 dbqrc));
                  }
              }
            ++qit;
          }
        ++vit;
      }
  }

  void rank_estimator::destroy_records(hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> &records)
  {
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
  }

  void rank_estimator::destroy_query_data(hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata)
  {
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit,hit2;
    hit = qdata.begin();
    while (hit!=qdata.end())
      {
        query_data *qd = (*hit).second;
        hit2 = hit;
        ++hit;
        delete qd;
      }
  }

  db_record* rank_estimator::find_dbr(user_db *udb, const std::string &key,
                                      const std::string &plugin_name)
  {
    if (udb == seeks_proxy::_user_db) // local
      return udb->find_dbr(key,plugin_name);
    else
      {
        db_obj_remote *dorj = dynamic_cast<db_obj_remote*>(udb->_hdb);
        db_record *dbr = NULL;
        std::string rkey = user_db::generate_rkey(key,plugin_name);
        if (dorj)
          {
            if (cf_configuration::_config->_record_cache_timeout > 0)
              {
                dbr = rank_estimator::_store.find(dorj->_host,dorj->_port,dorj->_path,rkey);
                if (dbr)
                  return dbr;
              }
            errlog::log_error(LOG_LEVEL_DEBUG,"fetching record %s from %s%s",
                              key.c_str(),dorj->_host.c_str(),dorj->_path.c_str());
            dbr = udb->find_dbr(key,plugin_name);
            if (dbr && cf_configuration::_config->_record_cache_timeout > 0)
              rank_estimator::_store.add(dorj->_host,dorj->_port,dorj->_path,rkey,dbr);
            return dbr;
          }
        else return NULL;
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

  void simple_re::personalize(const std::string &query,
                              const std::string &lang,
                              const uint32_t &expansion,
                              std::vector<search_snippet*> &snippets,
                              std::multimap<double,std::string,std::less<double> > &related_queries,
                              hash_map<uint32_t,search_snippet*,id_hash_uint> &reco_snippets,
                              const std::string &host,
                              const int &port,
                              const std::string &path,
                              const std::string &rsc) throw (sp_exception)
  {
    // fetch queries from user DB.
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    try
      {
        rank_estimator::fetch_query_data(query,lang,expansion,qdata,host,port,path,rsc);
      }
    catch(sp_exception &e)
      {
        throw e;
      }

    // build up the filter.
    std::map<std::string,bool> filter;
    simple_re::build_up_filter(&qdata,filter);

    // rank, urls & queries.
    if (!qdata.empty())
      {
        mutex_lock(&_est_mutex);
        estimate_ranks(query,lang,snippets,&qdata,&filter);
        recommend_urls(query,lang,reco_snippets,&qdata,&filter);
        query_recommender::recommend_queries(query,lang,related_queries,&qdata);
        mutex_unlock(&_est_mutex);
      }

    // destroy query data.
    if (cf_configuration::_config->_record_cache_timeout == 0)
      rank_estimator::destroy_query_data(qdata);
  }

  void simple_re::estimate_ranks(const std::string &query,
                                 const std::string &lang,
                                 const uint32_t &expansion,
                                 std::vector<search_snippet*> &snippets,
                                 const std::string &host,
                                 const int &port) throw (sp_exception)
  {
    // fetch queries from user DB.
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    try
      {
        rank_estimator::fetch_query_data(query,lang,expansion,qdata,host,port);
      }
    catch(sp_exception &e)
      {
        throw e;
      }

    // build up the filter.
    std::map<std::string,bool> filter;
    simple_re::build_up_filter(&qdata,filter);

    // estimate ranks.
    estimate_ranks(query,lang,snippets,&qdata,&filter);

    // destroy query data.
    if (cf_configuration::_config->_record_cache_timeout == 0)
      rank_estimator::destroy_query_data(qdata);
  }

  void simple_re::estimate_ranks(const std::string &query,
                                 const std::string &lang,
                                 std::vector<search_snippet*> &snippets,
                                 hash_map<const char*,query_data*,hash<const char*>,eqstr> *qdata,
                                 std::map<std::string,bool> *filter)
  {
    // get stop word list.
    stopwordlist *swl = seeks_proxy::_lsh_config->get_wordlist(lang);

    // gather normalizing values.
    int i = 0;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
    = qdata->begin();
    float q_vurl_hits[qdata->size()];
    while (hit!=qdata->end())
      {
        q_vurl_hits[i++] = fabs((*hit).second->vurls_total_hits()); // normalization: avoid negative values.
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
        hit = qdata->begin();
        while (hit!=qdata->end())
          {
            query_data *qd = (*hit).second;
            if (!qd->_visited_urls)
              {

                ++hit;
                i++;
                continue;
              }

            float qpost = estimate_rank((*vit),filter->empty() ? NULL:filter,ns,qd,
                                        q_vurl_hits[i++],url,host);
            if (qpost > 0.0)
              {
                qpost *= simple_re::query_halo_weight(query,qd->_query,qd->_radius,swl);
                posteriors[j] += qpost; // boosting over similar queries.
                //std::cerr << "url: " << (*vit)->_url << " -- qpost: " << qpost << std::endl;
              }

            ++hit;
          }

        // estimate the url prior.
        float prior = 1.0;
        if (nuri != 0 && (*vit)->_doc_type != VIDEO_THUMB
            && (*vit)->_doc_type != TWEET && (*vit)->_doc_type != IMAGE) // not empty or type with not enought competition on domains.
          prior = estimate_prior((*vit),filter->empty() ? NULL:filter,url,host,nuri);
        posteriors[j] *= prior;

        //std::cerr << "url: " << (*vit)->_url << " -- prior: " << prior << " -- posterior: " << posteriors[j] << std::endl;

        sum_posteriors += posteriors[j++];

        if ((*vit)->_personalized && (*vit)->_engine.has_feed("seeks"))
          {
            (*vit)->_engine.add_feed("seeks","s.s");
            (*vit)->_meta_rank++;
            (*vit)->_npeers++;
          }

        ++vit;
      }

    // wrapup.
    if (sum_posteriors > 0.0)
      {
        for (size_t k=0; k<ns; k++)
          {
            posteriors[k] /= sum_posteriors; // normalize.
            snippets.at(k)->_seeks_rank += posteriors[k]; //TODO: additive filter.
            //std::cerr << "url: " << snippets.at(k)->_url << " -- seeks_rank: " << snippets.at(k)->_seeks_rank << std::endl;
          }
      }
  }

  float simple_re::estimate_rank(search_snippet *s,
                                 const std::map<std::string,bool> *filter,
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
    return estimate_rank(s,filter,ns,vd_url,vd_host,total_hits,
                         cf_configuration::_config->_domain_name_weight);
  }

  float simple_re::estimate_rank(search_snippet *s,
                                 const std::map<std::string,bool> *filter,
                                 const int &ns,
                                 const vurl_data *vd_url,
                                 const vurl_data *vd_host,
                                 const float &total_hits,
                                 const float &domain_name_weight)
  {
    float posterior = 0.0;
    bool filtered = false;

    if (vd_url && filter)
      {
        std::map<std::string,bool>::const_iterator hit = filter->find(vd_url->_url.c_str());
        if (hit!=filter->end())
          filtered = true;
      }

    if (!vd_url || vd_url->_hits < 0 || filtered)
      {
        float num = (vd_url && vd_url->_hits < 0) ? vd_url->_hits : 1.0;
        posterior = num / (log(fabs(total_hits) + 1.0) + ns); // XXX: may replace ns with a less discriminative value.
        if (filtered && !s)
          {
            posterior = 0.0; // if no snippet support, filtered out. XXX: this may change in the long term.
          }
      }
    else
      {
        posterior = (log(vd_url->_hits + 1.0) + 1.0)/ (log(fabs(total_hits) + 1.0) + ns);
        if (s)
          {
            s->_personalized = true;
            s->_engine.add_feed("seeks","s.s");
            /* s->_meta_rank++;
            s->_npeers++;*/
            s->_hits += vd_url->_hits;
          }
      }

    // host.
    if (domain_name_weight <= 0.0)
      return posterior;

    if (!vd_host || vd_host->_hits < 0 || !s || s->_doc_type == VIDEO_THUMB || s->_doc_type == TWEET
        || s->_doc_type == IMAGE // empty or type with not enough competition on domains.
        || (filter && (filtered || filter->find(vd_host->_url.c_str())!=filter->end()))) // filter domain out if url was filtered out.
      filtered = true;
    else filtered = false;
    if (filtered)
      {
        posterior *= domain_name_weight
                     / (log(fabs(total_hits) + 1.0) + ns); // XXX: may replace ns with a less discriminative value.
        if (vd_host && !s)
          posterior = 0.0; // if no snippet support, filtered out. XXX: this may change in the long term.
      }
    else
      {
        posterior *= domain_name_weight
                     * (log(vd_host->_hits + 1.0) + 1.0)
                     / (log(fabs(total_hits) + 1.0) + ns); // with domain-name weight factor.
        if (s && (!vd_url || vd_url->_hits > 0))
          s->_personalized = true;
      }
    //std::cerr << "posterior: " << posterior << std::endl;

    return posterior;
  }

  float simple_re::estimate_prior(search_snippet *s,
                                  const std::map<std::string,bool> *filter,
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
        std::map<std::string,bool>::const_iterator hit = filter->find(surl.c_str());
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
        if (cf_configuration::_config->_record_cache_timeout == 0)
          delete uc_dbr;
        if (s)
          {
            s->_personalized = true;
            s->_engine.add_feed("seeks","s.s");
            //s->_meta_rank++;
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
            if (s->_meta_rank <= s->_engine.size())
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
                                 const uint32_t &expansion,
                                 hash_map<uint32_t,search_snippet*,id_hash_uint> &snippets,
                                 const std::string &host,
                                 const int &port) throw (sp_exception)
  {
    // fetch queries from user DB.
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    try
      {
        rank_estimator::fetch_query_data(query,lang,expansion,qdata,host,port);
      }
    catch(sp_exception &e)
      {
        throw e;
      }

    // build up the filter.
    std::map<std::string,bool> filter;
    simple_re::build_up_filter(&qdata,filter);

    // urls.
    recommend_urls(query,lang,snippets,&qdata,&filter);

    // destroy query data.
    if (cf_configuration::_config->_record_cache_timeout == 0)
      rank_estimator::destroy_query_data(qdata);
  }

  void simple_re::recommend_urls(const std::string &query,
                                 const std::string &lang,
                                 hash_map<uint32_t,search_snippet*,id_hash_uint> &snippets,
                                 hash_map<const char*,query_data*,hash<const char*>,eqstr> *qdata,
                                 std::map<std::string,bool> *filter)
  {
    // get stop word list.
    stopwordlist *swl = seeks_proxy::_lsh_config->get_wordlist(lang);

    // gather normalizing values.
    int nvurls = 1.0;
    int i = 0;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator chit;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
    = qdata->begin();
    float q_vurl_hits[qdata->size()];
    while (hit!=qdata->end())
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
    hit = qdata->begin();
    while(hit!=qdata->end())
      {
        query_data *qd = (*hit).second;
        if (!qd->_visited_urls)
          {
            ++hit;
            continue;
          }

        float halo_weight = simple_re::query_halo_weight(query,qd->_query,qd->_radius,swl);

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
                posterior = estimate_rank(NULL,filter,nvurls,vd,NULL,
                                          q_vurl_hits[i],cf_configuration::_config->_domain_name_weight);

                // level them down according to query radius.
                //posterior *= 1.0 / static_cast<float>(log(qd->_radius + 1.0) + 1.0); // account for distance to original query.
                posterior *= halo_weight;

                // update or create snippet.
                if (posterior > 0.0)
                  {
                    std::string surl = urlmatch::strip_url(vd->_url);
                    uint32_t sid = mrf::mrf_single_feature(surl);
                    if ((sit = snippets.find(sid))!=snippets.end())
                      (*sit).second->_seeks_rank += posterior; // update.
                    else
                      {
                        search_snippet *sp = new search_snippet();
                        sp->set_url(vd->_url);
                        sp->set_title(vd->_title);
                        sp->set_summary(vd->_summary);
                        sp->_meta_rank = 1;
                        sp->_seeks_rank = posterior;
                        snippets.insert(std::pair<uint32_t,search_snippet*>(sp->_id,sp));
                      }
                  }
              }

            ++vit;
          }
        ++i;
        ++hit;
      }
  }

  void simple_re::thumb_down_url(const std::string &query,
                                 const std::string &lang,
                                 const std::string &url) throw (sp_exception)
  {
    static std::string qc_string = "query-capture";

    // fetch queries from user DB.
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    try
      {
        rank_estimator::fetch_query_data(query,lang,0,qdata);
      }
    catch(sp_exception &e)
      {
        throw e;
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
            short hits = vd->_hits > 0 ? vd->_hits : 1;

            if (rkey)
              {
                query_capture_element::remove_url(*rkey,query_clean,purl,host,
                                                  hits,qd->_radius,
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
        std::string msg = "thumb_down_url " + purl + " failed: cannot find query " + query_clean + " in records";
        errlog::log_error(LOG_LEVEL_ERROR,msg.c_str());

        // destroy query data.
        if (cf_configuration::_config->_record_cache_timeout == 0)
          rank_estimator::destroy_query_data(qdata);

        // exception.
        throw sp_exception(DB_ERR_NO_REC,msg);
      }

    // locate url and host in captured uris and remove / lower counters accordingly.
    plugin *ucpl = plugin_manager::get_plugin("uri-capture");
    if (ucpl)
      {
        uri_capture *uc = static_cast<uri_capture*>(ucpl);
        static_cast<uri_capture_element*>(uc->_interceptor_plugin)->remove_uri(purl,host); // may throw.
      }

    // destroy query data.
    if (cf_configuration::_config->_record_cache_timeout == 0)
      rank_estimator::destroy_query_data(qdata);
  }

  uint32_t simple_re::damerau_levenshtein_distance(const std::string &s1, const std::string &s2,
      const uint32_t &c)
  {
    const uint32_t len1 = s1.size(), len2 = s2.size();
    uint32_t inf = len1 + len2;
    uint32_t H[len1+2][len2+2];
    H[0][0] = inf;
    for(uint32_t i = 0; i<=len1; ++i)
      {
        H[i+1][1] = i;
        H[i+1][0] = inf;
      }
    for(uint32_t j = 0; j<=len2; ++j)
      {
        H[1][j+1] = j;
        H[0][j+1] = inf;
      }
    uint32_t DA[c];
    for(uint32_t d = 0; d<c; ++d) DA[d]=0;
    for(uint32_t i = 1; i<=len1; ++i)
      {
        uint32_t DB = 0;
        for(uint32_t j = 1; j<=len2; ++j)
          {
            uint32_t i1 = DA[(unsigned char)s2[j-1]];
            uint32_t j1 = DB;
            uint32_t d = ((s1[i-1]==s2[j-1])?0:1);
            if(d==0) DB = j;
            H[i+1][j+1] = std::min(std::min(H[i][j]+d, H[i+1][j] + 1),
                                   std::min(H[i][j+1]+1,
                                            H[i1][j1] + (i-i1-1) + 1 + (j-j1-1)));
          }
        DA[(unsigned char)s1[i-1]] = i;
      }
    return H[len1+1][len2+1];
  }

  uint32_t simple_re::query_distance(const std::string &s1, const std::string &s2,
                                     const stopwordlist *swl)
  {
    str_chain sc1(s1,0,true);
    str_chain sc2(s2,0,true);

    return simple_re::query_distance(sc1,sc2,swl);
  }

  uint32_t simple_re::query_distance(str_chain &sc1, str_chain &sc2,
                                     const stopwordlist *swl)
  {
    // prune strings with stop word list if any.
    if (swl)
      {
        for (size_t i=0; i<sc1.size(); i++)
          if (swl->has_word(sc1.at(i)))
            sc1.remove_token(i);
        for (size_t i=0; i<sc2.size(); i++)
          if (swl->has_word(sc2.at(i)))
            sc2.remove_token(i);
      }

    sc1 = sc1.rank_alpha();
    sc2 = sc2.rank_alpha();
    std::string rs1 = sc1.print_str();
    std::string rs2 = sc2.print_str();
    return damerau_levenshtein_distance(rs1,rs2);
  }

  float simple_re::query_halo_weight(const std::string &q1, const std::string &q2,
                                     const uint32_t &q2_radius, const stopwordlist *swl)
  {
    str_chain s1(q1,0,true);
    str_chain s2(q2,0,true);
    return log(1.0 + std::max(s1.size(),s2.size()) - q2_radius) / (log(simple_re::query_distance(s1,s2,swl) + 1.0) + 1.0);
  }

  void simple_re::build_up_filter(hash_map<const char*,query_data*,hash<const char*>,eqstr> *qdata,
                                  std::map<std::string,bool> &filter)
  {
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit = qdata->begin();
    while(hit!=qdata->end())
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
                        filter.insert(std::pair<std::string,bool>(vd->_url,true));
                      }
                    ++vit;
                  }
              }
            break;
          }
        ++hit;
      }
  }

} /* end of namespace. */

