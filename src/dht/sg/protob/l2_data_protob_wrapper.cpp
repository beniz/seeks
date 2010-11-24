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

#include "l2_data_protob_wrapper.h"
#include "l2_protob_wrapper.h"
#include "l1_protob_wrapper.h"
#include "errlog.h"

#include <algorithm>
#include <iterator>

using sp::errlog;

namespace dht
{
   /* search groups. */
   l2::sg::searchgroup* l2_data_protob_wrapper::create_l2_searchgroup(const Searchgroup *sg)
     {
	l2::sg::searchgroup *l2_sg = new l2::sg::searchgroup();
	std::vector<unsigned char> dkser = DHTKey::serialize(sg->_idkey);
	std::string sg_key_str(dkser.begin(),dkser.end());
	dkser.clear();
	l1::dht_key *l2_sg_key = l2_sg->mutable_key();
	l2_sg_key->set_key(sg_key_str);
	l2_sg->set_radius(sg->_replication_level);
	size_t vsize = sg->_vec_subscribers.size();
	for (size_t i=0;i<vsize;i++)
	  {
	     l2::sg::subscriber *l2_sg_sub = l2_sg->add_subscribers();
	     l1::vnodeid *l2_sg_vnode = l2_sg_sub->mutable_id();
	     l1::dht_key *l2_sg_sub_key = l2_sg_vnode->mutable_key();
	     dkser = DHTKey::serialize(sg->_vec_subscribers.at(i)->_idkey);
	     std::string sub_key_str(dkser.begin(),dkser.end());
	     dkser.clear();
	     l2_sg_sub_key->set_key(sub_key_str);
	     l1::net_address *l2_sg_sub_addr = l2_sg_vnode->mutable_addr();
	     l2_sg_sub_addr->set_ip_addr(sg->_vec_subscribers.at(i)->getNetAddress());
	     l2_sg_sub_addr->set_net_port(sg->_vec_subscribers.at(i)->getPort());
	     l2_sg_sub->set_join_date(sg->_vec_subscribers.at(i)->_join_date);
	  }
	l2_sg->set_last_use(sg->_last_time_of_use);
	return l2_sg;
     }
      
