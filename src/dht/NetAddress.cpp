/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
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

#include "NetAddress.h"
#include "miscutil.h"
#include "serialize.h"
#include <stdlib.h>

using sp::miscutil;
using sp::serialize;

namespace dht
{
   NetAddress::NetAddress()
     : _net_address(""), _port(0)
       {
       }
   
   NetAddress::NetAddress(const std::string& naddress, const short& port)
     : _net_address(naddress), _port(port)
       {
       }

   NetAddress::NetAddress(const uint32_t &ip, const std::string &port)
     {
	_net_address = NetAddress::unserialize_ip(ip);
	_port = NetAddress::unserialize_port(port);
     }
   	
   NetAddress::NetAddress(const NetAddress &na)
     :_net_address(na.getNetAddress()),_port(na.getPort())
       {
       }

   bool NetAddress::empty() const
     {
	return _net_address.empty();
     }
      
   std::string NetAddress::toString(const std::string& protocol,
				    const std::string& aend) const
     {
	std::string port_str = miscutil::to_string(_port);
	std::string res = protocol + _net_address 
	  + ":" + port_str + aend;
	return res;
     }

   bool NetAddress::operator==(const NetAddress& na) const
     {
	if (_net_address == na.getNetAddress()
	    && _port == na.getPort())
	  return true;
	return false;
     }

   bool NetAddress::operator!=(const NetAddress& na) const
     {
	if (_net_address != na.getNetAddress()
	    || _port != na.getPort())
	  return true;
	return false;
     }
   
   /**
    * printing.
    */
   std::ostream& NetAddress::print (std::ostream& output) const
     {
	output << "ip/port: " << _net_address << ":" << _port;
	return output;
     }

   std::ostream &operator<<(std::ostream& output, const NetAddress& na)
     {	
	return na.print(output);
     }
   
   std::ostream &operator<<(std::ostream& output, NetAddress* na)
     {
	return na->print(output);
     }

   /**
    * serialization.
    */
   uint32_t NetAddress::serialize_ip() const
     {
	uint32_t res = 0;
	std::vector<std::string> tokens;
	miscutil::tokenize(_net_address,tokens,".");
	for (int i=3;i>=0;i--)
	  {
	     unsigned char seg = atoi(tokens.at(i).c_str());
	     res  = (res << 8*i) + seg;
	  }
	return res;
     }
   
   std::string NetAddress::unserialize_ip(const uint32_t &ip)
     {
	std::string res;
	
	unsigned int mask = 0;
	for (unsigned int i = 0; i < 8; ++i)
	  mask = (mask << 1) | 0x1;
	
	for (int i=0;i<4; i++)
	  {
	     uint32_t seg = (ip >> 8*i) & mask;
	     std::ostringstream os;
	     os << seg;
	     res += os.str();
	     if (i<3)
	       res += ".";
	  }
	return res;
     }

   std::string NetAddress::serialize_port() const
     {
	std::vector<unsigned char> v = serialize::to_network_order(_port);
	std::string res(v.begin(),v.end());
	return res;
     }
   
   short NetAddress::unserialize_port(const std::string &p)
     {
	std::vector<unsigned char> v;
	std::copy(p.begin(),p.end(),std::back_inserter(v));
	short res = 0;
	res = serialize::from_network_order(res,v);
	return res;
     }
      
} /* end of namespace. */
