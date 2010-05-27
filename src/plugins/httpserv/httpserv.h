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
  
#ifndef HTTPSERV_H
#define HTTPSERV_H

#include "plugin.h"

#include <event.h>
#include <evhttp.h>

using sp::plugin;

namespace seeks_plugins
{
   
   class httpserv : public plugin
     {
      public:
	httpserv();
	
	httpserv(const std::string &address,
		 const u_short &port);
	
	~httpserv();
	
	void init_callbacks();
	
	/* responses. */
	static void reply_with_error_400(struct evhttp_request *r);
	
	static void reply_with_error(struct evhttp_request *r,
				     const int &http_code,
				     const char *message,
				     const std::string &error_message);
	
	static void reply_with_empty_body(struct evhttp_request *r,
					  const int &http_code,
					  const char *message);
	
	static void reply_with_body(struct evhttp_request *r,
				    const int &http_code,
				    const char *message,
				    const std::string &content,
				    const std::string &content_type="text/html");
	
	/* callbacks. */
	static void seeks_search_css(struct evhttp_request *r, void *arg);
	static void file_service(struct evhttp_request *r, void *arg);
	static void websearch(struct evhttp_request *r, void *arg);
	static void unknown_path(struct evhttp_request *r, void *arg);
	
	/* utils. */
	static int parse_query(const std::string &str,
			       hash_map<const char*,const char*,hash<const char*>,eqstr> &parameters);
	
      public:
	std::string _address;
	u_short _port;
	struct evhttp *_srv;
	struct event_base *_evbase;
     };
      
} /* end of namespace. */

#endif
