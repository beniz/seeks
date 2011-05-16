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
  if (argc < 5)
    {
      std::cout << "Usage: <db_file> <seeks base dir> <plugin_name> <list of record keys, if keys ends with '*', does matching of the WHOLE string, not just the end of it>\n";
      exit(0);
    }

  std::string dbfile = argv[1];
  std::string basedir = argv[2];
  std::string plugin_name = argv[3];
  std::vector<std::string> rkeys;
  int i=4;
  while (i<argc)
    {
      rkeys.push_back(argv[i]);
      i++;
    }

  seeks_proxy::_configfile = "config";
  seeks_proxy::_configfile = basedir + "/config";

  seeks_proxy::initialize_mutexes();
  errlog::init_log_module();
  errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO);

  seeks_proxy::_basedir = basedir.c_str();
  plugin_manager::_plugin_repository = basedir + "/plugins/";
  seeks_proxy::_config = new proxy_configuration(seeks_proxy::_configfile);

  seeks_proxy::_user_db = new user_db(dbfile);
  seeks_proxy::_user_db->open_db();

  plugin_manager::load_all_plugins();
  plugin_manager::start_plugins();


  for (size_t i=0; i<rkeys.size(); i++)
    {
      std::string rkey = rkeys.at(i);
      if (rkey[rkey.length()-1] == '*')
        {
          std::vector<std::string> matching_keys;
          rkey = rkey.substr(0,rkey.length()-2);
          seeks_proxy::_user_db->find_matching(rkey,plugin_name,matching_keys);
          size_t nmk = matching_keys.size();
          for (size_t j=0; j<nmk; j++)
            seeks_proxy::_user_db->remove_dbr(matching_keys.at(j));
        }
      else
        {
          rkey = user_db::generate_rkey(rkeys.at(i),plugin_name);
          seeks_proxy::_user_db->remove_dbr(rkey);
        }
    }

  seeks_proxy::_user_db->close_db();
}
