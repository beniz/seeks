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

namespace seeks_plugins
{
#define hash_call_timeout                         3944957977ul  /* "call-timeout" */

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
  }

  void udb_service_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
      char *buf, const unsigned long &linenum)
  {
    switch(cmd_hash)
      {
      case hash_call_timeout:
        _call_timeout = atol(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Sets the timeout on connection and transfer of data from P2P net.");
        break;

      default:
        break;
      }
  }

  void udb_service_configuration::finalize_configuration()
  {
  }

} /* end of namespace. */
