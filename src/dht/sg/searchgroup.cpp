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

#include "searchgroup.h"
#include "seeks_proxy.h" // for mutexes.

#include <algorithm>
#include <sys/time.h>

using sp::seeks_proxy;

namespace dht
{
   Searchgroup::Searchgroup(const DHTKey &idkey)
     :sweepable(),_idkey(idkey),_lock(false)
       {
	  set_creation_time();
	  _last_time_of_use = _creation_time;
	  seeks_proxy::mutex_init(&_sg_mutex);
       }
   
   Searchgroup::~Searchgroup()
     {
	hash_map<const DHTKey*,Subscriber*,hash<const DHTKey*>,eqdhtkey>::iterator hit
	  = _hash_subscribers.begin();
	while(hit!=_hash_subscribers.end())
	  {
	     delete (*hit).second;
	     ++hit;
	  }
     }
   
   void Searchgroup::set_creation_time()
     {
	struct timeval tv_now;
	gettimeofday(&tv_now,NULL);
	_creation_time = tv_now.tv_sec;
     }
   
   void Searchgroup::set_last_time_of_use()
     {
	struct timeval tv_now;
	gettimeofday(&tv_now,NULL);
	_last_time_of_use = tv_now.tv_sec;
     }
     
   bool Searchgroup::add_subscriber(Subscriber *su)
     {
	hash_map<const DHTKey*,Subscriber*,hash<const DHTKey*>,eqdhtkey>::iterator hit;
	if ((hit=_hash_subscribers.find(&su->_idkey))!=_hash_subscribers.end())
	  {
	     (*hit).second->set_join_date(); // reset join date.
	     return false;
	  }
	_hash_subscribers.insert(std::pair<const DHTKey*,Subscriber*>(&su->_idkey,su));
	return true;
     }
   
   bool Searchgroup::remove_subscriber(const DHTKey *sukey)
     {
	hash_map<const DHTKey*,Subscriber*,hash<const DHTKey*>,eqdhtkey>::iterator hit;
	if ((hit=_hash_subscribers.find(sukey))!=_hash_subscribers.end())
	  {
	     delete (*hit).second;
	     return true;
	  }
	return false;
     }
      
   //TODO.
   int Searchgroup::move_to_db()
     {
	return 0;
     }
      
   bool Searchgroup::sweep_me()
     {
	// don't delete if locked.
	if (_lock)
	  return false;
     
	// delete if the group is empty.
	if (_hash_subscribers.empty())
	  return true;
	
	// check last_time_of_use + delay against current time.
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	double dt = difftime(tv_now.tv_sec,_last_time_of_use);
	if (dt >= sg_configuration::_sg_config->_sg_mem_delay)
	  return true;
	else return false;
     }

   void Searchgroup::random_peer_selection(const int &npeers, std::vector<Subscriber*> &rpeers)
     {
	rpeers = _vec_subscribers;
	std::random_shuffle(rpeers.begin(),rpeers.end()); // XXX: uses built-in random generator.
	std::vector<Subscriber*>::iterator vit = rpeers.begin();
	vit += npeers;
	rpeers.erase(vit,rpeers.end());
     }
      
} /* end of namespace. */
