/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, <ebenazer@seeks-project.info>
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

#include "peer_list.h"
#include "miscutil.h"
#include "errlog.h"

using sp::miscutil;
using sp::errlog;

namespace seeks_plugins
{

  /*- peer -*/
  peer::peer(const std::string &host,
             const int &port,
             const std::string &path,
             const std::string &rsc)
    :_host(host),_port(port),_path(path),_status(PEER_UNKNOWN),_rsc(rsc)
  {
    _key = peer::generate_key(host,port,path);
  }

  peer::~peer()
  {
  }

  std::string peer::generate_key(const std::string &host,
                                 const int &port,
                                 const std::string &path)
  {
    if (port == -1)
      return host + path;
    else return host + ":" + miscutil::to_string(port) + path;
  }

  /*- peer_list -*/
  peer_list::peer_list()
  {
  }

  peer_list::~peer_list()
  {
    hash_map<const char*,peer*,hash<const char*>,eqstr>::iterator hit
    = _peers.begin();
    hash_map<const char*,peer*,hash<const char*>,eqstr>::iterator chit;
    while(hit!=_peers.end())
      {
        chit = hit;
        ++hit;
        delete (*chit).second;
      }
  }

  void peer_list::add(const std::string &host,
                      const int &port,
                      const std::string &path,
                      const std::string &rsc)
  {
    add(new peer(host,port,path,rsc));
  }

  void peer_list::add(peer *p)
  {
    hash_map<const char*,peer*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=_peers.find(p->_key.c_str()))!=_peers.end())
      {
        // update.
        delete (*hit).second;
        (*hit).second = p;
      }
    else _peers.insert(std::pair<const char*,peer*>(p->_key.c_str(),p));

    //TODO: hook up callback for incoming peers.
  }

  void peer_list::remove(const std::string &host,
                         const int &port,
                         const std::string &path)
  {
    std::string key = peer::generate_key(host,port,path);
    remove(key);
  }

  void peer_list::remove(const std::string &key)
  {
    hash_map<const char*,peer*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=_peers.find(key.c_str()))!=_peers.end())
      {
        _peers.erase(hit);
      }
    errlog::log_error(LOG_LEVEL_ERROR,"Cannot find peer %s to remove from peer list",
                      key.c_str());
  }

} /* end of namespace. */
