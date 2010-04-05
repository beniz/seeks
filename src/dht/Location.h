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

#ifndef LOCATION_H
#define LOCATION_H

#include "DHTKey.h"
#include "NetAddress.h"

namespace dht
{
   class Location
     {
      public:
	Location(const DHTKey& key, const NetAddress& na);
	
	~Location();
	
	/**
	 * accessors.
	 */
	DHTKey getDHTKey() const { return _key; }
	DHTKey& getDHTKeyRef() { return _key; };
	void setDHTKey(const DHTKey &dk) { _key = dk; }
	NetAddress getNetAddress() const { return _na; }
	void setNetAddress(const NetAddress& na) { _na = na; }
	
	/**
	 * /brief updates the net address of this location.
	 * @param na net address.
	 * TODO: update based on peer proximity analysis.
	 */
	void update(const NetAddress& na);
	
      private:
	DHTKey _key;  /**< location key id. */
	NetAddress _na;
	
	time_t _dead_time;
	time_t _up_time;
	bool _alive;
	
	/**
	 * TODO: opened socket to this location, if any.
	 */
	
	/**
	 * TODO: back-pointers to related internal structures, if needed.
	 */
     };
    
} /* end of namespace */

#endif
