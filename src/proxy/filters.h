/*********************************************************************
 * Purpose     :  Declares functions to parse/crunch headers and pages.
 *                Functions declared include:
 *                   `acl_addr', `add_stats', `block_acl', `block_imageurl',
 *                   `block_url', `url_actions', `filter_popups', `forward_url'
 *                   `ij_untrusted_url', `intercept_url', `re_process_buffer',
 *                   `show_proxy_args', and `trust_url'
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001, 2004 the SourceForge
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

#ifndef FILTERS_H
#define FILTERS_H

#include "proxy_dts.h"
#include "pcrs.h"

#include <stdint.h>
#include <arpa/inet.h>

namespace sp
{

  /* forward declarations. */
  class access_control_addr;
  class client_state;
  class http_request;
  class http_response;
  class current_action_spec;
  class url_actions;
  class url_spec;

  typedef char *(*filter_function_ptr)(client_state*);

  class filters
  {
    public:
      /*
       * ACL checking
       */
#ifdef FEATURE_ACL
      static int block_acl(const access_control_addr *dst, const client_state *csp);
      static int acl_addr(const char *aspec, access_control_addr *aca);
#endif /* def FEATURE_ACL */

      /*
       * Interceptors
       */
      // static http_response* redirect_url(client_state *csp);

      /*
       * Request inspectors
       */
#ifdef FEATURE_IMAGE_BLOCKING
      static int is_imageurl(const client_state *csp);
#endif /* def FEATURE_IMAGE_BLOCKING */
//	static int connect_port_is_forbidden(const client_state *csp);

      /*
       * Determining applicable actions
       */
      /* static void get_url_actions(client_state *csp,
      			    http_request *http);
      static void apply_url_actions(current_action_spec *action,
      			      http_request *http,
      			      url_actions *b); */
      /*
       * Determining parent proxies
       */
      static const forward_spec* forward_url(client_state *csp,
                                             const http_request *http);

      /*
       * Content modification
       */
      static char* execute_content_filter(client_state *csp, filter_function_ptr content_filter);
      static char* rewrite_url(char *old_url, const char *pcrs_command);
      static char* get_last_url(char *subject, const char *redirect_mode);

      static pcrs_job *compile_dynamic_pcrs_job_list(const client_state *csp, const re_filterfile_spec *b);

      static int content_filters_enabled(const current_action_spec *action);

      /*
       * Handling Max-Forwards:
       */
      static http_response *direct_response(client_state *csp);

      /* private. */
    private:
      static sp_err remove_chunked_transfer_coding(char *buffer, size_t *size);

    public:
      static sp_err prepare_for_filtering(client_state *csp);

#ifdef FEATURE_ACL
# ifdef HAVE_RFC2553
      static int sockaddr_storage_to_ip(const struct sockaddr_storage *addr,
                                        uint8_t **ip, unsigned int *len,
                                        in_port_t **port);
      static int match_sockaddr(const struct sockaddr_storage *network,
                                const struct sockaddr_storage *netmask,
                                const struct sockaddr_storage *address);
# endif // RFC2553
#endif // ACL

    private:
      static char* pcrs_filter_response(client_state *csp);

      //static forward_spec* get_forward_override_settings(client_state *csp);

      /*
       * Solaris fix:
       */
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
  };

} /* end of namespace. */

#endif /* ndef FILTERS_H */
