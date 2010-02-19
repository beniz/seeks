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
 
#ifndef L1_RPC_CLIENT_H
#define L1_RPC_CLIENT_H

#include "rpc_client.h"
#include "l1_rpc_interface.h"

namespace dht
{
   class l1_rpc_client : public rpc_client, public l1_rpc_client_interface
     {
      public:
	l1_rpc_client()
	  :rpc_client(),l1_rpc_client_interface()
	    {};
	
	~l1_rpc_client() {};	
     };
      
} /* end of namespace. */

#endif
