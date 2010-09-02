/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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
  
#ifndef USER_DB_H
#define USER_DB_H

#include "db_record.h"

#include <tcutil.h>
#include <tchdb.h> // tokyo cabinet.
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

namespace sp
{
   
   class user_db
     {
      public:
	/**
	 * \brief db constructor, called by system.
	 */
	user_db();
	
	/**
	 * \brief db desctructor, called by system.
	 */
	~user_db();
	
	/**
	 * \brief opens the db if not already opened.
	 */
	int open_db();
	
	/**
	 * \brief closes the db if opened.
	 */
	int close_db();
	
	/**
	 * \brief generates a unique key for the record, from a given key and the plugin name.
	 */
	static std::string generate_rkey(const std::string &key,
					 const std::string &plugin_name);
	
	/**
	 * \brief extracts the record key from the internal record key.
	 */
	static std::string extract_key(const std::string &rkey);
	
	/**
	 * \brief serializes and adds record to db.
	 * @param key is record key (the internal key is a mixture of plugin name and record key).
	 * @param plugin_name the name of the plugin the record belongs to.
	 * @param dbr the db record object to be serialized and added.
	 * @return 0 if no error.
	 */
	int add_dbr(const std::string &key,
		    const std::string &plugin_name,
		    const db_record &dbr);
	
	/**
	 * \brief removes record with key 'rkey'.
	 * Beware, as internal rkeys are constructed from both plugin name and a record key.
	 */
	int remove_dbr(const std::string &rkey);
	
	/**
	 * \brief removes record that has 'key' and belongs to plugin 'plugin_name'.
	 */
	int remove_dbr(const std::string &key,
		       const std::string &plugin_name);
	
	/**
	 * \brief removes all records in db.
	 */
	int clear_db();
	
	/**
	 * \brief removes all records older than date.
	 */
	int prune_db(const time_t &date);
	
	/**
	 * \brief removes all records that belong to plugin 'plugin_name'.
	 */
	int prune_db(const std::string &plugin_name);
     
	/**
	 * \brief returns db size on disk.
	 */
	uint64_t disk_size() const;
	                         
	/**
	 * \brief returns number of records in db.
	 */
	uint64_t number_records() const;
	
	/**
	 * \brief reads and prints out all db records.
	 */
	void read() const;
	
      public:
	TCHDB *_hdb; /**< Tokyo Cabinet hashtable db. */
	bool _opened; /**< whether the db is opened. */
	std::string _name; /**< db path and filename. */
	static std::string _db_name; /**< db file name. */
     };
      
} /* end of namespace. */

#endif
