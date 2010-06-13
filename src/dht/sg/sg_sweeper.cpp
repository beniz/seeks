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

#include "sg_sweeper.h"
#include "sg_manager.h"

namespace dht
{
   sg_manager* sg_sweeper::_sgm = NULL; // beware.

   void sg_sweeper::init(sg_manager *sgm)
     {
	sg_sweeper::_sgm = sgm;
     }
      
   int sg_sweeper::sweep()
     {
	sweep_memory();
	
	//TODO: sweep_db + condition (uptime, last_db_sweep, etc...).
	sweep_db();
     
	//TODO: status return.
     }
   
   int sg_sweeper::sweep_memory()
     {
	// go through the list of in-memory searchgroups
	// and apply LRU by moving the least recently called group
	// to db (even if memory cap has not been reached).
	int count = 0;
	hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey>::iterator hit
	  = _sgm->_searchgroups.begin();
	while(hit!=_sgm->_searchgroups.end())
	  {
	     if ((*hit).second->sweep_me())
	       {
		  //TODO: check on db cap here.
		  (*hit).second->move_to_db(); //TODO.
		  delete (*hit).second;
		  hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey>::iterator hit2 = hit;
		  hit++;
		  _sgm->_searchgroups.erase(hit2);
		  count++;
	       }
	     else ++hit;
	  }
	return count;
     }
   
   int sg_sweeper::sweep_db()
     {
	//TODO
     }
      
}; /* end of namespace. */
