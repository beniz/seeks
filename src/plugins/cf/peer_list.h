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
#include "sweeper.h"
#include "mutexes.h"

using sp::sweepable;

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
      peer();

      peer(const std::string &host,
           const int &port,
           const std::string &path,
           const std::string &rsc);

      virtual ~peer();

      static std::string generate_key(const std::string &host,
                                      const int &port,
                                      const std::string &path);

      // safe setters & getters.
      void set_status_ok();
      void set_status_no_connect();
      void set_status_unknown();
      enum PEER_STATUS get_status();

      std::string _host;
      int _port;
      std::string _path;
      enum PEER_STATUS _status;
      sp_mutex_t _st_mutex; // mutex around status variable.
      short _retries;
      std::string _rsc; // "tt", "sn" or "bsn", that is tokyo tyrant, seeks node, or 'batch' seeks node.
      std::string _key;
  };

  class peer_list;

  class dead_peer : public peer, public sweepable
  {
    public:
      dead_peer();

      dead_peer(const std::string &host,
                const int &port,
                const std::string &path,
                const std::string &rsc);

      virtual ~dead_peer();

      virtual bool sweep_me();

      void update_last_check();

      bool is_alive() const;

      time_t _last_check;

      static peer_list *_dpl; /**< pointer to peer_list, for removal. */
      static peer_list *_pl; /**< pointer to full list of peers, for status update. */
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

      peer* get(const std::string &key);

      hash_map<const char*,peer*,hash<const char*>,eqstr> _peers;

      sp_mutex_t _pl_mutex;
  };

} /* end of namespace. */

#endif
