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

#ifndef L1_PROTOB_WRAPPER_H
#define L1_PROTOB_WRAPPER_H

#include "dht_layer1.pb.h"
#include "dht_err.h"
#include "dht_exception.h"
#include "DHTKey.h"
#include "NetAddress.h"

#include <string>
#include <stdint.h>
#include <list>

namespace dht
{
   class l1_protob_wrapper
     {
	/**
	 * From data to protobuffers.
	 */
	
      public:
	static l1::l1_query* create_l1_query(const uint32_t &fct_id,
					     const DHTKey &recipient_dhtkey,
					     const NetAddress &recipient_na);
	
	static l1::l1_query* create_l1_query(const uint32_t &fct_id,
					     const DHTKey &recipient_dhtkey,
					     const NetAddress &recipient_na,
					     const DHTKey &sender_dhtkey,
					     const NetAddress &sender_na);
	
	static l1::l1_query* create_l1_query(const uint32_t &fct_id,
					     const DHTKey &recipient_dhtkey,
					     const NetAddress &recipient_na,
					     const DHTKey &node_key);
	
      private:
	static l1::l1_query* create_l1_query(const uint32_t &fct_id,
					     const std::string &recipient_key,
					     const std::string &recipient_ip_addr,
					     const short &recipient_net_port);
	
	static l1::l1_query* create_l1_query(const uint32_t &fct_id,
					     const std::string &recipient_key,
					     const std::string &recipient_ip_addr,
					     const short &recipient_net_port,
					     const std::string &sender_key,
					     const std::string &sender_ip_addr,
					     const short &sender_net_port);
      public:
	static void serialize_to_string(const l1::l1_query *l1q, std::string &str);
	
	// responses.
      public:
	static l1::l1_response* create_l1_response(const uint32_t &error_status,
						   const DHTKey &resultKey,
						   const NetAddress &resultAddress,
						   const DHTKey &foundKey,
						   const NetAddress &foundAddress);
	
	static l1::l1_response* create_l1_response(const uint32_t &error_status,
						   const DHTKey &resultKey,
						   const NetAddress &resultAddress);
	
	static l1::l1_response* create_l1_response(const uint32_t &error_status,
						   const std::list<DHTKey> &resultKeyList,
						   const std::list<NetAddress> &resultAddressList);
	static l1::l1_response* create_l1_response(const uint32_t &error_status);
						   
      private:
	static l1::l1_response* create_l1_response(const uint32_t &error_status,
						   const std::list<std::string> &result_key_list,
						   const std::list<std::pair<std::string,uint32_t> > &result_address_list);
	
	static l1::l1_response* create_l1_response(const uint32_t &error_status,
						   const std::string &result_key,
						   const std::string &result_ip_addr,
						   const short &result_net_port);
	
	static l1::l1_response* create_l1_response(const uint32_t &error_status,
						   const std::string &result_key,
						   const std::string &result_ip_addr,
						   const short &result_net_port,
						   const std::string &found_key,
						   const std::string &found_ip_addr,
						   const short &found_net_port);
	
      public:
	static void serialize_to_string(const l1::l1_response *l1r, std::string &str);
	
	/**
	 * From protobuffers to data.
	 */	
      public:
	static dht_err read_l1_query(const l1::l1_query *l1q,
				     uint32_t &layer_id,
				     uint32_t &fct_id,
				     DHTKey &recipient_dhtkey,
				     NetAddress &recipient_na,
				     DHTKey &sender_dhtkey,
				     NetAddress &sender_na);
	
	static dht_err read_l1_query(const l1::l1_query *l1q,
				     uint32_t &layer_id,
				     uint32_t &fct_id,
				     DHTKey &recipient_dhtkey,
				     NetAddress &recipient_na,
				     DHTKey &sender_dhtkey,
				     NetAddress &sender_na,
				     DHTKey &nodekey);
      
      private:
	static dht_err read_l1_query(const l1::l1_query *l1q,
				     uint32_t &layer_id,
				     uint32_t &fct_id,
				     std::string &recipient_key,
				     std::string &recipient_ip_addr,
				     uint32_t &recipient_net_port,
				     std::string &sender_key,
				     std::string &sender_ip_addr,
				     uint32_t &sender_net_port);
      public:
	static void deserialize(const std::string &str, l1::l1_query *l1q);
     
	// responses.
      public:
	static dht_err read_l1_response(const l1::l1_response *l1r,
					uint32_t &error_status,
					DHTKey &resultKey,
					NetAddress &resultAddress,
					DHTKey &foundKey,
					NetAddress &foundAddress);
	
	static dht_err read_l1_response(const l1::l1_response *l1r,
					uint32_t &error_status,
					DHTKey &resultKey,
					NetAddress &resultAddress);
	
	static dht_err read_l1_response(const l1::l1_response *l1r,
					uint32_t &error_status,
					std::list<DHTKey> &dkres_list,
					std::list<NetAddress> &na_list);
	
      private:
	static dht_err read_l1_response(const l1::l1_response *l1r,
					uint32_t &error_status,
					std::list<std::string> &result_key_list,
					std::list<std::pair<std::string,uint32_t> > &result_address_list);
	
	static dht_err read_l1_response(const l1::l1_response *l1r,
					uint32_t &error_status,
					std::string &result_key,
					std::string &result_ip_addr,
					uint32_t &result_net_port,
					std::string &found_key,
					std::string &found_ip_addr,
					uint32_t &found_net_port);
   
	static dht_err read_l1_response(const l1::l1_response *l1r,
					uint32_t &error_status,
					std::string &result_key,
					std::string &result_ip_addr,
					uint32_t &result_net_port);
	
      public:
	static dht_err read_l1_response(const l1::l1_response *l1r,
					uint32_t &error_status);
	
      public:
	static void deserialize(const std::string &str, l1::l1_response *l1r);
     };
   
   /*- exceptions. -*/
   class l1_fail_serialize_exception : public dht_exception
     {
      public:
	l1_fail_serialize_exception()
	  :dht_exception()
	    {
	       _message = "failed serialization of l1 message";
	    };
	virtual ~l1_fail_serialize_exception() {};
     };
   
   class l1_fail_deserialize_exception : public dht_exception
     {
      public:
	l1_fail_deserialize_exception()
	  :dht_exception()
	    {
	       _message = "failed deserialization of l1 message";
	    };
	virtual ~l1_fail_deserialize_exception() {};
     };
      
} /* end of namespace. */

#endif
