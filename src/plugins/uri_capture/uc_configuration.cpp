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

#include "uc_configuration.h"
#include "errlog.h"

using sp::errlog;

namespace seeks_plugins
{
#define hash_uc_sweep_cycle             3107023570ul  /* "uc-sweep-cycle" */
#define hash_uc_retention               1589330358ul  /* "uc-retention" */

  uc_configuration* uc_configuration::_config = NULL;

  uc_configuration::uc_configuration(const std::string &filename)
      :configuration_spec(filename)
  {
    if (uc_configuration::_config)
      delete uc_configuration::_config;
    uc_configuration::_config = this;
    load_config();
  }

  uc_configuration::~uc_configuration()
  {
  }

  void uc_configuration::set_default_config()
  {
    _sweep_cycle = 2592000;  // one month, in seconds.
    _retention = 15552000;   // six months, in seconds.
  }

  void uc_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
      char *buf, const unsigned long &linenum)
  {
    switch (cmd_hash)
      {
      case hash_uc_sweep_cycle :
        _sweep_cycle = atoi(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Time between two sweeping cycles of the URI user db records, in seconds");
        break;

      case hash_uc_retention:
        _retention = atoi(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "URI user db retention of records, in seconds");
        break;

      default:
        break;
      }
  }

  void uc_configuration::finalize_configuration()
  {
  }

} /* end of namespace. */
