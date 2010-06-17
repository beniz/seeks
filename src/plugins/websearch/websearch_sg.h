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
  
#ifndef WEBSEARCH_SG_H
#define WEBSEARCH_SG_H

#include "websearch.h"

namespace seeks_plugins
{
   
   class websearch_sg : public websearch
     {
      public:
	websearch_sg();
	
	~websearch_sg();
	
	/* cgi calls. */
	static sp_err cgi_websearch_search_sg(client_state *csp,
					      http_response *rsp,
					      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	/* websearch. */
	static sp_err perform_websearch_sg(client_state *csp,
					   http_response *rsp,
					   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					   bool render=true);
     };
      
} /* end of namespace. */

#endif
