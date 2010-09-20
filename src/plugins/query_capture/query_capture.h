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

#ifndef QUERY_CAPTURE_H
#define QUERY_CAPTURE_H

#include "plugin.h"
#include "interceptor_plugin.h"
#include "DHTKey.h"

using namespace sp;
using dht::DHTKey;

namespace seeks_plugins
{

   class query_db_sweepable : public user_db_sweepable
     {
      public:
	query_db_sweepable();
	
	virtual ~query_db_sweepable();

	virtual bool sweep_me();
	
	virtual int sweep_records();
     };
      
   class query_capture : public plugin
     {
      public:
	query_capture();
	
	virtual ~query_capture();
	
	virtual void start();
	
	virtual void stop();
	
	virtual sp::db_record* create_db_record();
	
	int remove_all_query_records();
     
	//TODO: store_query called from websearch plugin.
     };
      
   class query_capture_element : public interceptor_plugin
     {
      public:
	query_capture_element(plugin *parent);
	
	virtual ~query_capture_element();
     
	virtual http_response* plugin_response(client_state *csp);
	
	void store_url(const DHTKey &key, const std::string &query,
		       const std::string &url, const std::string &host,
		       const uint32_t &radius);
	
	void get_useful_headers(const std::list<const char*> &headers,
				std::string &host, std::string &referer,
				std::string &get);
	
      public:
	query_db_sweepable _qds;
	static std::string _capt_filename; // pattern capture file.
	static std::string _cgi_site_host; // default local cgi address.
     };
      
} /* end of namespace. */

#endif
