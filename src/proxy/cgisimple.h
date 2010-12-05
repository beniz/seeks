/*********************************************************************
 * Purpose     :  Declares functions to intercept request, generate
 *                html or gif answers, and to compose HTTP resonses.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001-2007 the SourceForge
 *                Privoxy team. http://www.privoxy.org/
 *
 *                Based on the Internet Junkbuster originally written
 *                by and Copyright (C) 1997 Anonymous Coders and
 *                Junkbusters Corporation.  http://www.junkbusters.com
 *
 *                This program is free software; you can redistribute it
 *                and/or modify it under the terms of the GNU General
 *                Public License as published by the Free Software
 *                Foundation; either version 2 of the License, or (at
 *                your option) any later version.
 *
 *                This program is distributed in the hope that it will
 *                be useful, but WITHOUT ANY WARRANTY; without even the
 *                implied warranty of MERCHANTABILITY or FITNESS FOR A
 *                PARTICULAR PURPOSE.  See the GNU General Public
 *                License for more details.
 *
 *                The GNU General Public License should be included with
 *                this file.  If not, you can view it at
 *                http://www.gnu.org/copyleft/gpl.html
 *                or write to the Free Software Foundation, Inc., 59
 *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 **********************************************************************/

#ifndef CGISIMPLE_H
#define CGISIMPLE_H

#include "proxy_dts.h"

#if (__GNUC__ >=3)
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#if (__GNUC__ >= 3)
using __gnu_cxx::hash;
using __gnu_cxx::hash_map;
#else
using std::hash;
using std::hash_map;
#endif

namespace sp
{

  class cgisimple
  {
    public:
      /*
       * CGI functions
       */
      static sp_err cgi_default      (client_state *csp,
                                      http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_error_404    (client_state *csp,
                                      http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_robots_txt   (client_state *csp,
                                      http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_send_banner  (client_state *csp,
                                      http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_show_status  (client_state *csp,
                                      http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_show_url_info(client_state *csp,
                                      http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_show_version (client_state *csp,
                                      http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_show_request (client_state *csp,
                                      http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_transparent_image (client_state *csp,
                                           http_response *rsp,
                                           const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_send_error_favicon (client_state *csp,
                                            http_response *rsp,
                                            const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_send_stylesheet(client_state *csp,
                                        http_response *rsp,
                                        const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_send_url_info_osd(client_state *csp,
                                          http_response *rsp,
                                          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_send_user_manual(client_state *csp,
                                         http_response *rsp,
                                         const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_file_server(client_state *csp,
                                    http_response *rsp,
                                    const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err cgi_plugin_file_server(client_state *csp,
                                           http_response *rsp,
                                           const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

#ifdef FEATURE_GRACEFUL_TERMINATION
      static sp_err cgi_die (client_state *csp,
                             http_response *rsp,
                             const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
#endif

#ifdef FEATURE_TOGGLE
      static sp_err cgi_toggle(client_state *csp,
                               http_response *rsp,
                               const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
#endif /* def FEATURE_TOGGLE */

//private:
      static char* show_rcs();
      static sp_err show_defines(hash_map<const char*,const char*,hash<const char*>,eqstr> *exports);
      static sp_err cgi_show_plugin(client_state *csp, http_response *rsp,
                                    const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
      static sp_err load_file(const char *filename, char **buffer, size_t *length);
      static void file_response_content_type(const std::string &ext_str, http_response *rsp);
  };

} /* end of namespace. */

#endif /* ndef CGISIMPLE_H */

