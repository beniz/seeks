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

#include "SGNode.h"
#include "l2_protob_rpc_server.h"
#include "l2_protob_rpc_client.h"

namespace dht
{
   SGNode::SGNode(const char *net_addr, const short &net_port,
		  const bool &generate_vnode)
     :DHTNode(net_addr,net_port,generate_vnode)
     {
     }
   
   SGNode::~SGNode()
     {
     }
   
   void SGNode::init_server()
     {
	_l1_server = new l2_protob_rpc_server(_l1_na.getNetAddress(),_l1_na.getPort(),this);
	_l1_client = new l2_protob_rpc_client(); 
     }
   
   dht_err SGNode::RPC_subscribe_cb(const DHTKey &recipientKey,
				    const NetAddress &recipient,
				    const DHTKey &senderKey,
				    const NetAddress &sender,
				    const DHTKey &sgKey,
				    std::vector<Subscriber*> &peers,
				    int &status)
     {
	// fill up the peers list.
	// subscribe or not.
	// trigger a sweep (condition to alleviate the load ?)
     	
	/* check on parameters. */
	if (sgKey.count() == 0)
	  {
	     status = DHT_ERR_UNSPECIFIED_SEARCHGROUP;
	     return status;
	  }
		
	/* check on subscription, i.e. if a sender address is specified. */
	bool subscribe = false;
	if (!sender.empty())
	  subscribe = true;
	
	/* find / create searchgroup. */
	Searchgroup *sg = _sgmanager.find_load_or_create_sg(&sgKey);
	if (!sg)
	  {
	     status = DHT_ERR_UNKNOWN_PEER;
	     return status;
	  }
		
	/* select peers. */
	if ((int)sg->_vec_subscribers.size() > sg_configuration::_sg_config->_max_returned_peers)
	  sg->random_peer_selection(sg_configuration::_sg_config->_max_returned_peers,peers);
	else peers = sg->_vec_subscribers;
	
	/* subscription. */
	if (subscribe)
	  {
	     Subscriber *nsub = new Subscriber(senderKey,
					       sender.getNetAddress(),sender.getPort());
	     if (!sg->add_subscriber(nsub))
	       delete nsub;
	  }
	
	/* trigger a call to sweep (from sg_manager). */
	_sgmanager._sgsw.sweep();
	
	return DHT_ERR_OK;
     }
        
} /* end of namespace. */
