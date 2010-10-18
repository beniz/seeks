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
#include "dht_configuration.h"
#include "errlog.h"

using sp::errlog;

namespace dht
{
   sg_manager::sg_manager()
     {
	if (dht_configuration::_dht_config->_routing)
	  _sdb.open_db();
     
	//TODO: if replication activated, create replication dbs.
	
     }
   
   sg_manager::~sg_manager()
     {
	if (_sdb._opened)
	  _sdb.close_db();
     }
   
   void sg_manager::find_sg_range(const DHTKey &start_key,
				  const DHTKey &end_key,
				  hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey> &res)
     {
	/* look first in memory. */
	hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit
	  = _searchgroups.begin();
	while(hit!=_searchgroups.end())
	  {
	     if ((*hit).first->between(start_key,end_key))
	       {
		  Searchgroup *sg = new Searchgroup(*(*hit).second);
		  res.insert(std::pair<const DHTKey*,Searchgroup*>(&sg->_idkey,sg));
	       }
	     ++hit;
	  }
		
	/* then look in db. */
	_sdb.find_sg_range(start_key,end_key,res);
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
	
	if (sg_configuration::_sg_config->_db_sync_mode == 1) // full sync.
	  add_sg_db(sg);
	
	return sg;
     }
      
   bool sg_manager::add_sg_memory(Searchgroup *sg)
     {
	_searchgroups.insert(std::pair<const DHTKey*,Searchgroup*>(&sg->_idkey,sg));
	return true;
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
	
	if (sg_configuration::_sg_config->_db_sync_mode == 1) // full sync.
	  remove_sg_db(*sgkey);
	
	delete sg;
	return true;
     }
   
   Searchgroup* sg_manager::find_sg_db(const DHTKey &sgkey)
     {
	return _sdb.find_sg_db(sgkey);
     }
   
   bool sg_manager::remove_sg_db(const DHTKey &sgkey)
     {
	return _sdb.remove_sg_db(sgkey);
     }
   
   bool sg_manager::add_sg_db(Searchgroup *sg)
     {
	std::cerr << "add sg " << sg->_idkey << " to db\n";
	return _sdb.add_sg_db(sg);
     }
   
   bool sg_manager::move_to_db(Searchgroup *sg)
     {
	return add_sg_db(sg);
     }
      
   bool sg_manager::clear_sg_db()
     {
	return _sdb.clear_db();
     }
   
   Searchgroup* sg_manager::find_load_or_create_sg(const DHTKey *sgkey)
     {
	Searchgroup *sg = find_sg(sgkey);
	if (sg)
	  return sg;
	
	/* search group does not exist, create it. */
	sg = create_sg_memory(*sgkey);
	return sg;
     }
   
   Searchgroup* sg_manager::find_sg(const DHTKey *sgkey)
     {
	Searchgroup *sg = find_sg_memory(sgkey);
	if (sg)
	  return sg;
	
	/* search group not in memory, lookup in db. */
	sg = find_sg_db(*sgkey);
	if (sg)
	  {
	     // put it in memory cache.
	     add_sg_memory(sg);
	     return sg;
	  }
	return NULL;
     }
      
   bool sg_manager::sync()
     {
	hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey>::iterator hit
	  = _searchgroups.begin();
	while(hit!=_searchgroups.end())
	  {
	     std::cerr << "moving sg " << (*hit).second->_idkey << std::endl;
	     if (!move_to_db((*hit).second))
	       return false;
	     ++hit;
	  }
	errlog::log_error(LOG_LEVEL_DHT,"sg db sync successful (%u searchgroups)",
			  _sdb.number_records());
	return true;
     }

   bool sg_manager::replication_decrement_all_sgs(const DHTKey &host_key)
     {
	hash_map<const DHTKey*,sg_db*,hash<const DHTKey*>,eqdhtkey>::iterator hit;
	if ((hit=_replicated_sgs.find(&host_key))!=_replicated_sgs.end())
	  {
	     sg_db *sdb = (*hit).second;
	     
	     // decrement replication levels and extract searchgroup
	     // to be put under control of this virtual node.
	     std::vector<Searchgroup*> nsgs;
	     sdb->decrement_replication_level_all_sgs(nsgs);
	     
	     // move those search group to the local sg db.
	     for (size_t i=0;i<nsgs.size();i++)
	       {
		  Searchgroup *sg = nsgs.at(i);
		  int dberr = move_to_db(sg);
		  //TODO: check on error.
		  
		  // remove those sgs from the replicated db.
	       	  dberr = sdb->remove_sg_db(sg->_idkey);
		  //TODO: check on error.
		  
		  delete sg;
	       }
	     return true;
	  }
	else
	  {
	     //TODO: error.
	     return false;
	  }
     }
         
   bool sg_manager::increment_replicated_sgs(const DHTKey &host_key,
					     hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey> &h_sgs)
     {
	// fetch replicated db.
	sg_db *sdb = NULL;
	hash_map<const DHTKey*,sg_db*,hash<const DHTKey*>,eqdhtkey>::iterator sit;
	if ((sit=_replicated_sgs.find(&host_key))!=_replicated_sgs.end())
	  {
	     sdb = (*sit).second;
	  }
	else
	  {
	     //TODO: error.
	     return false;
	  }
	
	// add searchgroups to replicated db.
	hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey>::iterator hit
	  = h_sgs.begin();
	while(hit!=h_sgs.end())
	  {
	     Searchgroup *sg = (*hit).second;
	     sg->_replication_level++; // increment replication level.
	     int dberr = sdb->add_sg_db(sg);
	     //TODO: check on error.
	     
	     // remove sgs from local db of searchgroups.
	     dberr = remove_sg_db(sg->_idkey);
	     //TODO: check on error.
	     
	     delete sg;
	     
	     ++hit;
	  }
	
	h_sgs.clear();
	return true;
     }
      
} /* end of namespace. */
