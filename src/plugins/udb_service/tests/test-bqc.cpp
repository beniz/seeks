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

#include "udb_client.h"
#include "query_capture.h"
#include "urlmatch.h"
#include "db_query_record.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "errlog.h"

#include <stdlib.h>
#include <iostream>

using namespace sp;
using namespace seeks_plugins;

std::string get_usage()
{
  std::string usage = "Usage: <query> <host URL> <seeks base dir>";
  return usage;
}

int main(int argc, char **argv)
{
  if (argc < 4)
    {
      std::cout << get_usage() << std::endl;
      exit(0);
    }

  std::string query = argv[1];
  std::string url = argv[2];
  std::string basedir = argv[3];

  seeks_proxy::initialize_mutexes();
  errlog::init_log_module();
  errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO | LOG_LEVEL_DEBUG);

  seeks_proxy::_basedir = basedir.c_str();
  plugin_manager::_plugin_repository = basedir + "/plugins/";
  seeks_proxy::_config = new proxy_configuration(seeks_proxy::_configfile);

  plugin_manager::load_all_plugins();
  plugin_manager::start_plugins();

  int port = -1;
  std::string host,path;
  urlmatch::parse_url_host_and_path(url,host,path);
  std::vector<std::string> elts;
  miscutil::tokenize(host,elts,":");
  if (elts.size()>1)
    {
      host = elts.at(0);
      port = atoi(elts.at(1).c_str());
    }

  udb_client udbc;
  db_record *dbr = NULL;
  try
    {
      udbc.find_bqc(host,port,path,query,1);
    }
  catch(sp_exception &e)
    {
      std::cout << e.what() << std::endl;
      exit(-1);
    }
  if (dbr)
    {
      db_query_record *dbqr = static_cast<db_query_record*>(dbr);
      dbqr->print(std::cout);
      delete dbqr;
    }
  else
    {
      std::cout << "No results found\n";
    }
}
