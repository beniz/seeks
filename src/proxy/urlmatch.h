/*********************************************************************
 * Purpose     :  Declares functions to match URLs against URL
 *                patterns.
 *
 * Copyright   :   Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001-2002, 2006 the SourceForge
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

#ifndef URLMATCH_H
#define URLMATCH_H

#include "proxy_dts.h"

namespace sp
{

   enum regex_anchoring
     {
	NO_ANCHORING,
	  LEFT_ANCHORED,
	  RIGHT_ANCHORED,
	  RIGHT_ANCHORED_HOST
     };
   
   class urlmatch
     {
      public:
	static void free_http_request(http_request *http);
#ifndef FEATURE_EXTENDED_HOST_PATTERNS
	static sp_err init_domain_components(http_request *http);
#endif
	static sp_err parse_http_request(const char *req, http_request *&http);
	static sp_err parse_http_url(const char *url,
				     http_request *http,
				     int require_protocol);
#define REQUIRE_PROTOCOL 1
	
	static int url_match(const url_spec *pattern,
			     const http_request *http);
	static int match_portlist(const char *portlist, int port);
	static sp_err parse_forwarder_address(char *address, char **hostname, int *port);
     
      private:
	static sp_err compile_host_pattern(url_spec *url, const char *host_pattern);
	static int unknown_method(const char *method);
	
      public:
	static sp_err compile_pattern(const char *pattern, enum regex_anchoring anchoring,
				      url_spec *url, regex_t **regex);
	static sp_err compile_url_pattern(url_spec *url, char *buf);
	
      private:
	static int simplematch(const char *pattern, const char *text);
	static int simple_domaincmp(char **pcv, char **fv, int len);
	static int domain_match(const url_spec *pattern, const http_request *fqn);
	
      public:
	static int port_matches(const int port, const char *port_list);
	static int host_matches(const http_request *http, const url_spec *pattern);
	static int path_matches(const char *path, const url_spec *pattern);
     };
   
} /* end of namespace. */
   
#endif /* ndef URLMATCH_H */
