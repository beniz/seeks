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

#ifndef DHTNODE_H
#define DHTNODE_H

#if (__GNUC__ >= 3)
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include "stl_hash.h"
#include "dht_configuration.h"
#include "DHTVirtualNode.h"
//#include "LocationTable.h"
#include "Stabilizer.h"
#include "l1_rpc_server.h"
#include "l1_rpc_client.h"

namespace dht
{
   class DHTNode
     {
      public:
	/**
	 * \brief constructor.
	 */
	DHTNode(const char *net_addr, const short &net_port);

	~DHTNode();
	
	/**
	 * DHTNode key generation, one per virtual node.
	 */
	static DHTKey generate_uniform_key();
	
	/**
	 * join at startup.
	 * First tries with the LocationTable information if any, and if no reset specified.
	 * Otherwise go for the bootstrap nodelist.
	 */
	dht_err join_start(std::vector<NetAddress> &bootstrap_nodelist, 
			   const bool &reset);
	
	/**
	 * self-boostrap.
	 * Bootstraps itself by building a circle of its virtual nodes.
	 * Useful only for the first node of a circle.
	 */
	dht_err self_bootstrap();

      private:
	void rank_vnodes(std::vector<const DHTKey*> &vnode_keys_ord);
	
      public:
	/**
	 * accessors.
	 */
	DHTVirtualNode* findVNode(const DHTKey& dk) const;
	NetAddress getNetAddress() const { return _l1_na; }
	
	/**----------------------------**/
	/**
	 * \brief getSuccessor local callback.
	 * @param status returned result status:
	 *                     - 0 ok
	 *                     - 1 no successor found (node is too young on the circle, or bug).
	 *                     - 2 severe error: successor's location is unknown (bug).
	 *                     - 3 error: recipient (key) is unknown on this peer.
	 */
	dht_err getSuccessor_cb(const DHTKey& recipientKey,
				DHTKey& dkres, NetAddress& na,
				int& status);

	dht_err getPredecessor_cb(const DHTKey& recipientKey,
				  DHTKey& dkres, NetAddress& na,
				  int& status);
	
	/**
	 * \brief notify callback.
	 * @param status returned result status:
	 *                        - 0 ok
	 *                        - 3 error: recipient (key) is unknown on this peer.
	 */
	dht_err notify_cb(const DHTKey& recipientKey,
			  const DHTKey& senderKey,
			  const NetAddress& senderAddress,
			  int& status);
	
	/**
	 * \brief getSuccList callback.
	 */
	dht_err getSuccList_cb(const DHTKey &recipientKey,
			       std::list<DHTKey> &dkres_list,
			       std::list<NetAddress> &dkres_na,
			       int &status);

	/**
	 * \brief findClosestPredecessor callback.
	 */
	dht_err findClosestPredecessor_cb(const DHTKey& recipientKey,
					  const DHTKey& nodeKey,
					  DHTKey& dkres, NetAddress& na,
					  DHTKey& dkres_succ, NetAddress &dkres_succ_na,
					  int& status);
	
	/**
	 * \brief joinGetSucc callback.
	 */
	dht_err joinGetSucc_cb(const DHTKey &recipientKey,
			       const DHTKey &senderKey,
			       DHTKey& dkres, NetAddress& na,
			       int& status);
	
	/**
	 * \brief ping callback.
	 */
	dht_err ping_cb(const DHTKey& recipientKey,
			const DHTKey& senderKey,
			const NetAddress& senderAddress,
			int& status);
	
	/**----------------------------**/
	/**
	 * Main routines using RPCs.
	 */
	
	/**
	 * \brief joins the circle by asking dk for the successor to its own key. 
	 *        This is done for _all_ the virtual nodes of a peer.
	 * @param na_bootstrap network address of a bootstrap node (any known peer).
	 * @param dk_bootstrap DHT key of the bootstrap node.
	 * @return status.
	 */
	dht_err join(const NetAddress& dk_bootstrap_na,
		     const DHTKey &dk_bootstrap);
	
	/**
	 * \brief find nodeKey's successor.
	 * @param recipientKey identification key of the target node.
	 * @param nodekey identification key to which the successor must be found.
	 * @param dkres result identification key of nodeKey's successor.
	 * @param na result net address of nodeKey's successor.
	 * @return status
	 */
	dht_err find_successor(const DHTKey& recipientKey,
			       const DHTKey& nodeKey,
			       DHTKey& dkres, NetAddress& na);
	
	/**
	 * \brief find nodekey's predecessor.
	 * @param recipientKey identification key of the first target node.
	 * @param nodeKey identification key to which the predecessor must be found.
	 * @param dkres result identification key of nodeKey's predecessor.
	 * @param na result net address of nodeKey's predecessor.
	 * @return status.
	 */
	dht_err find_predecessor(const DHTKey& recipientKey,
				 const DHTKey& nodeKey,
				 DHTKey& dkres, NetAddress& na);
	
	/**
	 * \brief stabilize: verify a vnode's immediate successor,
	 *        and tell the successor about itself.
	 * @param recipientKey identification key of the target node.
	 * @return status.
	 */
	//dht_err stabilize(const DHTKey& recipientKey);
	
      public:
	/**
	 * configuration.
	 */
	static std::string _dht_config_filename;
	static dht_configuration *_dht_config;
	
	/**
	 * array of DHT node. TODO: hashtable.
	 */
	//TODO: should be made persistent.
	hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey> _vnodes;

	/**
	 * this peer net address.
	 */
	NetAddress _l1_na;
	
	/**
	 * TODO:
	 * estimate of the number of peers on the circle.
	 */
	int _n_estimate;
	
	/**
	 * rpc layer1 server.
	 */
	l1_rpc_server *_l1_server;
	
	/**
	 * rpc layer1 client.
	 */
	l1_rpc_client *_l1_client;
	
	/**
	 * node's stabilizer.
	 */
	Stabilizer* _stabilizer;
     };
      
} /* end of namespace. */

#endif
