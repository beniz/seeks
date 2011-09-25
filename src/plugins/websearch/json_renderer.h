/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010, 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#ifndef JSON_RENDERER_H
#define JSON_RENDERER_H

#include "websearch.h"
#include "clustering.h"

namespace seeks_plugins
{
  class json_renderer
  {
    public:
      static std::string render_engines(const feeds &engines);

      static sp_err render_node_options(client_state *csp,
                                        std::list<std::string> &opts);

      static std::string render_suggested_queries(const query_context *qc,
          const int &nsuggs);

      static sp_err render_json_suggested_queries(const query_context *qc,
          http_response *rsp,
          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static std::string render_recommendations(const query_context *qc,
          const int &nreco,
          const double &qtime,
          const uint32_t &radius,
          const std::string &lang);

      static sp_err render_json_recommendations(const query_context *qc,
          http_response *rsp,
          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
          const double &qtime,
          const int &radius,
          const std::string &lang);

      static std::string render_cached_queries(const std::string &query,
          const int &nq);

      static std::string render_img_engines(const query_context *qc);

      static std::string render_snippet(const search_snippet *sp,
                                        const bool &thumbs,
                                        const std::vector<std::string> &query_words);

      static sp_err render_snippets(const std::string &query_clean,
                                    const int &current_page,
                                    const std::vector<search_snippet*> &snippets,
                                    std::string &json_str,
                                    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static sp_err render_clustered_snippets(const std::string &query_clean,
                                              cluster *clusters, const short &K,
                                              const query_context *qc,
                                              std::string &json_str,
                                              const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err render_json_node_options(client_state *csp,
                                             http_response *rsp,
                                             const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static sp_err render_json_results(const std::vector<search_snippet*> &snippets,
                                        client_state *csp, http_response *rsp,
                                        const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                        const query_context *qc,
                                        const double &qtime,
                                        const bool &img=false);

      static sp_err render_json_snippet(const search_snippet *sp,
                                        http_response *rsp,
                                        const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                        query_context *qc);

      static sp_err render_json_words(const std::set<std::string> &words,
                                      http_response *rsp,
                                      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static sp_err render_cached_queries(http_response *rsp,
                                          const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                          const std::string &query,
                                          const int &nq);

      static sp_err render_clustered_json_results(cluster *clusters,
          const short &K,
          client_state *csp, http_response *rsp,
          const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
          const query_context *qc,
          const double &qtime);
  };
} /* end of namespace. */

#endif
