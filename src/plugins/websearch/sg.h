/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#ifndef SG_H
#define SG_H

#include "DHTKey.h"
#include "Location.h"
#include "subscriber.h"

#include <vector>

using dht::Location;
using dht::Subscriber;

namespace seeks_plugins
{
   
   class sg
     {
      public:
	sg(const DHTKey &sg_key, const Location &host,
	   const int &radius);
	
	~sg();
	
	void add_peers(std::vector<Subscriber*> &peers);
	
	//TODO: remove peer.
	
	void clear_peers();
	
      public:
	DHTKey _sg_key; /**< search group key, aka query fragment. */
	Location _host;
	int _radius;
	hash_map<const DHTKey*,Subscriber*,hash<const DHTKey*>,eqdhtkey> _peers;
     };
      
} /* end of namespace. */

#endif
