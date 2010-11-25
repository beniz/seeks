/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#ifndef L2_PROTOB_RPC_SERVER_H
#define L2_PROTOB_RPC_SERVER_H

#include "l1_protob_rpc_server.h"
#include "l2_rpc_interface.h"
#include "SGNode.h"

namespace dht
{
   
   class l2_protob_rpc_server : public l1_protob_rpc_server, public l2_rpc_server_interface
     {
      public:
	l2_protob_rpc_server(const std::string &hostname, const short &port,
			     SGNode *pnode);
	
	~l2_protob_rpc_server();
     
	virtual void lx_server_response(const uint32_t &fct_id,
					   const DHTKey &recipient_key,
					   const NetAddress &recipient_na,
					   const DHTKey &sender_key,
					   const NetAddress &sender_na,
					   const DHTKey &node_key,
					   int &status,
					   std::string &resp_msg,
					   const std::string &inc_msg);
     
	void execute_callback(const uint32_t &fct_id,
				 const DHTKey &recipient_key,
				 const NetAddress &recipient_na,
				 const DHTKey &sender_key,
				 const NetAddress &sender_na,
				 const DHTKey &node_key,
				 int &status,
				 std::string &resp_msg);
	
	/*- l2 interface. -*/
	virtual void RPC_subscribe_cb(const DHTKey &recipientKey,
					 const NetAddress &recipient,
					 const DHTKey &senderKey,
					 const NetAddress &sender,
					 const DHTKey &sgKey,
					 std::vector<Subscriber*> &peers,
					 int &status);
     
	virtual void RPC_replicate_cb(const DHTKey &recipientKey,
					 const NetAddress &recipient,
					 const DHTKey &senderKey,
					 const NetAddress &sender,
					 const DHTKey &ownerKey,
					 const std::vector<Searchgroup*> &sgs,
					 const bool &sdiff,
					 int &status);
     };
   
} /* end of namespace. */

#endif
  
