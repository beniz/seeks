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
	/**
	 * \brief constructor.
	 */
	sg_db();
	
	/**
	 * \brief destructor.
	 */
	~sg_db();
	
	/**
	 * \brief opens the db.
	 */
	int open_db();
	
	/**
	 * \brief closes the db.
	 */
	int close_db();
	
	/**
	 * \brief erases all records in the db.
	 */
	int clear_db();
	
	/**
	 * \brief finds searchgroup object in db, if it exists.
	 * @param sgkey searchgroup id key.
	 * @return searchgroup object if it exists in db, NULL otherwise.
	 */
	Searchgroup* find_sg_db(const DHTKey &sgkey);
	
	/**
	 * \brief returns all searchgroups with keys in a given range.
	 * @param start_key beginning of the key range.
	 * @param end_key ending of the key range.
	 * @param res hashtable of results.
	 */
	void find_sg_range(const DHTKey &start_key,
			   const DHTKey &end_key,
			   hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey> &res);
	
	/**
	 * \brief erase a searchgroup from db.
	 * @param sgkey key of the searchgroup to remove.
	 * @return true if searchgroup was removed, false otherwise.
	 */
	bool remove_sg_db(const DHTKey &sgkey);
	
	/**
	 * \brief add searchgroup to db.
	 * @param sg searchgroup object to store in db.
	 * @return true if searchgroup was stored, false otherwise.
	 */
	bool add_sg_db(Searchgroup *sg);
	
	/**
	 * \brief size of db on disk.
	 * @return size of db on disk.
	 */
	uint64_t disk_size() const;
	
	/**
	 * \brief number of records in db.
	 * @return number of records in db.
	 */
	uint64_t number_records() const;
	
	/**
	 * \brief optimizes db and removes records until size of db is
	 *        below 'sg-db-cap' in configuration.
	 */
	void prune();
	
	/**
	 * \brief decrements replication level of all searchgroups in db,
	 *        and returns all searchgroups with replication level newly
	 *        set to 0.
	 * @param nsgs returned set of searchgroups with replication level 
	 *        newly set to 0.
	 * @return true if no error, false if error(s) occured.
	 */
	bool decrement_replication_level_all_sgs(std::vector<Searchgroup*> &nsgs);
	
	/**
	 * \brief traverses all records and prints them out on standard output.
	 */
	void read() const;
	
      public:
	TCHDB *_hdb;  /**< TokyoCabinet hashtable db. */
	bool _opened; /**< db open flag. */
	std::string _name; /**< db file name, automatically set. */
     };
      
} /* end of namespace. */

#endif
