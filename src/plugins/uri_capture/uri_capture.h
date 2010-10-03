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

#ifndef URI_CAPTURE_H
#define URI_CAPTURE_H

#define NO_PERL // we do not use Perl.

#include "stl_hash.h"
#include "plugin.h"
#include "interceptor_plugin.h"
 
using namespace sp;

namespace seeks_plugins
{
   
   class uri_db_sweepable : public user_db_sweepable
     {
      public:
	uri_db_sweepable();
	
	virtual ~uri_db_sweepable();
	
	virtual bool sweep_me();
	
	virtual int sweep_records();
     
	time_t _last_sweep;
     };
      
   class uri_capture : public plugin
     {
      public:
	uri_capture();
	
	virtual ~uri_capture();
	
	virtual void start();
	
	virtual void stop();
     
	virtual sp::db_record* create_db_record();
     
	int remove_all_uri_records();
     
      public:
	uint64_t _nr; /**< number of captured URI in user db. */
     };
      
   class uri_capture_element : public interceptor_plugin
     {
      public:
	uri_capture_element(plugin *parent);
	
	virtual ~uri_capture_element();
	
	virtual http_response* plugin_response(client_state *csp);
	
	void store_uri(const std::string &uri, const std::string &host) const;
	
	static std::string prepare_uri(const std::string &uri);
	
	void get_useful_headers(const std::list<const char*> &headers,
				std::string &host, std::string &referer,
				std::string &accept, std::string &get, 
				bool &connect);
	
	static void init_file_ext_list();
	
	static bool is_path_to_no_page(const std::string &path);

      private:
	static std::string _capt_filename;
	static hash_map<const char*,bool,hash<const char*>,eqstr> _img_ext_list;
	static std::string _cgi_site_host;
	
      public:
	uri_db_sweepable _uds;
     };   
   
} /* end of namespace. */

#endif
  
