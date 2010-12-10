/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
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

#ifndef NETADDRESS_H
#define NETADDRESS_H

#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>

namespace dht
{
  class NetAddress
  {
      friend std::ostream &operator<<(std::ostream& output, const NetAddress& na);

    public:
      NetAddress();

      NetAddress(const std::string& naddress, const unsigned short& port);

      // deserialize constructor.
      NetAddress(const uint32_t &ip, const std::string &port);

      NetAddress(const NetAddress &na);

      ~NetAddress() {};

      bool empty() const;
      std::string getNetAddress() const
      {
        return _net_address;
      }
      unsigned short getPort() const
      {
        return _port;
      }
      void setNetAddress(const std::string& naddress)
      {
        _net_address = naddress;
      }
      void setPort(const unsigned short& p)
      {
        _port = p;
      }

      /**
       * \brief converts to a full address string.
       */
      std::string toString(const std::string& protocol="",
                           const std::string& aend="") const;

      /**
       * operators.
       */
      bool operator==(const NetAddress& na) const;
      bool operator!=(const NetAddress& na) const;

      /**
       * printing.
       */
      std::ostream& print (std::ostream &output) const;

      /**
       * serialization.
       */
      uint32_t serialize_ip() const;
      static std::string unserialize_ip(const uint32_t &ip);

      std::string serialize_port() const;
      static unsigned short unserialize_port(const std::string &p);

    private:
      /**
       * ip or dns address.
       */
      std::string _net_address;

      /**
       * communication port.
       */
      unsigned short _port;
  };

} /* end of namespace. */

#endif
