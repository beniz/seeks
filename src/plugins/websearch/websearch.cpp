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

#include "websearch.h"
#include "cgi.h"
#include "cgisimple.h"
#include "encode.h"
#include "miscutil.h"
#include "errlog.h"
#include "query_interceptor.h"
#include "proxy_configuration.h"
#include "se_handler.h"
#include "static_renderer.h"
#include "sort_rank.h"

#include <iostream>
#include <algorithm>
#include <bitset>
#include <assert.h>

using namespace sp;

namespace seeks_plugins
{
   websearch_configuration* websearch::_wconfig = NULL;
   hash_map<uint32_t,query_context*,hash<uint32_t> > websearch::_active_qcontexts 
     = hash_map<uint32_t,query_context*,hash<uint32_t> >();
   
   websearch::websearch()
     : plugin()
       {
	  _name = "websearch";
	  
	  _version_major = "0";
	  _version_minor = "1";
	  
	  _config_filename = plugin_manager::_plugin_repository + "websearch/websearch-config";
	  if (websearch::_wconfig == NULL)
	    websearch::_wconfig = new websearch_configuration(_config_filename);
	  _configuration = websearch::_wconfig;
	  
	  // cgi dispatchers.
	  cgi_dispatcher *cgid_wb_hp
	    = new cgi_dispatcher("websearch-hp", &websearch::cgi_websearch_hp, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_hp);

	  cgi_dispatcher *cgid_wb_seeks_hp_search_css
	    = new cgi_dispatcher("seeks_hp_search.css", &websearch::cgi_websearch_search_hp_css, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_seeks_hp_search_css);
	  	  
	  cgi_dispatcher *cgid_wb_search
	    = new cgi_dispatcher("search", &websearch::cgi_websearch_search, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_search);
	  
	  // external cgi.
	  static_renderer::register_cgi(this);
	  
	  // interceptor plugins.
	  _interceptor_plugin = new query_interceptor(this);
       }

   websearch::~websearch()
     {
     }

   // CGI calls.
   sp_err websearch::cgi_websearch_hp(client_state *csp,
				      http_response *rsp,
				      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	assert(csp);
	assert(rsp);
	assert(parameters);
	
	// redirection to local file is forbidden by most browsers. Let's read the file instead.
	std::string seeks_ws_hp_str = plugin_manager::_plugin_repository + "websearch/html/seeks_ws_hp.html";
	sp_err err = cgisimple::load_file(seeks_ws_hp_str.c_str(), &rsp->_body, &rsp->_content_length);
	return err;
     }

