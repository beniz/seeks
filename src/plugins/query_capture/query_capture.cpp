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
 
#include "query_capture.h"
#include "db_query_record.h"
#include "seeks_proxy.h" // for user_db.
#include "user_db.h"
#include "proxy_dts.h"
#include "urlmatch.h"
#include "miscutil.h"
#include "errlog.h"

#include <algorithm>
#include <iterator>
#include <iostream>

using sp::seeks_proxy;
using sp::user_db;
using sp::db_record;
using sp::urlmatch;
using sp::miscutil;
using sp::errlog;

namespace seeks_plugins
{
   
   /*- query_db_sweepable -*/
   query_db_sweepable::query_db_sweepable()
     :user_db_sweepable()
     {
     }
   
   query_db_sweepable::~query_db_sweepable()
     {
     }
   
        
   bool query_db_sweepable::sweep_me()
     {
	//TODO: dates.
	return false;
     }
   
   int query_db_sweepable::sweep_records()
     {	
     }
   
   /*- query_capture -*/
   query_capture::query_capture()
     :plugin()
       {
	  _name = "query-capture";
	  _version_major = "0";
	  _version_minor = "1";
	  _configuration = NULL;
	  _interceptor_plugin = new query_capture_element(this);
       }
   
   query_capture::~query_capture()
     {
     }
   
   void query_capture::start()
     {
	// check for user db.
	if (!seeks_proxy::_user_db || !seeks_proxy::_user_db->_opened)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"user db is not opened for URI capture plugin to work with it");
	  }
     }
   
   void query_capture::stop()
     {
     }
   
   sp::db_record* query_capture::create_db_record()
     {
	return new db_query_record();
     }
   
   int query_capture::remove_all_query_records()
     {
	seeks_proxy::_user_db->prune_db(_name);
     }
   
    /*- uri_capture_element -*/
   std::string uri_capture_element::_capt_filename = "query_capture/query-patterns";
   hash_map<const char*,bool,hash<const char*>,eqstr> query_capture_element::_img_ext_list;
   std::string query_capture_element::_cgi_site_host = CGI_SITE_1_HOST;
   
   query_capture_element::query_capture_element(plugin *parent)
     : interceptor_plugin((seeks_proxy::_datadir.empty() ? std::string(plugin_manager::_plugin_repository
								       + query_capture_element::_capt_filename).c_str()
			   : std::string(seeks_proxy::_datadir + "/plugins/" + query_capture_element::_capt_filename).c_str()),
			  parent)
       {
	  query_capture_element::init_file_ext_list();
	  seeks_proxy::_user_db->register_sweeper(&_uds);
       }
   
   query_capture_element::~query_capture_element()
     {
     }
   
   http_response* query_capture_element::plugin_response(client_state *csp)
     {
	/**
	 * Captures clicked URLs from search results, and store them with the
	 * right queries.
	 */
     }
   
   void query_capture::get_useful_headers(const std::list<const char*> &headers,
					  std::string &host, std::string &referer,
					  std::string &get)
     {
	std::list<const char*>::const_iterator lit = headers.begin();
	while(lit!=headers.end())
	  {
	     if (miscutil::strncmpic((*lit),"get ",4) == 0)
	       {
		  get = (*lit);
		  get = get.substr(4);
	       }
	     else if (miscutil::strncmpic((*lit),"host:",5) == 0)
	       {
		  host = (*lit);
		  host = host.substr(6);
	       }
	     else if (miscutil::strncmpic((*lit),"referer:",8) == 0)
	       {
		  referer = (*lit);
		  referer = referer.substr(9);
	       }
	  }
      
#if defined(ON_OPENBSD) || defined(ON_OSX)
   extern "C"  
     {
	plugin* maker()
	  {
	     return new query_capture;
	  }
     }
#else
   plugin* makeruc()
     {
	return new query_capture;
     }
   
   class proxy_autor_capture
     {
      public:
	proxy_autor_capture()
	  {
	     plugin_manager::_factory["query-capture"] = makeruc; // beware: default plugin shell with no name.
	  }
     };
   
   proxy_autor_capture _p; // one instance, instanciated when dl-opening.
#endif
   
} /* end of namespace. */
