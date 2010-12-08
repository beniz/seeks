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

#ifndef SG_API_H
#define SG_API_H

#include "dht_err.h"
#include "dht_api.h"
#include "SGNode.h"
#include "Location.h"
#include "subscriber.h"

#include <vector>

namespace dht
{
   
   class sg_api
     {
      public:
	static dht_err find_sg(const SGNode &sgnode,
			       const DHTKey &sg_key, Location &node) throw (dht_exception);
	
	static dht_err get_sg_peers(const SGNode &sgnode,
				    const DHTKey &sg_key, 
				    const Location &node,
				    std::vector<Subscriber*> &peers) throw (dht_exception);
     
	static dht_err get_sg_peers_and_subscribe(const SGNode &sgnode,
						  const DHTKey &sg_key,
						  const Location &node,
						  std::vector<Subscriber*> &peers) throw (dht_exception);
     };
    
} /* end of namespace. */

#endif
