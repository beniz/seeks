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

#ifndef L2_PROTOB_RPC_CLIENT_H
#define L2_PROTOB_RPC_CLIENT_H

#include "l1_protob_rpc_client.h"
#include "l2_rpc_interface.h"
#include "l2_protob_wrapper.h"

namespace dht
{
   
   class l2_protob_rpc_client : public l1_protob_rpc_client, public l2_rpc_client_interface
     {
      public:
	l2_protob_rpc_client();
	
	~l2_protob_rpc_client();
	
	// call and response.
	dht_err RPC_call(const uint32_t &fct_id,
			 const DHTKey &recipientKey,
			 const NetAddress &recipient,
			 const DHTKey &sgKey,
			 l2::l2_subscribe_response *l2r);
	
	dht_err RPC_call(const uint32_t &fct_id,
			 const DHTKey &recipientKey,
			 const NetAddress &recipient,
			 const DHTKey &senderKey,
			 const NetAddress &sender,
			 const DHTKey &sgKey,
			 l2::l2_subscribe_response *l2r);
	
	dht_err RPC_call(const std::string &msg,
			 const NetAddress &recipient,
			 l2::l2_subscribe_response *l2r);
	
	/*- l2 interface. -*/
	virtual dht_err RPC_subscribe(const DHTKey &recipientKey,
				      const NetAddress &recipient,
				      const DHTKey &senderKey,
				      const NetAddress &senderAddress,
				      const DHTKey &sgKey,
				      std::vector<Subscriber*> &peers,
				      int &status);
     };
      
} /* end of namespace. */

#endif
