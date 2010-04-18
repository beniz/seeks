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
#include "errlog.h"

using sp::errlog;

namespace dht
{
   LocationTable::LocationTable()
     {	
	// initialize mutex.
	seeks_proxy::mutex_init(&_lt_mutex);
     }

   LocationTable::~LocationTable()
     {
	// free the memory.
	hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::iterator lit
	  = _hlt.begin();
	while(lit!=_hlt.end())
	  {
	     delete (*lit).second;
	     ++lit;
	  }
     }
      
   bool LocationTable::is_empty() const
     {
	return _hlt.empty();
     }
      
   Location* LocationTable::findLocation(const DHTKey& dk)
     {
	hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit;
	if ((hit = _hlt.find(&dk)) != _hlt.end())
	  {
	     return (*hit).second;
	  }
	else {
	   errlog::log_error(LOG_LEVEL_DHT, "findLocation: can't find location to key %s", dk.to_string().c_str());
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
	seeks_proxy::mutex_lock(&_lt_mutex);
	_hlt.insert(std::pair<const DHTKey*,Location*>(&loc->getDHTKeyRef(),loc));
	seeks_proxy::mutex_unlock(&_lt_mutex);
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
	hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit;
	if ((hit = _hlt.find(&dk)) != _hlt.end())
	  {
	     /**
	      * update the location.
	      */
	     (*hit).second->update(na);
	     return (*hit).second;
	  }
	else 
	  {
	     Location* loc = new Location(dk, na);
	     _hlt.insert(std::pair<const DHTKey*,Location*>(&loc->getDHTKeyRef(),loc));
	     //addToLocationTable(dk, na, loc);
	     return loc;
	  }	 
     }
   
   void LocationTable::removeLocation(Location *loc)
     {
	hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit;
	if ((hit = _hlt.find(&loc->getDHTKeyRef())) != _hlt.end())
	  {
	     delete (*hit).second;
	  }
	else 
	  {
	     errlog::log_error(LOG_LEVEL_DHT, "removeLocation: can't find location to remove with key %s", loc->getDHTKey().to_string().c_str());
	  }
     }
      
} /* end of namespace. */
