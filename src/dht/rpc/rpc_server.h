/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2009, 2010  Emmanuel Benazera, juban@free.fr
 * Copyright (C) 2010  Loic Dachary <loic@dachary.org>
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

#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include "NetAddress.h"
#include "mutexes.h"

#include <sys/select.h>
#include <pthread.h>

namespace dht
{
  class rpc_server
  {
    public:
      rpc_server(const std::string &hostname, const short &port);

      virtual ~rpc_server();

      void run();
      
      void bind();
      
      void run_loop_once();
      
      void run_thread();
      
      void stop_thread();
      
      void close_socket();

      static void run_static(rpc_server *server);

      /*- server responses. -*/
      virtual void serve_response(const std::string &msg,
                                     const std::string &addr,
                                     std::string &resp_msg);

    public:
      NetAddress _na;
      pthread_t _rpc_server_thread;
      int _udp_sock;
      bool _abort;
      sp_mutex_t _run_mutex;
      fd_set _rfds;
      timeval _select_timeout;
  };

} /* end of namespace. */

#endif
