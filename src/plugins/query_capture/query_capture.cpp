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
#include "query_capture_configuration.h"
#include "db_query_record.h"
#include "seeks_proxy.h" // for user_db.
#include "proxy_configuration.h"
#include "user_db.h"
#include "proxy_dts.h"
#include "urlmatch.h"
#include "encode.h"
#include "cgi.h"
#include "cgisimple.h"
#include "qprocess.h"
#include "miscutil.h"
#include "charset_conv.h"
#include "errlog.h"

#include <algorithm>
#include <iterator>
#include <iostream>

#include <sys/time.h>
#include <sys/stat.h>

using sp::sp_err;
using sp::seeks_proxy;
using sp::user_db;
using sp::db_record;
using sp::urlmatch;
using sp::encode;
using sp::cgi;
using sp::cgisimple;
using lsh::qprocess;
using sp::miscutil;
using sp::charset_conv;
using sp::errlog;

namespace seeks_plugins
{

  /*- query_db_sweepable -*/
  query_db_sweepable::query_db_sweepable()
      :user_db_sweepable()
  {
    // set last sweep to now.
    // sweeping is onde when plugin starts.
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    _last_sweep = tv_now.tv_sec;
  }

  query_db_sweepable::~query_db_sweepable()
  {
  }

  bool query_db_sweepable::sweep_me()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    if ((tv_now.tv_sec - _last_sweep)
        > query_capture_configuration::_config->_sweep_cycle)
      return true;
    else return false;
  }

  int query_db_sweepable::sweep_records()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    time_t sweep_date = tv_now.tv_sec - query_capture_configuration::_config->_retention;
    int err = seeks_proxy::_user_db->prune_db("query-capture",sweep_date);
    return err;
  }

  /*- query_capture -*/
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

    if (query_capture_configuration::_config == NULL)
      query_capture_configuration::_config = new query_capture_configuration(_config_filename);
    _configuration = query_capture_configuration::_config;

    // cgi dispatchers.
    _cgi_dispatchers.reserve(1);
    cgi_dispatcher *cgid_qc_redir
    = new cgi_dispatcher("qc_redir",&query_capture::cgi_qc_redir,NULL,TRUE);
    _cgi_dispatchers.push_back(cgid_qc_redir);

    if (query_capture_configuration::_config->_mode_intercept == "capture")
      _interceptor_plugin = new query_capture_element(this);
  }

  query_capture::~query_capture()
  {
    query_capture_configuration::_config = NULL; // configuration is deleted in parent class.
  }

  void query_capture::start()
  {
    // check for user db.
    if (!seeks_proxy::_user_db || !seeks_proxy::_user_db->_opened)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"user db is not opened for query capture plugin to work with it");
	return;
      }
    else if (seeks_proxy::_config->_user_db_startup_check)
      {
	// preventive sweep of records.
	static_cast<query_capture_element*>(_interceptor_plugin)->_qds.sweep_records();
      }
    
    // get number of captured queries already in user_db.
    uint64_t nr = seeks_proxy::_user_db->number_records(_name);
    
    errlog::log_error(LOG_LEVEL_INFO,"query_capture plugin: %u records",nr);
  }

  void query_capture::stop()
  {
  }

  sp_err query_capture::cgi_qc_redir(client_state *csp,
                                     http_response *rsp,
                                     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        char *urlp = NULL;
        sp_err err = query_capture::qc_redir(csp,rsp,parameters,urlp);
        if (err == SP_ERR_CGI_PARAMS)
          return cgi::cgi_error_bad_param(csp,rsp);
        else if (err == SP_ERR_PARSE)
          return cgi::cgi_error_disabled(csp,rsp); // wrong use of the resource.

        // redirect to requested url.
        urlp = encode::url_decode_but_not_plus(urlp);

        cgi::cgi_redirect(rsp,urlp);
        free(urlp);
        return SP_ERR_OK;
      }
    else return cgi::cgi_error_bad_param(csp,rsp);
  }

  sp_err query_capture::qc_redir(client_state *csp,
                                 http_response *rsp,
                                 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                 char *&urlp)
  {
    urlp = (char*)miscutil::lookup(parameters,"url"); // grab the url.
    if (!urlp)
      return SP_ERR_CGI_PARAMS;
    const char *q = miscutil::lookup(parameters,"q"); // grab the query.
    if (!q)
      return SP_ERR_CGI_PARAMS;

    // protection against abusive usage, check on referer.
    // XXX: this does prevent forging the referer and the query, but for
    // now let's issue a warning.
    std::string chost, referer, cget, base_url;
    query_capture_element::get_useful_headers(csp->_headers,chost,referer,
        cget,base_url);
    std::string ref_host, ref_path;
    urlmatch::parse_url_host_and_path(referer,ref_host,ref_path);

    /* if (base_url.empty())
      base_url = query_capture_element::_cgi_site_host; */

    // XXX: we could check on the exact referer domain.
    // But attackers forging referer would forge this too anyways.
    // So we perform a basic test, discouraging many, not all.
    if (query_capture_configuration::_config->_protected_redirection)
      {
        /* if (ref_host == base_url)
         {*/
        size_t p = ref_path.find("search?");
        if (p == std::string::npos)
          {
            p = ref_path.find("search_img?");
            if (p==std::string::npos)
              return SP_ERR_PARSE;
          }
        //else return SP_ERR_PARSE;
      }

    // capture queries and URL / HOST.
    // XXX: could be threaded and detached.
    char *queryp = encode::url_decode(q);
    std::string query = queryp;
    query = query_capture_element::no_command_query(query);
    free(queryp);
    
    std::string host,path;
    std::string url = std::string(urlp);
    query_capture::process_url(url,host,path);
        
    query_capture::store_queries(query,url,host);
    return SP_ERR_OK;
  }

  void query_capture::process_url(std::string &url, std::string &host)
  {
    if (url[url.length()-1]=='/') // remove trailing '/'.
      url = url.substr(0,url.length()-1);
    std::transform(url.begin(),url.end(),url.begin(),tolower);
    std::transform(host.begin(),host.end(),host.begin(),tolower);
  }
  
  void query_capture::process_url(std::string &url, std::string &host, std::string &path)
  {
    urlmatch::parse_url_host_and_path(url,host,path);
    host = urlmatch::strip_url(host);
    query_capture::process_url(url,host);
  }

  void query_capture::process_get(std::string &get)
  {
    size_t p = miscutil::replace_in_string(get," HTTP/1.1","");
    if (p == 0)
      miscutil::replace_in_string(get," HTTP/1.0","");
  }

  sp::db_record* query_capture::create_db_record()
  {
    return new db_query_record();
  }

  int query_capture::remove_all_query_records()
  {
    return seeks_proxy::_user_db->prune_db(_name);
  }

