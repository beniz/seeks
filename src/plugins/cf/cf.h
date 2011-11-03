/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#ifndef CF_H
#define CF_H

#include "sp_exception.h"
#include "plugin.h"
#include "search_snippet.h"
#include "db_query_record.h"

using namespace sp;

namespace seeks_plugins
{

  class cf : public plugin
  {
    public:
      cf();

      virtual ~cf();

      virtual void start();

      virtual void stop();

      static sp_err cgi_peers(client_state *csp,
                              http_response *rsp,
                              const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_tbd(client_state *csp,
                            http_response *rsp,
                            const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_suggestion(client_state *csp,
                                   http_response *rsp,
                                   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_recommendation(client_state *csp,
                                       http_response *rsp,
                                       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      // recommendation post & get.
      static sp_err recommendation_get(client_state *csp,
                                       http_response *rsp,
                                       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err recommendation_post(client_state *csp,
                                        http_response *rsp,
                                        const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err recommendation_delete(client_state *csp,
                                          http_response *rsp,
                                          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err tbd(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                        const std::string &url, const std::string &query);

      static void personalize(query_context *qc,
                              const bool &wait_external_sources=true,
                              const std::string &peers="ring",
                              const int &radius=-1,
                              const bool &swf=false);

      static void estimate_ranks(const std::string &query,
                                 const std::string &lang,
                                 const int &radius,
                                 std::vector<search_snippet*> &snippets,
                                 const std::string &host="",
                                 const int &port=-1) throw (sp_exception);

      static void thumb_down_url(const std::string &query,
                                 const std::string &lang,
                                 const std::string &url) throw (sp_exception);

      static void find_bqc_cb(const std::vector<std::string> &qhashes,
                              const int &radius,
                              db_query_record *&dbr);

      static std::string select_p2p_or_local(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

    public:
      static plugin *_uc_plugin;
      static plugin* _xs_plugin;
      static bool _xs_plugin_activated;

  };

} /* end of namespace. */

#endif
