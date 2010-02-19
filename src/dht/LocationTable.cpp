/**
 * The Locality Sensitive Distributed Hashtable (LSH-DHT) library is
 * part of the SEEKS project and does provide the components of
 * a peer-to-peer pattern matching overlay network.
 * Copyright (C) 2006 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "LocationTable.h"

namespace dht
{
   LocationTable::LocationTable()
     {	
     }
   
   Location* LocationTable::findLocation(const DHTKey& dk)
     {
	hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit;
	if ((hit = _hlt.find(&dk)) != _hlt.end())
	  {
	     return (*hit).second;
	  }
	else {
	   std::cout << "[Debug]:LocationTable::findLocation: can't find location to key: "
	     << dk << std::endl;
	   return NULL;  //beware: this should be handled outside this function.	
	}
     }
   
   void LocationTable::addToLocationTable(Location* loc)
     {
	hash_map<const DHTKey*,Location*,hash<const DHTKey*>,eqdhtkey>::iterator hit;
	if ((hit=_hlt.find(&loc->getDHTKeyRef()))!=_hlt.end())
	  delete (*hit).second;
	_hlt.insert(std::pair<const DHTKey*,Location*>(&loc->getDHTKeyRef(),loc));
     }
   
   void LocationTable::addToLocationTable(const DHTKey& key, const NetAddress& na, 
					  Location* loc)
     {
	loc = new Location(key, na);
	addToLocationTable(loc);
     
	/**
	 * TODO: table cleanup.
	 */
     }

   Location* LocationTable::addOrFindToLocationTable(const DHTKey& dk, const NetAddress& na)
     {
	hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit;
	if ((hit = _hlt.find(&dk)) != _hlt.end())
	  {
	     /**
	      * update the location.
	      * TODO: this should be based on proximity analysis.
	      */
	     (*hit).second->update(na);
	     return (*hit).second;
	  }
	else 
	  {
	     Location* loc = NULL;
	     addToLocationTable(dk, na, loc);
	     return loc;
	  }	 
     }
   
   
} /* end of namespace. */
