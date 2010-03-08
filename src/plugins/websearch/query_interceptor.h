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

#ifndef QUERY_INTERCEPTOR_H
#define QUERY_INTERCEPTOR_H

#include "interceptor_plugin.h"

using namespace sp;

namespace seeks_plugins
{
   class query_interceptor : public interceptor_plugin
     {
      public:
	query_interceptor(plugin *parent);
       
	// virtual functions from plugin_element.
	http_response* plugin_response(client_state *csp); // TODO.
	
	char* print() { return (char*)""; }; // TODO.
	
	// plugin functions.
	hash_map<const char*,const char*,hash<const char*>,eqstr>* parse_query(http_request *http) const;
	
      private:
	static std::string _p_filename; // pattern filename.
     };
   
} /* end of namespace. */

#endif
