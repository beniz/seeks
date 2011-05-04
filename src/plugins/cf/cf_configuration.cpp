/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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
#include "miscutil.h"
#include "urlmatch.h"
#include "errlog.h"

#include <iostream>

using sp::miscutil;
using sp::urlmatch;
using sp::errlog;

namespace seeks_plugins
{
#define hash_domain_name_weight       1333166351ul  /* "domain-name-weight" */
#define hash_record_cache_timeout     1954675964ul  /* "record-cache-timeout" */
#define hash_cf_peer                  1520012134ul  /* "cf-peer" */

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
    char tmp[BUFFER_SIZE];
    int vec_count;
    char *vec[4];
    int port;
    char *endptr;
    std::vector<std::string> elts;
    std::string host, path, address;

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

      case hash_cf_peer:
        strlcpy(tmp,arg,sizeof(tmp));
        vec_count = miscutil::ssplit(tmp," \t",vec,SZ(vec),1,1);
        div_t divresult;
        divresult = div(vec_count,2);
        if (divresult.rem != 0)
          {
            errlog::log_error(LOG_LEVEL_ERROR,"Wrong number of parameter when specifying static collaborative filtering peer");
            break;
          }
        address = vec[0];
        std::cerr << "address: " << address << std::endl;
        urlmatch::parse_url_host_and_path(address,host,path);
        miscutil::tokenize(host,elts,":");
        port = -1;
        if (elts.size()>1)
          port = atoi(elts.at(1).c_str());
        std::cerr << "host: " << host << " -- path: " << path << " -- port: " << port << " -- rsc: " << vec[1] << std::endl;
        _pl.add(host,port,path,std::string(vec[1]));
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Remote peer address for collaborative filtering");
        break;

      default:
        break;
      }
  }

  void cf_configuration::finalize_configuration()
  {
  }

} /* end of namespace. */
