/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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

#include "l2_protob_rpc_client.h"
#include "l2_data_protob_wrapper.h"
#include "errlog.h"
#include "miscutil.h"

using sp::errlog;

namespace dht
{
   l2_protob_rpc_client::l2_protob_rpc_client()
     :l1_protob_rpc_client()
       {
       }
   
   l2_protob_rpc_client::~l2_protob_rpc_client()
     {
     }
   
   void l2_protob_rpc_client::RPC_call(const uint32_t &fct_id,
					  const DHTKey &recipientKey,
					  const NetAddress &recipient,
					  const DHTKey &sgKey,
				       l2::l2_subscribe_response *l2r) throw (dht_exception)
     {
	// serialize.
	l1::l1_query *l2q = l1_protob_wrapper::create_l1_query(fct_id,recipientKey,
							       recipient,sgKey);
	l1::header *l2q_head = l2q->mutable_head();
	l2q_head->set_layer_id(2);
	
	std::string msg;
	try
	  {	
	     l1_protob_wrapper::serialize_to_string(l2q,msg);
	  }
	catch(dht_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"rpc %u l2 error: %s",fct_id,e.what().c_str());
	     throw dht_exception(DHT_ERR_CALL, "rpc " + sp::miscutil::to_string(fct_id) + " l2 error" + e.what());
	  }
		     
	RPC_call(msg,recipient,l2r);
     }

   void l2_protob_rpc_client::RPC_call(const uint32_t &fct_id,
					  const DHTKey &recipientKey,
					  const NetAddress &recipient,
					  const DHTKey &senderKey,
					  const NetAddress &sender,
					  const DHTKey &sgKey,
				       l2::l2_subscribe_response *l2r) throw (dht_exception)
     {
	// serialize.
	l1::l1_query *l2q = l1_protob_wrapper::create_l1_query(fct_id,recipientKey,recipient,
							       senderKey,sender,sgKey);
	l1::header *l2q_head = l2q->mutable_head();
	l2q_head->set_layer_id(2);
	
	std::string msg;
	try
	  {
	     l1_protob_wrapper::serialize_to_string(l2q,msg);
	  }
	catch(dht_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"rpc %u l2 error: %s",fct_id,e.what().c_str());
	     throw dht_exception(DHT_ERR_CALL, "rpc " + sp::miscutil::to_string(fct_id) + " l2 error " + e.what());
	  }
	
	RPC_call(msg,recipient,l2r);
     }

   void l2_protob_rpc_client::RPC_call(const uint32_t &fct_id,
					  const DHTKey &recipientKey,
					  const NetAddress &recipient,
					  const DHTKey &senderKey,
					  const NetAddress &sender,
					  const DHTKey &ownerKey,
					  const std::vector<Searchgroup*> &sgs,
					  const bool &sdiff,
				       l1::l1_response *l1r) throw (dht_exception)
     {
	// serialize.
	l1::l1_query *l2q = l2_data_protob_wrapper::create_l2_replication_query(fct_id,recipientKey,recipient,
										senderKey,sender,ownerKey,
										sgs,sdiff);
	l1::header *l2q_head = l2q->mutable_head();
	l2q_head->set_layer_id(2);
	
	std::string msg;
	try
	  {
	     l2_data_protob_wrapper::serialize_to_string(l2q,msg);
	  }
	catch (dht_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"rpc %u l2 error: %s",fct_id,e.what().c_str());
	     throw dht_exception(DHT_ERR_CALL, "rpc " + sp::miscutil::to_string(fct_id) + " l2 error " + e.what());
	  }
	RPC_call(msg,recipient,l1r);
     }
   					  
   void l2_protob_rpc_client::RPC_call(const std::string &msg,
					  const NetAddress &recipient,
				       l2::l2_subscribe_response *l2r) throw (dht_exception)
     {
	// send & get response.
	std::string resp_str;
        do_rpc_call(recipient,msg,true,resp_str);
	
	// deserialize response.
	try
	  {
	     l2_protob_wrapper::deserialize(resp_str,l2r);
	  }
	catch (dht_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"rpc l2 error: %s",e.what().c_str());
	     throw dht_exception(DHT_ERR_NETWORK, "rpc l2 error " + e.what());
	  }
     }
   
   void l2_protob_rpc_client::RPC_call(const std::string &msg,
					  const NetAddress &recipient,
				       l1::l1_response *l1r) throw (dht_exception)
     {
	
	// send & get response.
	std::string resp_str;
	do_rpc_call(recipient,msg,true,resp_str);
	
	// deserialize response.
	try
	  {
	     l1_protob_wrapper::deserialize(resp_str,l1r);
	  }
	catch (dht_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"rpc l2 error: %s",e.what().c_str());
	     throw dht_exception(DHT_ERR_NETWORK, "rpc l2 error " + e.what());
	  }
     }
	          
   void l2_protob_rpc_client::RPC_subscribe(const DHTKey &recipientKey,
					       const NetAddress &recipient,
					       const DHTKey &senderKey,
					       const NetAddress &senderAddress,
					       const DHTKey &sgKey,
					       std::vector<Subscriber*> &peers,
					    int &status) throw (dht_exception)
     {
	//debug
	std::cerr << "[Debug]: RPC_subscribe call\n";
	//debug
		
	// do call, wait and get response.
	l2::l2_subscribe_response l2r;
	
	try
	  {
	     if (senderKey.count() == 0)
	       l2_protob_rpc_client::RPC_call(hash_subscribe,
						    recipientKey,recipient,
						    sgKey,&l2r);
	     else 
	       l2_protob_rpc_client::RPC_call(hash_subscribe,
						    recipientKey,recipient,
						    senderKey,senderAddress,sgKey,&l2r);
	  }
	catch (dht_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_DHT, "Failed subscribe call to %s: %s",
			       recipient.toString().c_str(),e.what().c_str());
	     throw dht_exception(DHT_ERR_CALL, "Failed subscribe call to " + recipient.toString() + ":" + e.what());
	  }
	
	// handle the response.
	uint32_t error_status;
	l2_protob_wrapper::read_l2_subscribe_response(&l2r,error_status,peers);
	status = error_status;
     }

   void l2_protob_rpc_client::RPC_replicate(const DHTKey &recipientKey,
					       const NetAddress &recipient,
					       const DHTKey &senderKey,
					       const NetAddress &senderAddress,
					       const DHTKey &ownerKey,
					       const std::vector<Searchgroup*> &sgs,
					       const bool &sdiff,
					    int &status) throw (dht_exception)
     {
	//debug
	std::cerr << "[Debug]: RPC_replicate call\n";
	//debug
	
	// do call, wait and get response.
	l1::l1_response l1r;
	
	try
	  {
	     l2_protob_rpc_client::RPC_call(hash_replicate,
						  recipientKey,recipient,
						  senderKey,senderAddress,
						  ownerKey,sgs,sdiff,&l1r);
	  }
	catch (dht_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_DHT, "Failed replicate call to %s: %s",
			       recipient.toString().c_str(),e.what().c_str());
	     throw dht_exception(DHT_ERR_CALL, "Failed replicate call to " + recipient.toString() + ":" + e.what());
	  }

	// handle the response.
	uint32_t error_status;
	l1_protob_wrapper::read_l1_response(&l1r,error_status);
	status = error_status;
     }
      
} /* end of namespace. */
