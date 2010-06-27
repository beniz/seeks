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

#include "sg_configuration.h"
#include "miscutil.h"
#include "errlog.h"

using sp::errlog;
using sp::miscutil;

namespace dht
{
#define hash_sg_mem_cap                  3585265731ul  /* "sg-mem-cap" */
#define hash_sg_mem_delay                2777464586ul  /* "sg-mem-delay" */
#define hash_sg_dp_cap                   1951343971ul  /* "sg-db-cap" */
#define hash_sg_db_delay                 1236646847ul  /* "sg-db-delay" */
#define hash_db_sync_mode                 225381993ul  /* "db-sync-mode" */
#define hash_db_sync_delay               2907431392ul  /* "db-sync-delay" */
#define hash_max_returned_peers          3096475189ul  /* "max-returned-peers" */
   
   sg_configuration* sg_configuration::_sg_config = NULL;
   
   sg_configuration::sg_configuration(const std::string &filename)
     :configuration_spec(filename)
       {
	  errlog::log_error(LOG_LEVEL_DHT, "reading searchgroup configuration file %s", filename.c_str());
	  load_config();
	  sg_configuration::_sg_config = this;
       }
   
   sg_configuration::~sg_configuration()
     {
     }
   
   void sg_configuration::set_default_config()
     {
	_sg_mem_cap = 104857600; // 100Mo.
	_sg_mem_delay = 1800; // 30 mins.
	_sg_db_cap = 104857600; // 100Mo.
	_sg_db_delay = 864000; // 10 days.
	_db_sync_mode = 0; // time-delay sync.
	_db_sync_delay = 300; // 5 mins.
	_max_returned_peers = 20; // 20 peers.
     }
     
   void sg_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
					    char *buf, const unsigned long &linenum)
     {
	switch(cmd_hash)
	  {
	   case hash_sg_mem_cap:
	     _sg_mem_cap = strtod(arg,NULL);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the cap for in-memory retention of search groups.");
	     break;
	   
	   case hash_sg_mem_delay:
	     _sg_mem_delay = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the delay before a search group is removed from memory.");
	     break;
	     
	   case hash_sg_dp_cap:
	     _sg_db_cap = strtod(arg,NULL);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the cap for disk retention of search groups.");
	     break;
	     
	   case hash_sg_db_delay:
	     _sg_db_delay = strtod(arg,NULL);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the delay before a search group is removed from disk.");
	     break;
	     
	   case hash_db_sync_mode:
	     _db_sync_mode = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the sync mode of search group retention memory and disk.");
	     break;
	     
	   case hash_db_sync_delay:
	     _db_sync_delay = strtod(arg,NULL);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the sync delay between the search group retention memory and disk.");
	     break;
	     
	   case hash_max_returned_peers:
	     _max_returned_peers = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the maximum number of peers returned to a searchgroup request.");
	     break;
	     
	   default:
	     break;
	  }
     }
        
   void sg_configuration::finalize_configuration()
     {
     }
      
} /* end of namespace. */