   void l2_data_protob_wrapper::serialize_to_string(const l2::sg::searchgroup *l2_sg,
						    std::string &str)
     {
	if (!l2_sg)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"can't serialize null response");
	     throw dht_exception(DHT_ERR_MSG,"can't serialize null response");
	  }
	if (!l2_sg->SerializeToString(&str))
          throw dht_exception(DHT_ERR_MSG,"l2_sg::SerializeToString");
     }
   
   dht_err l2_data_protob_wrapper::read_l2_searchgroup(const l2::sg::searchgroup *l2_sg, Searchgroup *&sg)
     {
	std::string sg_key_str = l2_sg->key().key();
	std::vector<unsigned char> ser;
	std::copy(sg_key_str.begin(),sg_key_str.end(),std::back_inserter(ser));
	DHTKey sg_key = DHTKey::unserialize(ser);
	ser.clear();
	sg = new Searchgroup(sg_key);
	sg->_replication_level = l2_sg->radius();
	int nsubs = l2_sg->subscribers_size();
	for (int i=0;i<nsubs;i++)
	  {
	     l2::sg::subscriber l2_sg_sub = l2_sg->subscribers(i);
	     l1::vnodeid *l2_sg_sub_vnode = l2_sg_sub.mutable_id();
	     l1::dht_key *l2_sg_sub_key = l2_sg_sub_vnode->mutable_key();
	     std::string sub_key_str = l2_sg_sub_key->key();
	     std::copy(sub_key_str.begin(),sub_key_str.end(),std::back_inserter(ser));
	     DHTKey sub_key = DHTKey::unserialize(ser);
	     ser.clear();
	     l1::net_address *l2_sg_sub_addr = l2_sg_sub_vnode->mutable_addr();
	     std::string sub_addr = l2_sg_sub_addr->ip_addr();
	     uint32_t sub_port = l2_sg_sub_addr->net_port();
	     Subscriber *sub = new Subscriber(sub_key,sub_addr,sub_port);
	     sub->_join_date = l2_sg_sub.join_date();
	     sg->add_subscriber(sub);
	  }
	sg->_last_time_of_use = l2_sg->last_use();
	return DHT_ERR_OK;
     }
      
   void l2_data_protob_wrapper::deserialize_from_string(const std::string &str, l2::sg::searchgroup *l2_sg)
     {
	if (!l2_sg)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "can't deserialize to null response");
	     throw dht_exception(DHT_ERR_MSG,"l2_sg:can't deserialize to null response");
	  }
	if (!l2_sg->ParseFromString(str))
          throw dht_exception(DHT_ERR_MSG,"l2_sg::ParseFromString(" + str + ")");
     }
      
   /* messages. */
   l1::l1_query* l2_data_protob_wrapper::create_l2_replication_query(const uint32_t &fct_id,
								     const DHTKey &recipientKey,
								     const NetAddress &recipient,
								     const DHTKey &senderKey,
								     const NetAddress &sender,
								     const DHTKey &ownerKey,
								     const std::vector<Searchgroup*> &sgs,
								     const bool &sdiff)
     {
	l1::l1_query *l1r = l1_protob_wrapper::create_l1_query(fct_id,recipientKey,recipient,
							       senderKey,sender,ownerKey);
	l1r->SetExtension(l2::sg::diff,sdiff);
	l2::sg::r_searchgroups *rsgs = l1r->MutableExtension(l2::sg::rsg);
	l2_data_protob_wrapper::create_replicated_searchgroups(rsgs,sgs);
	return l1r;
     }
      
   l2::sg::r_searchgroups* l2_data_protob_wrapper::create_replicated_searchgroups(l2::sg::r_searchgroups *l2_rsgs,
										  const std::vector<Searchgroup*> &sgs)
     {
	size_t sgsize = sgs.size();
	for(size_t i=0;i<sgsize;i++)
	  {
	     Searchgroup *sg = sgs.at(i);
	     l2::sg::searchgroup *l2_sg = l2_rsgs->add_rsgs();
	     l2_sg = l2_data_protob_wrapper::create_l2_searchgroup(sg);
	  }
	return l2_rsgs;
     }
      
   void l2_data_protob_wrapper::serialize_to_string(const l1::l1_query *l1r, std::string &str)
     {
	if (!l1r)
	  {
	     errlog::log_error(LOG_LEVEL_DHT,"Can't serialize null replicated data");
	     throw dht_exception(DHT_ERR_MSG,"Can't serialize null replicated data");
	  }
	if (!l1r->SerializeToString(&str))
          throw dht_exception(DHT_ERR_MSG,"l1r::SerializeToString");
     }
   
   dht_err l2_data_protob_wrapper::read_l2_replication_query(const l1::l1_query *l1q,
							     uint32_t &fct_id,
							     DHTKey &recipientKey,
							     NetAddress &recipient,
							     DHTKey &senderKey,
							     NetAddress &sender,
							     DHTKey &ownerKey,
							     std::vector<Searchgroup*> &sgs,
							     bool &sdiff)
     {
	uint32_t layer_id;
	l1_protob_wrapper::read_l1_query(l1q,layer_id,fct_id,recipientKey,recipient,
					 senderKey,sender,ownerKey);
	sdiff = l1q->GetExtension(l2::sg::diff);
	l2::sg::r_searchgroups rsgs = l1q->GetExtension(l2::sg::rsg);
	l2_data_protob_wrapper::read_replicated_searchgroups(rsgs,sgs);
	return DHT_ERR_OK;
     }
   
   void l2_data_protob_wrapper::read_replicated_searchgroups(const l2::sg::r_searchgroups &rsgs,
							     std::vector<Searchgroup*> &sgs)
     {
	int nsgs = rsgs.rsgs_size();
	sgs.reserve(nsgs);
	for (int i=0;i<nsgs;i++)
	  {
	     l2::sg::searchgroup r = rsgs.rsgs(i);
	     Searchgroup *sg;
	     l2_data_protob_wrapper::read_l2_searchgroup(&r,sg);
	     sgs.push_back(sg);
	  }
     }

   void l2_data_protob_wrapper::deserialize_from_string(const std::string &str, l1::l1_query *l1q)
     {
	if (!l1q)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "can't deserialize to null response");
	     throw dht_exception(DHT_ERR_MSG,"can't deserialize to null response");
	  }
	if (!l1q->ParseFromString(str))
          throw dht_exception(DHT_ERR_MSG,"l1q::ParseFromString(" + str + ")");
     }
   
   
} /* end of namespace. */
