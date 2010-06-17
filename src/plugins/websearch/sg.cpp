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

#include "sg.h"

namespace seeks_plugins
{
   sg::sg(const DHTKey &sg_key, const Location &host,
	  const int &radius)
     :_sg_key(sg_key),_host(host),_radius(radius)
     {
     }
   
   sg::~sg()
     {
	clear_peers();
     }
   
   void sg::add_peers(std::vector<Subscriber*> &peers)
     {
	hash_map<const DHTKey*,Subscriber*,hash<const DHTKey*>,eqdhtkey>::iterator hit;
	std::vector<Subscriber*>::iterator vit = peers.begin();
	while(vit!=peers.end())
	  {
	     Subscriber *sub = (*vit);
	     if ((hit=_peers.find(&sub->_idkey))!=_peers.end())
	       {
		  // update peer information.
		  (*hit).second->setNetAddress(sub->getNetAddress());
		  (*hit).second->setPort(sub->getPort());
		  
		  vit = peers.erase(vit);
		  delete sub;
	       }
	     else _peers.insert(std::pair<const DHTKey*,Subscriber*>(&(*vit)->_idkey,(*vit)));
	     ++vit;
	  }
     }

   void sg::clear_peers()
     {
	hash_map<const DHTKey*,Subscriber*,hash<const DHTKey*>,eqdhtkey>::iterator hit
	  = _peers.begin();
	while(hit!=_peers.end())
	  {
	     delete (*hit).second;
	     ++hit;
	  }
     }
      
} /* end of namespace. */
