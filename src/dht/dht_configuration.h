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

#ifndef DHT_CONFIGURATION_H
#define DHT_CONFIGURATION_H

#include "configuration_spec.h"
#include "NetAddress.h"

using sp::configuration_spec;

namespace dht
{
   class dht_configuration : public configuration_spec
     {
      public:
	dht_configuration(const std::string &filename);
	
	~dht_configuration();
	
	// virtual
	virtual void set_default_config();
	
	virtual void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
				       char *buf, const unsigned long &linenum);
	
	virtual void finalize_configuration();
	
	// main options.
	short _nvnodes; /**< number of virtual nodes supported by this DHT node. */
	short _l1_port; /**< listening port for level 1 communications. */
	int _l1_server_max_msg_bytes; /**< maximum size of UDP datagrams served on layer 1. */
	int _l1_client_timeout; /**< l1 client communication timeout. */
	std::vector<NetAddress> _bootstrap_nodelist; /**< list of bootstrap nodes. */
	int _max_hops; /**< max number of hops in finding a route around the circle. */
	int _succlist_size; /**< max number of elements in successor list. */
	bool _routing; /**< whether routing is activated, i.e. our nodes are active or spectators. */
     };
   
} /* end of namespace. */

#endif
