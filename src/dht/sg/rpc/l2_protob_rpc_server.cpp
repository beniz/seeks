/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "l2_protob_rpc_server.h"
#include "l1_protob_wrapper.h"
#include "l2_protob_wrapper.h"

namespace dht
{
   l2_protob_rpc_server::l2_protob_rpc_server(const std::string &hostname, const short &port,
					      SGNode *pnode)
     :l1_protob_rpc_server(hostname,port,pnode),l2_rpc_server_interface()
       {
       }
   
   l2_protob_rpc_server::~l2_protob_rpc_server()
     {
     }
   
   void l2_protob_rpc_server::lx_response(const uint32_t &fct_id,
					  const DHTKey &recipient_key,
					  const NetAddress &recipient_na,
					  const DHTKey &sender_key,
					  const NetAddress &sender_na,
					  const DHTKey &node_key,
					  int &status,
					  std::string &resp_msg)
     {
        execute_callback(fct_id,recipient_key,recipient_na,
			 sender_key,sender_na,node_key,status,resp_msg);
     }
   
   dht_err l2_protob_rpc_server::execute_callback(const uint32_t &fct_id,
						  const DHTKey &recipient_key,
						  const NetAddress &recipient_na,
						  const DHTKey &sender_key,
						  const NetAddress &sender_na,
						  const DHTKey &node_key,
						  int& status,
						  std::string &resp_msg)
     {
	//debug
	std::cout << "[Debug]:l2 execute_callback: ";
	//debug
     
	if (fct_id == hash_subscribe)
	  {
	     //debug
	     std::cerr << "subscribe\n";
	     //debug
	     	     
	     // callback.
	     std::vector<Subscriber*> peers;
	     RPC_subscribe_cb(recipient_key,recipient_na,
			      sender_key,sender_na,
			      node_key,peers,status);
	     
	     // create response.	     
	     if (status == DHT_ERR_OK)
	       {
		  l2::l2_subscribe_response *l2r 
		    = l2_protob_wrapper::create_l2_subscribe_response(status,peers);
		  
		  // serialize the response.
		  l2_protob_wrapper::serialize_to_string(l2r,resp_msg);
		  delete l2r;
	       }
	     else
	       {
		  l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(status);
	       
		  // serialize the response.
		  l1_protob_wrapper::serialize_to_string(l1r,resp_msg);
		  delete l1r;
	       }
	  }
	//TODO: other callback come here.
     
	return DHT_ERR_OK;
     }
   
   dht_err l2_protob_rpc_server::RPC_subscribe_cb(const DHTKey &recipientKey,
						  const NetAddress &recipient,
						  const DHTKey &senderKey,
						  const NetAddress &sender,
						  const DHTKey &sgKey,
						  std::vector<Subscriber*> &peers,
						  int &status)
     {
	return static_cast<SGNode*>(_pnode)->RPC_subscribe_cb(recipientKey,recipient,
							      senderKey,sender,
							      sgKey,peers,status);
     }
        
} /* end of namespace. */
