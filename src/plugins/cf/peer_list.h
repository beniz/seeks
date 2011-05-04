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

#ifndef PEER_LIST_H
#define PEER_LIST_H

#include "stl_hash.h"

namespace seeks_plugins
{

  enum PEER_STATUS
  {
    PEER_OK,
    PEER_NO_CONNECT,
    PEER_UNKNOWN
  };

  class peer
  {
    public:
      peer(const std::string &host,
           const int &port,
           const std::string &path,
           const std::string &rsc);

      ~peer();

      static std::string generate_key(const std::string &host,
                                      const int &port,
                                      const std::string &path);

      std::string _host;
      int _port;
      std::string _path;
      enum PEER_STATUS _status;
      std::string _rsc; // "tt" or "sn", tokyo tyrant or seeks node.
      std::string _key;
  };

  class peer_list
  {
    public:
      peer_list();

      ~peer_list();

      void add(const std::string &host,
               const int &port,
               const std::string &path,
               const std::string &rsc);

      void add(peer *p);

      void remove(const std::string &host,
                  const int &port,
                  const std::string &path);

      void remove(const std::string &key);

      hash_map<const char*,peer*,hash<const char*>,eqstr> _peers;
  };

} /* end of namespace. */

#endif
