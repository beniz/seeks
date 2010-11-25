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
 
#ifndef L1_RPC_INTERFACE_H
#define L1_RPC_INTERFACE_H

#include "dht_err.h"
#include "DHTKey.h"
#include "NetAddress.h"
#include <list>

namespace dht
{
   class DHTNode;
   
   /* callbacks hash ids. */
   #define hash_get_successor                 3682586751ul    /* "get-successor" */
   #define hash_get_predecessor               3440834331ul    /* "get-predecessor" */
   #define hash_notify                        4267298065ul    /* "notify" */
   #define hash_find_closest_predecessor      3893622114ul    /* "find-closest-predecessor" */
   #define hash_join_get_succ                 2753881080ul    /* "join-get-succ" */
   #define hash_ping                           491110822ul    /* "ping" */
   #define hash_get_succlist                  2523960002ul    /* "get-succlist" */
   
   class l1_rpc_client_interface
     {
      public:
	l1_rpc_client_interface() {};

	virtual ~l1_rpc_client_interface() {};
	
	/**
	 * \brief getSuccessor RPC.
	 * @param recipientKey identification key of the target node.
	 * @param recipient net address of the target node.
	 * @param dkres target node's successor's identification key.
	 * @param na target node's successor's net address.
	 * @param status RPC result status for handling erroneous results.
	 * @return status.
	 */
	virtual void RPC_getSuccessor(const DHTKey& recipientKey,
					 const NetAddress& recipient,
					 DHTKey& dkres, NetAddress& na,
					 int& status) = 0;
	
	/**
	 * \brief getPredecessor RPC.
	 * @param recipientKey identification key of the target node.
	 * @param recipient net address of the target node.
	 * @param dkres target node's predecessor's identification key.
	 * @param na target node's predecessor's net address.
	 * @param status RPC result status for handling erroneous results.
	 * @return error status.
	 */
	virtual void RPC_getPredecessor(const DHTKey& recipientKey,
					   const NetAddress& recipient,
					   DHTKey& dkres, NetAddress& na,
					   int& status) = 0;
	
	/**
	 * \brief Notify RPC: sender notifies recipient that it thinks it is it's successor.
	 * @param recipientKey identification key of the target node.
	 * @param recipient node on which this function will execute.
	 * @param senderKey Identification key of the sender node on the circle.
	 * @param senderAddress Net address of the sender node.
	 * @return error status.
	 */
	virtual void RPC_notify(const DHTKey& recipientKey,
				   const NetAddress& recipient,
				   const DHTKey& senderKey,
				   const NetAddress& senderAddress,
				   int& status) = 0;
	
	/**
	 * \brief getSuccList RPC: returns the recipient's successor list.
	 * @param recipientKey identification key of the target node
	 *                     (Note: this is required due to the presence of virtual nodes).
	 * @param recipient Net address of the target node.
	 * @param dkres target node's successor list.
	 * @param na target node's successor net address list.
	 * @param status result status for handling erroneous results.
	 * @return status.
	 */
	virtual void RPC_getSuccList(const DHTKey &recipientKey,
					const NetAddress &recipient,
					std::list<DHTKey> &dkres_list,
					std::list<NetAddress> &na_list,
					int &status) = 0;
	
	/**
	 * \brief findClosestPredecessor RPC: ask recipient to find
	 *        the closest predecessor to nodeKey.
	 * @param recipientKey identification key of the target node.
	 * @param recipient Net address of the target node.
	 * @param nodeKey identification key to which the closest predecessor is sought.
	 * @param dkres result identification key of the closest predecessor to nodeKey.
	 * @param na result net address of the closest predecessor to nodeKey.
	 * @param dkres_succ result identification key of the successor to dkres.
	 * @param dkres_succ_na net address of the successor.
	 * @param status result status.
	 * @return error status.
	 */
	virtual void RPC_findClosestPredecessor(const DHTKey& recipientKey,
						   const NetAddress& recipient,
						   const DHTKey& nodeKey,
						   DHTKey& dkres, NetAddress& na,
						   DHTKey& dkres_succ,
						   NetAddress &dkres_succ_na,
						   int& status) = 0;
	/**
	 * \brief joinGetSucc: find_successor remote call when bootstrapping.
	 * @param recipientKey identification key of the target node.
	 * @param recipient net address of the target node.
	 * @param senderKey Identification key of the sender node on the circle.
	 * @param dkres target node's predecessor's identification key.
	 * @param na target node's predecessor's net address.
	 * @param status RPC result status for handling erroneous results.
	 * @return error status.
	 */
	virtual void RPC_joinGetSucc(const DHTKey &recipientKey,
					const NetAddress &recipient,
					const DHTKey &senderKey,
					DHTKey &dkres, NetAddress &na,
					int &status) = 0;
     
