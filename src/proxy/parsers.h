/*********************************************************************
 * Purpose     :  Declares functions to parse/crunch headers and pages.
 *                Functions declared include:
 *                   `add_to_iob', `client_cookie_adder', `client_from',
 *                   `client_referrer', `client_send_cookie', `client_ua',
 *                   `client_uagent', `client_x_forwarded',
 *                   `client_x_forwarded_adder', `client_xtra_adder',
 *                   `content_type', `crumble', `destroy_list', `enlist',
 *                   `flush_socket', `free_http_request', `get_header',
 *                   `list_to_text', `parse_http_request', `sed',
 *                   and `server_set_cookie'.
 *
 * Copyright   :   Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001 the SourceForge
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

#ifndef PARSERS_H
#define PARSERS_H

#include "proxy_dts.h"

namespace sp
{

  /* Used for sed()'s second argument. */
#define FILTER_CLIENT_HEADERS 0
#define FILTER_SERVER_HEADERS 1

  class parsers_list;

  class parsers
  {
    public:
      static long flush_socket(sp_socket fd, struct iob *iob);
      static sp_err add_to_iob(client_state *csp, char *buf, long n);
      static sp_err decompress_iob(client_state *csp);
      static char *get_header(iob *iob);
      static char *get_header_value(const std::list<const char*> *header_list,
                                    const char *header_name);
      static sp_err sed(client_state *csp, int filter_server_headers);
      static sp_err update_server_headers(client_state *csp);
      static sp_err get_destination_from_headers(const std::list<const char*> *headers,
          http_request *http);

    private:
      static char *get_header_line(iob *iob);
      static sp_err scan_headers(client_state *csp);
      static sp_err header_tagger(client_state *csp, char *header);
      static sp_err parse_header_time(const char *header_time, time_t *result);

      static sp_err crumble                   (client_state *csp, char **header);
      static sp_err filter_header             (client_state *csp, char **header);
      static sp_err client_connection         (client_state *csp, char **header);
      static sp_err client_referrer           (client_state *csp, char **header);
      static sp_err client_uagent             (client_state *csp, char **header);
      static sp_err client_ua                 (client_state *csp, char **header);
      static sp_err client_from               (client_state *csp, char **header);
      static sp_err client_send_cookie        (client_state *csp, char **header);
      static sp_err client_x_forwarded        (client_state *csp, char **header);
      static sp_err client_accept_encoding    (client_state *csp, char **header);
      static sp_err client_te                 (client_state *csp, char **header);
      static sp_err client_max_forwards       (client_state *csp, char **header);
      static sp_err client_host               (client_state *csp, char **header);
      static sp_err client_if_modified_since  (client_state *csp, char **header);
      static sp_err client_accept_language    (client_state *csp, char **header);
      static sp_err client_if_none_match      (client_state *csp, char **header);
      static sp_err crunch_client_header      (client_state *csp, char **header);
      static sp_err client_x_filter           (client_state *csp, char **header);
      static sp_err client_range              (client_state *csp, char **header);
      static sp_err server_set_cookie         (client_state *csp, char **header);
      static sp_err server_connection         (client_state *csp, char **header);
      static sp_err server_content_type       (client_state *csp, char **header);
      static sp_err server_adjust_content_length(client_state *csp, char **header);
      static sp_err server_content_md5        (client_state *csp, char **header);
      static sp_err server_content_encoding   (client_state *csp, char **header);
      static sp_err server_transfer_coding    (client_state *csp, char **header);
      static sp_err server_http               (client_state *csp, char **header);
      static sp_err crunch_server_header      (client_state *csp, char **header);
      static sp_err server_last_modified      (client_state *csp, char **header);
      static sp_err server_content_disposition(client_state *csp, char **header);
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
      static sp_err server_save_content_length(client_state *csp, char **header);
      static sp_err server_keep_alive(client_state *csp, char **header);
      static sp_err server_proxy_connection(client_state *csp, char **header);
      static sp_err client_keep_alive(client_state *csp, char **header);
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

      static sp_err client_host_adder       (client_state *csp);
      static sp_err client_xtra_adder       (client_state *csp);
      static sp_err client_x_forwarded_for_adder(client_state *csp);
      static sp_err client_connection_header_adder(client_state *csp);
      static sp_err server_connection_adder(client_state *csp);
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
      static sp_err server_proxy_connection_adder(client_state *csp);
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

      static sp_err create_forged_referrer(char **header, const char *hostport);
      static sp_err create_fake_referrer(char **header, const char *fake_referrer);
      static sp_err handle_conditional_hide_referrer_parameter(char **header,
          const char *host,
          const int parameter_conditional_block);
      static void create_content_length_header(unsigned long long content_length,
          char *header, size_t buffer_length);

      static void string_move(char *ds, char *src);
      static void normalize_lws(char *header);

      static long int pick_from_range(long int range);

      /* variables. */
      static parsers_list _client_patterns[];
      static parsers_list _server_patterns[];

      static const add_header_func_ptr _add_client_headers[];
      static const add_header_func_ptr _add_server_headers[];
  };

  /*
   * List of functions to run on a list of headers.
   */
  class parsers_list  // parsers struct in original code.
  {
    public:
      parsers_list(const char *str, const size_t &len,
                   parser_func_ptr parser)//sp_err (*parser_func_ptr)(client_state*, char **) parser)
        :_str(str),_len(len),_parser(parser)
      {};

      ~parsers_list() {};  // beware of memory leaks...

      /** The header prefix to match */
      const char *_str;

      /** The length of the prefix to match */
      const size_t _len;

      /** The function to apply to this line */
      const parser_func_ptr _parser;
  };

} /* end of namespace. */

#endif /* ndef PARSERS_H */
