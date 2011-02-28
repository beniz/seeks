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

#include "cf_configuration.h"

namespace seeks_plugins
{
#define hash_domain_name_weight       1333166351ul  /* "domain-name-weight" */
#define hash_record_cache_timeout     1954675964ul  /* "record-cache-timeout" */

  cf_configuration* cf_configuration::_config = NULL;

  cf_configuration::cf_configuration(const std::string &filename)
    :configuration_spec(filename)
  {
    load_config();
  }

  cf_configuration::~cf_configuration()
  {
  }

  void cf_configuration::set_default_config()
  {
    _domain_name_weight = 0.7;
    _record_cache_timeout = 600; // 10 mins.
  }

  void cf_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
      char *buf, const unsigned long &linenum)
  {
    switch (cmd_hash)
      {
      case hash_domain_name_weight:
        _domain_name_weight = atof(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Weight given to the domain names in the simple filter");
        break;

      case hash_record_cache_timeout:
        _record_cache_timeout = atoi(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Timeout on cached remote records, in seconds");
        break;

      default:
        break;
      }
  }

  void cf_configuration::finalize_configuration()
  {
  }

} /* end of namespace. */
