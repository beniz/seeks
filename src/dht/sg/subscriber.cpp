/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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

#include "subscriber.h"

#include <sys/time.h>

namespace dht
{
   Subscriber::Subscriber()
     :NetAddress()
       {
	  _idkey = DHTKey();
	  set_join_date();
       }
   
   Subscriber::Subscriber(const DHTKey &idkey,
			  const std::string &naddress, const short &port)
     :NetAddress(naddress,port),_idkey(idkey)
       {
	  set_join_date();
       }
   
   Subscriber::Subscriber(const std::string &idkey,
			  const uint32_t &ip, const std::string &port)
     :NetAddress(ip,port)
     {
	std::vector<unsigned char> ser;
	std::copy(idkey.begin(),idkey.end(),std::back_inserter(ser));
	_idkey = DHTKey::unserialize(ser);
	set_join_date();
     }
      
   Subscriber::Subscriber(const Subscriber &su)
     :NetAddress(su.getNetAddress(),su.getPort()),_idkey(su._idkey)
       {
	  set_join_date();
       }

   Subscriber::~Subscriber()
     {
     }
      
   void Subscriber::set_join_date()
     {
	struct timeval tv_now;
	gettimeofday(&tv_now,NULL);
	_join_date = tv_now.tv_sec;
     }
      
   
} /* end of namespace. */
