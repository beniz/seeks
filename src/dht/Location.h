/* -*- mode: c++ -*-
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
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

#ifndef LOCATION_H
#define LOCATION_H

#include "DHTKey.h"
#include "NetAddress.h"

namespace dht
{
  class Location : public DHTKey
  {
    public:
      Location();

      Location(const DHTKey& key, const NetAddress& na);

      Location(const DHTKey& dk) : DHTKey(dk) {}

      ~Location();

      NetAddress getNetAddress() const
      {
        return _na;
      }
      void setNetAddress(const NetAddress& na)
      {
        _na = na;
      }

      /**
       * /brief updates the net address of this location.
       * @param na net address.
       */
      void update(const NetAddress& na);
#if 0
      void update_check_time();
#endif

      /**
       * operators.
       */
      bool operator==(const Location &loc) const;
      bool operator!=(const Location &loc) const;

    private:
      NetAddress _na; /**< location address. */
      uint32_t _last_check_time; /**< last time this node has responded. */
  };

} /* end of namespace */

#endif
