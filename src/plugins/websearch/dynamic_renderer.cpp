/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "dynamic_renderer.h"
#include "static_renderer.h"
#include "cgi.h"
#include "seeks_proxy.h"
#include "plugin_manager.h"
#include "errlog.h"

using sp::cgi;
using sp::seeks_proxy;
using sp::plugin_manager;
using sp::errlog;

namespace seeks_plugins
{
   
   sp_err dynamic_renderer::render_result_page(client_state *csp, http_response *rsp,
					       const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	static const char *result_tmpl_name = "websearch/templates/seeks_result_template_dyn.html";
	return dynamic_renderer::render_result_page(csp,rsp,parameters,result_tmpl_name);
     }
      
   sp_err dynamic_renderer::render_result_page(client_state *csp, http_response *rsp,
					       const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					       const std::string &result_tmpl_name,
					       const std::string &cgi_base,
					       const std::vector<std::pair<std::string,std::string> > *param_exports)
     {
	hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
	  = static_renderer::websearch_exports(csp,param_exports);
	
	// query.
	std::string html_encoded_query;
	std::string url_encoded_query;
	static_renderer::render_query(parameters,exports,html_encoded_query,
				      url_encoded_query);
		
	// clean query.
	std::string query_clean;
	static_renderer::render_clean_query(html_encoded_query,
					    exports,query_clean);
	
	
	// rendering.
	sp_err err = cgi::template_fill_for_cgi(csp,result_tmpl_name.c_str(),
						(seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
						  : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
						exports,rsp);
	
	return err;
     }
        
} /* end of namespace. */

