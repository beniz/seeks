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
  
#include "l1_protob_rpc_server.h"
#include "l1_protob_wrapper.h"
#include "DHTNode.h"

namespace dht
{
   l1_protob_rpc_server::l1_protob_rpc_server(const std::string &hostname, const short &port,
					      DHTNode *pnode)
     :l1_rpc_server(hostname,port,pnode)
       {
       }
   
   l1_protob_rpc_server::~l1_protob_rpc_server()
     {
     }
   
   dht_err l1_protob_rpc_server::server_response(const std::string &msg,
						 std::string &resp_msg)
     {
	// deserialize query message.
	l1::l1_query l1q;
	
	try 
	  {
	     l1_protob_wrapper::deserialize(msg,&l1q);
	  }
	catch (l1_fail_deserialize_exception ex) 
	  {
	     throw rpc_server_wrong_layer_exception();
	  }
			
	// read query.
	uint32_t layer_id;
	uint32_t fct_id;
	DHTKey recipient_key, sender_key, node_key;
	NetAddress recipient_na, sender_na;
	l1_protob_wrapper::read_l1_query(&l1q,layer_id,fct_id,recipient_key,
					 recipient_na,sender_key,sender_na,node_key);
	// check on the layer id.
	if (layer_id != 1)
	  {
	     // XXX: we could communicate something nice back the sender,
	     // but we won't.
	     throw rpc_server_wrong_layer_exception();
	  }
		
	// decides which response to give.
	int status = DHT_ERR_OK;
	dht_err err = execute_callback(fct_id,recipient_key,recipient_na,
				       sender_key,sender_na,node_key,status,resp_msg);
	
	return err;
     }
   
   dht_err l1_protob_rpc_server::execute_callback(const uint32_t &fct_id,
						  const DHTKey &recipient_key,
						  const NetAddress &recipient_na,
						  const DHTKey &sender_key,
						  const NetAddress &sender_na,
						  const DHTKey &node_key,
						  int& status,
						  std::string &resp_msg)
     {
	l1::l1_response *l1r = NULL;
	
	if (fct_id == hash_get_successor)
	  {
	     DHTKey dkres;
	     NetAddress dkres_na;
	     RPC_getSuccessor_cb(recipient_key,recipient_na,
				 sender_key,sender_na,
				 dkres,dkres_na,status);
	     
	     // create a response.
	     l1r = l1_protob_wrapper::create_l1_response(status,dkres,dkres_na);
	  }
	else if (fct_id == hash_get_predecessor)
	  {
	     DHTKey dkres;
	     NetAddress dkres_na;
	     RPC_getPredecessor_cb(recipient_key,recipient_na,
				   sender_key,sender_na,
				   dkres,dkres_na,status);
	     
	     // create a response.
	     l1r = l1_protob_wrapper::create_l1_response(status,dkres,dkres_na);
	  }
	else if (fct_id == hash_notify)
	  {
	     RPC_notify_cb(recipient_key,recipient_na,
			   sender_key,sender_na,
			   status);
	     
	     // create a response.
	     l1r = l1_protob_wrapper::create_l1_response(status);
	  }
	else if (fct_id == hash_find_closest_predecessor)
	  {
	     DHTKey dkres, dkres_succ;
	     NetAddress dkres_na, dkres_succ_na;
	     RPC_findClosestPredecessor_cb(recipient_key,recipient_na,
					   sender_key,sender_na,
					   node_key,
					   dkres,dkres_na,
					   dkres_succ,dkres_succ_na,
					   status);
	     
	     // create a response.
	     l1r = l1_protob_wrapper::create_l1_response(status,dkres,dkres_na,
							 dkres_succ,dkres_succ_na);
	  }
	else 
	  {
	     // TODO: unknown cb.
	  }
	
	// serialize the response.
	l1_protob_wrapper::serialize_to_string(l1r,resp_msg);
	delete l1r;
	
	return DHT_ERR_OK;	
     }

   /*- l1 interface. -*/
   dht_err l1_protob_rpc_server::RPC_getSuccessor_cb(const DHTKey& recipientKey,
						     const NetAddress &recipient,
						     const DHTKey& senderKey,
						     const NetAddress& senderAddress,
						     DHTKey& dkres, NetAddress& na,
						     int& status)
     {
	return _pnode->getSuccessor_cb(recipientKey,dkres,na,status);
     }
      
   dht_err l1_protob_rpc_server::RPC_getPredecessor_cb(const DHTKey& recipientKey,
						       const NetAddress &recipient,
						       const DHTKey& senderKey,
						       const NetAddress& senderAddress,
						       DHTKey& dkres, NetAddress& na,
						       int& status)
     {
	return _pnode->getPredecessor_cb(recipientKey,dkres,na,status); 
     }
      
   dht_err l1_protob_rpc_server::RPC_notify_cb(const DHTKey& recipientKey,
					       const NetAddress &recipient,
					       const DHTKey& senderKey,
					       const NetAddress& senderAddress,
					       int& status)
     {
	return _pnode->notify_cb(recipientKey,senderKey,senderAddress,status);
     }
      
   dht_err l1_protob_rpc_server::RPC_findClosestPredecessor_cb(const DHTKey& recipientKey,
							       const NetAddress &recipient,
							       const DHTKey& senderKey,
							       const NetAddress& senderAddress,
							       const DHTKey& nodeKey,
							       DHTKey& dkres, NetAddress& na,
							       DHTKey& dkres_succ, NetAddress &dkres_succ_na,
							       int& status)
     {
	return _pnode->findClosestPredecessor_cb(recipientKey,nodeKey,dkres,na,
						 dkres_succ,dkres_succ_na,status);	
     }
   
} /* end of namespace. */
