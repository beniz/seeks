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
 
#ifndef SG_MANAGER_H
#define SG_MANAGER_H

#include "searchgroup.h"
#include "sg_db.h"
#include "sg_sweeper.h"

namespace dht
{
   
   class sg_manager
     {
      public:
	sg_manager();
	
	~sg_manager();
	
	/* access. */
	void find_sg_range(const DHTKey &start_key,
			   const DHTKey &end_key,
			   hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey> &res);
	
	/* memory management. */
	Searchgroup* find_sg(const DHTKey *sgkey);
	
	Searchgroup* find_sg_memory(const DHTKey *sgkey);
	
	Searchgroup* create_sg_memory(const DHTKey &sgkey);
	
	bool add_sg_memory(Searchgroup *sg);
	
	bool remove_sg_memory(const DHTKey *sgkey);
	
	/* db management. */
	Searchgroup* find_sg_db(const DHTKey &sgkey);
	
	bool remove_sg_db(const DHTKey &sgkey);
	
	bool add_sg_db(Searchgroup *sg);
	
	bool move_to_db(Searchgroup *sg);
	
	bool clear_sg_db();
	
	/* general management. */
	Searchgroup* find_load_or_create_sg(const DHTKey *sgkey);
	
	bool sync();
	
	/* replication. */
	/**
	 * \brief decrements all searchgroups replication level for
	 * the host vnode and move nodes with level 0 to this vnode
	 * db of searchgroups.
	 */
	bool replication_decrement_all_sgs(const DHTKey &host_key);
	
	/**
	 * \brief increments all searchgroups replication for the host vnode.
	 * Searchgroups in h_sgs are those passed to our prededecessor,
	 * that were local and must be moved to replicated db.
	 */
	//TODO.
	bool increment_replicated_sgs(const DHTKey &host_key,
				      hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey> &h_sgs);
	
      public:
	hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey> _searchgroups;
        std::vector<const DHTKey*> _sorted_sg_keys;
		
	// search group sweeper.
	sg_sweeper _sgsw;
	
	// db of sgs.
	sg_db _sdb;
	
	// dbs of replicateds sgs, one db per vnode.
     	hash_map<const DHTKey*,sg_db*,hash<const DHTKey*>,eqdhtkey> _replicated_sgs;
     };
   
} /* end of namespace. */

#endif
  
