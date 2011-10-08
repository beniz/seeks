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

#include "query_capture_configuration.h"
#include "errlog.h"

using sp::errlog;

namespace seeks_plugins
{

#define hash_max_radius                    1988906041ul  /* "query-max-radius" */
#define hash_query_sweep_cycle             2195388340ul  /* "query-sweep-cycle" */
#define hash_query_retention               1932741391ul  /* "query-retention" */
#define hash_query_protect_redir            645686780ul  /* "protected-redirection" */
#define hash_save_url_data                 3465855637ul  /* "save-url-data" */
#define hash_cross_post_url                4153795065ul  /* "cross-post-url" */

  query_capture_configuration* query_capture_configuration::_config = NULL;

  query_capture_configuration::query_capture_configuration(const std::string &filename)
    :configuration_spec(filename)
  {
    if (query_capture_configuration::_config)
      delete query_capture_configuration::_config;
    query_capture_configuration::_config = this;
    load_config();
  }

  query_capture_configuration::~query_capture_configuration()
  {
  }

  void query_capture_configuration::set_default_config()
  {
    _max_radius = 5;
    _sweep_cycle = 2592000;  // one month, in seconds.
    _retention = 31104000;   // one year, in seconds.
    _protected_redirection = false; // should be activated on public nodes.
    _save_url_data = true;
    _cross_post_url = ""; // no cross-posting is default.
  }

  void query_capture_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
      char *buf, const unsigned long &linenum)
  {
    switch (cmd_hash)
      {
      case hash_max_radius :
        _max_radius = atoi(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Maximum radius of the query generation halo");
        break;

      case hash_query_sweep_cycle :
        _sweep_cycle = atoi(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Time between two sweeping cycles of the query user db records, in seconds");
        break;

      case hash_query_retention:
        _retention = atoi(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Query user db retention of records, in seconds");
        break;

      case hash_query_protect_redir:
        _protected_redirection = static_cast<bool>(atoi(arg));
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Whether the protection against abusive use of the URL redirection scheme is activated");
        break;

      case hash_save_url_data:
        _save_url_data = static_cast<bool>(atoi(arg));
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Whether to save URL snippet's title and summary.");
        break;

      case hash_cross_post_url:
        _cross_post_url = arg;
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "URL to which to cross-post recommendations.");
        break;

      default:
        break;
      }
  }

  void query_capture_configuration::finalize_configuration()
  {
  }

} /* end of namespace. */
