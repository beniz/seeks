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

#ifndef SG_DB_H
#define SG_DB_H

#include "DHTKey.h"
#include "searchgroup.h"

#include <tcutil.h>
#include <tchdb.h> // tokyo cabinet.
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

namespace dht
{
   
   class sg_db
     {
      public:
	sg_db();
	
	~sg_db();
	
	int open_db();
	
	int close_db();
	
	Searchgroup* find_sg_db(const DHTKey &sgkey);
	
	bool remove_sg_db(const DHTKey &sgkey);
	
	bool add_sg_db(Searchgroup *sg);
	
	uint64_t disk_size() const;
	
	void prune();
	
      public:
	TCHDB *_hdb;
	bool _opened;
	std::string _name;
     };
      
} /* end of namespace. */

#endif
