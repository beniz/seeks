/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009-2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#ifndef WEBSEARCH_H
#define WEBSEARCH_H

#define NO_PERL // we do not use Perl.

#include "wb_err.h"
#include "plugin.h"
#include "search_snippet.h"
#include "query_context.h"
#include "websearch_configuration.h"
#include "miscutil.h"
#include "mutexes.h"

#include <string>

using sp::client_state;
using sp::http_response;
using sp::plugin;
using sp::miscutil;

namespace seeks_plugins
{

  struct wo_thread_arg
  {
    wo_thread_arg(client_state *csp, http_response *rsp,
                  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                  bool render=true)
      :_csp(csp),_rsp(rsp),_render(render)
    {
      _parameters = miscutil::copy_map(parameters);
    };

    ~wo_thread_arg()
    {
      miscutil::free_map(_parameters);
    };

    client_state *_csp;
    http_response *_rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *_parameters;
    bool _render;
  };

  class websearch : public plugin
  {
    public:
      websearch();

      ~websearch();

      virtual void start();
      virtual void stop();

      /* cgi calls. */
      static sp_err cgi_websearch_hp(client_state *csp,
                                     http_response *rsp,
                                     const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_websearch_search_hp_css(client_state *csp,
          http_response *rsp,
          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_websearch_search_css(client_state *csp,
                                             http_response *rsp,
                                             const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_websearch_opensearch_xml(client_state *csp,
          http_response *rsp,
          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_websearch_search(client_state *csp,
                                         http_response *rsp,
                                         const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_websearch_search_cache(client_state *csp,
          http_response *rsp,
          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_websearch_neighbors_url(client_state *csp, http_response *rsp,
          const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static sp_err cgi_websearch_neighbors_title(client_state *csp, http_response *rsp,
          const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static sp_err cgi_websearch_clustered_types(client_state *csp, http_response *rsp,
          const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static sp_err cgi_websearch_similarity(client_state *csp, http_response *rsp,
                                             const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static sp_err cgi_websearch_clusterize(client_state *csp, http_response *rsp,
                                             const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static sp_err cgi_websearch_node_info(client_state *csp, http_response *rsp,
                                            const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      /* websearch. */
      static void perform_action_threaded(wo_thread_arg *args);

      static sp_err perform_action(client_state *csp,
                                   http_response *rsp,
                                   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                   bool render = true);

      static sp_err perform_websearch(client_state *csp,
                                      http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                      bool render=true);

      static query_context* lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static query_context* lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                      hash_map<uint32_t,query_context*,id_hash_uint> &active_contexts);

      static std::string no_command_query(const std::string &oquery);

      static void preprocess_parameters(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                        client_state *csp) throw (sp_exception);

      /* error handling. */
      static sp_err failed_ses_connect(client_state *csp, http_response *rsp);

    public:
      static websearch_configuration *_wconfig;
      static hash_map<uint32_t,query_context*,id_hash_uint> _active_qcontexts;
      static double _cl_sec; // clock ticks per second.

      /* dependent plugins. */
    public:
      static plugin *_qc_plugin; /**< query capture plugin. */
      static bool _qc_plugin_activated;
      static plugin *_cf_plugin; /**< (collaborative) filtering plugin. */
      static bool _cf_plugin_activated;

      /* multithreading. */
    private:
      static sp_mutex_t _context_mutex;
  };

} /* end of namespace. */

#endif
