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

#include "udb_client.h"
#include "udbs_err.h"
#include "udb_service_configuration.h"
#include "DHTKey.h"
#include "qprocess.h"
#include "halo_msg_wrapper.h"
#include "curl_mget.h"
#include "plugin_manager.h"
#include "plugin.h"
#include "miscutil.h"
#include "errlog.h"

#include <iostream>

using sp::curl_mget;
using sp::plugin_manager;
using sp::plugin;
using sp::miscutil;
using sp::errlog;
using lsh::qprocess;

namespace seeks_plugins
{

  udb_client::udb_client()
  {
  }

  udb_client::~udb_client()
  {
  }

  db_record* udb_client::find_dbr_client(const std::string &host,
                                         const int &port,
                                         const std::string &path,
                                         const std::string &key,
                                         const std::string &pn) throw (sp_exception)
  {
    std::string url = host;
    if (port != -1)
      url += ":" + miscutil::to_string(port);
    url += path + "/find_dbr?";
    url += "urkey=" + key;
    url += "&pn=" + pn;
    curl_mget cmg(1,udb_service_configuration::_config->_call_timeout,0,
                  udb_service_configuration::_config->_call_timeout,0);
    std::vector<std::string> urls;
    urls.reserve(1);
    urls.push_back(url);
    std::vector<int> status;
    if (udb_service_configuration::_config->_p2p_proxy_addr.empty())
      cmg.www_mget(urls,1,NULL,"",0,status); // not going through a proxy.
    else cmg.www_mget(urls,1,NULL,
                        udb_service_configuration::_config->_p2p_proxy_addr,
                        udb_service_configuration::_config->_p2p_proxy_port,
                        status); // through a proxy.
    if (status[0] != 0)
      {
        // failed connection.
        delete[] cmg._outputs;
        std::string port_str = (port != -1) ? ":" + miscutil::to_string(port) : "";
        std::string msg = "failed connection or transmission error in response to fetching record "
                          + key + " from " + host + port_str + path;
        errlog::log_error(LOG_LEVEL_ERROR,msg.c_str());
        throw sp_exception(UDBS_ERR_CONNECT,msg);
      }
    else if (status[0] && !cmg._outputs[0])
      {
        // no result.
        delete cmg._outputs[0];
        delete[] cmg._outputs;
        return NULL;
      }
    db_record *dbr = udb_client::deserialize_found_record(*cmg._outputs[0],pn);
    delete cmg._outputs[0];
    delete[] cmg._outputs;
    if (!dbr)
      {
        // transmission or deserialization error.
        std::string port_str = (port != -1) ? ":" + miscutil::to_string(port) : "";
        std::string msg = "transmission or deserialization error fetching record "
                          + key + " from " + host + port_str + path;
        errlog::log_error(LOG_LEVEL_ERROR,msg.c_str());
        throw sp_exception(UDBS_ERR_DESERIALIZE,msg);
      }
    return dbr;
  }

  db_record* udb_client::find_bqc(const std::string &host,
                                  const int &port,
                                  const std::string &path,
                                  const std::string &query,
                                  const uint32_t &expansion) throw (sp_exception)
  {
    static std::string ctype = "Content-Type: application/x-protobuf";

    // create halo of hashes.
    hash_multimap<uint32_t,DHTKey,id_hash_uint> qhashes;
    qprocess::generate_query_hashes(query,0,5,qhashes); // TODO: 5 in configuration (cf).
    std::string msg;
    try
      {
        errlog::log_error(LOG_LEVEL_DEBUG, "udb_client::find_bqc(): Calling halo_msg_wrapper::serialize(%d,???,%s) ...", expansion, msg.c_str());
        halo_msg_wrapper::serialize(expansion,qhashes,msg);
      }
    catch(sp_exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR,e.what().c_str());
        throw e;
      }

    std::string url = host;
    if (port != -1)
      url += ":" + miscutil::to_string(port);
    url += path + "/find_bqc?output=json";
    curl_mget cmg(1,udb_service_configuration::_config->_call_timeout,0,
                  udb_service_configuration::_config->_call_timeout,0);
    std::vector<std::string> urls;
    urls.reserve(1);
    urls.push_back(url);
    errlog::log_error(LOG_LEVEL_DEBUG, "udb_client::find_bqc(): url=%s,msg=%s,msg.length()=%d", url.c_str(), msg.c_str(), msg.length());
    std::vector<int> status;
    if (udb_service_configuration::_config->_p2p_proxy_addr.empty())
      cmg.www_mget(urls,1,NULL,"",0,status,
                   NULL,NULL,"POST",&msg,msg.length()*sizeof(char),
                   ctype); // not going through a proxy.
    else cmg.www_mget(urls,1,NULL,
                        udb_service_configuration::_config->_p2p_proxy_addr,
                        udb_service_configuration::_config->_p2p_proxy_port,
                        status,NULL,NULL,"POST",&msg,msg.length()*sizeof(char));
    errlog::log_error(LOG_LEVEL_DEBUG, "udb_client::find_bqc(): status[0]=%d", status[0]);
    if (status[0] !=0)
      {
        // failed connection.
        std::string port_str = (port != -1) ? ":" + miscutil::to_string(port) : "";
        std::string msg = "failed connection or transmission error, nothing found in find_bqc response to query "
                          + query + " from " + host + port_str + path;
        errlog::log_error(LOG_LEVEL_DEBUG,msg.c_str());
        delete[] cmg._outputs;
        throw sp_exception(UDBS_ERR_CONNECT,msg);
      }
    else if (status[0] == 0 &&
             (!cmg._outputs[0]
              || (cmg._outputs[0] && *cmg._outputs[0]=="{\"error\":\"not found\"}")))
      {
        // no result.
        delete cmg._outputs[0];
        delete[] cmg._outputs;
        return NULL;
      }
    db_record *dbr = udb_client::deserialize_found_record(*cmg._outputs[0],"query-capture");
    delete cmg._outputs[0];
    delete[] cmg._outputs;
    if (!dbr)
      {
        // transmission or deserialization error.
        std::string port_str = (port != -1) ? ":" + miscutil::to_string(port) : "";
        std::string msg = "transmission or deserialization error fetching batch records for query "
                          + query + " from " + host + port_str + path;
        errlog::log_error(LOG_LEVEL_ERROR,msg.c_str());
        throw sp_exception(UDBS_ERR_DESERIALIZE,msg);
      }
    return dbr;
  }

  db_record* udb_client::deserialize_found_record(const std::string &str, const std::string &pn)
  {
    plugin *pl = plugin_manager::get_plugin(pn);
    if (!pl)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"plugin %s not found for deserializing udb served record",pn.c_str());
        return NULL;
      }
    db_record *dbr = pl->create_db_record();
    int serr = dbr->deserialize_compressed(str);
    if (serr == 0)
      return dbr;
    else
      {
        delete dbr;
        return NULL;
      }
  }

} /* end of namespace. */
