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

#include "uri_capture.h"
#include "seeks_proxy.h" // for user_db.
#include "user_db.h"
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
   /*- uri_capture -*/
   uri_capture::uri_capture()
     : plugin()
       {
	  _name = "uri-capture";
	  _version_major = "0";
	  _version_minor = "1";
	  _configuration = NULL;
	  _interceptor_plugin = new uri_capture_element(this);
       }
   
   uri_capture::~uri_capture()
     {
     }
   
   void uri_capture::start()
     {
	// check for user db.
	if (!seeks_proxy::_user_db || !seeks_proxy::_user_db->_opened)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"user db is not opened for URI capture plugin to work with it");
	  }
     }
   
   void uri_capture::stop()
     {
     }
      
   /*- uri_capture_element -*/
   std::string uri_capture_element::_capt_filename = "uri_capture/uri-patterns";
   hash_map<const char*,bool,hash<const char*>,eqstr> uri_capture_element::_img_ext_list;
   
   uri_capture_element::uri_capture_element(plugin *parent)
     : interceptor_plugin((seeks_proxy::_datadir.empty() ? std::string(plugin_manager::_plugin_repository
								       + uri_capture_element::_capt_filename).c_str()
			   : std::string(seeks_proxy::_datadir + "/plugins/" + uri_capture_element::_capt_filename).c_str()),
			  parent)
       {
	  uri_capture_element::init_file_ext_list();
       }
   
   uri_capture_element::~uri_capture_element()
     {
     }
      
   http_response* uri_capture_element::plugin_response(client_state *csp)
     {
	// store domain names.
	/* std::cerr << "[uri_capture]: headers:\n";
	std::copy(csp->_headers.begin(),csp->_headers.end(),
		  std::ostream_iterator<const char*>(std::cout,"\n")); */
	//std::cerr << std::endl;
	
	std::string host, referer, accept, get;
	bool connect = false;
	uri_capture_element::get_useful_headers(csp->_headers,
						host,referer,
						accept,get,connect);
	
	/**
	 * URI domain name is store in two cases:
	 * - there is no referer in the HTTP request headers.
	 * - the host domain is different than the referer, indicating a move
	 *   to a different website.
	 * 
	 * We do not record:
	 * - 'CONNECT' requests.
	 * - paths to images.
	 */
	bool store = true;
	if (connect)
	  {
	     store = false;
	  }
	else if (store)
	  {
	     size_t p = accept.find("image");
	     
	     if (p!=std::string::npos)
	       store = false;		  
	     else
	       {
		  // XXX: we could use regexps here.
		  if ((p=get.find(".css"))!=std::string::npos
		      || (p=get.find(".js"))!=std::string::npos)
		    store = false;
		  else 
		    {
		       p = miscutil::replace_in_string(get," HTTP/1.1","");
		       if (p == 0)
			 miscutil::replace_in_string(get," HTTP/1.0","");
		       if (uri_capture_element::is_path_to_image(get))
			 store = false;
		    }
	       }
	  }
	if (store && referer.empty())
	  {
	     //std::cerr << "****************** no referer!\n";
	     store = true;
	  }
	else if (store)
	  {
	     std::string ref_host, ref_path;
	     urlmatch::parse_url_host_and_path(referer,ref_host,ref_path);
	     ref_host = urlmatch::strip_url(ref_host);
	     host = urlmatch::strip_url(host);
	     
	     //std::cerr << "****************** host: " << host << " -- ref_host: " << ref_host << std::endl;
	     
	     if (ref_host == host)
	       store = false;
	  }
	
	//std::cerr << "-----------------> store: " << store << std::endl;
	std::cerr << std::endl;
	
	return NULL; // no response, so the proxy does not crunch this call.
     }
   
   void uri_capture_element::get_useful_headers(const std::list<const char*> &headers,
						std::string &host, std::string &referer,
						std::string &accept, std::string &get,
						bool &connect)
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
	     else if (miscutil::strncmpic((*lit),"accept:",7) == 0)
	       {
		  accept = (*lit);
		  accept = accept.substr(8);
	       }
	     else if (miscutil::strncmpic((*lit),"connect ",8) == 0) // XXX: could grab GET and negate the test.
	       connect = true;
	     ++lit;
	  }
     }
      
   void uri_capture_element::init_file_ext_list()
     {
	static std::string ext_list[28]
	  = { "jpg","jpeg","raw","png","gif","bmp","ppm","pgm","pbm","pnm","tga","pcx",
	      "ecw","img","sid","pgf","cgm","svg","eps","odg","pdf","pgml","swf",
	      "vml","wmf","emf","xps","ico"};
	
	if (!uri_capture_element::_img_ext_list.empty())
	  return;
	
	for (int i=0;i<28;i++)
	  uri_capture_element::_img_ext_list.insert(std::pair<const char*,bool>(strdup(ext_list[i].c_str()),true));   
     }
   
   bool uri_capture_element::is_path_to_image(const std::string &path)
     {
	size_t pos = path.find_last_of(".");
	if (pos == std::string::npos || path.size() < pos+1)
	  return false;
	std::string ext = path.substr(pos+1);
	hash_map<const char*,bool,hash<const char*>,eqstr>::const_iterator hit;
	if ((hit=uri_capture_element::_img_ext_list.find(ext.c_str()))!=uri_capture_element::_img_ext_list.end())
	  return true;
	return false;
     }
         
   /* auto-registration */
#if defined(ON_OPENBSD) || defined(ON_OSX)
   extern "C"
     {
	plugin* maker()
	  {
	     return new uri_capture;
	  }
     }
#else   
   plugin* makeruc()
     {
	return new uri_capture;
     }
   
   class proxy_autor_capture
     {
      public:
	proxy_autor_capture()
	  {
	     plugin_manager::_factory["uri-capture"] = makeruc; // beware: default plugin shell with no name.
	  }
     };
   
   proxy_autor_capture _p; // one instance, instanciated when dl-opening.
#endif
   
} /* end of namespace. */
