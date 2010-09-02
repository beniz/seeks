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

#ifndef USER_RECORD_H
#define USER_RECORD_H

#include <time.h>
#include <string>

namespace sp
{
   
   class db_record
     {
      public:
	/**
	 * \brief constructor with time and plugin name.
	 */
	db_record(const time_t &creation_time,
		  const std::string plugin_name);
	
	/**
	 * \brief constructor, from a serialized message.
	 */
	db_record(const std::string &msg);
	
	/**
	 * destructor.
	 */
	~db_record();
     
	/**
	 * \brief serializes the object.
	 */
	virtual int serialize(std::string &msg) const;
	
	/**
	 * \brief fill up the object from a serialized version of it.
	 */
	virtual int deserialize(const std::string &msg);
	
	/**
	 * \brief base serialization.
	 */
	int serialize_base_record(std::string &msg) const;
	
	/**
	 * \brief base deserialization.
	 */
	int deserialize_base_record(const std::string &msg);
	
      public:
	time_t _creation_time;
	std::string _plugin_name;
     };
      
} /* end of namespace. */

#endif
