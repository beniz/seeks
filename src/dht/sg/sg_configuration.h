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

#ifndef SG_CONFIGURATION_H
#define SG_CONFIGURATION_H

#include "configuration_spec.h"

using sp::configuration_spec;

namespace dht
{
   class sg_configuration : public configuration_spec
     {
      public:
	sg_configuration(const std::string &filename);
	
	~sg_configuration();
	
	// virtual
	virtual void set_default_config();
     
	virtual  void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
					char *buf, const unsigned long &linenum);
	
	virtual void finalize_configuration();
     
	// main options.

	// memory cache cap.
	double _sg_mem_cap;
	
	// delay before deletion from memory, in seconds.
	double _sg_mem_delay;
	
	// disk size cap.
	double _sg_db_cap;
	
	// delay for removal from disk.
	double _sg_db_delay;
	
	// sync mode memory/disk: 0 for time-delay synced, 1 for full-sync.
	int _db_sync_mode;
     
	// one configuration across all objects that need access to it.
	static sg_configuration *_sg_config;
     };
      
} /* end of namespace. */

#endif
