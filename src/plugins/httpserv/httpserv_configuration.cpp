/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "httpserv_configuration.h"

namespace seeks_plugins
{

#define hash_server_port               2258587232ul /* "server-port" */
#define hash_server_host                494776476ul /* "server-host" */

  httpserv_configuration* httpserv_configuration::_hconfig = NULL;

  httpserv_configuration::httpserv_configuration(const std::string &filename)
      :configuration_spec(filename)
  {
    load_config();
  }

  httpserv_configuration::~httpserv_configuration()
  {
  }

  void httpserv_configuration::set_default_config()
  {
    _port = 8080;
    _host = "localhost";
  }

  void httpserv_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
      char *buf, const unsigned long &linenum)
  {
    switch (cmd_hash)
      {
      case hash_server_port:
        _port = atoi(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,"HTTP server listening port.");
        break;

      case hash_server_host:
        _host = std::string(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,"HTTP server host.");
        break;

      default:
        break;
      } // end of switch.
  }

  void httpserv_configuration::finalize_configuration()
  {
  }

} /* end of namespace. */
