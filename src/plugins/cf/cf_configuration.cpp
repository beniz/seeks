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
#define hash_dead_peer_check          1043267473ul  /* "dead-peer-check" */
#define hash_dead_peer_retries         681362871ul  /* "dead-peer-retries" */
#define hash_post_url_check           3323226172ul  /* "post-url-check" */
#define hash_post_radius              2436628877ul  /* "post-radius" */
#define hash_post_ua                  1442804836ul  /* "post-ua" */
#define hash_stop_words_filtering     4002206625ul  /* "stop-words-filtering" */
#define hash_remote_post              4059800377ul  /* "remote-post" */

  cf_configuration* cf_configuration::_config = NULL;

  cf_configuration::cf_configuration(const std::string &filename)
    :configuration_spec(filename)
  {
    // external static variables.
    _pl = new peer_list();
    _dpl = new peer_list();
    dead_peer::_pl = _pl;
    dead_peer::_dpl = _dpl;

    load_config();
  }

  cf_configuration::~cf_configuration()
  {
    // XXX: should mutex to avoid crash (this is only called when node is dying, for now).
    dead_peer::_pl = NULL;
    dead_peer::_dpl = NULL;
    delete _pl;
    delete _dpl;
  }

  void cf_configuration::set_default_config()
  {
    _domain_name_weight = 0.7;
    _record_cache_timeout = 600; // 10 mins.
    _dead_peer_check = 300; // 5 mins.
    _dead_peer_retries = 3;
    _post_url_check = true;
    _post_radius = 5;
    _post_url_ua = "Mozilla/5.0 (X11; Linux x86_64; rv:2.0.1) Gecko/20100101 Firefox/4.0.1"; // default.
    _stop_words_filtering = false;
    _remote_post = true;
  }

  void cf_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
      char *buf, const unsigned long &linenum)
  {
    char tmp[BUFFER_SIZE];
    int vec_count;
    char *vec[4];
    int port;
    std::vector<std::string> elts;
    std::string host, path, address, port_str;

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
        urlmatch::parse_url_host_and_path(address,host,path);
        miscutil::tokenize(host,elts,":");
        port = -1;
        if (elts.size()>1)
          {
            host = elts.at(0);
            port = atoi(elts.at(1).c_str());
          }
        port_str = (port != -1) ? ":" + miscutil::to_string(port) : "";
        errlog::log_error(LOG_LEVEL_DEBUG,"adding peer %s%s%s with resource %s",
                          host.c_str(),port_str.c_str(),path.c_str(),vec[1]);
        _pl->add(host,port,path,std::string(vec[1]));
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Remote peer address for collaborative filtering");
        break;

      case hash_dead_peer_check:
        _dead_peer_check = atoi(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Interval of time between two dead peer checks");
        break;

      case hash_dead_peer_retries:
        _dead_peer_retries = atoi(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Number of retries before marking a peer as dead");
        break;

      case hash_post_url_check:
        _post_url_check = static_cast<bool>(atoi(arg));
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Whether to ping and check on posted URLs");
        break;

      case hash_post_radius:
        _post_radius = static_cast<bool>(atoi(arg));
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Query similarity impact radius of posted URLs");
        break;

      case hash_post_ua:
        _post_url_ua = std::string(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "default 'user-agent' header used to retrieve posted URLs");
        break;

      case hash_stop_words_filtering:
        _stop_words_filtering = static_cast<bool>(atoi(arg));
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Whether to filter similar queries with stop words");
        break;

      case hash_remote_post:
        _remote_post = static_cast<bool>(atoi(arg));
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Whether to allow remote posting of results");
        break;

      default:
        break;
      }
  }

  void cf_configuration::finalize_configuration()
  {
  }

} /* end of namespace. */
