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

#ifndef ROUTEITERATOR_H
#define ROUTEITERATOR_H

#include "Location.h"
#include <vector>

namespace dht
{
   class RouteIterator
     {
      public:
	RouteIterator();
	
	~RouteIterator();
	
	/**
	 * erase location from tit (included), to the end.
	 */
	void erase_from(std::vector<Location*>::iterator &tit);
	
      public:
	std::vector<Location*> _hops;
     };
      
} /* end of namespace. */

#endif
