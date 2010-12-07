/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
 * Copyright (C) 2010  Loic Dachary <loic@dachary.org>
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
#include "l2_data_protob_wrapper.h"
#include "errlog.h"

//#define DEBUG

using sp::errlog;

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
   
   void l2_protob_rpc_server::lx_server_response(const uint32_t &fct_id,
						    const DHTKey &recipient_key,
						    const NetAddress &recipient_na,
						    const DHTKey &sender_key,
						    const NetAddress &sender_na,
						    const DHTKey &node_key,
						    int &status,
						    std::string &resp_msg,
						 const std::string &inc_msg) throw (dht_exception)
     {
#ifdef DEBUG
	//debug
	std::cout << "[Debug]:l2 lx_server_response:\n";
	//debug
#endif
	
	if (fct_id == hash_subscribe)
	  {
#ifdef DEBUG
	     //debug
	     std::cerr << "subscribe\n";
	     //debug
#endif
	     
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
	else if (fct_id == hash_replicate)
	  {
	     //debug
	     std::cerr << "replicate\n";
	     //debug
	     
	     // need to re-deserialize the message to get the extra
	     // information it contains.
	     l1::l1_query *l1q = new l1::l1_query();
	     try
	       {
		  l2_data_protob_wrapper::deserialize_from_string(inc_msg,l1q);
	       }
	     catch (dht_exception &e)
	       {
		  delete l1q;
		  errlog::log_error(LOG_LEVEL_DHT,"l2_protob_rpc_server::lx_server_response exception %s",e.what().c_str());
		  throw e;
	       }
	     uint32_t fct_id2;
	     DHTKey recipient_key2;
	     NetAddress recipient_na2;
	     DHTKey sender_key2;
	     NetAddress sender_na2;
	     DHTKey owner_key;
	     std::vector<Searchgroup*> sgs;
	     bool sdiff = false;
	     l2_data_protob_wrapper::read_l2_replication_query(l1q,fct_id2,recipient_key2,recipient_na2,
							       sender_key2,sender_na2,owner_key,sgs,sdiff);
	     
	     // callback.
	     RPC_replicate_cb(recipient_key2,recipient_na2,sender_key2,sender_na2,
			      owner_key,sgs,sdiff,status);
	     
	     // create response.
	     l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(status);
	     
	     // serialize the response.
	     l1_protob_wrapper::serialize_to_string(l1r,resp_msg);
	     delete l1r;
	  }
	//TODO: other callback come here.
     }
   
   void l2_protob_rpc_server::RPC_subscribe_cb(const DHTKey &recipientKey,
						  const NetAddress &recipient,
						  const DHTKey &senderKey,
						  const NetAddress &sender,
						  const DHTKey &sgKey,
						  std::vector<Subscriber*> &peers,
						  int &status)
     {
	static_cast<SGNode*>(_pnode)->RPC_subscribe_cb(recipientKey,recipient,
							      senderKey,sender,
							      sgKey,peers,status);
     }

   void l2_protob_rpc_server::RPC_replicate_cb(const DHTKey &recipientKey,
						  const NetAddress &recipient,
						  const DHTKey &senderKey,
						  const NetAddress &sender,
						  const DHTKey &ownerKey,
						  const std::vector<Searchgroup*> &sgs,
						  const bool &sdiff,
						  int &status)
     {
	static_cast<SGNode*>(_pnode)->RPC_replicate_cb(recipientKey,recipient,
							      senderKey,sender,
							      ownerKey,sgs,sdiff,status);
     }
      
} /* end of namespace. */
