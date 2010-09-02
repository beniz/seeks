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
 
#include "db_record.h"
#include "db_record_msg.pb.h"

namespace sp
{
   db_record::db_record(const time_t &creation_time,
			const std::string plugin_name)
     :_creation_time(creation_time),_plugin_name(plugin_name)
     {
     }

   db_record::db_record(const std::string &msg)
     :_creation_time(0)
     {
	deserialize(msg);
     }
      
   db_record::~db_record()
     {
     }
   
   int db_record::serialize(std::string &msg) const
     {
	return serialize_base_record(msg);
     }
   
   int db_record::deserialize(const std::string &msg)
     {
	return deserialize_base_record(msg);
     }
   
   int db_record::serialize_base_record(std::string &msg) const
     {
	sp::db::record r;
	r.set_creation_time(_creation_time);
	r.set_plugin_name(_plugin_name);
	return r.SerializeToString(&msg); // != 0 on error.
     }
   
   int db_record::deserialize_base_record(const std::string &msg)
     {
	sp::db::record r;
	int err = r.ParseFromString(msg);
	if (err != 0)
	  return err; // error.
	_creation_time = r.creation_time();
	_plugin_name = r.plugin_name();	
	return 0;
     }
      
} /* end of namespace. */
