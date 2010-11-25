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

#ifndef DHT_API_H
#define DHT_API_H

#include "DHTNode.h"
#include "DHTVirtualNode.h"

namespace dht
{
   
   class dht_api
     {
      public:
	static void findClosestPredecessor(const DHTNode &dnode,
					      const DHTKey& nodeKey,
					      DHTKey& dkres, NetAddress& na,
					      int& status);
	
	static dht_err findSuccessor(const DHTNode &dnode,
				     const DHTKey &nodekey,
				     DHTKey &dkres, NetAddress &na);
     
	static dht_err findPredecessor(const DHTNode &dnode,
				       const DHTKey &nodekey,
				       DHTKey &dkres, NetAddress &na);
	
	static dht_err ping(const DHTNode &dnode,
			    const DHTKey &nodekey,
			    const NetAddress &na,
			    bool &alive);
     };
      
} /* end of namespace. */

#endif
