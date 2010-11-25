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

#ifndef L2_DATA_PROTOB_WRAPPER_H
#define L2_DATA_PROTOB_WRAPPER_H

#include "dht_err.h"
#include "dht_exception.h"
#include "dht_layer1.pb.h"
#include "dht_layer2_data.pb.h"
#include "searchgroup.h"

#include <vector>

namespace dht
{
   
   class l2_data_protob_wrapper
     {
      public:
	/* search groups. */
	static l2::sg::searchgroup* create_l2_searchgroup(const Searchgroup *sg);
	
	static void serialize_to_string(const l2::sg::searchgroup *l2_sg, std::string &str);
	
	static void read_l2_searchgroup(const l2::sg::searchgroup *l2_sg, Searchgroup *&sg);
	
	static void deserialize_from_string(const std::string &str, l2::sg::searchgroup *l2_sg);
	
	/* messages. */
	static l1::l1_query* create_l2_replication_query(const uint32_t &fct_id,
							 const DHTKey &recipientKey,
							 const NetAddress &recipient,
							 const DHTKey &senderKey,
							 const NetAddress &sender,
							 const DHTKey &ownerKey,
							 const std::vector<Searchgroup*> &sgs,
							 const bool &sdiff=0);
	
	static l2::sg::r_searchgroups* create_replicated_searchgroups(l2::sg::r_searchgroups *l2_rsgs,
								      const std::vector<Searchgroup*> &sgs);
	
	static void serialize_to_string(const l1::l1_query *l1r, std::string &str);
	
	static void read_l2_replication_query(const l1::l1_query *l1q,
						 uint32_t &fct_id,
						 DHTKey &recipientKey,
						 NetAddress &recipient,
						 DHTKey &senderKey,
						 NetAddress &sender,
						 DHTKey &ownerKey,
						 std::vector<Searchgroup*> &sgs,
						 bool &sdiff);
	
	static void read_replicated_searchgroups(const l2::sg::r_searchgroups &rsgs,
						 std::vector<Searchgroup*> &sgs);
	
	static void deserialize_from_string(const std::string &str, l1::l1_query *l1q);	
     };
      
} /* end of namespace. */

#endif
