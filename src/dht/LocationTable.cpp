/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
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

#include "LocationTable.h"
#include "DHTNode.h"
#include "errlog.h"

#include <assert.h>

using sp::errlog;

namespace dht
{
   LocationTable::LocationTable()
     {	
	// initialize mutex.
	mutex_init(&_lt_mutex);
     }

   LocationTable::~LocationTable()
     {
       mutex_lock(&_lt_mutex);
	// free the memory.
	hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::iterator lit
	  = _hlt.begin();
	while(lit!=_hlt.end())
	  {
            Location *loc = (*lit).second;
             ++lit;
             delete loc;
	  }
        mutex_unlock(&_lt_mutex);
     }
      
   bool LocationTable::is_empty() const
     {
	return _hlt.empty();
     }
      
   Location* LocationTable::findLocation(const DHTKey& dk)
     {
	hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit;
	mutex_lock(&_lt_mutex);
	if ((hit = _hlt.find(&dk)) != _hlt.end())
	  {
	     mutex_unlock(&_lt_mutex);
	     return (*hit).second;
	  }
	else {
	   mutex_unlock(&_lt_mutex);
	   //errlog::log_error(LOG_LEVEL_DHT, "findLocation: can't find location to key %s", dk.to_rstring().c_str());
	   return NULL;  //beware: this should be handled outside this function.	
	}
     }
   
   void LocationTable::addToLocationTable(Location *&loc)
     {
	hash_map<const DHTKey*,Location*,hash<const DHTKey*>,eqdhtkey>::iterator hit;
	if ((hit=_hlt.find(&loc->getDHTKeyRef()))!=_hlt.end())
	  {
	     if (loc)
	       delete loc;
	     loc = (*hit).second;
	     return;
	  }
	mutex_lock(&_lt_mutex);
	_hlt.insert(std::pair<const DHTKey*,Location*>(&loc->getDHTKeyRef(),loc));
	mutex_unlock(&_lt_mutex);
     }
   
   // DEPRECATED ?
   void LocationTable::addToLocationTable(const DHTKey& key, const NetAddress& na, 
					  Location *&loc)
     {
	loc = new Location(key, na);
	addToLocationTable(loc);
     }

   Location* LocationTable::addOrFindToLocationTable(const DHTKey& dk, const NetAddress& na)
     {
	hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::iterator hit;
	if ((hit = _hlt.find(&dk)) != _hlt.end())
	  {
	     /**
	      * update the location.
	      */
	     mutex_lock(&_lt_mutex);
	     (*hit).second->update(na);
	     mutex_unlock(&_lt_mutex);
	     return (*hit).second;
	  }
	else 
	  {
	     Location* loc = new Location(dk, na);
	     _hlt.insert(std::pair<const DHTKey*,Location*>(&loc->getDHTKeyRef(),loc));
	     return loc;
	  }	 
     }
   
   void LocationTable::removeLocation(Location *loc)
     {
	mutex_lock(&_lt_mutex);
	hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::iterator hit;
	if ((hit = _hlt.find(&loc->getDHTKeyRef())) != _hlt.end())
	  {
	     Location *loc = (*hit).second;
	     _hlt.erase(hit);
	     delete loc;
	  }
	else 
	  {
	     errlog::log_error(LOG_LEVEL_DHT, "removeLocation: can't find location to remove with key %s", loc->getDHTKey().to_rstring().c_str());
	  }
	mutex_unlock(&_lt_mutex);
     }
      
   void LocationTable::findClosestSuccessor(const DHTKey &dk,
					    DHTKey &dkres)
     {
	DHTKey min_diff;
	min_diff.set();
	Location *succ_loc = NULL;
	
	mutex_lock(&_lt_mutex);
	Location *loc = NULL;
	hash_map<const DHTKey*,Location*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit
	  = _hlt.begin();
	while(hit!=_hlt.end())
	  {
	     loc = (*hit).second;
	     hit++;
	     if (!loc)
	       {
		  continue;
	       }
	     
	     DHTKey lkey = loc->getDHTKey();
	     DHTKey diff = lkey - dk;
	     
	     if (diff.count() == 0)
	       {
		  continue;
	       }
	     	     
	     if (diff < min_diff)
	       {
		  min_diff = diff;
		  succ_loc = loc;
	       }
	  }
	mutex_unlock(&_lt_mutex);
     
	//debug
	assert(succ_loc!=NULL);
	//debug
	
	dkres = succ_loc->getDHTKey();
     }

   bool LocationTable::has_only_virtual_nodes(DHTNode *pnode) const
     {
	hash_map<const DHTKey*,Location*,hash<const DHTKey*>,eqdhtkey>::const_iterator
	  hit = _hlt.begin();
	while(hit!=_hlt.end())
	  {
	     DHTKey lkey = (*hit).second->getDHTKey();
	     if (!pnode->findVNode(lkey))
	       return false;
	     ++hit;
	  }
	return true;
     }
      
   void LocationTable::print(std::ostream &out) const 
     {
	out << "location table:\n";
	hash_map<const DHTKey*,Location*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit
	  = _hlt.begin();
	while(hit!=_hlt.end())
	  {
	     Location *loc = (*hit).second;
	     out << loc->getDHTKey() << " -- ";
	     loc->getNetAddress().print(out);
	     out << std::endl;
	     ++hit;
	  }
     }
      
} /* end of namespace. */
