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

#ifndef IMG_WEBSEARCH_H
#define IMG_WEBSEARCH_H

#include "plugin.h"
#include "img_websearch_configuration.h"
#include "img_query_context.h"
#include "stl_hash.h"

using sp::client_state;
using sp::http_response;
using sp::sp_err;
using sp::plugin;

namespace seeks_plugins
{

   class img_websearch : public plugin
     {
      public:
	img_websearch();
	
	~img_websearch();
	
	virtual void start() {};
	virtual void stop() {};
	
	/* cgi calls. */
	static sp_err cgi_img_websearch_search(client_state *csp,
					       http_response *rsp,
					       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	static sp_err cgi_img_websearch_similarity(client_state *csp,
						   http_response *rsp,
						   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	/* image websearch. */
	static sp_err perform_img_websearch(client_state *csp,
					    http_response *rsp,
					    const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					    bool render=true);
	
	/* error handling. */
     
      public:
	static img_websearch_configuration *_iwconfig;
	static hash_map<uint32_t,query_context*,id_hash_uint> _active_img_qcontexts;
     };
      
} /* end of namespace. */

#endif
