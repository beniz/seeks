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

#include "sg_manager.h"

namespace dht
{
   sg_manager::sg_manager()
     {
     }
   
   sg_manager::~sg_manager()
     {
     }
   
   Searchgroup* sg_manager::find_sg_memory(const DHTKey *sgkey)
     {
	hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit;
	if ((hit=_searchgroups.find(sgkey))!=_searchgroups.end())
	  return (*hit).second;
	return NULL;
     }
   
   Searchgroup* sg_manager::create_sg_memory(const DHTKey &sgkey)
     {
	Searchgroup *sg = new Searchgroup(sgkey);
	_searchgroups.insert(std::pair<const DHTKey*,Searchgroup*>(&sg->_idkey,sg));
	return sg;
     }
      
   bool sg_manager::add_sg_memory(Searchgroup *sg)
     {
	//TODO.
     }
      
   bool sg_manager::remove_sg_memory(const DHTKey *sgkey)
     {
	hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey>::iterator hit;
	if ((hit=_searchgroups.find(sgkey))==_searchgroups.end())
	  return false;
	Searchgroup *sg = (*hit).second;
	if (sg->_lock)
	  return false;
	_searchgroups.erase(hit);
	delete sg;
	return true;
     }
   
   Searchgroup* sg_manager::find_sg_db(const DHTKey &sgkey)
     {
	//TODO: find search group in db, and recreate it if found.
	return NULL;
     }
   
   bool sg_manager::remove_sg_db(const DHTKey &sgkey)
     {
	
     }
   
   bool sg_manager::add_sg_db(Searchgroup *sg)
     {
	
     }
      
   Searchgroup* sg_manager::find_load_or_create_sg(const DHTKey *sgkey)
     {
	Searchgroup *sg = find_sg_memory(sgkey);
	if (sg)
	  return sg;
	
	/* search group not in memory, lookup in db. */
	sg = find_sg_db(*sgkey); //TODO.
	if (sg)
	  {
	     //TODO: put it in memory cache.
	     bool was_added = add_sg_memory(sg);
	     if (was_added)
	       return sg;
	     else 
	       {
		  //TODO.
	       }
	  }
	
	/* search group does not exist, create it. */
	sg = create_sg_memory(*sgkey);
	return sg;
     }
      
} /* end of namespace. */
