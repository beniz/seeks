/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010, 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
 * Copyright (C) 2011 Stéphane Bonhomme <stephane.bonhomme@seeks.pro>
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

#ifndef XML_RENDERER_H
#define XML_RENDERER_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include "clustering.h"


namespace seeks_plugins
{
  class xml_renderer
  {
    public:
      static sp_err render_engines(const feeds &engines,
                                   const bool &img,
                                   xmlNodePtr parent);

      static sp_err render_node_options(client_state *csp,
                                        xmlNodePtr parent);

      static sp_err render_suggested_queries(const query_context *qc,
                                             const int &nsuggs,
                                             xmlNodePtr parent);

      static sp_err render_recommendations(const query_context *qc,
                                           const int &nreco,
                                           const double &qtime,
                                           const uint32_t &radius,
                                           const std::string &lang,
                                           xmlNodePtr parent);

      static sp_err render_cached_queries(const std::string &query,
                                          const int &nq,
                                          xmlNodePtr parent);

      static sp_err render_img_engines(const query_context *qc,
                                       xmlNodePtr parent);

      /*static sp_err render_snippet(search_snippet *sp,
      		 const bool &thumbs,
      		 const std::vector<std::string> &query_words,
      		 xmlNodePtr parent);*/

      static sp_err render_snippets(const std::string &query_clean,
                                    const int &current_page,
                                    const std::vector<search_snippet*> &snippets,
                                    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                    xmlNodePtr parent);

      static sp_err render_clustered_snippets(const std::string &query_clean,
                                              hash_map<int,cluster*> *clusters,
                                              const short &K,
                                              const query_context *qc,
                                              const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                              xmlNodePtr parent);

      static sp_err render_xml_cached_queries(const std::string &query,
                                              const int &nq,
                                              xmlDocPtr doc);

      static sp_err render_xml_clustered_results(const query_context *qc,
          const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
          hash_map<int,cluster*> *clusters,
          const short &K,
          const double &qtime,
          xmlDocPtr doc);

      static sp_err render_xml_engines(const feeds &engines,
                                       xmlDocPtr doc);

      static sp_err render_xml_node_options(client_state *csp,
                                            xmlDocPtr doc);

      static sp_err render_xml_peers(std::list<std::string> *peers,
                                     xmlDocPtr doc);

      static sp_err render_xml_recommendations(const query_context *qc,
          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
          const double &qtime,
          const int &radius,
          const std::string &lang,
          xmlDocPtr doc);

      static sp_err render_xml_results(const query_context *qc,
                                       const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                       const std::vector<search_snippet*> &snippets,
                                       const double &qtime,
                                       const bool &img,
                                       xmlDocPtr doc);

      static sp_err render_xml_snippet(query_context *qc,
                                       search_snippet *sp,
                                       xmlDocPtr doc);

      static sp_err render_xml_suggested_queries(const query_context *qc,
          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
          xmlDocPtr doc);

      static sp_err render_xml_words(const std::set<std::string> &words,
                                     xmlDocPtr doc);



  };
} /* end of namespace. */

#endif
