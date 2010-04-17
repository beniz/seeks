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
  
#ifndef SUCCLIST_H
#define SUCCLIST_H

#include "DHTKey.h"
#include "NetAddress.h"
#include "Stabilizer.h"
#include "dht_err.h"

#include <list>

namespace dht
{
   class DHTVirtualNode;
   
   class SuccList : public Stabilizable
     {
      public:
	SuccList(DHTVirtualNode *vnode);
	
	~SuccList();
     
	void clear();
	
	std::list<const DHTKey*>::const_iterator next() const { return _succs.begin(); };
	
	std::list<const DHTKey*>::const_iterator end() const { return _succs.end(); };
	
	void erase_front();
	
	void set_direct_successor(const DHTKey *succ_key);
	
	bool empty() const { return _succs.empty(); };
	
	size_t size() const { return _succs.size(); };
	
	dht_err update_successors();
	
	void merge_succ_list(std::list<DHTKey> &dkres_list, std::list<NetAddress> &na_list);
	
	bool has_key(const DHTKey &key) const;
	
	void removeKey(const DHTKey &key);
	
	dht_err findClosestPredecessor(const DHTKey &nodeKey,
				       DHTKey &dkres, NetAddress &na,
				       DHTKey &dkres_succ, NetAddress &dkres_succ_na,
				       int &status);
	
	/**
	 * virtual functions, from Stabilizable.
	 */
        virtual void stabilize_fast() {};
	
        virtual void stabilize_slow() { update_successors(); };
	
	// TODO.
	virtual bool isStable() { return false; } ;
	
      public:
	std::list<const DHTKey*> _succs;
	
	DHTVirtualNode *_vnode;
     
	static short _max_list_size;
     };
      
} /* end of namespace. */

#endif
