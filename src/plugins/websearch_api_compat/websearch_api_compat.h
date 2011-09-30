/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#ifndef WEBSEARCH_API_COMPAT_H
#define WEBSEARCH_API_COMPAT_H

#include "plugin.h"
#include "websearch.h"

namespace seeks_plugins
{

  class websearch_api_compat : public plugin
  {
    public:
      websearch_api_compat();

      ~websearch_api_compat();

      virtual void start() {};
      virtual void stop() {};

      static sp_err cgi_search_compat(client_state *csp,
                                      http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_search_cache_compat(client_state *csp,
                                            http_response *rsp,
                                            const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_qc_redir_compat(client_state *csp,
                                        http_response *rsp,
                                        const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err cgi_tbd_compat(client_state *csp,
                                   http_response *rsp,
                                   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
      static sp_err cgi_img_search_compat(client_state *csp,
                                          http_response *rsp,
                                          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
#endif
  };

} /* end of namespace. */

#endif
