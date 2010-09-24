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
#include "encode.h"
#include "cgi.h"
#include "qprocess.h"
#include "miscutil.h"
#include "errlog.h"

#include <algorithm>
#include <iterator>
#include <iostream>

#include <sys/stat.h>

using sp::seeks_proxy;
using sp::user_db;
using sp::db_record;
using sp::urlmatch;
using sp::encode;
using sp::cgi;
using lsh::qprocess;
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
   
   query_capture_configuration* query_capture::_config = NULL;
   
   query_capture::query_capture()
     :plugin()
       {
	  _name = "query-capture";
	  _version_major = "0";
	  _version_minor = "1";
	    
	  if (seeks_proxy::_datadir.empty())
	    _config_filename = plugin_manager::_plugin_repository + "query_capture/query-capture-config";
	  else
	    _config_filename = seeks_proxy::_datadir + "/plugins/query_capture/query-capture-config";
	  
#ifdef SEEKS_CONFIGDIR
	  struct stat stFileInfo;
	  if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
	    {
	       _config_filename = SEEKS_CONFIGDIR "/query-capture-config";
	    }
#endif
	  
	  if (query_capture::_config == NULL)
	    query_capture::_config = new query_capture_configuration(_config_filename);
	  _configuration = query_capture::_config;
	  
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

   void query_capture::store_query(const std::string &query) const
     {
	static_cast<query_capture_element*>(_interceptor_plugin)->store_query(query);
     }
      
    /*- uri_capture_element -*/
   std::string query_capture_element::_capt_filename = "query_capture/query-patterns";
   std::string query_capture_element::_cgi_site_host = CGI_SITE_1_HOST;
   
   query_capture_element::query_capture_element(plugin *parent)
     : interceptor_plugin((seeks_proxy::_datadir.empty() ? std::string(plugin_manager::_plugin_repository
								       + query_capture_element::_capt_filename).c_str()
			   : std::string(seeks_proxy::_datadir + "/plugins/" + query_capture_element::_capt_filename).c_str()),
			  parent)
       {
	  seeks_proxy::_user_db->register_sweeper(&_qds);
       }
   
   query_capture_element::~query_capture_element()
     {
     }
   
   http_response* query_capture_element::plugin_response(client_state *csp)
     {
	/* std::cerr << "[query_capture]: headers:\n";
	std::copy(csp->_headers.begin(),csp->_headers.end(),
		  std::ostream_iterator<const char*>(std::cout,"\n"));
	std::cerr << std::endl; */
		
	/**
	 * Captures clicked URLs from search results, and store them along with
	 * the query fragments.
	 */
	std::string host, referer, get;
	query_capture_element::get_useful_headers(csp->_headers,
						  host,referer,get);
	
	std::string ref_host, ref_path;
	urlmatch::parse_url_host_and_path(referer,ref_host,ref_path);
	if (ref_host == query_capture_element::_cgi_site_host)
	  {
	     // check that is not a query itself.
	     size_t p = get.find("search?");
	     if (p == std::string::npos)
	       {
		  p = get.find("search_img?");
		  if (p!=std::string::npos)
		    return NULL;
	       }
	     else return NULL;
	     
	     // check that it comes from an API call to the websearch plugin.
	     p = referer.find("search?");
	     if (p == std::string::npos)
	       {
		  p = referer.find("search_img?");
		  if (p == std::string::npos)
		    return NULL;
	       }
	     
	     char *argstring = strdup(ref_path.c_str());
	     hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters 
	       = cgi::parse_cgi_parameters(argstring);
	     free(argstring);
	     const char *query = miscutil::lookup(parameters,"q");
	     if (!query)
	       {
		  delete parameters;
		  return NULL;
	       }
	     /* char *dec_query = encode::url_decode(query);
	     std::string query_str = std::string(dec_query);
	     free(dec_query); */
	     std::string query_str = query_capture_element::no_command_query(query);
	     
	     //std::cerr << "detected query: " << query_str << std::endl;
	     
	     host = urlmatch::strip_url(host);
	     p = miscutil::replace_in_string(get," HTTP/1.1","");
	     if (p == 0)
	       miscutil::replace_in_string(get," HTTP/1.0","");
	     std::string url = host + get;
	     if (url[url.length()-1]=='/') // remove trailing '/'.
	       url = url.substr(0,url.length()-1);
	     std::transform(url.begin(),url.end(),url.begin(),tolower);
	     std::transform(host.begin(),host.end(),host.begin(),tolower);
	     
	     // generate query fragments.
	     //std::cerr << "[query_capture]: detected refering query: " << query_str << std::endl;
	     hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
	     qprocess::generate_query_hashes(query_str,0,5,features); //TODO: configuration.
	     
	     // push URL into the user db buckets with query fragments as key.
	     hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
	     while(hit!=features.end())
	       {
		  store_url((*hit).second,query_str,url,host,(*hit).first);
		  ++hit;
	       }
	     
	     delete parameters;
	  }
		
	return NULL; // no response, so the proxy does not crunch this HTTP request.
     }
   
   void query_capture_element::store_query(const std::string &query) const
     {
	// strip query.
	std::string q = query_capture_element::no_command_query(query);
		
	// generate query fragments.
	hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
	qprocess::generate_query_hashes(q,0,5,features); // TODO: configuration.
	
	// store query with hash fragment as key.
	hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
	while(hit!=features.end())
	  {
	     std::string key_str = (*hit).second.to_rstring();
	     db_query_record dbqr(_parent->get_name(),q,(*hit).first);
	     seeks_proxy::_user_db->add_dbr(key_str,dbqr);
	     ++hit;
	  }
     }
      
   void query_capture_element::store_url(const DHTKey &key, const std::string &query,
					 const std::string &url, const std::string &host,
					 const uint32_t &radius)
     {
	std::string key_str = key.to_rstring();
	if (!url.empty())
	  {
	     //std::cerr << "adding url: " << url << std::endl;
	     db_query_record dbqr(_parent->get_name(),query,radius,url);
	     seeks_proxy::_user_db->add_dbr(key_str,dbqr);
	  }
	if (!host.empty() && host != url)
	  {
	     //std::cerr << "adding host: " << host << std::endl;
	     db_query_record dbqr(_parent->get_name(),query,radius,host);
	     seeks_proxy::_user_db->add_dbr(key_str,dbqr);
	  }
     }
      
   void query_capture_element::get_useful_headers(const std::list<const char*> &headers,
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
	     ++lit;
	  }
     }

   std::string query_capture_element::no_command_query(const std::string &oquery)
     {
	 
	std::string cquery = oquery;
	if (cquery[0] == ':')
	  {
	     try
	       {
		  cquery = cquery.substr(4);
	       }
	     catch (std::exception &e)
	       {
		  // do nothing.
	       }
	  }
	return cquery;
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
   plugin* makerqc()
     {
	return new query_capture;
     }
	
   class proxy_autor_qcapture
     {
      public:
	proxy_autor_qcapture()
	  {
	     plugin_manager::_factory["query-capture"] = makerqc; // beware: default plugin shell with no name.
	  }
     };
   
   proxy_autor_qcapture _p; // one instance, instanciated when dl-opening.
#endif
   
} /* end of namespace. */
