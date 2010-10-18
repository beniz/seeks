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
  
#ifndef SGNODE_H
#define SGNODE_H

#include "DHTNode.h"
#include "sg_manager.h"
#include "sg_configuration.h"

namespace dht
{
   
   class SGNode : public DHTNode
     {
      public:
	SGNode(const char *net_addr, const short &net_port);
		
	~SGNode();
	
	virtual DHTVirtualNode* create_vnode();
	virtual DHTVirtualNode* create_vnode(const DHTKey &idkey,
					     LocationTable *lt);
	
	virtual void init_server();

	virtual void reset_vnodes_dependent();
	
	/*- RPC callbacks. -*/
	dht_err RPC_subscribe_cb(const DHTKey &recipientKey,
				 const NetAddress &recipient,
				 const DHTKey &senderKey,
				 const NetAddress &sender,
				 const DHTKey &sgKey,
				 std::vector<Subscriber*> &peers,
				 int &status);
	
	dht_err RPC_replicate_cb(const DHTKey &recipientKey,
				 const NetAddress &recipient,
				 const DHTKey &senderKey,
				 const NetAddress &sender,
				 const DHTKey &ownerKey,
				 const std::vector<Searchgroup*> &sgs,
				 const bool &sdiff,
				 int &status);
	
      public:
	sg_manager _sgmanager;
	
	static std::string _sg_config_filename;
     };
   
} /* end of namespace. */

#endif
