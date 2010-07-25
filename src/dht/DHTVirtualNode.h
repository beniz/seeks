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

#ifndef DHTVIRTUALNODE_H
#define DHTVIRTUALNODE_H

#include "dht_err.h"
#include "DHTKey.h"
#include "NetAddress.h"
#include "LocationTable.h"
#include "SuccList.h"
#include "mutexes.h"

#if (__GNUC__ >= 3)
#include <ext/slist>
#else
#include <slist>
#endif

#if (__GNUC__ >= 3)
using __gnu_cxx::slist;
#else
using std::slist;
#endif

namespace dht
{
   
   class Location;
   class Stabilizer;
   class FingerTable;
   class DHTNode;
   
   /**
    * \brief Virtual node of the DHT.
    * \class DHTVirtualNode
    */
   class DHTVirtualNode
     {
      public:
	/**
	 * \brief create a new 'virtual' node on the local host.
	 * - The node's id is a randomly generated key.
	 * - successor and predecessor are NULL.
	 */
	DHTVirtualNode(DHTNode *pnode);
	
	/**
	 * \brief constructor based on persistent data, loaded at startup
	 *        in DHTNode.
	 */
	DHTVirtualNode(DHTNode *pnode, const DHTKey &idkey, LocationTable *lt);
	
	/**
	 * \brief destructor.
	 */
	~DHTVirtualNode();
	
	/**
	 * Initializes structures and mutexes.
	 */
	void init_vnode();
	
	/**-- functions used in RPC callbacks. --**/
	/**
	 * \brief notifies this virtual node that the argument key/address peer
	 *        thinks it is its predecessor.
	 */
	dht_err notify(const DHTKey& senderKey, const NetAddress& senderAddress);
	
	/**
	 * \brief find the closest predecessor of nodeKey in the finger table.
	 * @param nodeKey identification key which closest predecessor is asked for.
	 * @param dkres closest predecessor result variable,
	 * @param na closest predecessor net address variable,
	 * @param dkres_succ successor to dkres.
	 * @param status result status.
	 */
	dht_err findClosestPredecessor(const DHTKey& nodeKey,
				       DHTKey& dkres, NetAddress& na,
				       DHTKey& dkres_succ, NetAddress &dkres_succ_na,
				       int& status);
	/**
	 * \brief this virtual node is being pinged from the outside world.
	 */
	dht_err ping();
	
	/**---------------------------------------**/
	
	/**-- functions using RPCs. --**/
	dht_err join(const DHTKey& dk_bootstrap,
		     const NetAddress &dk_bootstrap_na,
		     const DHTKey& senderKey,
		     int& status);
			       
	dht_err find_successor(const DHTKey& nodeKey,
			       DHTKey& dkres, NetAddress& na);
	
	dht_err find_predecessor(const DHTKey& nodeKey,
				 DHTKey& dkres, NetAddress& na);
	
	bool is_dead(const DHTKey &recipientKey, const NetAddress &na,
		     int &status);
	
	/**---------------------------**/
	
	/**
	 * accessors.
	 */
	const DHTKey& getIdKey() const { return _idkey; }
	const DHTKey* getIdKeyPtr() const { return &_idkey; }
	DHTNode* getPNode() const { return _pnode; }
	DHTKey* getSuccessor() const { return _successor; }
	void setSuccessor(const DHTKey &dk);
	void setSuccessor(const DHTKey& dk, const NetAddress& na);
	DHTKey* getPredecessor() const { return _predecessor; }
	void setPredecessor(const DHTKey &dk);
	void setPredecessor(const DHTKey& dk, const NetAddress& na);
	void clearSuccsList() { _successors.clear(); };
	void clearPredsList() { _predecessors.clear(); };
	Location* getLocation() const { return _loc; }
	FingerTable* getFingerTable() { return _fgt; }
	std::list<const DHTKey*>::const_iterator getFirstSuccessor() const { return _successors.begin(); };
	std::list<const DHTKey*>::const_iterator endSuccessor() const { return _successors.end(); };
	
	/* location finding. */
	LocationTable* getLocationTable() const;
	Location* findLocation(const DHTKey& dk) const;
	void addToLocationTable(const DHTKey& dk, const NetAddress& na,
				Location *&loc) const;
	void removeLocation(Location *loc);
	NetAddress getNetAddress() const;
	Location* addOrFindToLocationTable(const DHTKey& key, const NetAddress& na);
	bool isPredecessorEqual(const DHTKey &key) const;
	bool not_used_location(Location *loc) const;
	
	/* update of successor list. */
	void update_successor_list_head();
		
	/**
	 * \brief estimates the number of nodes and virtual nodes on the circle,
	 *        from this virtual node point of view.
	 */
	void estimate_nodes();
	void estimate_nodes(unsigned long &nnodes, 
			    unsigned long &nnvnodes);
	
	/**
	 * replication of content.
	 * XXX: replication is not implemented at this level. Inherited classes
	 * from DHTVirtualNode can define their response to replication calls.
	 */
	virtual dht_err replication_host_keys(const DHTKey &start_key) { return DHT_ERR_OK; };
	
	virtual dht_err replication_move_keys_backward(const DHTKey &start_key) { return DHT_ERR_OK; };
	
	virtual dht_err replication_move_keys_forward(const DHTKey &end_key) { return DHT_ERR_OK; };
	
	virtual dht_err replication_trickle_forward(const DHTKey &start_key,
						    const short &start_replication_radius) { return DHT_ERR_OK; };
	
      public:
	/**
	 * location table.
	 */
	LocationTable *_lt;
	
      protected:
	/**
	 * virtual node's id key.
	 */
	DHTKey _idkey;

	/**
	 * parent node (local main DHT node).
	 */
	DHTNode* _pnode;
	
	/**
	 * closest known successor on the circle.
	 */
	DHTKey* _successor;
	
	/**
	 * closest known predecessor on the circle.
	 */
	DHTKey* _predecessor;
     
      public:
	/**
	 * Sorted list of successors.
	 */
	SuccList _successors;
	
	/**
	 * Sorted list of predecessors.
	 */
	slist<const DHTKey*> _predecessors;
	
      protected:
	/**
	 * finger table.
	 */
	FingerTable* _fgt;

	/**
	 * this virtual node local location.
	 */
	Location* _loc;
     
	/**
	 * predecessor and successor mutexes.
	 */
	sp_mutex_t _pred_mutex;
	sp_mutex_t _succ_mutex;
     
      public:
	/**
	 * estimates of the number of nodes and virtual nodes on the circle.
	 */
	unsigned long _nnodes;
	unsigned long _nnvnodes;
     
	/**
	 * whether this node is connected to the ring.
	 * By connected it is meant that the virtual node has at least a working
	 * successor.
	 */
	bool _connected;
     };  
   
} /* end of namespace. */

#endif
