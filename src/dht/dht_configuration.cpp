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

namespace dht
{
#define hash_l1_port                                      2857811716ul  /* "l1-port" */
#define hash_l1_server_max_msg_bytes                      1170059721ul  /* "l1-server-max-msg-bytes" */
#define hash_l1_client_timeout                            1673776173ul  /* "l1-client-timeout" */
   
   dht_configuration::dht_configuration(const std::string &filename)
     :configuration_spec(filename)
       {
	  load_config();
       }
   
   dht_configuration::~dht_configuration()
     {
     }
   
   void dht_configuration::set_default_config()
     {
	_l1_port = 8231;  // the 8200 range by default.
	_l1_server_max_msg_bytes = 8192; // 1Kb for now.
	_l1_client_timeout = 5; // 5 seconds.
     }
   
   void dht_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
					     char *buf, const unsigned long &linenum)
     {
	switch(cmd_hash)
	  {
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
	     
	   default:
	     break;
	     
	  }
     }

   void dht_configuration::finalize_configuration()
     {
     }
   
} /* end of namespace. */
