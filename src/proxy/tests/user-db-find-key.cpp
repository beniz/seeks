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
  if (argc < 4)
    {
      std::cout << "Usage: <db_file> <seeks base dir> <key string to be found>\n";
      exit(0);
    }

  std::string dbfile = argv[1];
  std::string basedir = argv[2];
  std::string key_str = argv[3];

  seeks_proxy::_configfile = "config";
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

  std::vector<std::string> matching_keys;
  seeks_proxy::_user_db->find_matching(key_str,"",matching_keys);
  std::cout << "Found " << matching_keys.size() << " records with matching keys:\n";

  for (size_t i=0; i<matching_keys.size(); i++)
    {
      if (matching_keys.at(i) == "db-version")
        {
          const char *keyc = matching_keys.at(i).c_str();
          int value_size;
          void *value = seeks_proxy::_user_db->_hdb->dbget(keyc,strlen(keyc),&value_size);
          std::cout << "db_record[db-version]: " << *((double*)value) << std::endl;
          continue;
        }

      std::string plugin_name, key;
      user_db::extract_plugin_and_key(matching_keys.at(i),
                                      plugin_name,key);
      db_record *dbr = seeks_proxy::_user_db->find_dbr(key,plugin_name);
      std::cout  << "db_record[" << key << "]\n\tplugin_name: " << dbr->_plugin_name
                 << "\n\tcreation time: " << dbr->_creation_time << std::endl;
      dbr->print(std::cout);
      std::cout << std::endl;
      delete dbr;
    }

  seeks_proxy::_user_db->close_db();
}
