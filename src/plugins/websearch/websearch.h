/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 **/
 
#ifndef WEBSEARCH_H
#define WEBSEARCH_H

#define NO_PERL // we do not use Perl.

#include "plugin.h"
#include "search_snippet.h"
#include "query_context.h"
#include "websearch_configuration.h"
#include "websearch.h" // for configuration.

#include <string>

using sp::client_state;
using sp::http_response;
using sp::sp_err;
using sp::plugin;

namespace seeks_plugins
{
   
   class websearch : public plugin
     {
      public:
	websearch();
	
	~websearch();

	/* cgi calls. */
	static sp_err cgi_websearch_hp(client_state *csp,
				       http_response *rsp,
				       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	static sp_err cgi_websearch_search_hp_css(client_state *csp,
						  http_response *rsp,
						  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	static sp_err cgi_websearch_search(client_state *csp,
					   http_response *rsp,
					   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

	/* websearch. */
	static sp_err perform_websearch(client_state *csp,
					http_response *rsp,
					const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

	static query_context* lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	/* error handling. */
	static sp_err failed_ses_connect(client_state *csp, http_response *rsp);
	
      public:
	static websearch_configuration *_wconfig;
	static hash_map<uint32_t,query_context*,hash<uint32_t> > _active_qcontexts;
     };
   
} /* end of namespace. */

#endif
