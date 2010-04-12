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
  
#include "SuccList.h"
#include "DHTVirtualNode.h"
#include "DHTNode.h"
#include "Location.h"
#include "errlog.h"

#include <iostream>

using sp::errlog;

namespace dht
{
   SuccList::SuccList(DHTVirtualNode *vnode)
     : Stabilizable(), _vnode(vnode)
       {
       }
   
   SuccList::~SuccList()
     {
     }

   void SuccList::clear()
     {
	// TODO: beware, should remove keys from LocationTable as well.
	_succs.clear();
     }
      
   dht_err SuccList::update_successors()
     {
	//debug
	std::cerr << "[Debug]:SuccList::update_successors()\n";
	//debug
	
	/**
	 * RPC call to get successor list.
	 */
	int status = 0;
	DHTKey *dk_succ = _vnode->getSuccessor();
	if (!dk_succ)
	  {
	     // TODO: errlog.
	     std::cerr << "[Error]:SuccList::update_successors: this virtual node has no successor:"
	       << *dk_succ << ".Exiting\n";
	     exit(-1);
	  }
	
	Location *loc_succ = _vnode->findLocation(*dk_succ);
	
	slist<DHTKey> dkres_list;
	slist<NetAddress> na_list;
	dht_err loc_err = _vnode->getPNode()->getSuccList_cb(*dk_succ,dkres_list,na_list,status);
	if (loc_err == DHT_ERR_UNKNOWN_PEER)
	  _vnode->getPNode()->_l1_client->RPC_getSuccList(*dk_succ, loc_succ->getNetAddress(),
							  _vnode->getIdKey(), _vnode->getNetAddress(),
					      dkres_list, na_list, status);
	
	// TODO: handle failure, retry, and move to next successor in the list.
	
	if ((dht_err)status != DHT_ERR_OK)
	  {
	     //debug
	     std::cerr << "getSuccList failed\n";
	     //debug
	     
	     errlog::log_error(LOG_LEVEL_DHT, "getSuccList failed\n");
	     return (dht_err)status;
	  }
		
	// TODO: merge succlist.
     	merge_succ_list(dkres_list,na_list);
     }
   
   void SuccList::merge_succ_list(const slist<DHTKey> &dkres_list, const slist<NetAddress> &na_list)
     {
	// TODO.
	
     }
   
   
} /* end of namespace. */
