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
   class DHTVirtualNode;
   
   class SuccList : public Stabilizable
     {
      public:
	SuccList(DHTVirtualNode *vnode);
	
	~SuccList();
     
	void clear();
	
	dht_err update_successors();
	
	void merge_succ_list(const slist<DHTKey> &dkres_list, const slist<NetAddress> &na_list);
	
	/**
	 * virtual functions, from Stabilizable.
	 */
        virtual void stabilize_fast() {};
	
        virtual void stabilize_slow() { update_successors(); };
	
	// TODO.
	virtual bool isStable() { return false; } ;
	
	slist<const DHTKey*> _succs;
	
	DHTVirtualNode *_vnode;
     };
      
} /* end of namespace. */

#endif
