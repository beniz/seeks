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

#ifndef SEARCHGROUP_H
#define SEARCHGROUP_H

#include "DHTKey.h"
#include "subscriber.h"
#include "sweeper.h"
#include "sg_configuration.h"

#include <time.h>
#include <vector>

using sp::sweepable;

typedef pthread_mutex_t sp_mutex_t;

namespace dht
{
   
   class Searchgroup : public sweepable
     {
      public:
	Searchgroup(const DHTKey &idkey);
	
	Searchgroup(const Searchgroup &sg);
	
	~Searchgroup();
	
	void set_creation_time();
	
	void set_last_time_of_use();
	
        bool add_subscriber(Subscriber *su);
	
	bool remove_subscriber(const DHTKey *sukey);
	
	/* virtual functions. */
	virtual bool sweep_me();
	
	//TODO: storage.
	bool serialize_to_string(std::string &str);
	
	static Searchgroup* deserialize_from_string(const std::string &str); //TODO.
		
	/* peer selection. */
	void random_peer_selection(const int &npeers, std::vector<Subscriber*> &rpeers);
	
      public:
	DHTKey _idkey;  /**< searchgroup hash key. */
	
	hash_map<const DHTKey*,Subscriber*,hash<const DHTKey*>,eqdhtkey> _hash_subscribers; /**< subscribers to the group. */
	std::vector<Subscriber*> _vec_subscribers; /**< sortable subscribers. */
     
	/* timer. */
	time_t _creation_time;
	time_t _last_time_of_use;
     
	/* locking (protection against deletion). */
	bool _lock;
	
	/* mutex. */
	sp_mutex_t _sg_mutex;
     
	/* replication level of this group. */
	short _replication_level;
     };
   
} /* end of namespace. */

#endif
