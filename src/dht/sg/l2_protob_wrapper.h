/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
 * Copyright (C) 2010  Loic Dachary <loic@dachary.org>
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

#ifndef L2_PROTOB_WRAPPER_H
#define L2_PROTOB_WRAPPER_H

#include "dht_err.h"
#include "dht_layer2.pb.h"
#include "dht_exception.h"
#include "subscriber.h"

#include <vector>

namespace dht
{

  class l2_protob_wrapper
  {
    public:
      static l2::l2_subscribe_response* create_l2_subscribe_response(const uint32_t &error_status,
          const std::vector<Subscriber*> &peers);

    public:
      static void serialize_to_string(const l2::l2_subscribe_response*, std::string &str) throw (dht_exception);

    public:
      static void read_l2_subscribe_response(const l2::l2_subscribe_response *l2r,
                                             uint32_t &error_status,
                                             std::vector<Subscriber*> &peers);

    public:
      static void deserialize(const std::string &str, l2::l2_subscribe_response *l2r) throw (dht_exception);
  };

} /* end of namespace. */

#endif
