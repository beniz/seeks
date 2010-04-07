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

#include "websearch.h"
#include "cgi.h"
#include "cgisimple.h"
#include "encode.h"
#include "miscutil.h"
#include "errlog.h"
#include "query_interceptor.h"
#include "proxy_configuration.h"
#include "se_parser.h"
#include "se_handler.h"
#include "static_renderer.h"
#include "sort_rank.h"
#include "content_handler.h"
#include "oskmeans.h"
#include "mrf.h"

#include <sys/stat.h> 
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
	  _version_minor = "2";
	  

	  if (seeks_proxy::_datadir.empty())
	    _config_filename = plugin_manager::_plugin_repository + "websearch/websearch-config";
	  else
	    _config_filename = seeks_proxy::_datadir + "/plugins/websearch/websearch-config";
	  
#ifdef SEEKS_CONFIGDIR
          struct stat stFileInfo; 
	  if (!stat(_config_filename.c_str(), &stFileInfo)  == 0){
	     _config_filename = SEEKS_CONFIGDIR "/websearch-config";
	  }
#endif

	  if (websearch::_wconfig == NULL)
	    websearch::_wconfig = new websearch_configuration(_config_filename);
	  _configuration = websearch::_wconfig;
	
	  // load tagging patterns.
	  search_snippet::load_patterns();
	  
	  // cgi dispatchers.
	  cgi_dispatcher *cgid_wb_hp
	    = new cgi_dispatcher("websearch-hp", &websearch::cgi_websearch_hp, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_hp);

	  cgi_dispatcher *cgid_wb_seeks_hp_search_css
	    = new cgi_dispatcher("seeks_hp_search.css", &websearch::cgi_websearch_search_hp_css, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_seeks_hp_search_css);
	  	  
	  cgi_dispatcher *cgid_wb_seeks_search_css
	    = new cgi_dispatcher("seeks_search.css", &websearch::cgi_websearch_search_css, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_seeks_search_css);
	  
	  cgi_dispatcher *cgid_wb_opensearch_xml
	    = new cgi_dispatcher("opensearch.xml", &websearch::cgi_websearch_opensearch_xml, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_opensearch_xml);
	  
	  cgi_dispatcher *cgid_wb_search
	    = new cgi_dispatcher("search", &websearch::cgi_websearch_search, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_search);

	  cgi_dispatcher *cgid_wb_search_cache
	    = new cgi_dispatcher("search_cache", &websearch::cgi_websearch_search_cache, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_search_cache);

	  cgi_dispatcher *cgid_wb_search_similarity
	    = new cgi_dispatcher("search_similarity", &websearch::cgi_websearch_similarity, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_search_similarity);
	  
	  cgi_dispatcher *cgid_wb_search_clusterize
	    = new cgi_dispatcher("search_clusterize", &websearch::cgi_websearch_clusterize, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_wb_search_clusterize);
	  
	  // external cgi.
	  static_renderer::register_cgi(this);
	  
	  // interceptor plugins.
	  _interceptor_plugin = new query_interceptor(this);

	  // initializes the libxml for multithreaded usage.
	  se_parser::libxml_init();
       
	  // initialize mrf.
	  mrf::init_delims();
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
	sp_err err = static_renderer::render_hp(csp,rsp);
	
	return err;
     }

   sp_err websearch::cgi_websearch_search_hp_css(client_state *csp,
						 http_response *rsp,
						 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	assert(csp);
	assert(rsp);
	assert(parameters);
	
	std::string seeks_search_css_str = "websearch/templates/css/seeks_hp_search.css";
	hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
	  = static_renderer::websearch_exports(csp);
	csp->_content_type = CT_CSS;
	sp_err err = cgi::template_fill_for_cgi_str(csp,seeks_search_css_str.c_str(),
						    (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
						     : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
						     exports,rsp);
		
	if (err != SP_ERR_OK)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "Could not load seeks_hp_search.css");
	  }
	
	rsp->_is_static = 1;
	
	return SP_ERR_OK;
     }
      
   sp_err websearch::cgi_websearch_search_css(client_state *csp,
					      http_response *rsp,
					      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	assert(csp);
	assert(rsp);
	assert(parameters);
	
	std::string seeks_search_css_str = "websearch/templates/css/seeks_search.css";
	hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
	  = static_renderer::websearch_exports(csp);
	csp->_content_type = CT_CSS;
	sp_err err = cgi::template_fill_for_cgi_str(csp,seeks_search_css_str.c_str(),
						    (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
						     : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
						    exports,rsp);
	
	if (err != SP_ERR_OK)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "Could not load seeks_search.css");
	  }
	
	rsp->_is_static = 1;
	
	return SP_ERR_OK;
     }
   
   sp_err websearch::cgi_websearch_opensearch_xml(client_state *csp,
						  http_response *rsp,
						  const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	assert(csp);
	assert(rsp);
	assert(parameters);
	
	std::string seeks_opensearch_xml_str = "websearch/templates/opensearch.xml";
	hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
	  = static_renderer::websearch_exports(csp);
	csp->_content_type = CT_XML;
	sp_err err = cgi::template_fill_for_cgi(csp,seeks_opensearch_xml_str.c_str(),
						(seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
						 : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
						exports,rsp);
	
	if (err != SP_ERR_OK)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "Could not load opensearch.xml");
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
	     if (!query || strlen(query) == 0)
	       {
		  // return websearch homepage instead.
		  return websearch::cgi_websearch_hp(csp,rsp,parameters);
	       }
	     else se_handler::preprocess_parameters(parameters); // preprocess the query...
	     
	     // perform websearch or other requested action.
	     const char *action = miscutil::lookup(parameters,"action");
	     if (!action)
	       return websearch::cgi_websearch_hp(csp,rsp,parameters);
	     
	     /**
	      * Action can be of type:
	      * - "expand": requests an expansion of the search results, expansion horizon is 
	      *           specified by parameter "expansion".
	      * - "page": requests a result page, already in cache.
	      * - "similarity": requests a reordering of results, in decreasing order from the
	      *                 specified search result, identified by parameter "id".
	      * - "clusterize": requests the forming of a cluster of results, the number of specified
	      *                 clusters is given by parameter "clusters".
	      * - "urls": requests a grouping by url.
	      * - "titles": requests a grouping by title.
	      */
	     sp_err err = SP_ERR_OK;
	     if (strcmp(action,"expand") == 0 || strcmp(action,"page") == 0)
	       err = websearch::perform_websearch(csp,rsp,parameters);
	     else if (strcmp(action,"similarity") == 0)
	       err = websearch::cgi_websearch_similarity(csp,rsp,parameters);
	     else if (strcmp(action,"clusterize") == 0)
	       err = websearch::cgi_websearch_clusterize(csp,rsp,parameters);
	     else if (strcmp(action,"urls") == 0)
	       err = websearch::cgi_websearch_neighbors_url(csp,rsp,parameters);
	     else if (strcmp(action,"titles") == 0)
	       err = websearch::cgi_websearch_neighbors_title(csp,rsp,parameters);
	     else if (strcmp(action,"types") == 0)
	       err = websearch::cgi_websearch_clustered_types(csp,rsp,parameters);
	     else return websearch::cgi_websearch_hp(csp,rsp,parameters);
	     
	     return err;
	  }
	else 
	  {
	     return SP_ERR_OK;
	  }
     }

   sp_err websearch::cgi_websearch_search_cache(client_state *csp, http_response *rsp,
						const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	if (!parameters->empty())
	  {
	     const char *url = miscutil::lookup(parameters,"url"); // grab the url.
	     query_context *qc = websearch::lookup_qc(parameters);
	     
	     if (!qc)
	       {
		  // redirect to the url.
		  cgi::cgi_redirect(rsp,url);
		  return SP_ERR_OK;
	       }
	     
	     seeks_proxy::mutex_lock(&qc->_qc_mutex);
	     qc->_lock = true;
	     
	     search_snippet *sp = NULL;
	     if ((sp = qc->get_cached_snippet(url))!=NULL 
		 && (sp->_cached_content!=NULL))
	       {
		  errlog::log_error(LOG_LEVEL_INFO,"found cached url %s",url);
		  
		  rsp->_body = strdup(sp->_cached_content->c_str());
		  rsp->_is_static = 1;
		  
		  qc->_lock = false;
		  seeks_proxy::mutex_unlock(&qc->_qc_mutex);
		  
		  return SP_ERR_OK;
	       }
	     else
	       {
		  // redirect to the url.
		  cgi::cgi_redirect(rsp,url);
		  
		  qc->_lock = false;
		  seeks_proxy::mutex_unlock(&qc->_qc_mutex);
		  
		  return SP_ERR_OK;
	       }
	  }
	else
	  {
	     // do nothing. beware.
	     return SP_ERR_OK;
	  }
     }

   sp_err websearch::cgi_websearch_neighbors_url(client_state *csp, http_response *rsp,
						 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	if (!parameters->empty())
	  {
	     query_context *qc = websearch::lookup_qc(parameters);
	     
	     if (!qc)
	       {
		  // no cache, (re)do the websearch first.
		  sp_err err = websearch::perform_websearch(csp,rsp,parameters);
		  qc = websearch::lookup_qc(parameters);
		  if (err != SP_ERR_OK)
		    return err;
	       }
	   
	     seeks_proxy::mutex_lock(&qc->_qc_mutex);
	     qc->_lock = true;
	     	     
	     // render result page.
	     sp_err err = static_renderer::render_neighbors_result_page(csp,rsp,parameters,qc,0); // 0: urls.
	     
	     qc->_lock = false;
	     seeks_proxy::mutex_unlock(&qc->_qc_mutex);
	     
	     return err;
	  }
	else return SP_ERR_OK;
     }
   
   sp_err websearch::cgi_websearch_neighbors_title(client_state *csp, http_response *rsp,
						   const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	if (!parameters->empty())
	  {
	     query_context *qc = websearch::lookup_qc(parameters);
	     
	     if (!qc)
	       {
		  // no cache, (re)do the websearch first.
		  sp_err err = websearch::perform_websearch(csp,rsp,parameters);
		  qc = websearch::lookup_qc(parameters);
		  if (err != SP_ERR_OK)
		    return err;
	       }
	     
	     seeks_proxy::mutex_lock(&qc->_qc_mutex);
	     qc->_lock = true;
	     
	     // render result page.
	     sp_err err = static_renderer::render_neighbors_result_page(csp,rsp,parameters,qc,1); // 1: titles.
	     
	     qc->_lock = false;
	     seeks_proxy::mutex_unlock(&qc->_qc_mutex);
	      
	     return err;
	  }
	else return SP_ERR_OK;
     }

   sp_err websearch::cgi_websearch_clustered_types(client_state *csp, http_response *rsp,
						   const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	if (!parameters->empty())
	  {
	     query_context *qc = websearch::lookup_qc(parameters);
	     if (!qc)
	       {
		  // no cache, (re)do the websearch first.
		  sp_err err = websearch::perform_websearch(csp,rsp,parameters);
		  qc = websearch::lookup_qc(parameters);
		  if (err != SP_ERR_OK)
		    return err;
	       }
	     
	     seeks_proxy::mutex_lock(&qc->_qc_mutex);
	     qc->_lock = true;
	     
	     // regroup search snippets by types.
	     cluster *clusters;
	     short K = 0;
	     sort_rank::group_by_types(qc,clusters,K);
	     
	     // render result page.
	     sp_err err = static_renderer::render_clustered_result_page_static(clusters,K,
									       csp,rsp,parameters,qc);
	     
	     qc->_lock = false;
	     seeks_proxy::mutex_unlock(&qc->_qc_mutex);
	     
	     return err;
	  }
	else return SP_ERR_OK;
     }
      
   sp_err websearch::cgi_websearch_similarity(client_state *csp, http_response *rsp,
					      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	if (!parameters->empty())
	  {
	     query_context *qc = websearch::lookup_qc(parameters);
	     
	     if (!qc)
	       {
		  // no cache, (re)do the websearch first.
		  sp_err err = websearch::perform_websearch(csp,rsp,parameters);
		  if (err != SP_ERR_OK)
		    return err;
		  qc = websearch::lookup_qc(parameters);
		  if (!qc)
		    return SP_ERR_MEMORY;
	       }
	     const char *id = miscutil::lookup(parameters,"id");
	     
	     seeks_proxy::mutex_lock(&qc->_qc_mutex);
	     qc->_lock = true;
	     search_snippet *ref_sp = NULL;
	     sort_rank::score_and_sort_by_similarity(qc,id,ref_sp,qc->_cached_snippets);
	     
	     if (!ref_sp)
	       return SP_ERR_OK;
	     
	     sp_err err = static_renderer::render_result_page_static(qc->_cached_snippets,
								     csp,rsp,parameters,qc);
	      
	     // reset scores.
	     std::vector<search_snippet*>::iterator vit = qc->_cached_snippets.begin();
	     while(vit!=qc->_cached_snippets.end())
	       {
		  (*vit)->_seeks_ir = 0;
		  ++vit;
	       }
	      
	     ref_sp->set_similarity_link(); // reset sim_link.
	     qc->_lock = false;
	     seeks_proxy::mutex_unlock(&qc->_qc_mutex);
	     
	     return err;
	  }
	else return SP_ERR_OK;	
     }

   sp_err websearch::cgi_websearch_clusterize(client_state *csp, http_response *rsp,
					      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	if (!parameters->empty())
	  {
	     query_context *qc = websearch::lookup_qc(parameters);
	     
	     if (!qc)
	       {
		  // no cache, (re)do the websearch first.
		  sp_err err = websearch::perform_websearch(csp,rsp,parameters);
		  qc = websearch::lookup_qc(parameters);
		  if (err != SP_ERR_OK)
		    return err;
	       }
	     
	     seeks_proxy::mutex_lock(&qc->_qc_mutex);
	     qc->_lock = true;
	     
	     if (websearch::_wconfig->_content_analysis)
	       content_handler::fetch_all_snippets_content_and_features(qc);
	     else content_handler::fetch_all_snippets_summary_and_features(qc);
	     
	     const char *nclust_str = miscutil::lookup(parameters,"clusters");
	     int nclust = 2;
	     if (nclust_str)
	       nclust = atoi(nclust_str);
	     oskmeans km(qc,qc->_cached_snippets,nclust); // nclust clusters+ 1 garbage for now...
	     km.clusterize();
	     km.post_processing();
	     
	     sp_err err = static_renderer::render_clustered_result_page_static(km._clusters,km._K,
									       csp,rsp,parameters,qc);
	     
	     // reset scores.
	     std::vector<search_snippet*>::iterator vit = qc->_cached_snippets.begin();
	     while(vit!=qc->_cached_snippets.end())
	       {
		  (*vit)->_seeks_ir = 0;
		  ++vit;
	       }
	     
	     qc->_lock = false;
	     seeks_proxy::mutex_unlock(&qc->_qc_mutex);
	     
	     return err;
	  }
	else return SP_ERR_OK;
     }
      
   /*- internal functions. -*/
   sp_err websearch::perform_websearch(client_state *csp, http_response *rsp,
				      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
     // lookup a cached context for the incoming query.
     query_context *qc = websearch::lookup_qc(parameters);
     
     // check whether search is expanding or the user is leafing through pages.
     const char *action = miscutil::lookup(parameters,"action");
     
     // expansion: we fetch more pages from every search engine.
     bool expanded = false;
     if (qc) // we already had a context for this query.
       {
	  if (strcmp(action,"expand") == 0)
	    {
	       expanded = true;
	       
	       seeks_proxy::mutex_lock(&qc->_qc_mutex);
	       qc->_lock = true;
	       qc->generate(csp,rsp,parameters);
	       qc->_lock = false;
	       seeks_proxy::mutex_unlock(&qc->_qc_mutex);
	    }
       }
     else 
       {
	  // new context, whether we're expanding or not doesn't matter, we need
	  // to generate snippets first.
	  expanded = true;
	  qc = new query_context(parameters,csp->_headers);
	  seeks_proxy::mutex_lock(&qc->_qc_mutex);
	  qc->_lock = true;
	  qc->generate(csp,rsp,parameters);
	  qc->_lock = false;
	  seeks_proxy::mutex_unlock(&qc->_qc_mutex);
       }
     
     // sort and rank search snippets.
     if (expanded)
       {
	  seeks_proxy::mutex_lock(&qc->_qc_mutex);
	  qc->_lock = true;
	  sort_rank::sort_merge_and_rank_snippets(qc,qc->_cached_snippets);
	  qc->_compute_tfidf_features = true;
	  qc->_lock = false;
	  seeks_proxy::mutex_unlock(&qc->_qc_mutex);
       }
     
     seeks_proxy::mutex_lock(&qc->_qc_mutex);
     qc->_lock = true;
     
     // render the page (static).
     sp_err err = static_renderer::render_result_page_static(qc->_cached_snippets,
							     csp,rsp,parameters,qc);

     // unlock the query context.
     qc->_lock = false;
     seeks_proxy::mutex_unlock(&qc->_qc_mutex);
     
    // XXX: catch errors.
     return err;
  }

   query_context* websearch::lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
     {
	std::string query;
	std::string url_enc_query;
	uint32_t query_hash = query_context::hash_query_for_context(parameters,query,url_enc_query);
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
	err = cgi::template_fill_for_cgi_str(csp,"connect-failed",csp->_config->_templdir,
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
