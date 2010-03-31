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

#include "l1_protob_rpc_client.h"
#include "errlog.h"

using sp::errlog;

namespace dht
{
   l1_protob_rpc_client::l1_protob_rpc_client()
     {
     }
   
   l1_protob_rpc_client::~l1_protob_rpc_client()
     {
     }
   
   dht_err l1_protob_rpc_client::RPC_call(const uint32_t &fct_id,
					  const DHTKey &recipientKey,
					  const NetAddress& recipient,
					  const DHTKey &senderKey,
					  const NetAddress& senderAddress,
					  l1::l1_response *l1r)
     {
	// serialize.
	l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,
							       recipientKey,recipient,
							       senderKey,senderAddress);
	std::string msg_str;
	l1_protob_wrapper::serialize_to_string(l1q,msg_str);
	std::string resp_str;
	
	// send & get response.
	try
	  {
	     do_rpc_call_threaded(recipient,msg_str,true,resp_str); // XXX: the recipient port must be set...
	     delete l1q;
	     l1q = NULL;
	  }
	catch (dht_exception &e)
	  {
	     delete l1q;
	     l1q = NULL;
	     throw; // re-throw the exception.
	  }
	
	// deserialize response.
	try 
	  {
	     l1_protob_wrapper::deserialize(resp_str,l1r); // TODO: catch exception.
	  }
	catch (l1_fail_deserialize_exception &e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"rpc l1 error: %s",e.what().c_str());
	     return DHT_ERR_NETWORK;
	  }
		     
	return DHT_ERR_OK;
     }
   
   dht_err l1_protob_rpc_client::RPC_getSuccessor(const DHTKey &recipientKey,
						  const NetAddress& recipient,
						  const DHTKey &senderKey,
						  const NetAddress& senderAddress,
						  DHTKey& dkres, NetAddress& na,
						  int& status)
     {
	// do call, wait and get response.
	l1::l1_response *l1r = new l1::l1_response();	
	dht_err err = l1_protob_rpc_client::RPC_call(hash_get_successor,
						     recipientKey,recipient,
						     senderKey,senderAddress,
						     l1r);
	// handle the response.
     	uint32_t layer_id, error_status;
	err = l1_protob_wrapper::read_l1_response(l1r,layer_id,error_status,
						  dkres,na);
	status = error_status;
	delete l1r;
	return DHT_ERR_OK;
     }
   
   dht_err l1_protob_rpc_client::RPC_getPredecessor(const DHTKey& recipientKey,
						    const NetAddress& recipient,
						    const DHTKey& senderKey,
						    const NetAddress& senderAddress,
						    DHTKey& dkres, NetAddress& na,
						    int& status)
     {
	// do call, wait and get response.
	l1::l1_response *l1r = new l1::l1_response();
	dht_err err = l1_protob_rpc_client::RPC_call(hash_get_predecessor,
						     recipientKey,recipient,
						     senderKey,senderAddress,
						     l1r);
	// handle the response.
	uint32_t layer_id, error_status;
	err = l1_protob_wrapper::read_l1_response(l1r,layer_id,error_status,
						  dkres,na);
	status = error_status;
	delete l1r;
	return DHT_ERR_OK;
     }
     
   dht_err l1_protob_rpc_client::RPC_notify(const DHTKey& recipientKey,
					    const NetAddress& recipient,
					    const DHTKey& senderKey,
					    const NetAddress& senderAddress,
					    int& status)
     {
	// do call, wait and get response.
	l1::l1_response *l1r = new l1::l1_response();
	dht_err err = l1_protob_rpc_client::RPC_call(hash_notify,
						     recipientKey,recipient,
						     senderKey,senderAddress,
						     l1r);
	
	// handle the response.
	uint32_t layer_id, error_status;
	err = l1_protob_wrapper::read_l1_response(l1r,layer_id,error_status);
	status = error_status;
	delete l1r;
	return DHT_ERR_OK;
     }
        
   dht_err l1_protob_rpc_client::RPC_findClosestPredecessor(const DHTKey& recipientKey,
							    const NetAddress& recipient,
							    const DHTKey& senderKey,
							    const NetAddress& senderAddress,
							    const DHTKey& nodeKey,
							    DHTKey& dkres, NetAddress& na,
							    DHTKey& dkres_succ,
							    NetAddress &dkres_succ_na,
							    int& status)
     {
	// do call, wait and get response.
	l1::l1_response *l1r = new l1::l1_response();
	dht_err err = l1_protob_rpc_client::RPC_call(hash_find_closest_predecessor,
						     recipientKey,recipient,
						     senderKey,senderAddress,
						     l1r);
	// handle the response.
	uint32_t layer_id, error_status;
	err = l1_protob_wrapper::read_l1_response(l1r,layer_id,error_status,
						  dkres,na,
						  dkres_succ,dkres_succ_na);
	status = error_status;
	delete l1r;
	return DHT_ERR_OK;
     }
   
} /* end of namespace. */