void query_capture::store_queries(const std::string &query,
                                    const std::string &url, const std::string &host)
  {
    // check charset encoding.
    std::string queryc = charset_conv::charset_check_and_conversion(query,std::list<const char*>());
    if (queryc.empty())
      {
        errlog::log_error(LOG_LEVEL_ERROR,"bad charset encoding for query to be captured %s",
                          query.c_str());
        return;
      }
    query_capture_element::store_queries(query,url,host,"query-capture");
  }

  void query_capture::store_queries(const std::string &query) const
  {
    // check charset encoding.
    std::string queryc = charset_conv::charset_check_and_conversion(query,std::list<const char*>());
    if (queryc.empty())
      {
        errlog::log_error(LOG_LEVEL_ERROR,"bad charset encoding for query to be captured %s",
                          query.c_str());
        return;
      }
    query_capture_element::store_queries(query,get_name());
  }
  
  /*- query_capture_element -*/
  std::string query_capture_element::_capt_filename = "query_capture/query-patterns";
  std::string query_capture_element::_cgi_site_host = CGI_SITE_1_HOST;

  query_capture_element::query_capture_element(plugin *parent)
      : interceptor_plugin((seeks_proxy::_datadir.empty() ? std::string(plugin_manager::_plugin_repository
                            + query_capture_element::_capt_filename).c_str()
                            : std::string(seeks_proxy::_datadir + "/plugins/" + query_capture_element::_capt_filename).c_str()),
                           parent)
  {
    if (seeks_proxy::_user_db)
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
    std::string host, referer, get, base_url;
    query_capture_element::get_useful_headers(csp->_headers,
        host,referer,get,
        base_url);
    if (base_url.empty())
      base_url = query_capture_element::_cgi_site_host;

    std::string ref_host, ref_path;
    urlmatch::parse_url_host_and_path(referer,ref_host,ref_path);
    if (ref_host == base_url)
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
        std::string query_str = query_capture_element::no_command_query(query);

        //std::cerr << "detected query: " << query_str << std::endl;
        
        // check charset encoding.
        std::string query_strc = charset_conv::charset_check_and_conversion(query_str,csp->_headers);
        if (query_strc.empty())
          {
            errlog::log_error(LOG_LEVEL_ERROR,"bad charset encoding for query to be captured %s",
                              query_str.c_str());
            delete parameters;
            return NULL;
          }
        
	query_capture::process_get(get);
	host = urlmatch::strip_url(host);
        std::string url = host + get;
	query_capture::process_url(url,host);
	
        // store queries and URL / HOST.
        query_capture_element::store_queries(query_str,url,host,_parent->get_name());

        delete parameters;
      }

    return NULL; // no response, so the proxy does not crunch this HTTP request.
  }

  void query_capture_element::store_queries(const std::string &query,
      const std::string &url, const std::string &host,
      const std::string &plugin_name)
  {
    // generate query fragments.
    hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
    qprocess::generate_query_hashes(query,0,
                                    query_capture_configuration::_config->_max_radius,
                                    features);

    // push URL into the user db buckets with query fragments as key.
    // URLs are stored only for queries of radius 0. This scheme allows to save
    // DB space. To recover URLs from a query of radius > 1, a second lookup is necessary,
    // for the recorded query of radius 0 that holds the URL counters.
    hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
    while (hit!=features.end())
      {
        if ((*hit).first == 0)
          query_capture_element::store_url((*hit).second,query,url,host,(*hit).first,plugin_name);
        else query_capture_element::store_query((*hit).second,query,(*hit).first,plugin_name);
        ++hit;
      }
  }

  void query_capture_element::store_queries(const std::string &query,
      const std::string &plugin_name)
  {
    // strip query.
    std::string q = query_capture_element::no_command_query(query);
    q = miscutil::chomp_cpp(q);

    // generate query fragments.
    hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
    qprocess::generate_query_hashes(q,0,
                                    query_capture_configuration::_config->_max_radius,
                                    features);

    // store query with hash fragment as key.
    hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
    while (hit!=features.end())
      {
        query_capture_element::store_query((*hit).second,q,(*hit).first,plugin_name);
        ++hit;
      }
  }

  void query_capture_element::store_query(const DHTKey &key,
                                          const std::string &query,
                                          const uint32_t &radius,
                                          const std::string &plugin_name)
  {
    std::string key_str = key.to_rstring();
    db_query_record dbqr(plugin_name,query,radius);
    seeks_proxy::_user_db->add_dbr(key_str,dbqr);
  }

  void query_capture_element::store_url(const DHTKey &key, const std::string &query,
                                        const std::string &url, const std::string &host,
                                        const uint32_t &radius,
                                        const std::string &plugin_name)
  {
    std::string key_str = key.to_rstring();
    if (!url.empty())
      {
        db_query_record dbqr(plugin_name,query,radius,url);
        seeks_proxy::_user_db->add_dbr(key_str,dbqr);
      }
    if (!host.empty() && host != url)
      {
        db_query_record dbqr(plugin_name,query,radius,host);
        seeks_proxy::_user_db->add_dbr(key_str,dbqr);
      }
  }

  void query_capture_element::remove_url(const DHTKey &key, const std::string &query,
					 const std::string &url, const std::string &host,
					 const short &url_hits, const uint32_t &radius,
					 const std::string &plugin_name)
  {
    std::string key_str = key.to_rstring();
    if (!url.empty())
      {
        db_query_record dbqr(plugin_name,query,radius,url,1,-url_hits);
        seeks_proxy::_user_db->add_dbr(key_str,dbqr);
      }
    if (!host.empty() && host != url)
      {
        db_query_record dbqr(plugin_name,query,radius,host,1,-url_hits);
        seeks_proxy::_user_db->add_dbr(key_str,dbqr);
      }
  }

  void query_capture_element::get_useful_headers(const std::list<const char*> &headers,
      std::string &host, std::string &referer,
      std::string &get, std::string &base_url)
  {
    std::list<const char*>::const_iterator lit = headers.begin();
    while (lit!=headers.end())
      {
        if (miscutil::strncmpic((*lit),"get ",4) == 0)
          {
            get = (*lit);
            try
              {
                get = get.substr(4);
              }
            catch (std::exception &e)
              {
                get = "";
              }
          }
        else if (miscutil::strncmpic((*lit),"host:",5) == 0)
          {
            host = (*lit);
            try
              {
                host = host.substr(6);
              }
            catch (std::exception &e)
              {
                host = "";
              }
          }
        else if (miscutil::strncmpic((*lit),"referer:",8) == 0)
          {
            referer = (*lit);
            try
              {
                referer = referer.substr(9);
              }
            catch (std::exception &e)
              {
                referer = "";
              }
          }
        else if (miscutil::strncmpic((*lit),"Seeks-Remote-Location:",22) == 0)
          {
            base_url = (*lit);
            try
              {
                size_t pos = base_url.find_first_of(" ");
                base_url = base_url.substr(pos+1);
              }
            catch (std::exception &e)
              {
                base_url = "";
              }
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
    miscutil::replace_in_string(cquery,"\"",""); // prune out quote. XXX: could go elsewhere.
    return cquery;
  }

  /* plugin registration. */
  extern "C"
  {
    plugin* maker()
    {
      return new query_capture;
    }
  }

} /* end of namespace. */
