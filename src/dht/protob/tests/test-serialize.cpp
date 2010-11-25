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

#include "l1_protob_wrapper.h"
#include "miscutil.h"
#include "serialize.h"
#include "NetAddress.h"

#include <iostream>
#include <string>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "DHTKey.h"

using namespace sp;
using namespace dht;

int main(int argc, char *argv[])
{
   /*std::string fct = "get_successor";
   const uint32_t &fct_id 
     = miscutil::hash_string(fct.c_str(),fct.size());
   
   DHTKey dk = DHTKey::randomKey();
   std::vector<unsigned char> dkser = DHTKey::serialize(dk);
   std::string recipient_key(dkser.begin(),dkser.end());
   
   NetAddress na("127.0.0.1",8118);

   uint32_t recipient_ip_addr = na.serialize_ip();
   std::string ip_str = NetAddress::unserialize_ip(recipient_ip_addr);
   // std::cout << "ip_str: " << ip_str << std::endl;
   
   std::string recipient_net_port = na.serialize_port();
   short port2 = NetAddress::unserialize_port(recipient_net_port);
   // std::cout << "port2: " << port2 << std::endl;
   
   dht::l1::l1_query *l1q 
     = l1_protob_wrapper::create_l1_query(fct_id,recipient_key,
					 recipient_ip_addr,
					 recipient_net_port);
   std::string ser_query;
   l1q->SerializeToString(&ser_query);
   
   dht::l1::l1_query l1q2;
   l1q2.ParseFromString(ser_query);

   std::string dstr = l1q2.DebugString();
   std::cout << "dstr: " << dstr << std::endl; */
   
   std::cout << "Serialization...\n";
   std::string fct = "get_successor";
   const uint32_t &fct_id
     = miscutil::hash_string(fct.c_str(),fct.size());
   DHTKey recipient_key = DHTKey::randomKey();
   NetAddress recipient_na("10.0.0.2",8118);
   DHTKey sender_key = DHTKey::randomKey();
   NetAddress sender_na("10.0.0.1",8120);
   DHTKey node_key;
   
   l1::l1_query* l1q = l1_protob_wrapper::create_l1_query(fct_id,
							  recipient_key,recipient_na,
							  sender_key,sender_na,node_key);
   std::cout << l1q->DebugString() << std::endl;
   
   std::string str;
   l1_protob_wrapper::serialize_to_string(l1q,str);
   
   std::cout << "Deserialization...\n";
   l1::l1_query *l1qbis = new l1::l1_query();
   l1_protob_wrapper::deserialize(str,l1qbis);
   
   uint32_t layer_id;
   uint32_t fid;
   DHTKey rkey;
   NetAddress rna;
   DHTKey skey;
   NetAddress sna;
   DHTKey nodekey;
   l1_protob_wrapper::read_l1_query(l1qbis,layer_id,fid,
						  rkey,rna,skey,sna,nodekey);

   std::cout << l1qbis->DebugString() << std::endl;
}



