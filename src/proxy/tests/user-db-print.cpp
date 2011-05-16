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

#include "user_db.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "errlog.h"
#include "plugin_manager.h"

#include <iostream>
#include <stdlib.h>

using namespace sp;

int main(int argc, char **argv)
{
  if (argc < 3)
    {
      std::cout << "Usage: <db_file> <seeks base dir>\n";
      exit(0);
    }

  std::string dbfile = argv[1];
  std::string basedir = argv[2];

  seeks_proxy::_configfile = basedir + "/config";

  seeks_proxy::initialize_mutexes();
  errlog::init_log_module();
  errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO);

  seeks_proxy::_basedir = basedir.c_str();
  plugin_manager::_plugin_repository = basedir + "/plugins/";
  seeks_proxy::_config = new proxy_configuration(seeks_proxy::_configfile);

  seeks_proxy::_user_db = new user_db(dbfile);
  seeks_proxy::_user_db->open_db_readonly();

  plugin_manager::load_all_plugins();
  plugin_manager::start_plugins();

  seeks_proxy::_user_db->print(std::cout);

  seeks_proxy::_user_db->close_db();
}
