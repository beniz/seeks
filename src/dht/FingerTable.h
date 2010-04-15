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

#ifndef FINGERTABLE_H
#define FINGERTABLE_H

#include "Location.h"
#include "LocationTable.h"
#include "DHTVirtualNode.h"
#include "DHTKey.h"
#include "Stabilizer.h"

namespace dht
{
   class LocationTable;
   
   class FingerTable : public Stabilizable
     {
      public:
	FingerTable(DHTVirtualNode* vnode, LocationTable* lt);
	
	~FingerTable();
	
	/**
	 * accessors.
	 */
	Location* getFinger(const int& i) { return _locs[i]; } 
	void setFinger(const int& i, Location* loc);

	/**
	 * accessors to vnode stuff.
	 */
	const DHTKey& getVNodeIdKey() const { return _vnode->getIdKey(); }
	Location* getVNodeLocation() const { return _vnode->getLocation(); }
	NetAddress getVNodeNetAddress() const { return _vnode->getNetAddress(); }
	DHTKey* getVNodeSuccessor() const { return _vnode->getSuccessor(); }
	Location* findLocation(const DHTKey& dk) const { return _vnode->findLocation(dk); }  
	
	bool has_key(const int &index, Location *loc) const;
	
	/**
	 * \brief find closest predecessor to a given key.
	 * Successor information is unset by default, set only if the closest node 
	 * is this virtual node.
	 */
	dht_err findClosestPredecessor(const DHTKey& nodeKey,
				       DHTKey& dkres, NetAddress& na,
				       DHTKey& dkres_succ, NetAddress &dkres_succ_na,
				       int& status);

	/**
	 * \brief verifies our successor is the right one,
	 * and notify it about us. This function does RPC calls.
	 */
	int stabilize();
	
	/**
	 * \brief refresh a random table entry.
	 */
	int fix_finger();

	/**
	 * virtual functions, from Stabilizable.
	 */
	virtual void stabilize_fast() { stabilize(); };
	
	virtual void stabilize_slow() { fix_finger(); };
	
	/**
	 * TODO.
	 */
	virtual bool isStable() { return false; }
	
	void print(std::ostream &out) const;
	
      private:
	/**
	 * virtual node to which this table refers.
	 */
	DHTVirtualNode* _vnode;
	
	/**
	 * shared table of known peers on the network.
	 */
	LocationTable* _lt;
	
	/**
	 * \brief finger table starting point:
	 * i.e. [k] = (n+2^{k-1}) mod 2^m, 1<k<m, with m=KEYNBITS,
	 * and n is the virtual node's key.
	 */
	DHTKey _starts[KEYNBITS];

      public:
	/**
	 * \brief finger table: each location _locs[i] is the peer with key
	 * that is the closest known successor to _starts[i].
	 */
	Location* _locs[KEYNBITS];
     };
      
} /* end of namespace */

#endif
