/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
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

#include "Location.h"

#include <sys/time.h>

namespace dht
{
  Location::Location()
  {
  }

  Location::Location(const DHTKey& key, const NetAddress& na)
    : DHTKey(key), _na(na)
  {
  }

  Location::~Location()
  {
  }

  void Location::update(const NetAddress& na)
  {
    if (_na.operator!=(na))
      {
        _na = na;
      }
  }

#if 0
  void Location::update_check_time()
  {
    struct timeval *tvz = (struct timeval*) malloc(sizeof(struct timeval));
    gettimeofday(tvz,NULL);
    _last_check_time = tvz->tv_sec;
    free(tvz);
  }
#endif

  bool Location::operator==(const Location &loc) const
  {
    return DHTKey::operator==(loc) && _na == loc.getNetAddress();
  }

  bool Location::operator!=(const Location &loc) const
  {
    return DHTKey::operator!=(loc) || _na != loc.getNetAddress();
  }

} /* end of namespace. */
