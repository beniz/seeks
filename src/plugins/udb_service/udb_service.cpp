/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "udb_service.h"
#include "udb_server.h"
#include "udb_client.h"
#include "udb_service_configuration.h"
#include "seeks_proxy.h"
#include "miscutil.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <iostream>

namespace seeks_plugins
{

  udb_service::udb_service()
    :plugin()
  {
    _name = "udb-service";
    _version_major = "0";
    _version_minor = "1";

    // configuration.
    if (seeks_proxy::_datadir.empty())
      _config_filename = plugin_manager::_plugin_repository + "udb_service/udb-service-config";
    else
      _config_filename = seeks_proxy::_datadir + "/plugins/udb_service/udb-service-config";

#ifdef SEEKS_CONFIGDIR
    struct stat stFileInfo;
    if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
      {
        _config_filename = SEEKS_CONFIGDIR "/udb-service-config";
      }
#endif

    if (udb_service_configuration::_config == NULL)
      udb_service_configuration::_config = new udb_service_configuration(_config_filename);
    _configuration = udb_service_configuration::_config;

    // cgi dispatchers.
    _cgi_dispatchers.reserve(2);
    cgi_dispatcher *cgid_find_dbr
    = new cgi_dispatcher("find_dbr",&udb_service::cgi_find_dbr,NULL,TRUE);
    _cgi_dispatchers.push_back(cgid_find_dbr);

    cgi_dispatcher *cgid_find_bqc
    = new cgi_dispatcher("find_bqc",&udb_service::cgi_find_bqc,NULL,TRUE);
    _cgi_dispatchers.push_back(cgid_find_bqc);
  }

  udb_service::~udb_service()
  {
    udb_service_configuration::_config = NULL;
  }

  db_err udb_service::cgi_find_dbr(client_state *csp,
                                   http_response *rsp,
                                   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    const char *key_str = miscutil::lookup(parameters,"urkey");
    if (!key_str)
      {
        return SP_ERR_CGI_PARAMS;
      }
    const char *pn_str = miscutil::lookup(parameters,"pn");
    if (!pn_str)
      {
        return SP_ERR_CGI_PARAMS;
      }
    if (!seeks_proxy::_user_db)
      {
        return SP_ERR_FILE; // no user db.
      }
    return udb_server::find_dbr_cb(key_str,pn_str,rsp);
  }

  db_err udb_service::cgi_find_bqc(client_state *csp,
                                   http_response *rsp,
                                   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    if (!seeks_proxy::_user_db)
      {
        return SP_ERR_FILE; // no user db.
      }
    std::string content = std::string(csp->_iob._cur);//,csp->_iob._size); // XXX: beware...
    return udb_server::find_bqc_cb(content,rsp);
  }

  db_record* udb_service::find_dbr_client(const std::string &host,
                                          const int &port,
                                          const std::string &path,
                                          const std::string &key,
                                          const std::string &pn)
  {
    udb_client uc;
    return uc.find_dbr_client(host,port,path,key,pn);
  }

  /* plugin registration. */
  extern "C"
  {
    plugin* maker()
    {
      return new udb_service;
    }
  }

} /* end of namespace. */
