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

#include "udb_server.h"
#include "seeks_proxy.h"

#include <string.h>
#include <iostream>

using namespace sp;

namespace seeks_plugins
{

  db_err udb_server::find_dbr_cb(const char *key_str, const char *pn_str,
                                 http_response *rsp)
  {
    std::string key(key_str);
    std::string pn(pn_str);
    db_record *dbr = seeks_proxy::_user_db->find_dbr(key,pn);
    if (!dbr)
      {
        return DB_ERR_NO_REC;
      }
    else
      {
        // serialize dbr.
        std::string str;
        dbr->serialize(str);

        // fill up response.
        size_t body_size = str.length() * sizeof(char);
        if (!rsp->_body)
          rsp->_body = (char*)std::malloc(body_size);
        rsp->_content_length = body_size;
        for (size_t i=0; i<str.length(); i++)
          rsp->_body[i] = str[i];
        delete dbr;
        return SP_ERR_OK;
      }
  }

} /* end of namespace. */
