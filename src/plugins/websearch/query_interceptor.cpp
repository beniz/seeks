/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
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

#include "query_interceptor.h"
#include "websearch.h"
#include "cgi.h"
#include "miscutil.h"
#include "errlog.h"
#include "encode.h"

#include <iostream>

namespace seeks_plugins
{
   std::string query_interceptor::_p_filename = "websearch/patterns/qi_patterns";
   
   query_interceptor::query_interceptor(plugin *parent)
     :interceptor_plugin((seeks_proxy::_datadir.empty() ? std::string(plugin_manager::_plugin_repository
								      + query_interceptor::_p_filename).c_str() 
			  : std::string(seeks_proxy::_datadir + "/plugins/" + query_interceptor::_p_filename).c_str()),parent)
       {
       }
   
   http_response* query_interceptor::plugin_response(client_state *csp)
     {
	// - parse intercepted query.
	
	//std::cerr << "url: " << csp->_http._url << std::endl;
	
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
	char *intercepted_query_enc = encode::url_encode(intercepted_query);
		
	if (!intercepted_query)
	  {
	     return NULL; // wrong interception, cancel.
	  }
		
	// build up query to seeks proxy.
	char *q = strdup(CGI_PREFIX);
	miscutil::string_append(&q,"search?q=");
	miscutil::string_append(&q,intercepted_query_enc);
	free(intercepted_query_enc);
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
