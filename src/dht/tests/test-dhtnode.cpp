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

#include "DHTNode.h"
#include "miscutil.h"
#include "errlog.h"

#include <iostream>
#include <string.h>
#include <stdlib.h>

using namespace dht;
using sp::miscutil;
using sp::errlog;

const char *usage = "Usage: test_dhtnode <ip:port> (--join ip:port) or (--self-bootstrap)\n";

int main(int argc, char **argv)
{
   if (argc < 2)
     {
	std::cout << usage;
	exit(0);
     }
   
   char *net_addr = argv[1];
   char *vec1[2];
   miscutil::ssplit(net_addr,":",vec1,SZ(vec1),0,0);
   net_addr = vec1[0];
   int net_port = atoi(vec1[1]);

   // init logging module.
   errlog::init_log_module();
   errlog::set_debug_level(LOG_LEVEL_ERROR | LOG_LEVEL_DHT);
   
   // TODO: thread it ?
   DHTNode node(net_addr,net_port);
   
   bool joinb = false;
   bool sbootb = false;
   if (argc > 2)
     {
	const char *joinopt = argv[2];
	
	if (strcmp(joinopt,"--join")==0)
	  joinb = true;
	else if (strcmp(joinopt,"--self-bootstrap")==0)
	  sbootb = true;
	else 
	  {
	     std::cout << usage;
	     exit(0);
	  }
	
	if (joinb)
	  {
	     char *bootnode = argv[3];
	     char* vec[2];
	     miscutil::ssplit(bootnode,":",vec,SZ(vec),0,0);
	     
	     int bootstrap_port = atoi(vec[1]);
	     std::cout << "bootstrap node at ip: " << vec[0] << " -- port: " << bootstrap_port << std::endl;
	     
	     std::vector<NetAddress> bootstrap_nodelist;
	     NetAddress bootstrap_na(vec[0],bootstrap_port);
	     bootstrap_nodelist.push_back(bootstrap_na);
	     
	     bool reset = true;
	     if (joinb)
	       node.join_start(bootstrap_nodelist,reset);
	  }
	else if (sbootb)
	  node.self_bootstrap();
     }
   	
   pthread_join(node._l1_server->_rpc_server_thread,NULL);
}

  
