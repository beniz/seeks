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
     
	/* memory management. */
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
	void replication_decrement_all_sgs_between(const DHTKey &start_key, const DHTKey &end_key); //TODO: with db.
	
      public:
	hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey> _searchgroups;
        std::vector<const DHTKey*> _sorted_sg_keys;
	
	//TODO: in db only.
	/* hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey> _replicated_searchgroups;
	 std::vector<const DHTKey*> _sorted_replicated_sg_keys; */
	
	
	// search group sweeper.
	sg_sweeper _sgsw;
	
	//TODO: db of sgs.
	sg_db _sdb;
	
	//TODO: db of vnodes/sg keys.
     };
   
} /* end of namespace. */

#endif
  
