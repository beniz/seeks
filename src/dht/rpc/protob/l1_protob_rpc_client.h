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

#ifndef L1_PROTOB_RPC_CLIENT_H
#define L1_PROTOB_RPC_CLIENT_H

#include "l1_rpc_client.h"
#include "l1_protob_wrapper.h"

namespace dht
{
   class l1_protob_rpc_client : public l1_rpc_client
     {
      public:
	l1_protob_rpc_client();
	
	~l1_protob_rpc_client();
	
	// call and response.
	dht_err RPC_call(const uint32_t &fct_id,
			 const DHTKey &recipientKey,
			 const NetAddress& recipient,
			 const DHTKey &senderKey,
			 const NetAddress& senderAddress,
			 l1::l1_response *l1r);
	
	/*- l1 interface. -*/
	virtual dht_err RPC_getSuccessor(const DHTKey& recipientKey,
					 const NetAddress& recipient,
					 const DHTKey &senderKey,
					 const NetAddress& senderAddress,
					 DHTKey& dkres, NetAddress& na,
					 int& status);
	
	virtual dht_err RPC_getPredecessor(const DHTKey& recipientKey,
					   const NetAddress& recipient,
					   const DHTKey& senderKey,
					   const NetAddress& senderAddress,
					   DHTKey& dkres, NetAddress& na,
					   int& status);
	
	virtual dht_err RPC_notify(const DHTKey& recipientKey,
				   const NetAddress& recipient,
				   const DHTKey& senderKey,
				   const NetAddress& senderAddress,
				   int& status);
	
	virtual dht_err RPC_getSuccList(const DHTKey &recipientKey,
					const NetAddress &recipient,
					const DHTKey &senderKey,
					const NetAddress &senderAddress,
					std::list<DHTKey> &dkres_list,
					std::list<NetAddress> &na_list,
					int &status);
	
	virtual dht_err RPC_findClosestPredecessor(const DHTKey& recipientKey,
						   const NetAddress& recipient,
						   const DHTKey& senderKey,
						   const NetAddress& senderAddress,
						   const DHTKey& nodeKey,
						   DHTKey& dkres, NetAddress& na,
						   DHTKey& dkres_succ,
						   NetAddress &dkres_succ_na,
						   int& status);
	
	virtual dht_err RPC_joinGetSucc(const DHTKey& recipientKey,
					const NetAddress& recipient,
					const DHTKey &senderKey,
					const NetAddress& senderAddress,
					DHTKey& dkres, NetAddress& na,
					int& status);
     
	virtual dht_err RPC_ping(const DHTKey& recipientKey,
				 const NetAddress& recipient,
				 const DHTKey& senderKey,
				 const NetAddress& senderAddress,
				 int& status);
     };
   
} /* end of namespace. */

#endif
