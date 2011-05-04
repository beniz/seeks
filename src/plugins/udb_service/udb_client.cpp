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
                                         const std::string &pn)
  {
    std::string url = host;
    if (port != -1)
      url += ":" + miscutil::to_string(port);
    url += path + "/find_dbr?";
    url += "urkey=" + key;
    url += "&pn=" + pn;
    curl_mget cmg(1,3,0,3,0); // timeouts: 3 seconds. TODO: in config.
    std::vector<std::string> urls;
    urls.reserve(1);
    urls.push_back(url);
    cmg.www_mget(urls,1,NULL,"",0); // not going through a proxy. TODO: support for external proxy.
    if (!cmg._outputs[0])
      {
        // no result or failed connection.
        delete[] cmg._outputs;
        return NULL;
      }
    db_record *dbr = udb_client::deserialize_found_record(*cmg._outputs[0],pn);
    delete[] cmg._outputs;
    if (!dbr)
      {
        // transmission or deserialization error.
        errlog::log_error(LOG_LEVEL_ERROR,"transmission or deserialization error fetching record %s from %s:%s",
                          key.c_str(),host.c_str(),port!=-1 ? miscutil::to_string(port).c_str() : "");
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
    int serr = dbr->deserialize(str);
    if (serr == 0)
      return dbr;
    else
      {
        delete dbr;
        return NULL;
      }
  }

} /* end of namespace. */
