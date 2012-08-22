/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 - 2012 Emmanuel Benazera <ebenazer@seeks-project.info>
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
#include <jsoncpp/json/json.h>

namespace seeks_plugins
{
  class json_renderer
  {
    public:
      static Json::Value render_engines(const feeds &engines,
                                        const bool &img=false);

      static Json::Value render_tags(const hash_map<const char*,float,hash<const char*>,eqstr> &tags);
      
      static Json::Value render_tags(const std::multimap<float,std::string,std::greater<float> > &tags);
      
      static Json::Value render_node_options(client_state *csp);

      static Json::Value render_suggested_queries(const query_context *qc,
						  const int &nsuggs);

      static void render_json_suggested_queries(const query_context *qc,
						http_response *rsp,
						const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      
      static Json::Value render_recommendations(const query_context *qc,
						const int &nreco,
						const double &qtime,
						const uint32_t &radius,
						const std::string &lang);
      
      static void render_json_recommendations(const query_context *qc,
					      http_response *rsp,
					      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					      const double &qtime,
					      const int &radius,
					      const std::string &lang);

      static Json::Value render_cached_queries(const std::string &query,
					       const int &nq);

      static Json::Value render_img_engines(const query_context *qc);

      static Json::Value render_snippets(const std::string &query_clean,
					 const int &current_page,
					 const std::vector<search_snippet*> &snippets,
					 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);
      
      static Json::Value render_clustered_snippets(const std::string &query_clean,
						   hash_map<int,cluster*> *clusters, const short &K,
						   const query_context *qc,
						   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      
      static void render_json_node_options(client_state *csp,
					   http_response *rsp,
					   const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);
      
      static void render_json_results(const std::vector<search_snippet*> &snippets,
				      client_state *csp, http_response *rsp,
				      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				      const query_context *qc,
				      const double &qtime,
				      const bool &img=false);
      
      static void render_json_snippet(search_snippet *sp,
				      http_response *rsp,
				      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                        query_context *qc);

      static void render_json_words(const std::set<std::string> &words,
				    http_response *rsp,
				    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static void render_cached_queries(http_response *rsp,
					const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					const std::string &query,
					const int &nq);

      static void render_clustered_json_results(hash_map<int,cluster*> *clusters,
						const short &K,
						client_state *csp, http_response *rsp,
						const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
						const query_context *qc,
						const double &qtime);

      static void collect_json_results(Json::Value &results,
				       const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				       const query_context *qc,
				       const double &qtime,
				       const bool &img=false);
      
      static std::string jsonp(const std::string& input, const char* callback);
      static void response(http_response *rsp, const std::string& json_str);
  };
} /* end of namespace. */

#endif
