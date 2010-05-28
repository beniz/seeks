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

#include "l2_protob_rpc_client.h"
#include "errlog.h"

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
   
   dht_err l2_protob_rpc_client::RPC_call(const uint32_t &fct_id,
					  const DHTKey &recipientKey,
					  const NetAddress &recipient,
					  const DHTKey &sgKey,
					  l2::l2_subscribe_response *l2r)
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
	catch(l1_fail_serialize_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"rpc l2 error: %s",e.what().c_str());
	     return DHT_ERR_CALL;
	  }
		     
	return RPC_call(msg,recipient,l2r);
     }

   dht_err l2_protob_rpc_client::RPC_call(const uint32_t &fct_id,
					  const DHTKey &recipientKey,
					  const NetAddress &recipient,
					  const DHTKey &senderKey,
					  const NetAddress &sender,
					  const DHTKey &sgKey,
					  l2::l2_subscribe_response *l2r)
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
	catch(l1_fail_serialize_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"rpc l2 error: %s",e.what().c_str());
	     return DHT_ERR_CALL;
	  }
	
	return RPC_call(msg,recipient,l2r);
     }
      
   dht_err l2_protob_rpc_client::RPC_call(const std::string &msg,
					  const NetAddress &recipient,
					  l2::l2_subscribe_response *l2r)
     {
	// send & get response.
	std::string resp_str;
     	dht_err err = do_rpc_call_threaded(recipient,msg,true,resp_str);
	if (err != DHT_ERR_OK)
	  {
	     return err;
	  }
	
	// deserialize response.
	try
	  {
	     l2_protob_wrapper::deserialize(resp_str,l2r);
	  }
	catch (l2_fail_deserialize_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"rpc l2 error: %s",e.what().c_str());
	     return DHT_ERR_NETWORK;
	  }
	return DHT_ERR_OK;
     }
   
   dht_err l2_protob_rpc_client::RPC_subscribe(const DHTKey &recipientKey,
					       const NetAddress &recipient,
					       const DHTKey &senderKey,
					       const NetAddress &senderAddress,
					       const DHTKey &sgKey,
					       std::vector<Subscriber*> &peers,
					       int &status)
     {
	//debug
	std::cerr << "[Debug]: RPC_subscribe call\n";
	//debug
	
	// do call, wait and get response.
	l2::l2_subscribe_response *l2r = new l2::l2_subscribe_response();
	dht_err err = DHT_ERR_OK;
	
	try
	  {
	     if (senderKey.count() != 0)
	       err = l2_protob_rpc_client::RPC_call(hash_subscribe,
						    recipientKey,recipient,
						    sgKey,l2r);
	     else 
	       err = l2_protob_rpc_client::RPC_call(hash_subscribe,
						    recipientKey,recipient,
						    senderKey,senderAddress,sgKey,l2r);
	  }
	catch (dht_exception &e)
	  {
	     delete l2r;
	     errlog::log_error(LOG_LEVEL_DHT, "Failed subscribe call to %s: %s",
			       recipient.toString().c_str(),e.what().c_str());
	     return DHT_ERR_CALL;
	  }
	
	// check on local error status.
	if (err != DHT_ERR_OK)
	  {
	     delete l2r;
	     return err;
	  }
	
	// handle the response.
	uint32_t error_status;
	err = l2_protob_wrapper::read_l2_subscribe_response(l2r,error_status,peers);
	status = error_status;
	delete l2r;
	return err;
     }
      
} /* end of namespace. */
