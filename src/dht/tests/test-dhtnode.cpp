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

#include <iostream>
#include <stdlib.h>

using namespace dht;

int main(int argc, char **argv)
{
   if (argc < 3)
     {
	std::cout << "Usage test_dhtnode <ip addr> <port>\n";
	exit(0);
     }
   
   const char *net_addr = argv[1];
   const int net_port = atoi(argv[2]);
   DHTNode node(net_addr,net_port);
}

  
