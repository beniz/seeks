/*
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "dht_api.h"

namespace dht
{
  dht_err dht_api::findSuccessor(const DHTNode &dnode,
				  const DHTKey &nodekey,
				  DHTKey &dkres, NetAddress &na) throw (dht_exception)
     {
	// grab the closest virtual node to nodekey, to make the RPC call from it.
	DHTVirtualNode *vnode = dnode.find_closest_vnode(nodekey);
     
	// make the RPC call.
	return vnode->find_successor(nodekey,dkres,na);
     }

   dht_err dht_api::findPredecessor(const DHTNode &dnode,
				    const DHTKey &nodekey,
				    DHTKey &dkres, NetAddress &na) throw (dht_exception)
     {
	// grab the closest virtual node to nodekey, to make the RPC call from it.
	DHTVirtualNode *vnode = dnode.find_closest_vnode(nodekey);
     
	// make the RPC call.
	return vnode->find_predecessor(nodekey,dkres,na);
     }

   dht_err dht_api::ping(const DHTNode &dnode,
			 const DHTKey &nodekey,
			 const NetAddress &na,
			 bool &alive) throw (dht_exception)
     {
	// grab the closest virtual node to make the RPC call from it.
	DHTVirtualNode *vnode = dnode.find_closest_vnode(nodekey);
	
	// make the RPC call.
	dht_err status;
	alive = !vnode->is_dead(nodekey,na,status);
	return status;
     }
  
  dht_err dht_api::join_start(DHTNode &dnode,
			      const std::vector<NetAddress> &bootstrap_nodes,
			      const bool &reset)
  {
    return dnode.join_start(bootstrap_nodes,reset);
  }
  
  void dht_api::self_bootstrap(DHTNode &dnode)
  {
    dnode.self_bootstrap();
  }
      
} /* end of namespace. */
