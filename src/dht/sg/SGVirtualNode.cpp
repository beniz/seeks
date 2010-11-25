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
 
#include "SGVirtualNode.h"
#include "SGNode.h"
#include "l2_protob_rpc_client.h"

namespace dht
{
   SGVirtualNode::SGVirtualNode(SGNode *pnode)
     :DHTVirtualNode(pnode),_sgm(&pnode->_sgmanager)
       {
	  std::cerr << "creating SGVirtualNode\n";
       }
   
   SGVirtualNode::SGVirtualNode(SGNode *pnode, const DHTKey &idkey, LocationTable *lt)
     :DHTVirtualNode(pnode,idkey,lt),_sgm(&pnode->_sgmanager)
       {
       }
   
   SGVirtualNode::~SGVirtualNode()
     {
     }
   
   dht_err SGVirtualNode::replication_host_keys(const DHTKey &start_key)
     {
	// decrement replication radius of all other replicated search groups.
	// when replication_radius is 0, the search group is hosted by this virtual node.
	if (!_sgm->replication_decrement_all_sgs(_idkey))
	  return DHT_ERR_REPLICATION;
	return DHT_ERR_OK;
     }
   
   void SGVirtualNode::replication_move_keys_backward(const DHTKey &start_key,
							 const DHTKey &end_key,
							 const NetAddress &senderAddress)
     {
	// select keys and searchgroups...
	// XXX: we could use a single representation here, by using a hash_map
	// in the call back.
	hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey> h_sgs;
	_sgm->find_sg_range(start_key,end_key,h_sgs);
	std::vector<Searchgroup*> v_sgs;
	hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit
	  = h_sgs.begin();
	while(hit!=h_sgs.end())
	  {
	     v_sgs.push_back((*hit).second);
	     ++hit;
	  }
	
	// RPC call to predecessor to pass the keys.
	// TODO: local call if the virtual node belongs to this DHT node.
	int status = DHT_ERR_OK;
	DHTKey ownerKey; //TODO: unused.
	l2_protob_rpc_client *l2_client = static_cast<l2_protob_rpc_client*>(_pnode->_l1_client);
	l2_client->RPC_replicate(end_key,senderAddress,
					       _idkey,_pnode->_l1_na,
					       ownerKey,v_sgs,
					       false,status);
	//TODO: catch errors.
	
	
	//TODO: update local replication level of nodes, 
	// and remove those with level > k.
	// Searchgroup objects are destroyed.
	_sgm->increment_replicated_sgs(_idkey,h_sgs);
	h_sgs.clear();
     }
   
   void SGVirtualNode::replication_move_keys_forward(const DHTKey &end_key)
     {
	
     }
   
   void SGVirtualNode::replication_trickle_forward(const DHTKey &start_key,
						      const short &start_replication_radius)
     {
	
     }
      
} /* end of namespace. */
