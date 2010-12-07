/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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

#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include "dht_err.h"
#include "dht_exception.h"
#include "NetAddress.h"

#include <netdb.h>
#include <string>

namespace dht
{
  class rpc_client
  {
    public:
      rpc_client();

      virtual ~rpc_client();

      void do_rpc_call(const NetAddress &server_na,
                       const std::string &msg,
                       const bool &need_response,
                       std::string &response) const throw (dht_exception);

      int open() const throw (dht_exception);

      void send(int fd,
                const NetAddress &server_na,
                const std::string &msg) const throw (dht_exception);

      struct addrinfo* resolve(const NetAddress &server_na) const throw (dht_exception);

      void write(int fd, struct addrinfo* result, const std::string &msg) const throw (dht_exception);

      void receive(int fd, std::string &response) const;

      void wait(int fd) const throw (dht_exception);

      void read(int fd, std::string &response) const throw (dht_exception);

  };

} /* end of namespace. */

#endif
