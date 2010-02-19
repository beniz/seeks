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

#include "DHTKey.h"
#include "NetAddress.h"

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
   class LocationTable;
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
	DHTVirtualNode(DHTNode* pnode);
	
	~DHTVirtualNode();

	/**-- functions used in RPC callbacks. --**/
	/**
	 * \brief notifies this virtual node that the argument key/address peer
	 *        thinks it is its predecessor.
	 */
	void notify(const DHTKey& senderKey, const NetAddress& senderAddress);
	
	/**
	 * \brief find the closest predecessor of nodeKey in the finger table.
	 * @param nodeKey identification key which closest predecessor is asked for.
	 * @param dkres closest predecessor result variable,
	 * @param na closest predecessor net address variable,
	 * @param dkres_succ successor to dkres.
	 * @param status result status. TODO.
	 */
	void findClosestPredecessor(const DHTKey& nodeKey,
				    DHTKey& dkres, NetAddress& na,
				    DHTKey& dkres_succ, NetAddress &dkres_succ_na,
				    int& status);
	
	/**---------------------------------------**/
	
	/**-- functions using RPCs. --**/
	/* TODO. */
	int join(const DHTKey& dk_boostrap,
		 const DHTKey& senderKey,
		 int& status);
	
	int find_successor(const DHTKey& nodeKey,
			   DHTKey& dkres, NetAddress& na);
	
	int find_predecessor(const DHTKey& nodeKey,
			     DHTKey& dkres, NetAddress& na);
	
	int stabilize();
	
	/**---------------------------**/
	
	/**
	 * accessors.
	 */
	const DHTKey& getIdKey() const { return _idkey; }
	DHTNode* getPNode() const { return _pnode; }
	DHTKey* getSuccessor() const { return _successor; }
	void setSuccessor(const DHTKey &dk);
	void setSuccessor(const DHTKey& dk, const NetAddress& na);  //DEPRECATED ?
	void setSuccessor(Location* loc);
	DHTKey* getPredecessor() const { return _predecessor; }
	void setPredecessor(const DHTKey &dk);
	void setPredecessor(const DHTKey& dk, const NetAddress& na); //DEPRECATED ? nop...
	void setPredecessor(Location* loc);
	Location* getLocation() const { return _loc; }
	FingerTable* getFingerTable() { return _fgt; }
	
	/**
	 * function channeling to pnode (DHTNode) functions.
	 */
	LocationTable* getLocationTable() const;
	Location* findLocation(const DHTKey& dk) const;
	void addToLocationTable(const DHTKey& dk, const NetAddress& na,
				Location* loc) const;
	NetAddress getNetAddress() const;
	Location* addOrFindToLocationTable(const DHTKey& key, const NetAddress& na);
	
      private:
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
     
	/**
	 * Sorted list of successors.
	 */
	slist<DHTKey*> _successors;
	
	/**
	 * Sorted list of predecessors.
	 */
	slist<DHTKey*> _predecessors;

	/**
	 * Max size of the pred/succ lists.
	 */
	static size_t _maxSuccsListSize;
	
	/**
	 * TODO: finger table.
	 */
	FingerTable* _fgt;

	/**
	 * this virtual node local location.
	 */
	Location* _loc;
     };  
   
} /* end of namespace. */

#endif
