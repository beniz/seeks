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

#ifndef SG_SWEEPER_H
#define SG_SWEEPER_H

//TODO: let's schedule calls.
//TODO: memory & disk storage...

#include "sg_manager.h"
#include <vector>

using sp::sweepable;

namespace dht
{
   class sg_manager;
   
   class sg_sweeper
     {
      public:
	static int sweep();
	
	static int sweep_memory();
	
	static int sweep_db();
     
      public:
	sg_manager *_sgm;
     };
      
} /* end of namespace. */

#endif
  