	/**
	 * \brief ping: pings a remote node. 
	 * @param recipientKey identification key of the target node.
	 * @param recipient node on which this function will execute.
	 * @return error status.
	 */
	virtual void RPC_ping(const DHTKey &recipientKey,
				 const NetAddress &recipient,
				 int &status) = 0;
     };

   class l1_rpc_server_interface
     {
      public:
	l1_rpc_server_interface(DHTNode *pnode)
	  :_pnode(pnode)
	    {};
	
	virtual ~l1_rpc_server_interface() {};
	
	/**
	 * \brief getSuccessor RPC callback.
	 * @param recipientKey identification key of the target node
	 *                     (Note: this is required due to the presence of virtual nodes).
	 * @param recipient Net address of the target node.
	 * @param dkres target node's successor's identification key.
	 * @param na target node's successor's net address.
	 * @param status result status for handling erroneous results.
	 * @return status.
	 */
	virtual void RPC_getSuccessor_cb(const DHTKey& recipientKey,
					    const NetAddress &recipient,
					    DHTKey& dkres, NetAddress& na,
					    int& status) = 0;
	
	/**
	 * \brief getPredecessor RPC callback.
	 * @param recipientKey identification key of the target node
	 *                     (Note: this is required due to the presence of virtual nodes).
	 * @param recipient Net address of the target node.
	 * @param dkres target node's predecessor's identification key.
	 * @param na target node's predecessor's net address.
	 * @param status result status for handling erroneous results.
	 * @return error status.
	 */
	virtual void RPC_getPredecessor_cb(const DHTKey& recipientKey,
					      const NetAddress &recipient,
					      DHTKey& dkres, NetAddress& na,
					      int& status) = 0;
	
	/**
	 * \brief Notify RPC callback: updates recipient's predecessor according to sender's id key.
	 * @param recipientKey identification key of the target node.
	 * @param recipient Net address of the target node.
	 * @param senderKey Identification key of the sender node on the circle.
	 * @param senderAddress Net address of the sender node.
	 * @return status.
	 */
	virtual void RPC_notify_cb(const DHTKey& recipientKey,
				      const NetAddress &recipient,
				      const DHTKey& senderKey,
				      const NetAddress& senderAddress,
				      int& status) = 0;
	
	/**
	 * \brief getSuccList RPC callback: returns the recipient's successor list.
	 * @param recipientKey identification key of the target node
	 *                     (Note: this is required due to the presence of virtual nodes).
	 * @param recipient Net address of the target node.
	 * @param dkres target node's successor list.
	 * @param na target node's successor net address list.
	 * @param status result status for handling erroneous results.
	 * @return status.
	  */
	virtual void RPC_getSuccList_cb(const DHTKey &recipientKey,
					   const NetAddress &recipient,
					   std::list<DHTKey> &dkres_list,
					   std::list<NetAddress> &dkres_na_list,
					   int &status) = 0;
	/**
	 * \brief findClosestPredecessor RPC: ask recipient to find
	 *        the closest predecessor to nodeKey.
	 * @param recipientKey identification key of the target node.
	 * @param recipient Net address of the target node.
	 * @param nodeKey identification key to which the closest predecessor is sought.
	 * @param dkres result identification key of the closest predecessor to nodeKey.
	 * @param na result net address of the closest predecessor to nodeKey.
	 * @param dkres_succ result identification key of the successor to dkres.
	 * @param dkres_succ_na address of dkres's sucesssor.
	 * @param status result status.
	 * @return error status.
	 */
	 virtual void RPC_findClosestPredecessor_cb(const DHTKey& recipientKey,
						       const NetAddress &recipient,
						       const DHTKey& nodeKey,
						       DHTKey& dkres, NetAddress& na,
						       DHTKey& dkres_succ, NetAddress &dkres_succ_na,
						       int& status) = 0;	
	/**
	 * \brief joinGetSucc_cb: callback to joining node, at bootstrap.
	 * @param recipientKey identification key of the target node.
	 * @param recipient net address of the target node.
	 * @param senderKey Identification key of the sender node on the circle.
	 * @param dkres target node's predecessor's identification key.
	 * @param na target node's predecessor's net address.
	 * @param status RPC result status for handling erroneous results.
	 * @return error status.
	 */
	virtual void RPC_joinGetSucc_cb(const DHTKey &recipientKey,
					   const NetAddress &recipient,
					   const DHTKey &senderKey,
					   DHTKey &dkres, NetAddress &na,
					   int &status) = 0;
	
	/**
	 * \brief Ping RPC callback: returns status to caller node.
	 * @param recipientKey identification key of the target node.
	 * @param recipient Net address of the target node.
	 * @return status.
	 */
	virtual void RPC_ping_cb(const DHTKey& recipientKey,
				    const NetAddress &recipient,
				    int& status) = 0;
      
      public:
	DHTNode *_pnode;
     };
   
} /* end of namespace. */

#endif
