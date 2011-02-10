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

#ifndef RANK_ESTIMATORS_H
#define RANK_ESTIMATORS_H

#include "search_snippet.h"
#include "db_query_record.h"
#include "stopwordlist.h"
#include "mrf.h"

using lsh::stopwordlist;
using lsh::str_chain;

namespace seeks_plugins
{

  class rank_estimator
  {
    public:
      rank_estimator() {};

      virtual ~rank_estimator() {};

      virtual void estimate_ranks(const std::string &query,
                                  const std::string &lang,
                                  std::vector<search_snippet*> &snippets) {};

      virtual void recommend_urls(const std::string &query,
                                  const std::string &lang,
                                  std::vector<search_snippet*> &snippets) {};

      virtual void thumb_down_url(const std::string &query,
                                  const std::string &lang,
                                  const std::string &url) {};

      static void fetch_user_db_record(const std::string &query,
                                       hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> &records);

      static void extract_queries(const std::string &query,
                                  const std::string &lang,
                                  const hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> &records,
                                  hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata);

      static void destroy_records(hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> &records);

      static void destroy_query_data(hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata);
  };

  class simple_re : public rank_estimator
  {
    public:
      simple_re();

      virtual ~simple_re();

      virtual void estimate_ranks(const std::string &query,
                                  const std::string &lang,
                                  std::vector<search_snippet*> &snippets);

      virtual void recommend_urls(const std::string &query,
                                  const std::string &lang,
                                  hash_map<uint32_t,search_snippet*,id_hash_uint> &snippets);

      virtual void thumb_down_url(const std::string &query,
                                  const std::string &lang,
                                  const std::string &url);

      float estimate_rank(search_snippet *s,
                          const hash_map<const char*,const char*,hash<const char*>,eqstr> *filter,
                          const int &ns,
                          const query_data *qd,
                          const float &total_hits,
                          const std::string &surl,
                          const std::string &host);

      float estimate_rank(search_snippet *s,
                          const hash_map<const char*,const char*,hash<const char*>,eqstr> *filter,
                          const int &ns,
                          const vurl_data *vd_url,
                          const vurl_data *vd_host,
                          const float &total_hits);

      float estimate_prior(search_snippet *s,
                           const hash_map<const char*,const char*,hash<const char*>,eqstr> *filter,
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

      static void build_up_filter(hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata,
                                  hash_map<const char*,const char*,hash<const char*>,eqstr> &filter);

      static void destroy_filter(hash_map<const char*,const char*,hash<const char*>,eqstr> &filter);
  };

} /* end of namespace. */

#endif
