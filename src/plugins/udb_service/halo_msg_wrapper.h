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

#ifndef HALO_MSG_WRAPPER_H
#define HALO_MSG_WRAPPER_H

#include "halo_msg.pb.h"
#include "sp_exception.h"
#include "DHTKey.h"
#include "stl_hash.h"

#include <string>
#include <vector>

namespace seeks_plugins
{

  class halo_msg_wrapper
  {
    public:
      static void deserialize(const std::string &msg,
                              uint32_t &expansion,
                              std::vector<std::string> &qhashes) throw (sp_exception);

      static void serialize(const uint32_t &expansion,
                            const hash_multimap<uint32_t,DHTKey,id_hash_uint> &qhashes,
                            std::string &msg) throw (sp_exception);
  };

} /* end of namespace. */

#endif
