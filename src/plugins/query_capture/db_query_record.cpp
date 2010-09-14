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

#include "db_query_record.h"
#include "db_query_record_msg.pb.h"
#include "errlog.h"

#include <algorithm>
#include <iterator>
#include <iostream>

using sp::errlog;

namespace seeks_plugins
{
   
   db_query_record::db_query_record(const time_t &creation_time,
				    const std::string &plugin_name)
     :db_record(creation_time,plugin_name)
       {
       }
             
   db_query_record::db_query_record(const std::string &plugin_name,
				    const std::string &query)
     :db_record(plugin_name),_query(query)
       {
	  
       }
             
   db_query_record::db_query_record()
     :db_record()
       {
       }
   
   db_query_record::~db_query_record()
     {
     }
     
   int db_query_record::serialize(std::string &msg) const
     {
	sp::db::record r;
	create_query_record(r);
	return r.SerializeToString(&msg);
     }

   int db_query_record::deserialize(const std::string &msg)
     {
	sp::db::record r;
	if (!r.ParseFromString(msg))
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"failed deserializing db_query_record");
	     return 1; // error.
	  }
	read_query_record(r);
	return 0;
     }
      
   int db_query_record::merge_with(const db_query_record &dbr)
     {
	if (dbr._plugin_name != _plugin_name)
	  return -2;
	if (dbr._query != _query)
	  return -2;
	std::set<std::string> visited_urls;
	std::set_union(_visited_urls.begin(),_visited_urls.end(),
		       dbr._visited_urls.begin(),dbr._visited_urls.end(),
		       std::inserter(visited_urls,visited_urls.begin()));//,lt_string());
	_visited_urls = visited_urls;
	return 0;
     }
      
   void db_query_record::create_query_record(sp::db::record &r) const
     {
	create_base_record(r);
	r.SetExtension(sp::db::query,_query);
	sp::db::visited_urls *vurls = r.MutableExtension(sp::db::urls);
	std::set<std::string>::const_iterator sit = _visited_urls.begin();
	while(sit != _visited_urls.end())
	  {
	     vurls->add_urls((*sit));
	     ++sit;
	  }
     }
   
   void db_query_record::read_query_record(sp::db::record &r)
     {
	read_base_record(r);
	_query = r.GetExtension(sp::db::query);
	sp::db::visited_urls *vurls = r.MutableExtension(sp::db::urls);
	int nvurls = vurls->urls_size();
	for (int i=0;i<nvurls;i++)
	  _visited_urls.insert(vurls->urls(i));
     }

} /* end of namespace. */
