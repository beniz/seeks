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

#include "halo_msg_wrapper.h"
#include "udbs_err.h"
#include "errlog.h"

using sp::errlog;

namespace seeks_plugins
{

  void halo_msg_wrapper::deserialize(const std::string &msg,
                                     uint32_t &expansion,
                                     std::vector<std::string> &hashes) throw (sp_exception)
  {
    hash_halo h;
    if (!h.ParseFromString(msg))
      {
        std::string msg = "failed deserializing query halo";
        errlog::log_error(LOG_LEVEL_ERROR,msg.c_str());
        throw sp_exception(UDBS_ERR_DESERIALIZE,msg);
      }
    expansion = h.expansion();
    for (int i=0; i<h.key_size(); i++)
      hashes.push_back(h.key(i));
  }

  void halo_msg_wrapper::serialize(const uint32_t &expansion,
                                   const hash_multimap<uint32_t,DHTKey,id_hash_uint> &qhashes,
                                   std::string &msg) throw (sp_exception)
  {
    hash_halo h;
    h.set_expansion(expansion);
    hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = qhashes.begin();
    while(hit!=qhashes.end())
      {
        std::string key_str = (*hit).second.to_rstring();
        h.add_key(key_str);
        ++hit;
      }
    if (!h.SerializeToString(&msg))
      {
        std::string msg = "failed to serialize query halo";
        errlog::log_error(LOG_LEVEL_ERROR,msg.c_str());
        throw sp_exception(UDBS_ERR_SERIALIZE,msg);
      }
  }

} /* end of namespace. */
