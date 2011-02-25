/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#ifndef UDB_CLIENT_H
#define UDB_CLIENT_H

#include "db_record.h"

using sp::db_record;

namespace seeks_plugins
{

  class udb_client
  {
    public:
      udb_client();

      ~udb_client();

      db_record* find_dbr_client(const std::string &host,
                                 const int &port,
                                 const std::string &key,
                                 const std::string &pn);

      static db_record* deserialize_found_record(const std::string &str,
          const std::string &pn);
  };

} /* end of namespace. */

#endif
