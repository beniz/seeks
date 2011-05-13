/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#ifndef RANK_ESTIMATORS_H
#define RANK_ESTIMATORS_H

#include "peer_list.h"
#include "sp_exception.h"
#include "cr_store.h"
#include "search_snippet.h"
#include "db_query_record.h"
#include "stopwordlist.h"
#include "mrf.h"
#include "mutexes.h"

using lsh::stopwordlist;
using lsh::str_chain;

namespace seeks_plugins
{

  class rank_estimator;

  struct perso_thread_arg
  {
    perso_thread_arg()
      :_snippets(NULL),_related_queries(NULL),_reco_snippets(NULL),_estimator(NULL),_pe(NULL),_expansion(1)
    {};

    ~perso_thread_arg()
    {}

    std::string _query;
    std::string _lang;
    std::vector<search_snippet*> *_snippets;
    std::multimap<double,std::string,std::less<double> > *_related_queries;
    hash_map<uint32_t,search_snippet*,id_hash_uint> *_reco_snippets;
    rank_estimator *_estimator;
    peer *_pe;
    uint32_t _expansion;
    sp_err _err; // error code.
  };

  class rank_estimator
  {
    public:
      rank_estimator();

      virtual ~rank_estimator() {};

      void peers_personalize(const std::string &query,
                             const std::string &lang,
                             const uint32_t &expansion,
                             uint32_t &npeers,
                             std::vector<search_snippet*> &snippets,
                             std::multimap<double,std::string,std::less<double> > &related_queries,
                             hash_map<uint32_t,search_snippet*,id_hash_uint> &reco_snippets);

      void threaded_personalize(std::vector<perso_thread_arg*> &perso_args,
                                std::vector<pthread_t> &perso_threads,
                                const std::string &query, const std::string &lang,
                                const uint32_t &expansion,
                                std::vector<search_snippet*> *snippets,
                                std::multimap<double,std::string,std::less<double> > *related_queries,
                                hash_map<uint32_t,search_snippet*,id_hash_uint> *reco_snippets,
                                peer *pe = NULL);

      static void personalize_cb(perso_thread_arg *args);

      virtual void personalize(const std::string &query,
                               const std::string &lang,
                               const uint32_t &expansion,
                               std::vector<search_snippet*> &snippets,
                               std::multimap<double,std::string,std::less<double> > &related_queries,
                               hash_map<uint32_t,search_snippet*,id_hash_uint> &reco_snippets,
                               const std::string &host="",
                               const int &port=-1,
                               const std::string &path="",
                               const std::string &rsc="") throw (sp_exception) {};

      virtual void estimate_ranks(const std::string &query,
                                  const std::string &lang,
                                  std::vector<search_snippet*> &snippets,
                                  const std::string &host="",
                                  const int &port=-1,
                                  const std::string &rsc="") throw (sp_exception) {};

      virtual void recommend_urls(const std::string &query,
                                  const std::string &lang,
                                  hash_map<uint32_t,search_snippet*,id_hash_uint> &snippet,
                                  const std::string &host="",
                                  const int &port=-1) throw (sp_exception) {};

      virtual void thumb_down_url(const std::string &query,
                                  const std::string &lang,
                                  const std::string &url) throw (sp_exception) {};

      static void fetch_query_data(const std::string &query,
                                   const std::string &lang,
                                   const uint32_t &expansion,
                                   hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata,
                                   const std::string &host="",
                                   const int &port=-1,
                                   const std::string &path="",
                                   const std::string &rsc="") throw (sp_exception);

      static void fetch_user_db_record(const std::string &query,
                                       user_db *udb,
                                       hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> &records);

      static void extract_queries(const std::string &query,
                                  const std::string &lang,
                                  const uint32_t &expansion,
                                  user_db *udb,
                                  const hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> &records,
                                  hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata);

      static void destroy_records(hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> &records);

      static void destroy_query_data(hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata);

      static db_record* find_dbr(user_db *udb, const std::string &key,
                                 const std::string &plugin_name);

      static cr_store _store;

      static sp_mutex_t _est_mutex;
  };

  class simple_re : public rank_estimator
  {
    public:
      simple_re();

      virtual ~simple_re();

      virtual void personalize(const std::string &query,
                               const std::string &lang,
                               const uint32_t &personalize,
                               std::vector<search_snippet*> &snippets,
                               std::multimap<double,std::string,std::less<double> > &related_queries,
                               hash_map<uint32_t,search_snippet*,id_hash_uint> &reco_snippets,
                               const std::string &host="",
                               const int &port=-1,
                               const std::string &path="",
                               const std::string &rsc="") throw (sp_exception);

      virtual void estimate_ranks(const std::string &query,
                                  const std::string &lang,
                                  const uint32_t &expansion,
                                  std::vector<search_snippet*> &snippets,
                                  const std::string &host="",
                                  const int &port=-1,
                                  const std::string &rsc="") throw (sp_exception);

      void estimate_ranks(const std::string &query,
                          const std::string &lang,
                          std::vector<search_snippet*> &snippets,
                          hash_map<const char*,query_data*,hash<const char*>,eqstr> *qdata,
                          std::map<std::string,bool> *filter,
                          const std::string &rsc);

      virtual void recommend_urls(const std::string &query,
                                  const std::string &lang,
                                  const uint32_t &expansion,
                                  hash_map<uint32_t,search_snippet*,id_hash_uint> &snippets,
                                  const std::string &host="",
                                  const int &port=-1) throw (sp_exception);

      void recommend_urls(const std::string &query,
                          const std::string &lang,
                          hash_map<uint32_t,search_snippet*,id_hash_uint> &snippets,
                          hash_map<const char*,query_data*,hash<const char*>,eqstr> *qdata,
                          std::map<std::string,bool> *filter);

      virtual void thumb_down_url(const std::string &query,
                                  const std::string &lang,
                                  const std::string &url) throw (sp_exception);

      float estimate_rank(search_snippet *s,
                          const std::map<std::string,bool> *filter,
                          const int &ns,
                          const query_data *qd,
                          const float &total_hits,
                          const std::string &surl,
                          const std::string &host,
                          bool &pers);

      float estimate_rank(search_snippet *s,
                          const std::map<std::string,bool> *filter,
                          const int &ns,
                          const vurl_data *vd_url,
                          const vurl_data *vd_host,
                          const float &total_hits,
                          const float &domain_name_weight,
                          bool &pers);

      float estimate_prior(search_snippet *s,
                           const std::map<std::string,bool> *filter,
                           const std::string &surl,
                           const std::string &host,
                           const uint64_t &nuri);

      static uint32_t damerau_levenshtein_distance(const std::string &s1, const std::string &s2,
          const uint32_t &c=256);

      static uint32_t query_distance(const std::string &s1, const std::string &s2,
                                     const stopwordlist *swl=NULL);

      static uint32_t query_distance(str_chain &sc1, str_chain &sc2,
                                     const stopwordlist *swl=NULL);

      static float query_halo_weight(const std::string &q1, const std::string &q2,
                                     const uint32_t &q2_radius, const stopwordlist *swl=NULL);

      static void build_up_filter(hash_map<const char*,query_data*,hash<const char*>,eqstr> *qdata,
                                  std::map<std::string,bool> &filter);
  };

} /* end of namespace. */

#endif
