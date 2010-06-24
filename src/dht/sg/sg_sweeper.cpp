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
	return sweep_memory();
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
		  // check on db filesize cap.
		  if (sg_configuration::_sg_config->_sg_db_cap > 0
		      && sg_configuration::_sg_config->_sg_db_cap < _sgm->_sdb.disk_size())
		    {
		       sweep_db();
		    }
		  
		  if (!(*hit).second->_hash_subscribers.empty() 
		      && sg_configuration::_sg_config->_db_sync_mode == 0) // when not in full sync mode.
		    _sgm->move_to_db((*hit).second);
		  hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey>::iterator hit2 = hit;
		  hit++;
		  Searchgroup *dsg = (*hit2).second;
		  _sgm->_searchgroups.erase(hit2);
		  delete dsg;
		  count++;
	       }
	     else ++hit;
	  }
	return count;
     }
   
   int sg_sweeper::sweep_db()
     {
	_sgm->_sdb.prune();
	return 0;
     }
      
}; /* end of namespace. */
