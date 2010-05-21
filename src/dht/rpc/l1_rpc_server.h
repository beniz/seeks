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

#ifndef L1_RPC_SERVER_H
#define L1_RPC_SERVER_H

#include "rpc_server.h"
#include "l1_rpc_interface.h"

namespace dht
{
   class l1_rpc_server : public rpc_server, public l1_rpc_server_interface
     {
      public:
	l1_rpc_server(const std::string &hostname, const short &port,
		      DHTNode *pnode)
	  :rpc_server(hostname,port),l1_rpc_server_interface(pnode)
	    {};
	
	~l1_rpc_server() {};
     
	// response to lx calls, if any allowed.
	virtual void lx_server_response(const uint32_t &fct_id,
					const DHTKey &recipient_key,
					const NetAddress &recipient_na,
					const DHTKey &sender_key,
					const NetAddress &sender_na,
					const DHTKey &node_key,
					int &status,
					std::string &resp_msg) {};
     };
      
} /* end of namespace. */

#endif
