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

#ifndef UDB_SERVER_H
#define UDB_SERVER_H

#include "db_err.h"
#include "proxy_dts.h"

using namespace sp;

namespace seeks_plugins
{

  class udb_server
  {
    public:
      static db_err find_dbr_cb(const char *key_str, const char *pn_str,
                                http_response *rsp);

      // other cb come here.
  };

} /* end of namespace. */

#endif
