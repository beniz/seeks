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
#include "errlog.h"

#include <sys/time.h>

#include <iostream>

namespace sp
{
   db_record::db_record(const time_t &creation_time,
			const std::string &plugin_name)
     :_creation_time(creation_time),_plugin_name(plugin_name)
     {
     }
   
   db_record::db_record(const std::string &plugin_name)
     :_plugin_name(plugin_name)
       {
	  struct timeval tv_now;
	  gettimeofday(&tv_now, NULL);
	  _creation_time = tv_now.tv_sec;
       }
      
   db_record::db_record()
     :_creation_time(0)
     {
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
	std::cerr << "db_record.deserialize()\n";
	return deserialize_base_record(msg);
     }
   
   int db_record::serialize_base_record(std::string &msg) const
     {
	sp::db::record r;
	create_base_record(r);
	return r.SerializeToString(&msg); // != 0 on error.
     }
   
   int db_record::deserialize_base_record(const std::string &msg)
     {
	sp::db::record r;
	int err = r.ParseFromString(msg);
	if (err != 0)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"Error deserializing user db_record");
	     return err; // error.
	  }
	read_base_record(r);
	return 0;
     }

   void db_record::create_base_record(sp::db::record &r) const
     {
	r.set_creation_time(_creation_time);
	r.set_plugin_name(_plugin_name);
     }
      
   void db_record::read_base_record(const sp::db::record &r)
     {
	_creation_time = r.creation_time();
	_plugin_name = r.plugin_name();
     }
   
   std::ostream& db_record::print_header(std::ostream &output)
     {
	output << "\tplugin_name: " << _plugin_name << "\n\tcreation time: " << _creation_time << std::endl;
	return output;
     }
   
   std::ostream& db_record::print(std::ostream &output)
     {
	return print_header(output);
     }
         
} /* end of namespace. */
