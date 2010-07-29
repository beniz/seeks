/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "img_websearch.h"
#include "img_sort_rank.h"
#include "websearch.h" // websearch plugin.
#include "errlog.h"
#include "cgi.h"
#include "cgisimple.h"
#include "miscutil.h"
#include "json_renderer.h"
#include "static_renderer.h"
#include "seeks_proxy.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>

using namespace sp;

namespace seeks_plugins
{
   img_websearch_configuration* img_websearch::_iwconfig = NULL;
   hash_map<uint32_t,query_context*,id_hash_uint> img_websearch::_active_img_qcontexts
     = hash_map<uint32_t,query_context*,id_hash_uint>();
   
   img_websearch::img_websearch()
     : plugin()
       {
	  _name = "image-websearch";
	  _version_major = "0";
	  _version_minor = "1";
	  
	  // config filename.
	  if (seeks_proxy::_datadir.empty())
	    _config_filename = plugin_manager::_plugin_repository + "img_websearch/img-websearch-config";
	  else _config_filename = seeks_proxy::_datadir + "/plugins/img_websearch/img-websearch-config";
	  
#ifdef SEEKS_CONFIGDIR
	  struct stat stFileInfo;
	  if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
	    {
	       _config_filename = SEEKS_CONFIGDIR "/img-websearch-config";
	    }
#endif
	  
	  if (img_websearch::_iwconfig == NULL)
	    img_websearch::_iwconfig = new img_websearch_configuration(_config_filename);
	  
	  // cgi dispatchers.
	  cgi_dispatcher *cgid_img_wb_search
	    = new cgi_dispatcher("search_img", &img_websearch::cgi_img_websearch_search, NULL, TRUE);
	  _cgi_dispatchers.push_back(cgid_img_wb_search);
       }
   
   img_websearch::~img_websearch()
     {
     }
      
   // CGI calls.
   sp_err img_websearch::cgi_img_websearch_search(client_state *csp,
						  http_response *rsp,
						  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
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
	     
	     sp_err err = SP_ERR_OK;
	     if (strcmp(action,"expand") == 0 || strcmp(action,"page") == 0)
	       err = img_websearch::perform_img_websearch(csp,rsp,parameters);
	     else if (strcmp(action,"similarity") == 0)
	       err = img_websearch::cgi_img_websearch_similarity(csp,rsp,parameters);
	     return err;
	  }
	else return SP_ERR_OK;
     }			  
   
   sp_err img_websearch::cgi_img_websearch_similarity(client_state *csp, http_response *rsp,
						      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	static std::string tmpl_name = "templates/seeks_result_template.html";
	
	if (!parameters->empty())
	  {
	     img_query_context *qc = dynamic_cast<img_query_context*>(websearch::lookup_qc(parameters,csp,_active_img_qcontexts));
	     
	     if (!qc)
	       {
		  // no cache, (re)do the websearch first.
		  sp_err err = img_websearch::perform_img_websearch(csp,rsp,parameters,false);
		  if (err != SP_ERR_OK)
		    return err;
		  qc = dynamic_cast<img_query_context*>(websearch::lookup_qc(parameters,csp,_active_img_qcontexts));
		  if (!qc)
		    return SP_ERR_MEMORY;
	       }
	     const char *id = miscutil::lookup(parameters,"id");
	     
	     seeks_proxy::mutex_lock(&qc->_qc_mutex);
	     qc->_lock = true;
	     img_search_snippet *ref_sp = NULL;
	     
	     img_sort_rank::score_and_sort_by_similarity(qc,id,ref_sp,qc->_cached_snippets);
	     
	     if (!ref_sp)
	       {
		  qc->_lock = false;
		  seeks_proxy::mutex_unlock(&qc->_qc_mutex);
		  return SP_ERR_OK;
	       }
	     
	     const char *output = miscutil::lookup(parameters,"output");
	     sp_err err = SP_ERR_OK;
	     if (!output || strcmp(output,"html")==0)
	       err = static_renderer::render_result_page_static(qc->_cached_snippets,
								csp,rsp,parameters,qc,
								(seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository + tmpl_name
								 : std::string(seeks_proxy::_datadir) + "plugins/img_websearch/" + tmpl_name),
								"/search_img?");
	     else
	       {
		  csp->_content_type = CT_JSON;
		  err = json_renderer::render_json_results(qc->_cached_snippets,
							   csp,rsp,parameters,qc,0.0);
		  qc->_lock = false;
		  seeks_proxy::mutex_unlock(&qc->_qc_mutex);
		  return err;
	       }
	     
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
      
   // internal functions.
   sp_err img_websearch::perform_img_websearch(client_state *csp,
					       http_response *rsp,
					       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					       bool render)
     {
	static std::string tmpl_name = "templates/seeks_result_template.html";
	
	// time measure, returned.
	// XXX: not sure whether it is functional on all *nix platforms.
	struct tms st_cpu;
	struct tms en_cpu;
	clock_t start_time = times(&st_cpu);
		
	// lookup a cached context for the incoming query.
	img_query_context *qc = dynamic_cast<img_query_context*>(websearch::lookup_qc(parameters,csp,_active_img_qcontexts));
		
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
	     else if (strcmp(action,"page") == 0)
	       {
		  // XXX: update other parameters, as needed, qc vs parameters.
		  qc->update_parameters(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters));
	       }
	  }
	else
	  {
	     // new context, whether we're expanding or not doesn't matter, we need
	     // to generate snippets first.
	     expanded = true;
	     qc = new img_query_context(parameters,csp->_headers);
	     qc->register_qc();
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
	     img_sort_rank::sort_rank_and_merge_snippets(qc,qc->_cached_snippets);
	     qc->_compute_tfidf_features = true;
	     qc->_lock = false;
	     seeks_proxy::mutex_unlock(&qc->_qc_mutex);
	  }
		
	// rendering.
	seeks_proxy::mutex_lock(&qc->_qc_mutex);
	qc->_lock = true;
	
	// time measured before rendering, since we need to write it down.
	clock_t end_time = times(&en_cpu);
	double qtime = (end_time-start_time)/websearch::_cl_sec;
	if (qtime<0)
	  qtime = -1.0; // unavailable.
	
	if (!render)
	  {
	     qc->_lock = false;
	     seeks_proxy::mutex_unlock(&qc->_qc_mutex);    
	     return SP_ERR_OK;
	  }
	const char *output = miscutil::lookup(parameters,"output");
	sp_err err = SP_ERR_OK;
	if (!output || strcmp(output,"html")==0)
	  err = static_renderer::render_result_page_static(qc->_cached_snippets,
							   csp,rsp,parameters,qc,
							   (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository + tmpl_name 
							    : std::string(seeks_proxy::_datadir) + "plugins/img_websearch/" + tmpl_name),
							   "/search_img?");
	else
	  {
	     csp->_content_type = CT_JSON;
	     err = json_renderer::render_json_results(qc->_cached_snippets,
						      csp,rsp,parameters,qc,
						      qtime);
	  }
	
	// unlock or destroy the query context.
	qc->_lock = false;
	seeks_proxy::mutex_unlock(&qc->_qc_mutex);
	/* if (qc->empty())
	  delete qc; */
	
	return err;
     }

   /* auto-registration. */
   extern "C"
     {
	plugin* makeiw()
	  {
	     return new img_websearch;
	  }
     }
   
   class proxy_autoiw
     {
      public:
	proxy_autoiw()
	  {
	     plugin_manager::_factory["image-websearch"] = makeiw;
	  }
     };
   
   proxy_autoiw _p; // one instance, instanciated when dl-opening.
      
} /* end of namespace. */
