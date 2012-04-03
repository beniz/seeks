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

#include "udb_service_configuration.h"
#include "miscutil.h"
#include "errlog.h"

using sp::miscutil;
using sp::errlog;

namespace seeks_plugins
{
#define hash_call_timeout                         3944957977ul  /* "call-timeout" */
#define hash_p2p_proxy                            3750534420ul  /* "p2p-proxy-addr" */

  udb_service_configuration* udb_service_configuration::_config = NULL;

  udb_service_configuration::udb_service_configuration(const std::string &filename)
    :configuration_spec(filename)
  {
    load_config();
  }

  udb_service_configuration::~udb_service_configuration()
  {
  }

  void udb_service_configuration::set_default_config()
  {
    _call_timeout = 3; // 3 seconds.
    _p2p_proxy_port = -1; // unset.
  }

  void udb_service_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
      char *buf, const unsigned long &linenum)
  {
    std::vector<std::string> vec;
    switch(cmd_hash)
      {
      case hash_call_timeout:
        _call_timeout = atol(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Sets the timeout on connection and transfer of data from P2P net");
        break;

      case hash_p2p_proxy:
        _p2p_proxy_addr = arg;
        miscutil::tokenize(_p2p_proxy_addr,vec,":");
        if (vec.size()!=2)
          {
            errlog::log_error(LOG_LEVEL_ERROR, "wrong address:port for P2P proxy: %s",_p2p_proxy_addr.c_str());
            _p2p_proxy_addr = "";
          }
        else
          {
            _p2p_proxy_addr = vec.at(0);
            _p2p_proxy_port = atoi(vec.at(1).c_str());
          }
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Proxy through which to issue the P2P calls");
        break;

      default:
        break;
      }
  }

  void udb_service_configuration::finalize_configuration()
  {
  }

} /* end of namespace. */
