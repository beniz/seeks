/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2009, 2010  Emmanuel Benazera, juban@free.fr
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

#include "l1_protob_wrapper.h"
#include "errlog.h"

#include <algorithm>
#include <iterator>

using sp::errlog;

namespace dht
{
   /*- data to protobuffers. -*/
   
   /*- queries. -*/
   l1::l1_query* l1_protob_wrapper::create_l1_query(const uint32_t &fct_id,
						    const DHTKey &recipient_dhtkey,
						    const NetAddress &recipient_na,
						    const DHTKey &sender_dhtkey,
						    const NetAddress &sender_na)
     {
	std::vector<unsigned char> dkser = DHTKey::serialize(recipient_dhtkey);
	std::string recipient_key(dkser.begin(),dkser.end());
	uint32_t recipient_ip_addr = recipient_na.serialize_ip();
	std::string recipient_net_port = recipient_na.serialize_port();
	std::vector<unsigned char> dsser = DHTKey::serialize(sender_dhtkey);
	std::string sender_key(dsser.begin(),dsser.end());
	uint32_t sender_ip_addr = sender_na.serialize_ip();
	std::string sender_net_port = sender_na.serialize_port();
	l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,recipient_key,
							       recipient_ip_addr,recipient_net_port,
							       sender_key,sender_ip_addr,sender_net_port);
	return l1q;
     }
   
   l1::l1_query* l1_protob_wrapper::create_l1_query(const uint32_t &fct_id,
						    const std::string &recipient_key,
						    const uint32_t &recipient_ip_addr,
						    const std::string &recipient_net_port)
     {  
	l1::l1_query *l1q = new l1::l1_query();
	l1::header *l1q_head = l1q->mutable_head();
	l1q_head->set_layer_id(1);
	l1q_head->set_fct_id(fct_id);
	l1::vnodeid *l1q_recipient = l1q->mutable_recipient();
	l1::dht_key *l1q_recipient_key = l1q_recipient->mutable_key();
	l1q_recipient_key->set_key(recipient_key);
	l1::net_address *l1q_recipient_addr = l1q_recipient->mutable_addr();
	l1q_recipient_addr->set_ip_addr(recipient_ip_addr);
	l1q_recipient_addr->set_net_port(recipient_net_port);
	return l1q;
     }

   l1::l1_query* l1_protob_wrapper::create_l1_query(const uint32_t &fct_id,
						    const std::string &recipient_key,
						    const uint32_t &recipient_ip_addr,
						    const std::string &recipient_net_port,
						    const std::string &sender_key,
						    const uint32_t &sender_ip_addr,
						    const std::string &sender_net_port)
     {
	l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,recipient_key,
							       recipient_ip_addr,recipient_net_port);
	l1::vnodeid *l1q_sender = l1q->mutable_sender();
	l1::dht_key *l1q_sender_key = l1q_sender->mutable_key();
	l1q_sender_key->set_key(sender_key);
	l1::net_address *l1q_sender_addr = l1q_sender->mutable_addr();
	l1q_sender_addr->set_ip_addr(sender_ip_addr);
	l1q_sender_addr->set_net_port(sender_net_port);
	return l1q;
     }
   
   void l1_protob_wrapper::serialize_to_string(const l1::l1_query *l1q, std::string &str)
     {
	if (!l1q)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "can't serialize null query");
	     throw l1_fail_serialize_exception();
	  }
	if (l1q->SerializeToString(&str)) // != 0 on error.
	  throw l1_fail_serialize_exception();
     }
   
   /*- responses. -*/
   l1::l1_response* l1_protob_wrapper::create_l1_response(const uint32_t &error_status,
							  const DHTKey &resultKey,
							  const NetAddress &resultAddress,
							  const DHTKey &foundKey,
							  const NetAddress &foundAddress)
     {
	std::vector<unsigned char> dkresult = DHTKey::serialize(resultKey);
	std::string result_key(dkresult.begin(),dkresult.end());
	uint32_t result_ip_addr = resultAddress.serialize_ip();
	std::string result_net_port = resultAddress.serialize_port();
	std::vector<unsigned char> dkfound = DHTKey::serialize(foundKey);
	std::string found_key(dkfound.begin(),dkfound.end());
	uint32_t found_ip_addr = foundAddress.serialize_ip();
	std::string found_net_port = foundAddress.serialize_port();
	l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(error_status,result_key,
								     result_ip_addr,result_net_port,
								     found_key,found_ip_addr,
								     found_net_port);
	return l1r;
     }

   l1::l1_response* l1_protob_wrapper::create_l1_response(const uint32_t &error_status,
							  const DHTKey &resultKey,
							  const NetAddress &resultAddress)
     {
	std::vector<unsigned char> dkresult = DHTKey::serialize(resultKey);
	std::string result_key(dkresult.begin(),dkresult.end());
	uint32_t result_ip_addr = resultAddress.serialize_ip();
	std::string result_net_port = resultAddress.serialize_port();
	l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(error_status,result_key,
								     result_ip_addr,result_net_port);
	return l1r;
     }
   
   l1::l1_response* l1_protob_wrapper::create_l1_response(const uint32_t error_status,
							  const std::string &result_key,
							  const uint32_t &result_ip_addr,
							  const std::string &result_net_port)
     {
	l1::l1_response *l1r = new l1::l1_response();
	l1::header *l1r_head = l1r->mutable_head();
	l1r_head->set_layer_id(1);
	l1r->set_error_status(error_status);
	l1::vnodeid *l1r_found_vnode = l1r->mutable_found_vnode();
	l1::dht_key *l1r_result_key = l1r_found_vnode->mutable_key();
	l1r_result_key->set_key(result_key);
	l1::net_address *l1r_result_addr = l1r_found_vnode->mutable_addr();
	l1r_result_addr->set_ip_addr(result_ip_addr);
	l1r_result_addr->set_net_port(result_net_port);
	return l1r;
     }
   
   l1::l1_response* l1_protob_wrapper::create_l1_response(const uint32_t error_status,
							  const std::string &result_key,
							  const uint32_t &result_ip_addr,
							  const std::string &result_net_port,
							  const std::string &found_key,
							  const uint32_t &found_ip_addr,
							  const std::string &found_net_port)
     {
	l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(error_status,
								     result_key,result_ip_addr,
								     result_net_port);
	l1::vnodeid *l1r_found_vnode_succ = l1r->mutable_found_vnode_succ();
	l1::dht_key *l1r_found_key = l1r_found_vnode_succ->mutable_key();
	l1r_found_key->set_key(found_key);
	l1::net_address *l1r_found_addr = l1r_found_vnode_succ->mutable_addr();
	l1r_found_addr->set_ip_addr(found_ip_addr);
	l1r_found_addr->set_net_port(found_net_port);
	return l1r;
     }

   void l1_protob_wrapper::serialize_to_string(const l1::l1_response *l1r, std::string &str)
     {
	if (!l1r)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "can't serialize null response");
	     throw l1_fail_serialize_exception();
	  }
	if (l1r->SerializeToString(&str)) // != 0 on error.
	  throw l1_fail_serialize_exception();
     }
      
   /*- protobuffers to data. -*/
   dht_err l1_protob_wrapper::read_l1_query(const l1::l1_query *l1q,
					    uint32_t &layer_id,
					    uint32_t &fct_id,
					    DHTKey &recipient_dhtkey,
					    NetAddress &recipient_na,
					    DHTKey &sender_dhtkey,
					    NetAddress &sender_na)
     {
	std::string recipient_key, sender_key;
	uint32_t recipient_ip_addr, sender_ip_addr;
	std::string recipient_net_port, sender_net_port;
	l1_protob_wrapper::read_l1_query(l1q,layer_id,fct_id,recipient_key,
					 recipient_ip_addr,recipient_net_port,
					 sender_key,sender_ip_addr,sender_net_port);
	std::vector<unsigned char> ser;
	std::copy(recipient_key.begin(),recipient_key.end(),std::back_inserter(ser));
	recipient_dhtkey = DHTKey::unserialize(ser);
	std::string recipient_ip = NetAddress::unserialize_ip(recipient_ip_addr);
	short recipient_port = NetAddress::unserialize_port(recipient_net_port);
	recipient_na = NetAddress(recipient_ip,recipient_port);
	ser.clear();
	std::copy(sender_key.begin(),sender_key.end(),std::back_inserter(ser));
	sender_dhtkey = DHTKey::unserialize(ser);
	std::string sender_ip = NetAddress::unserialize_ip(sender_ip_addr);
	short sender_port = NetAddress::unserialize_port(sender_net_port);
	sender_na = NetAddress(sender_ip,sender_port);
	return DHT_ERR_OK;
     }
   
   dht_err l1_protob_wrapper::read_l1_query(const l1::l1_query *l1q,
					    uint32_t &layer_id,
					    uint32_t &fct_id,
					    std::string &recipient_key,
					    uint32_t &recipient_ip_addr,
					    std::string &recipient_net_port,
					    std::string &sender_key,
					    uint32_t &sender_ip_addr,
					    std::string &sender_net_port)
     {
	const l1::header l1q_head = l1q->head();
	layer_id = l1q_head.layer_id();
	fct_id = l1q_head.fct_id();
	l1::vnodeid l1q_recipient = l1q->recipient();
	l1::dht_key l1q_recipient_key = l1q_recipient.key();
	recipient_key = l1q_recipient_key.key();
	l1::net_address l1q_recipient_addr = l1q_recipient.addr();
	recipient_ip_addr = l1q_recipient_addr.ip_addr();
	recipient_net_port = l1q_recipient_addr.net_port();
	l1::vnodeid l1q_sender = l1q->sender();
	l1::dht_key l1q_sender_key = l1q_sender.key();
	sender_key = l1q_sender_key.key();
	l1::net_address l1q_sender_addr = l1q_sender.addr();
	sender_ip_addr = l1q_sender_addr.ip_addr();
	sender_net_port = l1q_sender_addr.net_port();
	return DHT_ERR_OK;
     }
   
   void l1_protob_wrapper::deserialize(const std::string &str, l1::l1_query *l1q)
     {
	if (!l1q)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "can't deserialize to null query");
	     throw l1_fail_deserialize_exception();
	  }
	if (l1q->ParseFromString(str))
	  throw l1_fail_deserialize_exception();
     }

   /*- responses -*/
   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &layer_id,
					       uint32_t &error_status,
					       DHTKey &resultKey,
					       NetAddress &resultAddress,
					       DHTKey &foundKey,
					       NetAddress &foundAddress)
     {
	std::string result_key, found_key;
	uint32_t result_ip_addr, found_ip_addr;
	std::string result_net_port, found_net_port;
	l1_protob_wrapper::read_l1_response(l1r,layer_id,error_status,
					    result_key,result_ip_addr,
					    result_net_port,found_key,
					    found_ip_addr,found_net_port);
	std::vector<unsigned char> ser;
	std::copy(result_key.begin(),result_key.end(),std::back_inserter(ser));
	resultKey = DHTKey::unserialize(ser);
	std::string result_ip = NetAddress::unserialize_ip(result_ip_addr);
	short result_port = NetAddress::unserialize_port(result_net_port);
	resultAddress = NetAddress(result_ip,result_port);
	ser.clear();
	std::copy(found_key.begin(),found_key.end(),std::back_inserter(ser));
	foundKey = DHTKey::unserialize(ser);
	std::string found_ip = NetAddress::unserialize_ip(found_ip_addr);
	short found_port = NetAddress::unserialize_port(found_net_port);
	foundAddress = NetAddress(found_ip,found_port);
	return DHT_ERR_OK;
     }
      
   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &layer_id,
					       uint32_t &error_status,
					       DHTKey &resultKey,
					       NetAddress &resultAddress)
     {
	std::string result_key;
	uint32_t result_ip_addr;
	std::string result_net_port, found_net_port;
	l1_protob_wrapper::read_l1_response(l1r,layer_id,error_status,
					    result_key,result_ip_addr,
					    result_net_port);
	std::vector<unsigned char> ser;
	std::copy(result_key.begin(),result_key.end(),std::back_inserter(ser));
	resultKey = DHTKey::unserialize(ser);
	std::string result_ip = NetAddress::unserialize_ip(result_ip_addr);
	short result_port = NetAddress::unserialize_port(result_net_port);
	resultAddress = NetAddress(result_ip,result_port);
	return DHT_ERR_OK;
     }
   
   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &layer_id,
					       uint32_t &error_status,
					       std::string &result_key,
					       uint32_t &result_ip_addr,
					       std::string &result_net_port,
					       std::string &found_key,
					       uint32_t &found_ip_addr,
					       std::string &found_net_port)
     {
	l1_protob_wrapper::read_l1_response(l1r,layer_id,error_status,
					    result_key,result_ip_addr,result_net_port);
	l1::vnodeid l1r_found_vnode_succ = l1r->found_vnode_succ();
	l1::dht_key l1r_found_key = l1r_found_vnode_succ.key();
	found_key = l1r_found_key.key();
	l1::net_address l1r_found_addr = l1r_found_vnode_succ.addr();
	found_ip_addr = l1r_found_addr.ip_addr();
	found_net_port = l1r_found_addr.net_port();
	return DHT_ERR_OK;
     }
        
   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &layer_id,
					       uint32_t &error_status,
					       std::string &result_key,
					       uint32_t &result_ip_addr,
					       std::string &result_net_port)
     {
	const l1::header l1r_head = l1r->head();
	layer_id = l1r_head.layer_id();
	error_status = l1r->error_status();
	l1::vnodeid l1r_found_vnode = l1r->found_vnode();
	l1::dht_key l1r_result_key = l1r_found_vnode.key();
	result_key = l1r_result_key.key();
	l1::net_address l1r_result_addr = l1r_found_vnode.addr();
	result_ip_addr = l1r_result_addr.ip_addr();
	result_net_port = l1r_result_addr.net_port();
	return DHT_ERR_OK;
     }

   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &layer_id,
					       uint32_t &error_status)
     {
	const l1::header l1r_head = l1r->head();
	layer_id = l1r_head.layer_id();
	error_status = l1r->error_status();
	return DHT_ERR_OK;
     }
   
   void l1_protob_wrapper::deserialize(const std::string &str, l1::l1_response *l1r)
     {
	if (!l1r)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "can't deserialize to null response");
	     throw l1_fail_deserialize_exception();
	  }
	if (l1r->ParseFromString(str))
	  throw l1_fail_deserialize_exception();
     }
   
} /* end of namespace. */
