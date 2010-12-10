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
#include "uc_configuration.h"
#include "db_uri_record.h"
#include "seeks_proxy.h" // for user_db.
#include "proxy_configuration.h"
#include "user_db.h"
#include "proxy_dts.h"
#include "urlmatch.h"
#include "miscutil.h"
#include "errlog.h"

#include <sys/time.h>
#include <sys/stat.h>

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

  /*- uri_db_sweepable -*/
  uri_db_sweepable::uri_db_sweepable()
      :user_db_sweepable()
  {
    // set last sweep to now.
    // sweeping is done when plugin starts.
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    _last_sweep = tv_now.tv_sec;
  }

  uri_db_sweepable::~uri_db_sweepable()
  {
  }

  bool uri_db_sweepable::sweep_me()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    if ((tv_now.tv_sec - _last_sweep)
        > uc_configuration::_config->_sweep_cycle)
      return true;
    else return false;
  }

  int uri_db_sweepable::sweep_records()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    time_t sweep_date = tv_now.tv_sec - uc_configuration::_config->_retention;
    seeks_proxy::_user_db->prune_db("uri-capture",sweep_date);
  }

  /*- uri_capture -*/
  uri_capture::uri_capture()
      : plugin(),_nr(0)
  {
    _name = "uri-capture";
    _version_major = "0";
    _version_minor = "1";
    _configuration = NULL;
    _interceptor_plugin = new uri_capture_element(this);

    if (seeks_proxy::_datadir.empty())
      _config_filename = plugin_manager::_plugin_repository + "uri_capture/uri-capture-config";
    else
      _config_filename = seeks_proxy::_datadir + "/plugins/uri_capture/uri-capture-config";

#ifdef SEEKS_CONFIGDIR
    struct stat stFileInfo;
    if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
      {
        _config_filename = SEEKS_CONFIGDIR "/uri-capture-config";
      }
#endif

    if (uc_configuration::_config == NULL)
      uc_configuration::_config = new uc_configuration(_config_filename);
    _configuration = uc_configuration::_config;
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
    else if (seeks_proxy::_config->_user_db_startup_check)
      {
        // preventive sweep of records.
	static_cast<uri_capture_element*>(_interceptor_plugin)->_uds.sweep_records();

        // get number of captured URI already in user_db.
        _nr = seeks_proxy::_user_db->number_records(_name);

        errlog::log_error(LOG_LEVEL_INFO,"uri_capture plugin: %u records",_nr);
      }
  }

  void uri_capture::stop()
  {
  }

  sp::db_record* uri_capture::create_db_record()
  {
    return new db_uri_record();
  }

  int uri_capture::remove_all_uri_records()
  {
    seeks_proxy::_user_db->prune_db(_name);
  }

  /*- uri_capture_element -*/
  std::string uri_capture_element::_capt_filename = "uri_capture/uri-patterns";
  std::string uri_capture_element::_cgi_site_host = CGI_SITE_1_HOST;

  uri_capture_element::uri_capture_element(plugin *parent)
      : interceptor_plugin((seeks_proxy::_datadir.empty() ? std::string(plugin_manager::_plugin_repository
                            + uri_capture_element::_capt_filename).c_str()
                            : std::string(seeks_proxy::_datadir + "/plugins/" + uri_capture_element::_capt_filename).c_str()),
                           parent)
  {
    if (seeks_proxy::_user_db)
      seeks_proxy::_user_db->register_sweeper(&_uds);
  }

  uri_capture_element::~uri_capture_element()
  {
  }

  http_response* uri_capture_element::plugin_response(client_state *csp)
  {
    // store domain names.
    /* std::cerr << "[uri_capture]: headers:\n";
    std::copy(csp->_headers.begin(),csp->_headers.end(),
    	  std::ostream_iterator<const char*>(std::cout,"\n"));
    std::cerr << std::endl; */

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
    std::string uri;

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
            p = miscutil::replace_in_string(get," HTTP/1.1","");
            if (p == 0)
              miscutil::replace_in_string(get," HTTP/1.0","");
          }
      }
    host = uri_capture_element::prepare_uri(host);
    std::transform(get.begin(),get.end(),get.begin(),tolower);
    if (host == uri_capture_element::_cgi_site_host) // if proxy domain.
      store = false;

    if (store && referer.empty())
      {
        if (get != "/")
          uri = host + get;
      }
    else if (store)
      {
        if (get != "/")
          uri = host + get;
      }

    if (store)
      {
        store_uri(uri,host);
      }

    return NULL; // no response, so the proxy does not crunch this HTTP request.
  }

  void uri_capture_element::store_uri(const std::string &uri, const std::string &host) const
  {
    // add record to user db.
    db_uri_record dbur(_parent->get_name());
    if (!uri.empty())
      {
        seeks_proxy::_user_db->add_dbr(uri,dbur);
        static_cast<uri_capture*>(_parent)->_nr++;
      }
    if (!host.empty() && uri != host)
      {
        seeks_proxy::_user_db->add_dbr(host,dbur);
        static_cast<uri_capture*>(_parent)->_nr++;
      }
  }

  std::string uri_capture_element::prepare_uri(const std::string &uri)
  {
    std::string prep_uri = urlmatch::strip_url(uri);
    std::transform(prep_uri.begin(),prep_uri.end(),prep_uri.begin(),tolower);
    return prep_uri;
  }

  void uri_capture_element::get_useful_headers(const std::list<const char*> &headers,
      std::string &host, std::string &referer,
      std::string &accept, std::string &get,
      bool &connect)
  {
    std::list<const char*>::const_iterator lit = headers.begin();
    while (lit!=headers.end())
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
