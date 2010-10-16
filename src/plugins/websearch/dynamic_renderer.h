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

#ifndef DYNAMIC_RENDERER_H
#define DYNAMIC_RENDERER_H

#include "websearch.h"

namespace seeks_plugins
{
   
   class dynamic_renderer
     {
      public:
	static sp_err render_result_page(client_state *csp, http_response *rsp,
					 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);
	
	static sp_err render_result_page(client_state *csp, http_response *rsp,
					 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					 const std::string &result_tmpl_name,
					 const std::string &cgi_base="/search?",
					 const std::vector<std::pair<std::string,std::string> > *param_exports=NULL);
     };
      
} /* end of namespace. */

#endif
