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

#include "db_uri_record.h"
#include "db_uri_record_msg.pb.h"
#include "errlog.h"

#include <iostream>

using sp::errlog;

namespace seeks_plugins
{

  db_uri_record::db_uri_record(const time_t &creation_time,
                               const std::string &plugin_name)
    :db_record(creation_time,plugin_name),_hits(1)
  {
  }

  db_uri_record::db_uri_record(const std::string &plugin_name)
    :db_record(plugin_name),_hits(1)
  {
  }

  db_uri_record::db_uri_record()
    :db_record(),_hits(0)
  {
  }

  db_uri_record::~db_uri_record()
  {
  }

  int db_uri_record::serialize(std::string &msg) const
  {
    sp::db::record r;
    create_uri_record(r);
    if (!r.SerializeToString(&msg))
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Failed serializing db_uri_record");
        return 1; // error.
      }
    else return 0;
  }

  int db_uri_record::deserialize(const std::string &msg)
  {
    sp::db::record r;
    if (!r.ParseFromString(msg))
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Failed deserializing db_uri_record");
        return 1; // error.
      }
    read_uri_record(r);
    return 0;
  }

  int db_uri_record::merge_with(const db_uri_record &dbr)
  {
    if (dbr._plugin_name != _plugin_name)
      return -2;
    _hits += dbr._hits;
    return 0;
  }

  void db_uri_record::create_uri_record(sp::db::record &r) const
  {
    create_base_record(r);
    r.SetExtension(sp::db::hits,_hits);
  }

  void db_uri_record::read_uri_record(const sp::db::record &r)
  {
    read_base_record(r);
    _hits = r.GetExtension(sp::db::hits);
  }

} /* end of namespace. */
