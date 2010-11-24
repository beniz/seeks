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

#include "dht_configuration.h"
#include "miscutil.h"
#include "errlog.h"

using sp::errlog;
using sp::miscutil;

namespace dht
{
#define hash_number_vnodes                                 103843901ul  /* "number-vnodes" */
#define hash_l1_port                                      2857811716ul  /* "l1-port" */
#define hash_l1_server_max_msg_bytes                      1170059721ul  /* "l1-server-max-msg-bytes" */
#define hash_l1_client_timeout                            1673776173ul  /* "l1-client-timeout" */
#define hash_bootstrap_node                               1741523628ul  /* "bootstrap-node" */
#define hash_max_hops                                     1040979360ul  /* "max-hops" */
#define hash_succlist_size                                2998048309ul  /* "succlist-size" */
#define hash_routing                                      2057335831ul  /* "routing" */
#define hash_rejoin_timeout                               2877119161ul  /* "rejoin-timeout" */
#define hash_replication_factor                           1408115358ul  /* "replication-factor" */
   
   dht_configuration* dht_configuration::_dht_config = NULL;
   
   dht_configuration::dht_configuration(const std::string &filename)
     :configuration_spec(filename)
       {
	  errlog::log_error(LOG_LEVEL_DHT, "reading DHT configuration file %s", filename.c_str());
	  if (dht_configuration::_dht_config == NULL)
	    dht_configuration::_dht_config = this;
	  load_config();
       }
   
   dht_configuration::~dht_configuration()
     {
     }
   
   void dht_configuration::set_default_config()
     {
	_nvnodes = 32; // 32 vnodes by default.
	_l1_port = 8231;  // the 8200 range by default.
	_l1_server_max_msg_bytes = 1024; // 1Kb for now.
	_l1_client_timeout = 5; // 5 seconds.
	_max_hops = 12; // 12 hops ~ l(100000).
	_succlist_size = 10; // XXX: in the future, should be reset dynamically w.r.t. estimated number of nodes on the ring.
	_routing = true; // XXX: in stable releases, routing will be set to false.
	_rejoin_timeout = 30; // periodic rejoin check every 30 seconds after being cut from the network.
	_replication_factor = 2; // up to _succlist_size.
     }
   
   void dht_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
					     char *buf, const unsigned long &linenum)
     {
	int vec_count;
	char *vec[3];
	char tmp[BUFFER_SIZE];
	std::string bootstrap_na;
	int bootstrap_port;
	NetAddress na;
	
	switch(cmd_hash)
	  {
	   case hash_number_vnodes:
	     _nvnodes = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the number of virtual nodes supported by this DHT node.");
	     break;
	     
	   case hash_l1_port:
	     _l1_port = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the port for the DHT level 1 communications.");
	     break;
	     
	   case hash_l1_server_max_msg_bytes:
	     _l1_server_max_msg_bytes = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the maximum length of a served message on the layer 1 of the DHT.");
	     
	   case hash_l1_client_timeout:
	     _l1_client_timeout = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the communication timeout for the client on the layer 1 of the DHT.");
	     break;
	     
	   case hash_bootstrap_node:
	     strlcpy(tmp, arg, sizeof(tmp));
	     vec_count = miscutil::ssplit(tmp, " \t", vec, SZ(vec), 1, 1);
	     
	     if ((vec_count < 2))
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for "
				    "bootstrap-node directive in DHT configuration file.");
		  break;
	       }
	     
	     bootstrap_na = std::string(vec[0]);
	     bootstrap_port = atoi(vec[1]);
	     na = NetAddress(bootstrap_na,bootstrap_port);
	     
	     // TODO: test address.
	     
	     _bootstrap_nodelist.push_back(na);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets a bootstrap node for the DHT.");
	     break;
	     
	   case hash_max_hops:
	     _max_hops = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the maximum number of hops when finding a route around the circle");
	     break;
	     
	   case hash_succlist_size:
	     _succlist_size = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the maximum size of the successor list.");
	     break;
	     
	   case hash_routing:
	     _routing = static_cast<bool>(atoi(arg));
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Whether all virtual nodes are active or spectators (safe with TOR)");
	     break;
	     
	   case hash_rejoin_timeout:
	     _rejoin_timeout = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Timeout between two attempts to rejoin the circle of DHT nodes");
	     break;
	     
	   case hash_replication_factor:
	     _replication_factor = atoi(arg);
	     if (_replication_factor > _succlist_size)
	       errlog::log_error(LOG_LEVEL_INFO,"Replication factor cannot be above succlist size, forced to succlist size");
	     _replication_factor = std::min(static_cast<int>(_replication_factor),_succlist_size);
	     break;
	     
	   default:
	     break;
	     
	  }
     }

   void dht_configuration::finalize_configuration()
     {
     }
   
} /* end of namespace. */
