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

#ifndef L1_PROTOB_RPC_SERVER_H
#define L1_PROTOB_RPC_SERVER_H

#include "l1_rpc_server.h"

namespace dht
{
   class l1_protob_rpc_server : public l1_rpc_server
     {
      public:
	l1_protob_rpc_server(const std::string &hostname, const short &port,
			     DHTNode *pnode);
	
	~l1_protob_rpc_server();
	
	virtual dht_err serve_response(const std::string &msg,
				       std::string &resp_msg);
	
	dht_err execute_callback(const uint32_t &fct_id,
				 const DHTKey &recipient_key,
				 const NetAddress &recipient_na,
				 const DHTKey &sender_key,
				 const NetAddress &sender_na,
				 const DHTKey &node_key,
				 int &status,
				 std::string &resp_msg);
     
	/*- l1 interface. -*/
	virtual dht_err RPC_getSuccessor_cb(const DHTKey& recipientKey,
					    const NetAddress &recipient,
					    DHTKey& dkres, NetAddress& na,
					    int& status);
	
	virtual dht_err RPC_getPredecessor_cb(const DHTKey& recipientKey,
					      const NetAddress &recipient,
					      DHTKey& dkres, NetAddress& na,
					      int& status);
	
	virtual dht_err RPC_notify_cb(const DHTKey& recipientKey,
				      const NetAddress &recipient,
				      const DHTKey& senderKey,
				      const NetAddress& senderAddress,
				      int& status);
	
	virtual dht_err RPC_getSuccList_cb(const DHTKey& recipientKey,
					   const NetAddress &recipient,
					   std::list<DHTKey> &dkres_list, 
					   std::list<NetAddress> &na_list,
					   int& status);
	
	virtual dht_err RPC_findClosestPredecessor_cb(const DHTKey& recipientKey,
						      const NetAddress &recipient,
						      const DHTKey& nodeKey,
						      DHTKey& dkres, NetAddress& na,
						      DHTKey& dkres_succ, NetAddress &dkres_succ_na,
						      int& status);
	
	virtual dht_err RPC_joinGetSucc_cb(const DHTKey& recipientKey,
					   const NetAddress& recipient,
					   const DHTKey &senderKey,
					   DHTKey& dkres, NetAddress& na,
					   int& status);
     
	virtual dht_err RPC_ping_cb(const DHTKey& recipientKey,
				    const NetAddress &recipient,
				    int& status);
     };
   
} /* end of namespace. */

#endif
