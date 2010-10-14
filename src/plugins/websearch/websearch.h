/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009, 2010 Emmanuel Benazera, juban@free.fr
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

#ifndef WEBSEARCH_H
#define WEBSEARCH_H

#define NO_PERL // we do not use Perl.

#include "plugin.h"
#include "search_snippet.h"
#include "query_context.h"
#include "websearch_configuration.h"
#include "mutexes.h"

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

	virtual void start();
	virtual void stop() {};
	
	/* cgi calls. */
	static sp_err cgi_websearch_hp(client_state *csp,
				       http_response *rsp,
				       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	static sp_err cgi_websearch_search_hp_css(client_state *csp,
						  http_response *rsp,
						  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	static sp_err cgi_websearch_search_css(client_state *csp,
					       http_response *rsp,
					       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	static sp_err cgi_websearch_opensearch_xml(client_state *csp,
						   http_response *rsp,
						   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
		
	static sp_err cgi_websearch_search(client_state *csp,
					   http_response *rsp,
					   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

	static sp_err cgi_websearch_search_cache(client_state *csp,
						 http_response *rsp,
						 const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

	static sp_err cgi_websearch_neighbors_url(client_state *csp, http_response *rsp,
						  const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

	static sp_err cgi_websearch_neighbors_title(client_state *csp, http_response *rsp,
						    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);
	
	static sp_err cgi_websearch_clustered_types(client_state *csp, http_response *rsp,
						    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);
	
	static sp_err cgi_websearch_similarity(client_state *csp, http_response *rsp,
					       const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);
	
	static sp_err cgi_websearch_clusterize(client_state *csp, http_response *rsp,
					       const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);
	
	/* websearch. */
	static sp_err perform_websearch(client_state *csp,
					http_response *rsp,
					const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					bool render=true);

	static query_context* lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					client_state *csp);
	
	static query_context* lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					client_state *csp, hash_map<uint32_t,query_context*,id_hash_uint> &active_contexts);
	
	/* error handling. */
	static sp_err failed_ses_connect(client_state *csp, http_response *rsp);
	
      public:
	static websearch_configuration *_wconfig;
	static hash_map<uint32_t,query_context*,id_hash_uint> _active_qcontexts;
	static double _cl_sec; // clock ticks per second.
     
	/* dependent plugins. */
      public:
	static plugin *_qc_plugin; /**< query capture plugin. */ 
	static bool _qc_plugin_activated;
	static plugin *_cf_plugin; /**< (collaborative) filtering plugin. */
	static bool _cf_plugin_activated;
     
	/* multithreading. */
      private:
	static sp_mutex_t _context_mutex;
     };
   
} /* end of namespace. */

#endif
