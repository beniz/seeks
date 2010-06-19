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

#ifndef L2_RPC_INTERFACE_H
#define L2_RPC_INTERFACE_H

#include "dht_err.h"
#include "DHTKey.h"
#include "NetAddress.h"
#include "searchgroup.h"
#include "subscriber.h"

#include <vector>

namespace dht
{
   #define hash_subscribe                434989955ul   /* "subscribe" */
   #define hash_replicate                780233041ul   /* "replicate" */
   
   class l2_rpc_client_interface
     {
      public:
	l2_rpc_client_interface() {};

	virtual ~l2_rpc_client_interface() {};
     
	/**
	 * \brief subscribe to a search group.
	 * @param recipientKey identification key of the target node.
	 * @param recipient net address of the target node.
	 * @param senderKey identification key of the sender node (optional, required for susbcription).
	 * @param senderAddress address of the sender (for verification and server port, optional).
	 * @param sgKey search group key.
	 * @param peers list of peer subscribers to the search group.
	 * @param status RPC result status for handling erroneous results.
	 * @return status.
	 */
	virtual dht_err RPC_subscribe(const DHTKey &recipientKey,
				      const NetAddress &recipient,
				      const DHTKey &senderKey,
				      const NetAddress &senderAddress,
				      const DHTKey &sgKey,
				      std::vector<Subscriber*> &peers,
				      int &status) = 0;
	
	/**
	 * \brief replicate callback.
	 * @param recipientKey identification key of the target node.
	 * @param recipient net address of the target node.
	 * @param senderKey identification key of the sender node (optional, required for susbcription).
	 * @param senderAddress address of the sender (for verification and server port, optional).
	 * @param ownerKey original owner of the search groups to be replicated.
	 * @param sgs list of search groups to be replicated.
	 * @param status RPC result status for handling erroneous results.
	 * @return status.
	 */
	virtual dht_err RPC_replicate(const DHTKey &recipientKey,
				      const NetAddress &recipient,
				      const DHTKey &senderKey,
				      const NetAddress &senderAddress,
				      const DHTKey &ownerKey,
				      const std::vector<Searchgroup*> &sgs,
				      const bool &sdiff,
				      int &status) = 0;
     };
   
   class l2_rpc_server_interface
     {
      public:
	l2_rpc_server_interface() {};
	
	virtual ~l2_rpc_server_interface() {};

	/**
	 * \brief subscribe callback.
	 * @param recipientKey identification key of the target node.
	 * @param recipient net address of the target node.
	 * @param senderKey identification key of the sender node (optional, required for susbcription).
	 * @param senderAddress address of the sender (for verification and server port, optional).
	 * @param sgKey search group key.
	 * @param peers list of peer subscribers to the search group.
	 * @param status RPC result status for handling erroneous results.
	 * @return status.
	 */
	virtual dht_err RPC_subscribe_cb(const DHTKey &recipientKey,
					 const NetAddress &recipient,
					 const DHTKey &senderKey,
					 const NetAddress &sender,
					 const DHTKey &sgKey,
					 std::vector<Subscriber*> &peers,
					 int &status) = 0;
     
	/**
	 * \brief replicate callback.
	 * @param recipientKey identification key of the target node.
	 * @param recipient net address of the target node.
	 * @param senderKey identification key of the sender node (optional, required for susbcription).
	 * @param senderAddress address of the sender (for verification and server port, optional).
	 * @param ownerKey original owner of the search groups to be replicated.
	 * @param sgs list of search groups to be replicated.
	 * @param status RPC result status for handling erroneous results.
	 * @return status.
	 */
	virtual dht_err RPC_replicate_cb(const DHTKey &recipientKey,
					 const NetAddress &recipient,
					 const DHTKey &senderKey,
					 const NetAddress &sender,
					 const DHTKey &ownerKey,
					 const std::vector<Searchgroup*> &sgs,
					 const bool &sdiff,
					 int &status) = 0;
     };
      
} /* end of namespace. */

#endif
