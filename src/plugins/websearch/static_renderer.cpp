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

#include "static_renderer.h"
#include "mem_utils.h"
#include "plugin_manager.h"
#include "cgi.h"
#include "cgisimple.h"
#include "miscutil.h"
#include "encode.h"
#include "errlog.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using sp::miscutil;
using sp::cgi;
using sp::cgisimple;
using sp::plugin_manager;
using sp::cgi_dispatcher;
using sp::encode;
using sp::errlog;

namespace seeks_plugins
{
   void static_renderer::register_cgi(websearch *wbs)
     {
     }
   
  /*- rendering. -*/
  sp_err static_renderer::render_result_page_static(const std::vector<search_snippet*> &snippets,
						    client_state *csp, http_response *rsp,
						    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
						    const query_context *qc)
  {
     static const char *result_tmpl_name = "websearch/templates/seeks_result_template.html";
     
     hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
       = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
     
     // query.
     const char *query = encode::html_encode(miscutil::lookup(parameters,"q"));
     miscutil::add_map_entry(exports,"$fullquery",1,query,1);
     
     // clean query.
     std::string query_clean = se_handler::cleanup_query(std::string(query));
     miscutil::add_map_entry(exports,"$qclean",1,query_clean.c_str(),1);
     std::vector<std::string> words;
     miscutil::tokenize(query_clean,words," "); // tokenize query for highlighting keywords.
     
     const char *current_page = miscutil::lookup(parameters,"page");
     int cp = atoi(current_page);
     if (cp == 0) cp = 1;

     // suggestions.
     if (!qc->_suggestions.empty())
       {
	  std::string suggestion_str = "Suggestion:&nbsp;<a href=\"";
	  // for now, let's grab the first suggestion only.
	  std::string suggested_q_str = qc->_suggestions[0];
	  miscutil::replace_in_string(suggested_q_str," ","+");
	  suggestion_str += "http://s.s/search?q=" + suggested_q_str + "&expansion=1&action=expand";
	  suggestion_str += "\">";
	  const char *sugg_enc = encode::html_encode(qc->_suggestions[0].c_str());
	  suggestion_str += std::string(sugg_enc);
	  free_const(sugg_enc);
	  suggestion_str += "</a>";
	  miscutil::add_map_entry(exports,"$xxsugg",1,suggestion_str.c_str(),1);
       }
     else miscutil::add_map_entry(exports,"$xxsugg",1,strdup(""),0);
     
     // TODO: check whether we have some results.
     /* if (snippets->empty())
       {
	  // no results found.
	  std::string no_results = "";
	  return SP_ERR_OK;
       } */
     
     // search snippets.
     std::string snippets_str;
     size_t snisize = std::min(cp*websearch::_wconfig->_N,(int)snippets.size());
     size_t snistart = (cp-1) * websearch::_wconfig->_N;
     
     for (size_t i=snistart;i<snisize;i++)
       {
	  snippets_str += snippets.at(i)->to_html_with_highlight(words);
       }
     miscutil::add_map_entry(exports,"$search_snippets",1,snippets_str.c_str(),1);
     
     // current page.
     std::string cp_str = miscutil::to_string(cp);
     miscutil::add_map_entry(exports,"$xxpage",1,cp_str.c_str(),1);
     
     // expand button.
     const char *expansion = miscutil::lookup(parameters,"expansion");
     miscutil::add_map_entry(exports,"$xxexp",1,expansion,1);
     int expn = atoi(expansion)+1;
     
     std::string expn_str = miscutil::to_string(expn);
     miscutil::add_map_entry(exports,"$xxexpn",1,expn_str.c_str(),1);
     
     // next link.
     double nl = static_cast<double>(snippets.size()) / static_cast<double>(websearch::_wconfig->_N);
          
     if (cp < nl)
       {
	  std::string np_str = miscutil::to_string(cp+1);
	  std::string np_link = "<a href=\"http://s.s/search?page=" + np_str + "&q="
	    + query + "&expansion=" + std::string(expansion) + "&action=page\" id=\"search_page_next\" title=\"Next (ctrl+&gt;)\">&nbsp;</a>";
	  miscutil::add_map_entry(exports,"$xxnext",1,np_link.c_str(),1);
       }
     else miscutil::add_map_entry(exports,"$xxnext",1,strdup(""),0);
       
     // 'previous' link.
     if (cp > 1)
       {
	  std::string pp_str = miscutil::to_string(cp-1);
	  std::string pp_link = "<a href=\"http://s.s/search?page=" + pp_str + "&q=" 
	    + query + "&expansion=" + std::string(expansion) + "&action=page\"  id=\"search_page_prev\" title=\"Previous (ctrl+&lt;)\">&nbsp;</a>";
	  miscutil::add_map_entry(exports,"$xxprev",1,pp_link.c_str(),1);
       }
     else miscutil::add_map_entry(exports,"$xxprev",1,strdup(""),0);
     
     sp_err err = cgi::template_fill_for_cgi_str(csp,result_tmpl_name,plugin_manager::_plugin_repository.c_str(),
						 exports,rsp);
     
     return err;
  }

   /* sp_err static_renderer::render_neighbors_result_page(const std::vector<search_snippet*> &snippets,
							client_state *csp, http_response *rsp,
							const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
							const query_context *qc, const int &mode)
     {
	
     } */
   
   
} /* end of namespace. */
