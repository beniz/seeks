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
#include "SGVirtualNode.h"
#include "l2_protob_rpc_server.h"
#include "l2_protob_rpc_client.h"
#include "errlog.h"

using sp::errlog;

namespace dht
{
   std::string SGNode::_sg_config_filename = "";
   
   SGNode::SGNode(const char *net_addr, const short &net_port)
     :DHTNode(net_addr,net_port,false)
     {
	if (SGNode::_sg_config_filename.empty())
	  {
	     SGNode::_sg_config_filename = DHTNode::_dht_config_filename;
	  }
	
	if (!sg_configuration::_sg_config)
	  sg_configuration::_sg_config = new sg_configuration(SGNode::_sg_config_filename);
	
	/* check whether our search groups are in sync with our virtual nodes. */
	if (!_has_persistent_data) // set in DHTNode constructor.
	  reset_vnodes_dependent(); // resets data that is dependent on virtual nodes.
	
	/* init server. */
	start_node();
	//init_server();
	//_l1_server->run_thread();
	
	/* init sweeper. */
	sg_sweeper::init(&_sgmanager);
     }
   
   SGNode::~SGNode()
     {
     }
   
   DHTVirtualNode* SGNode::create_vnode()
     {
	return new SGVirtualNode(this);
     }
      
   DHTVirtualNode* SGNode::create_vnode(const DHTKey &idkey,
					LocationTable *lt)
     {
	return new SGVirtualNode(this,idkey,lt);
     }
      
   void SGNode::init_server()
     {
	_l1_server = new l2_protob_rpc_server(_l1_na.getNetAddress(),_l1_na.getPort(),this);
	_l1_client = new l2_protob_rpc_client(); 
     }
   
   void SGNode::reset_vnodes_dependent()
     {
	// resets searchgroup data, as it is dependent on the virtual nodes.
	_sgmanager.clear_sg_db();
     }
      
   void SGNode::RPC_subscribe_cb(const DHTKey &recipientKey,
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
	     return;
	  }
		
	/* check on subscription, i.e. if a sender address is specified. */
	bool subscribe = false;
	if (!sender.empty())
	  subscribe = true;
	
	/* find / create searchgroup. */
	Searchgroup *sg = _sgmanager.find_load_or_create_sg(&sgKey);
		
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
	
	/* update usage. */
	sg->set_last_time_of_use();
	
	/* trigger a call to sweep (from sg_manager). */
	_sgmanager._sgsw.sweep();
     }

   void SGNode::RPC_replicate_cb(const DHTKey &recipientKey,
				    const NetAddress &recipient,
				    const DHTKey &senderKey,
				    const NetAddress &sender,
				    const DHTKey &ownerKey,
				    const std::vector<Searchgroup*> &sgs,
				    const bool &sdiff,
				    int &status)
     {
	// verify that sender address is non empty (empty means either not divulged, 
	// either fake, eliminated by server).
	if (sender.empty())
	  {
	     errlog::log_error(LOG_LEVEL_DHT,"rejected replication call with empty sender address");
	     status = DHT_ERR_SENDER_ADDR;
             return;
	  }
		 
	// if no peer in searchgroup, simply update the replication radius.
	int sgs_size = sgs.size();
	for (int i=0;i<sgs_size;i++)
	  {
	     Searchgroup *sg = sgs.at(i);
	     if (sg->_hash_subscribers.empty())
	       {
		  //TODO: locate search group.
		  Searchgroup *lsg = _sgmanager.find_sg(&sg->_idkey);
		  if (!lsg)
		    {
		       
		    }
		  		  
		  //TODO: update replication status.
		  //lsg->_replication_level = ;
	       }
	     else
	       {
		  //TODO: add sg to local db if replication level is 0.
		  if (sg->_replication_level == 0)
		    _sgmanager.add_sg_db(sg);
		  else
		    {
		       //TODO: add to replicated db.
		    }
	       }
	  }
     }
   
} /* end of namespace. */
