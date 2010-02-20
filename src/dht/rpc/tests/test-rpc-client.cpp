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
 
#include "rpc_client.h"

#include <stdlib.h>
#include <iostream>

using namespace dht;

int main(int argc, char *argv[])
{
   if (argc<4)
     {
	std::cout << "Usage: test_rpc_client <server hostname> <server port> <message>\n";
	exit(0);
     }

   const std::string hostname = std::string(argv[1]);
   int port = atoi(argv[2]);
   std::string msg = std::string(argv[3]);
   
   rpc_client rpccl;
   
   NetAddress server_na(hostname,port);
   std::string resp_msg;
   rpccl.do_rpc_call(server_na,msg,false,resp_msg);
}
