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
 
#include "query_interceptor.h"
#include "websearch.h"
#include "cgi.h"
#include "miscutil.h"
#include "errlog.h"

#include <iostream>

namespace seeks_plugins
{
   std::string query_interceptor::_p_filename = "websearch/qi_patterns";
   
   query_interceptor::query_interceptor(plugin *parent)
     :interceptor_plugin(std::string(plugin_manager::_plugin_repository
				     + query_interceptor::_p_filename).c_str(),parent)
       {
       }
   
   http_response* query_interceptor::plugin_response(client_state *csp)
     {
	// - parse intercepted query.
	hash_map<const char*,const char*,hash<const char*>,eqstr> *params
	  = parse_query(&csp->_http);
	
	if (!params)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "No parameters to intercepted query: %s%s",
			       csp->_http._host, csp->_http._path);
	     return cgi::cgi_error_memory(); // ok, this is not a memory error, but this does the job for now.
	  }
		
	// debug
	/* std::cerr << "params size: " << params->size() << std::endl;
	hash_map<const char*, const char*, hash<const char*>, eqstr>::const_iterator hit
	  = params->begin();
	while(hit!=params->end())
	  {
	     std::cerr << (*hit).first << " --> " << (*hit).second << std::endl;
	     ++hit;
	  } */
	// debug
		
	// - send it to the generic search interface.
	http_response *rsp;
	if (NULL == (rsp = new http_response()))
	  {
	     return cgi::cgi_error_memory();
	  }
	
	// - return the response to the client.
	// default query detection.
	const char *intercepted_query = miscutil::lookup(params,"q");
	//if (!intercepted_query || strlen(intercepted_query) == 0)
	// otherwise fall back onto se specific translation tables.
	
	// std::cout << "intercepted query: " << intercepted_query << std::endl;
	
	// build up query to seeks proxy.
	char *q = strdup(CGI_PREFIX);
	miscutil::string_append(&q,"search?q=");
	miscutil::string_append(&q,intercepted_query);
	miscutil::string_append(&q,"&page=1");
	miscutil::string_append(&q,"&expansion=1");
	miscutil::string_append(&q,"&action=expand");
	cgi::cgi_redirect(rsp,q);
     
	return cgi::finish_http_response(csp,rsp);
     }
   
   hash_map<const char*,const char*,hash<const char*>,eqstr>* query_interceptor::parse_query(http_request *http) const
     {
	if (http->_path)
	  {
	     hash_map<const char*,const char*,hash<const char*>,eqstr> *search_params
	       = cgi::parse_cgi_parameters(http->_path);
	     return search_params;
	  }
	else return NULL;
     }
   
   
} /* end of namespace. */
