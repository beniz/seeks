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
						    const NetAddress &recipient_na)
     {
	std::vector<unsigned char> dkser = DHTKey::serialize(recipient_dhtkey);
	std::string recipient_key(dkser.begin(),dkser.end());
	l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,recipient_key,
							       recipient_na.getNetAddress(),
							       recipient_na.getPort());
	return l1q;
     }
      
   l1::l1_query* l1_protob_wrapper::create_l1_query(const uint32_t &fct_id,
						    const DHTKey &recipient_dhtkey,
						    const NetAddress &recipient_na,
						    const DHTKey &sender_dhtkey,
						    const NetAddress &sender_na)
     {
	std::vector<unsigned char> dkser = DHTKey::serialize(recipient_dhtkey);
	std::string recipient_key(dkser.begin(),dkser.end());
	std::vector<unsigned char> dsser = DHTKey::serialize(sender_dhtkey);
	std::string sender_key(dsser.begin(),dsser.end());
	uint32_t sender_ip_addr = 0;
	std::string sender_net_port;
	l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,recipient_key,
							       recipient_na.getNetAddress(),recipient_na.getPort(),
							       sender_key,
							       sender_na.getNetAddress(),sender_na.getPort());
	return l1q;
     }
   
   l1::l1_query* l1_protob_wrapper::create_l1_query(const uint32_t &fct_id,
						    const DHTKey &recipient_dhtkey,
						    const NetAddress &recipient_na,
						    const DHTKey &node_key)
     {
	l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,recipient_dhtkey,
							       recipient_na);
	std::vector<unsigned char> dkser = DHTKey::serialize(node_key);
	std::string node_key_str(dkser.begin(),dkser.end());
	l1::dht_key *l1q_node_key = l1q->mutable_lookedup_key();
	l1q_node_key->set_key(node_key_str);
	return l1q;
     }
      
   l1::l1_query* l1_protob_wrapper::create_l1_query(const uint32_t &fct_id,
						    const std::string &recipient_key,
						    const std::string &recipient_ip_addr,
						    const short &recipient_net_port)
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
						    const std::string &recipient_ip_addr,
						    const short &recipient_net_port,
						    const std::string &sender_key,
						    const std::string &sender_ip_addr,
						    const short &sender_net_port)
     {
	l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,recipient_key,
							       recipient_ip_addr,recipient_net_port);
	l1::vnodeid *l1q_sender = l1q->mutable_sender();
	l1::dht_key *l1q_sender_key = l1q_sender->mutable_key();
	l1q_sender_key->set_key(sender_key);
	if (!sender_ip_addr.empty())
	  {
	     l1::net_address *l1q_sender_addr = l1q_sender->mutable_addr();
	     l1q_sender_addr->set_ip_addr(sender_ip_addr);
	     l1q_sender_addr->set_net_port(sender_net_port);
	  }
	return l1q;
     }
   
   void l1_protob_wrapper::serialize_to_string(const l1::l1_query *l1q, std::string &str)
     {
	if (!l1q)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "can't serialize null query");
	     throw l1_fail_serialize_exception();
	  }
	if (!l1q->SerializeToString(&str)) // != 0 on error.
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
	std::vector<unsigned char> dkfound = DHTKey::serialize(foundKey);
	std::string found_key(dkfound.begin(),dkfound.end());
	l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(error_status,result_key,
								     resultAddress.getNetAddress(),resultAddress.getPort(),
								     found_key,
								     foundAddress.getNetAddress(),foundAddress.getPort());
	return l1r;
     }

   l1::l1_response* l1_protob_wrapper::create_l1_response(const uint32_t &error_status,
							  const DHTKey &resultKey,
							  const NetAddress &resultAddress)
     {
	std::vector<unsigned char> dkresult = DHTKey::serialize(resultKey);
	std::string result_key(dkresult.begin(),dkresult.end());
	l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(error_status,result_key,
								     resultAddress.getNetAddress(),
								     resultAddress.getPort());
	return l1r;
     }
   
   l1::l1_response* l1_protob_wrapper::create_l1_response(const uint32_t &error_status,
							  const std::list<DHTKey> &resultKeyList,
							  const std::list<NetAddress> &resultAddressList)
     {
	std::vector<unsigned char> dkresult;
	std::list<std::string> result_key_list;
	std::list<DHTKey>::const_iterator sit = resultKeyList.begin();
	while(sit!=resultKeyList.end())
	  {
	     dkresult = DHTKey::serialize((*sit));
	     std::string result_key(dkresult.begin(),dkresult.end());
	     dkresult.clear();
	     result_key_list.push_back(result_key);
	     ++sit;
	  }
	std::list<std::pair<std::string,uint32_t> > result_address_list;
	std::list<NetAddress>::const_iterator tit = resultAddressList.begin();
	while(tit!=resultAddressList.end())
	  {
	     std::pair<std::string,uint32_t> pp;
	     pp.first = (*tit).getNetAddress();
	     pp.second = (*tit).getPort();
	     result_address_list.push_back(pp);
	     ++tit;
	  }
	l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(error_status,result_key_list,
								     result_address_list);
	return l1r;
     }
   
   l1::l1_response* l1_protob_wrapper::create_l1_response(const uint32_t &error_status,
							  const std::list<std::string> &result_key_list,
							  const std::list<std::pair<std::string,uint32_t> > &result_address_list)
     {
	l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(error_status);
	l1::nodelist *l1r_vnodelist = l1r->mutable_vnodelist();
	std::list<std::string>::const_iterator sit = result_key_list.begin();
	while(sit!=result_key_list.end())
	  {
	     l1::vnodeid *l1r_found_vnode = l1r_vnodelist->add_nodes();
	     l1::dht_key *l1r_result_key = l1r_found_vnode->mutable_key();
	     l1r_result_key->set_key((*sit));
	     ++sit;
	  }
	std::list<std::pair<std::string,uint32_t> >::const_iterator tit = result_address_list.begin();
	int i = 0;
	while(tit!=result_address_list.end())
	  {
	     l1::vnodeid *l1r_found_vnode = l1r_vnodelist->mutable_nodes(i);
	     l1::net_address *l1r_result_addr = l1r_found_vnode->mutable_addr();
	     l1r_result_addr->set_ip_addr((*tit).first);
	     l1r_result_addr->set_net_port((*tit).second);
	     i++;
	     ++tit;
	  }
	return l1r;
     }
      
   l1::l1_response* l1_protob_wrapper::create_l1_response(const uint32_t &error_status)
     {
	l1::l1_response *l1r = new l1::l1_response();
	l1r->set_error_status(error_status);
	return l1r;
     }
      
   l1::l1_response* l1_protob_wrapper::create_l1_response(const uint32_t &error_status,
							  const std::string &result_key,
							  const std::string &result_ip_addr,
							  const short &result_net_port)
     {
	l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(error_status);
	l1::vnodeid *l1r_found_vnode = l1r->mutable_found_vnode();
	l1::dht_key *l1r_result_key = l1r_found_vnode->mutable_key();
	l1r_result_key->set_key(result_key);
	l1::net_address *l1r_result_addr = l1r_found_vnode->mutable_addr();
	l1r_result_addr->set_ip_addr(result_ip_addr);
	l1r_result_addr->set_net_port(result_net_port);
	return l1r;
     }

   l1::l1_response* l1_protob_wrapper::create_l1_response(const uint32_t &error_status,
							  const std::string &result_key,
							  const std::string &result_ip_addr,
							  const short &result_net_port,
							  const std::string &found_key,
							  const std::string &found_ip_addr,
							  const short &found_net_port)
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
	if (!l1r->SerializeToString(&str))
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
	std::string recipient_ip_addr, sender_ip_addr;
	uint32_t recipient_net_port, sender_net_port;
	l1_protob_wrapper::read_l1_query(l1q,layer_id,fct_id,recipient_key,
					 recipient_ip_addr,recipient_net_port,
					 sender_key,sender_ip_addr,sender_net_port);
	std::vector<unsigned char> ser;
	std::copy(recipient_key.begin(),recipient_key.end(),std::back_inserter(ser));
	recipient_dhtkey = DHTKey::unserialize(ser);
	recipient_na = NetAddress(recipient_ip_addr,recipient_net_port);
	ser.clear();
	if (!sender_key.empty())
	  {
	     std::copy(sender_key.begin(),sender_key.end(),std::back_inserter(ser));
	     sender_dhtkey = DHTKey::unserialize(ser);
	     sender_na = NetAddress(sender_ip_addr,sender_net_port);
	  }
	else
	  {
	     sender_dhtkey = DHTKey();
	     sender_na = NetAddress();
	  }
	return DHT_ERR_OK;
     }
   
   dht_err l1_protob_wrapper::read_l1_query(const l1::l1_query *l1q,
					    uint32_t &layer_id,
					    uint32_t &fct_id,
					    DHTKey &recipient_dhtkey,
					    NetAddress &recipient_na,
					    DHTKey &sender_dhtkey,
					    NetAddress &sender_na,
					    DHTKey &nodekey)
     {
	dht_err err = l1_protob_wrapper::read_l1_query(l1q,layer_id,fct_id,
						       recipient_dhtkey,recipient_na,
						       sender_dhtkey,sender_na);
	l1::dht_key l1q_node_key = l1q->lookedup_key();
	std::string node_key_str = l1q_node_key.key();
	if (!node_key_str.empty()) // optional field.
	  {
	     std::vector<unsigned char> ser;
	     std::copy(node_key_str.begin(),node_key_str.end(),std::back_inserter(ser));
	     nodekey = DHTKey::unserialize(ser);
	  }
	if (err != DHT_ERR_OK)
	  return err;
	else return DHT_ERR_OK;
     }
        
   dht_err l1_protob_wrapper::read_l1_query(const l1::l1_query *l1q,
					    uint32_t &layer_id,
					    uint32_t &fct_id,
					    std::string &recipient_key,
					    std::string &recipient_ip_addr,
					    uint32_t &recipient_net_port,
					    std::string &sender_key,
					    std::string &sender_ip_addr,
					    uint32_t &sender_net_port)
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
	if (!l1q->ParseFromString(str))
	  throw l1_fail_deserialize_exception();
     }

   /*- responses -*/
   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &error_status,
					       DHTKey &resultKey,
					       NetAddress &resultAddress,
					       DHTKey &foundKey,
					       NetAddress &foundAddress)
     {
	std::string result_key, found_key;
	std::string result_ip_addr, found_ip_addr;
	uint32_t result_net_port, found_net_port;
	dht_err err = l1_protob_wrapper::read_l1_response(l1r,error_status,
							  result_key,result_ip_addr,
							  result_net_port,found_key,
							  found_ip_addr,found_net_port);
	if (err != DHT_ERR_OK)
	  return err;
	std::vector<unsigned char> ser;
	std::copy(result_key.begin(),result_key.end(),std::back_inserter(ser));
	resultKey = DHTKey::unserialize(ser);
	ser.clear();
	resultAddress = NetAddress(result_ip_addr,result_net_port);
	if (!found_key.empty())
	  {
	     std::copy(found_key.begin(),found_key.end(),std::back_inserter(ser));
	     foundKey = DHTKey::unserialize(ser);
	     foundAddress = NetAddress(found_ip_addr,found_net_port);
	  }
	return DHT_ERR_OK;
     }
      
   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &error_status,
					       DHTKey &resultKey,
					       NetAddress &resultAddress)
     {
	std::string result_key;
	std::string result_ip_addr;
	uint32_t result_net_port;
	dht_err err = l1_protob_wrapper::read_l1_response(l1r,error_status,
							  result_key,result_ip_addr,
							  result_net_port);
	if (err != DHT_ERR_OK)
	  return err;
	std::vector<unsigned char> ser;
	std::copy(result_key.begin(),result_key.end(),std::back_inserter(ser));
	resultKey = DHTKey::unserialize(ser);
	resultAddress = NetAddress(result_ip_addr,result_net_port);
	return DHT_ERR_OK;
     }
   
   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &error_status,
					       std::list<DHTKey> &dkres_list,
					       std::list<NetAddress> &na_list)
     {
	std::list<std::string> result_key_list;
	std::list<std::pair<std::string,uint32_t> > result_address_list;
	dht_err err = l1_protob_wrapper::read_l1_response(l1r,error_status,
							  result_key_list,result_address_list);
	if (err != DHT_ERR_OK)
	  return err;
	std::vector<unsigned char> ser;
	std::list<std::string>::const_iterator sit = result_key_list.begin();
	while(sit!=result_key_list.end())
	  {
	     std::copy((*sit).begin(),(*sit).end(),std::back_inserter(ser));
	     DHTKey resultKey = DHTKey::unserialize(ser);
	     ser.clear();
	     dkres_list.push_back(resultKey);
	     ++sit;
	  }
	std::list<std::pair<std::string,uint32_t> >::const_iterator tit = result_address_list.begin();
	while(tit!=result_address_list.end())
	  {
	     NetAddress resultAddress = NetAddress((*tit).first,(*tit).second);
	     na_list.push_back(resultAddress);
	     ++tit;
	  }
	return DHT_ERR_OK;
     }
        
   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &error_status,
					       std::list<std::string> &result_key_list,
					       std::list<std::pair<std::string,uint32_t> > &result_address_list)
     {
	dht_err err = l1_protob_wrapper::read_l1_response(l1r,error_status);
	if (err != DHT_ERR_OK)
	  return err;
	l1::nodelist l1r_vnodelist = l1r->vnodelist();
	int nnodes = l1r_vnodelist.nodes_size();
	for (int i=0;i<nnodes;i++)
	  {
	     l1::vnodeid l1r_found_vnode = l1r_vnodelist.nodes(i);
	     l1::dht_key l1r_result_key = l1r_found_vnode.key();
	     std::string result_key = l1r_result_key.key();
	     result_key_list.push_back(result_key);
	     l1::net_address l1r_result_addr = l1r_found_vnode.addr();
	     std::string result_ip_addr = l1r_result_addr.ip_addr();
	     uint32_t result_net_port = l1r_result_addr.net_port();
	     std::pair<std::string,uint32_t> pp(result_ip_addr,result_net_port);
	     result_address_list.push_back(pp);
	  }
	return DHT_ERR_OK;
     }
   
   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &error_status,
					       std::string &result_key,
					       std::string &result_ip_addr,
					       uint32_t &result_net_port,
					       std::string &found_key,
					       std::string &found_ip_addr,
					       uint32_t &found_net_port)
     {
	dht_err err = l1_protob_wrapper::read_l1_response(l1r,error_status,
							  result_key,result_ip_addr,result_net_port);
	if (err != DHT_ERR_OK)
	  return err;
	l1::vnodeid l1r_found_vnode_succ = l1r->found_vnode_succ();
	l1::dht_key l1r_found_key = l1r_found_vnode_succ.key();
	found_key = l1r_found_key.key();
	l1::net_address l1r_found_addr = l1r_found_vnode_succ.addr();
	found_ip_addr = l1r_found_addr.ip_addr();
	found_net_port = l1r_found_addr.net_port();
	return DHT_ERR_OK;
     }
        
   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &error_status,
					       std::string &result_key,
					       std::string &result_ip_addr,
					       uint32_t &result_net_port)
     {
	dht_err err = l1_protob_wrapper::read_l1_response(l1r,error_status);
	if (err != DHT_ERR_OK)
	  return err;
	l1::vnodeid l1r_found_vnode = l1r->found_vnode();
	l1::dht_key l1r_result_key = l1r_found_vnode.key();
	result_key = l1r_result_key.key();
	l1::net_address l1r_result_addr = l1r_found_vnode.addr();
	result_ip_addr = l1r_result_addr.ip_addr();
	result_net_port = l1r_result_addr.net_port();
	return DHT_ERR_OK;
     }

   dht_err l1_protob_wrapper::read_l1_response(const l1::l1_response *l1r,
					       uint32_t &error_status)
     {
	error_status = l1r->error_status();
	return error_status;
     }
   
   void l1_protob_wrapper::deserialize(const std::string &str, l1::l1_response *l1r)
     {
	if (!l1r)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "can't deserialize to null response");
	     throw l1_fail_deserialize_exception();
	  }
	if (!l1r->ParseFromString(str))
	  throw l1_fail_deserialize_exception();
     }
   
} /* end of namespace. */