   sp_err websearch::cgi_websearch_search_hp_css(client_state *csp,
						 http_response *rsp,
						 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	assert(csp);
	assert(rsp);
	assert(parameters);
	
	std::string seeks_search_css_str = plugin_manager::_plugin_repository + "websearch/css/seeks_hp_search.css";
	sp_err err = cgisimple::load_file(seeks_search_css_str.c_str(),&rsp->_body,&rsp->_content_length);
	
	csp->_content_type = CT_CSS;
	
	if (err != SP_ERR_OK)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "Could not load seeks_hp_search.css");
	  }
		
	rsp->_is_static = 1;
	
	return SP_ERR_OK;
     }
      
   sp_err websearch::cgi_websearch_search(client_state *csp, http_response *rsp,
					  const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	if (!parameters->empty())
	  {
	     const char *query = miscutil::lookup(parameters,"q"); // grab the query.
	     if (strlen(query) == 0)
	       {
		  // return websearch homepage instead.
		  return websearch::cgi_websearch_hp(csp,rsp,parameters);
	       }
	     else se_handler::preprocess_parameters(parameters); // preprocess the query...
	     
	     // perform websearch.
	     sp_err err = websearch::perform_websearch(csp,rsp,parameters);
	     
	     return err;
	  }
	else 
	  {
	     // TODO.
	     return SP_ERR_OK;
	  }
     }

  sp_err websearch::perform_websearch(client_state *csp, http_response *rsp,
				      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
     // lookup a cached context for the incoming query.
     query_context *qc = websearch::lookup_qc(parameters);
     
     // check whether search is expanding or the user is leafing through pages.
     const char *action = miscutil::lookup(parameters,"action");
     
     // expansion: we fetch more pages from every search engine.
     if (qc) // we already had a context for this query.
       {
	  if (strcmp(action,"expand") == 0)
	    qc->generate(csp,rsp,parameters);
       }
     else 
       {
	  // new context, whether we're expanding or not doesn't matter, we need
	  // to generate snippets first.
	  qc = new query_context(parameters);
	  qc->generate(csp,rsp,parameters);
       }
     	  
     // grab (updated) set of cached search result snippets.
     std::vector<search_snippet*> snippets = qc->_cached_snippets;
     
     // sort and rank search snippets !                                                                                 
     // TODO: strategies and configuration.                                                                                     
     std::vector<search_snippet*> unique_ranked_snippets;
     sort_rank::sort_merge_and_rank_snippets(snippets,unique_ranked_snippets);
     
     // render the page.                                                                                                
     // TODO: dynamic renderer.                                                                                                 
     sp_err err = static_renderer::render_result_page_static(unique_ranked_snippets,
							     csp,rsp,parameters,qc);

    // clear snippets.                                                                                                       
     unique_ranked_snippets.clear();

    // TODO: catch errors.                                                                                                      
     return err;
  }

   query_context* websearch::lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
     {
	std::string query;
	uint32_t query_hash = query_context::hash_query_for_context(parameters,query);
	hash_map<uint32_t,query_context*,hash<uint32_t> >::iterator hit;
	if ((hit = websearch::_active_qcontexts.find(query_hash))!=websearch::_active_qcontexts.end())
	  {
	     /**
	      * Already have a context for this query, update its flags, and return it.
	      */
	     (*hit).second->update_last_time();
	     return (*hit).second;
	  }
	else return NULL;
     }
      
   /* error handling. */
   sp_err websearch::failed_ses_connect(client_state *csp, http_response *rsp)
     {
	errlog::log_error(LOG_LEVEL_CONNECT, "connect to the search engines failed: %E");
	
	rsp->_reason = RSP_REASON_CONNECT_FAILED;
	hash_map<const char*,const char*,hash<const char*>,eqstr> *exports = cgi::default_exports(csp,NULL);
	char *path = strdup("");
	sp_err err = miscutil::string_append(&path, csp->_http._path);
	
	if (!err)
	  err = miscutil::add_map_entry(exports, "host", 1, encode::html_encode(csp->_http._host), 0);
	if (!err)
	  err = miscutil::add_map_entry(exports, "hostport", 1, encode::html_encode(csp->_http._hostport), 0);
	if (!err)
	  err = miscutil::add_map_entry(exports, "path", 1, encode::html_encode_and_free_original(path), 0);
	if (!err)
	  err = miscutil::add_map_entry(exports, "protocol", 1, csp->_http._ssl ? "https://" : "http://", 1);
	if (!err)
	  {
	     err = miscutil::add_map_entry(exports, "host-ip", 1, encode::html_encode(csp->_http._host_ip_addr_str), 0);
	     if (err)
	       {
		  /* Some failures, like "404 no such domain", don't have an IP address. */
		  err = miscutil::add_map_entry(exports, "host-ip", 1, encode::html_encode(csp->_http._host), 0);
	       }
	  }
	
	// not catching error on template filling...
	err = cgi::template_fill_for_cgi(csp,"connect-failed",csp->_config->_templdir,
					 exports,rsp);
	return err;
     }
   
   /* auto-registration */
   extern "C"
     {
	plugin* maker()
	  {
	     return new websearch;
	  }
     }
   
   class proxy_autor
     {
      public:
	proxy_autor()
	  {
	     plugin_manager::_factory["websearch-hp"] = maker; // beware: default plugin shell with no name.
	  }
     };

  proxy_autor _p; // one instance, instanciated when dl-opening. 
   
} /* end of namespace. */
