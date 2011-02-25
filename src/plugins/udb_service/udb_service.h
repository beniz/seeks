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

#ifndef UDB_SERVICE_H
#define UDB_SERVICE_H

#include "plugin.h"
#include "db_err.h"

using namespace sp;

namespace seeks_plugins
{

  class udb_service : public plugin
  {
    public:
      udb_service();

      virtual ~udb_service();

      virtual void start() {};

      virtual void stop() {};

      //TODO
      static db_err cgi_find_dbr(client_state *csp,
                                 http_response *rsp,
                                 const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static db_record* find_dbr_client(const std::string &host,
                                        const int &port,
                                        const std::string &key,
                                        const std::string &pn);
  };

} /* end of namespace. */

#endif
