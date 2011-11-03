/**
 * Purpose     :  Declares functions to intercept request, generate
 *                html or JSON answers, and to compose HTTP responses.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009 - 2011.
 *
 *                Written by and Copyright (C) 2001-2009 the SourceForge
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
 *********************************************************************/

#ifndef CGI_H
#define CGI_H

#include "proxy_dts.h"
#include "configuration_spec.h"
#include "stl_hash.h"

#include <string>

namespace sp
{
  class cgi
  {
    public:
      /*
       * Main dispatch function
       */
      static http_response* dispatch_cgi(client_state *csp);
      static http_response* dispatch_known_cgi(client_state *csp,
          const char *path);
      static http_response* dispatch(const cgi_dispatcher *d, char* path_copy,
                                     client_state *csp,
                                     hash_map<const char*,const char*,hash<const char*>,eqstr> *param_list,
                                     http_response *rsp);

      static hash_map<const char*,const char*,hash<const char*>,eqstr>* parse_cgi_parameters(char *argstring);

      static std::string build_url_from_parameters(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static cgi_dispatcher _cgi_dispatchers[];

      static cgi_dispatcher _cgi_file_server; /* cgi for file service ('public' repository). */

      static cgi_dispatcher _cgi_plugin_file_server; /* cgi for every plugin file service. */

      /* Not exactly a CGI */
      static http_response* error_response(client_state *csp,
                                           const char *templatename);

      /*
       * CGI support functions
       */
      static http_response* finish_http_response(const client_state *csp,
          http_response *rsp);
      static hash_map<const char*,const char*,hash<const char*>,eqstr>* default_exports(const client_state *csp,
          const char *caller);

      static sp_err map_block_killer (hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
                                      const char *name);
      static sp_err map_block_keep   (hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
                                      const char *name);
      static sp_err map_conditional  (hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
                                      const char *name, int choose_first);

      static sp_err template_load(const client_state *csp, char ** template_ptr,
                                  const char *templatename, const char* templatedir,
                                  int recursive);
      static sp_err template_fill(char ** template_ptr,
                                  const hash_map<const char*,const char*,hash<const char*>,eqstr> *exports);
      static sp_err template_fill_str(char **template_ptr,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *exports);
      static sp_err template_fill_for_cgi(const client_state *csp,
                                          const char *templatename,
                                          const char *templatedir,
                                          hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
                                          http_response *rsp);
      static sp_err template_fill_for_cgi_str(const client_state *csp,
                                              const char *templatename,
                                              const char *templatedir,
                                              hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
                                              http_response *rsp);

      static void cgi_init_error_messages();
      static http_response* cgi_error_memory();
      static sp_err cgi_redirect(http_response *rsp, const char *target);

      static sp_err cgi_error_no_template(const client_state *csp,
                                          http_response *rsp,
                                          const char *template_name);
      static sp_err cgi_error_bad_param(const client_state *csp,
                                        http_response *rsp,
                                        const hash_map<const char*,const char*,hash<const char*>,eqstr> *param_list,
                                        const std::string &def="");
      static sp_err cgi_error_disabled(const client_state *csp,
                                       http_response *rsp);
      static sp_err cgi_error_plugin(const client_state *csp,
                                     http_response *rsp,
                                     const sp_err &error_to_report,
                                     const std::string &pname);
      static sp_err cgi_error_unknown(const client_state *csp,
                                      http_response *rsp,
                                      sp_err error_to_report,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *param_list,
                                      const std::string &def="");

      static sp_err get_number_param(client_state *csp,
                                     const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                     char *name,
                                     unsigned *pvalue);
      static sp_err get_string_param(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                     const char *param_name,
                                     const char **pparam);
      static char get_char_param(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                 const char *param_name);

      static const char* grep_cgi_referrer(const client_state *csp);
      static int referrer_is_safe(const client_state *csp);

      static http_response _cgi_error_memory_response;

      static sp_err client_accept_language(client_state *csp, char **header);

      /*
       * Text generators
       */
      static void get_http_time(int time_offset, char *buf, size_t buffer_size);
      static void get_locale_time(char *buf, size_t buffer_size);
      static char* add_help_link(const char *item, proxy_configuration *config);
      static char* make_menu(const char *self, const unsigned feature_flags);
      static char* make_plugins_list();
      static char* dump_map(const hash_map<const char*,const char*,hash<const char*>,eqstr> *the_map);

      /*
       * Ad replacement images
       */
      static const char _image_pattern_data[];
      static size_t  _image_pattern_length;
      static const char _image_blank_data[];
      static size_t _image_blank_length;

  };

} /* end of namespace. */

#endif
