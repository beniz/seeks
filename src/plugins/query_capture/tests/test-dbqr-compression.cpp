/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#include "db_query_record.h"
#include "seeks_proxy.h"
#include "qprocess.h"
#include "plugin_manager.h"
#include "proxy_configuration.h"
#include "rank_estimators.h"
#include "errlog.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>

using namespace seeks_plugins;
using namespace sp;
using namespace lsh;

int main(int argc, char **argv)
{
  if (argc < 5)
    {
      std::cout << "Usage: <query> <db_file> <seeks base dir> <compress: 0 or 1>\n";
      exit(0);
    }

  std::string query = argv[1];
  std::string dbfile = argv[2];
  std::string basedir = argv[3];
  int compress = atoi(argv[4]);

  seeks_proxy::_configfile = basedir + "/config";
  seeks_proxy::_lshconfigfile = basedir + "/lsh/lsh-config";

  seeks_proxy::initialize_mutexes();
  errlog::init_log_module();
  errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO);

  seeks_proxy::_basedir = basedir.c_str();
  plugin_manager::_plugin_repository = basedir + "/plugins/";
  seeks_proxy::_config = new proxy_configuration(seeks_proxy::_configfile);
  seeks_proxy::_config->_user_db_startup_check = false;
  seeks_proxy::_config->_user_db_optimize = false;
  seeks_proxy::_lsh_config = new lsh_configuration(seeks_proxy::_lshconfigfile);

  seeks_proxy::_user_db = new user_db(dbfile);
  seeks_proxy::_user_db->open_db_readonly();

  plugin_manager::load_all_plugins();
  plugin_manager::start_plugins();

  hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
  rank_estimator::fetch_user_db_record(query,seeks_proxy::_user_db,
                                       records);
  db_query_record *dbr = NULL;
  std::string lang;
  hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
  hash_map<const char*,std::vector<query_data*>,hash<const char*>,eqstr> inv_qdata;
  rank_estimator::extract_queries(query,lang,1,seeks_proxy::_user_db,records,qdata,inv_qdata);
  if (!qdata.empty())
    dbr = new db_query_record(qdata); // no copy.
  else dbr = NULL;

  if (!dbr)
    {
      std::cout << "no record found!\n";
      exit(1);
    }
  std::string str;
  time_t start = clock();
  if (compress == 1)
    dbr->serialize_compressed(str);
  else dbr->serialize(str);
  time_t stop = clock();

  std::cout << "record size (bytes): " << str.size() * sizeof(char);
  if (compress == 1)
    std::cout << " (compressed)";
  std::cout << std::endl;
  std::cout << "in " << static_cast<double>((stop - start) / CLOCKS_PER_SEC) << " seconds.\n";

  delete dbr;
  seeks_proxy::_user_db->close_db();
}
