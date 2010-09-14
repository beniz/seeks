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
 
#ifndef DB_QUERY_RECORD_H
#define DB_QUERY_RECORD_H

#include "db_record.h"

#include <set>

using sp::db_record;

namespace seeks_plugins
{
   struct lt_string
     {
	bool operator()(const std::string &s1, const std::string &s2) const
	  {
	     return std::lexicographical_compare(s1.begin(),s1.end(),
						 s2.begin(),s2.end());
	  }
     };
      
   class db_query_record : public db_record
     {
      public:
	db_query_record(const time_t &creation_time,
			const std::string &plugin_name);
	
	db_query_record(const std::string &plugin_name,
			const std::string &query);
	
	db_query_record();
	
	virtual ~db_query_record();
     
	virtual int serialize(std::string &msg) const;
	
	virtual int deserialize(const std::string &msg);
	
	virtual int merge_with(const db_query_record &dqr);
	
	void create_query_record(sp::db::record &r) const;
	
	void read_query_record(sp::db::record &r);
	
	virtual std::ostream& print(std::ostream &output);
     
      public:
	std::string _query;
	std::set<std::string> _visited_urls;
     };
      
} /* end of namespace. */

#endif
