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

#ifndef STATIC_RENDERER_H
#define STATIC_RENDERER_H

#include "websearch.h"

namespace seeks_plugins
{
   class static_renderer
     {
      public:
	/*- cgi. -*/
	static void register_cgi(websearch *wbs);
	
	static sp_err cgi_websearch_search_css(client_state *csp,
					       http_response *rsp,
					       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	static sp_err cgi_websearch_yui_reset_fonts_grids_css(client_state *csp,
							      http_response *rsp,
							      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
							      
	static sp_err cgi_websearch_search_button(client_state *csp,
						  http_response *rsp,
						  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

	static sp_err cgi_websearch_search_expand(client_state *csp,
						  http_response *rsp,
						  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	static sp_err cgi_websearch_seeks_wb_google_icon(client_state *csp,
							 http_response *rsp,
							 const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	static sp_err cgi_websearch_seeks_wb_cuil_icon(client_state *csp,
						       http_response *rsp,
						       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	static sp_err cgi_websearch_seeks_wb_bing_icon(client_state *csp,
						       http_response *rsp,
						       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	/*- renderer. -*/
	static sp_err render_result_page_static(const std::vector<search_snippet*> &snippets,
						client_state *csp, http_response *rsp,
						const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
						const query_context *qc);
					
     };
   
} /* end of namespace. */

#endif
