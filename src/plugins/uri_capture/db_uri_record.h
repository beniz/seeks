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

#ifndef DB_URI_RECORD_H
#define DB_URI_RECORD_H

#include "db_record.h"

using sp::db_record;

namespace seeks_plugins
{

  class db_uri_record : public db_record
  {
    public:
      db_uri_record(const time_t &creation_time,
                    const std::string &plugin_name);

      db_uri_record(const std::string &plugin_name);

      db_uri_record(const std::string &plugin_name,
		    const int &hits);

      db_uri_record();

      virtual ~db_uri_record();

      virtual int serialize(std::string &msg) const;

      virtual int deserialize(const std::string &msg);

      virtual int merge_with(const db_record &dbr);

      void create_uri_record(sp::db::record &r) const;

      void read_uri_record(const sp::db::record &r);

    public:
      int _hits; /**< number of hits on this URI. */
  };

} /* end of namespace. */

#endif

