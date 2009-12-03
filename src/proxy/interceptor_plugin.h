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

#ifndef INTERCEPTOR_PLUGIN_H
#define INTERCEPTOR_PLUGIN_H

#include "plugin_element.h"

#include <string>
#include <vector>

namespace sp
{
   class interceptor_plugin : public plugin_element
     {
      public:
	interceptor_plugin(const std::vector<url_spec*> &pos_patterns,
			   const std::vector<url_spec*> &neg_patterns,
			   plugin *parent);

	interceptor_plugin(const char *filename,
			   plugin *parent);
	
	interceptor_plugin(const std::vector<std::string> &pos_patterns,
			   const std::vector<std::string> &neg_patterns,
			   plugin *parent);
	
	virtual http_response* plugin_response(client_state *csp) { return NULL; };
	
	virtual char* print() { return (char*)""; }; // virtual
	
      private:

     };
   
} /* end of namespace. */

#endif
