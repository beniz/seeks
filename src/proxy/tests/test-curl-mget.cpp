/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include "curl_mget.h"

#include <iostream>
#include <stdlib.h>

using namespace sp;

int main(int argc, char **argv)
{
   if (argc < 3)
     {
	std::cout << "Usage: <number of requests> <host1> ... <hostn>\n";
	exit(0);
     }
   
   int n_requests = atoi(argv[1]);
   std::vector<std::string> addr;
   for (int i=0;i<n_requests;i++)
     {
	addr.push_back(std::string(argv[i+2]));
	//std::cout << "addr #" << i << ": " << argv[i+2] << std::endl;
     }
   
   curl_mget cmg(n_requests,60,0,60,0); // 60 seconds connection & transfer timeout.
   std::string **outputs = cmg.www_mget(addr,n_requests,NULL,"",0); // don't use a proxy.

   //std::cout << "outputs:\n";
   for (int i=0;i<n_requests;i++)
     {
	//std::cout << "\n\n";
	std::cout << *outputs[i] << std::endl;
     }
}
