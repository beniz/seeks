/** -*- mode: c++ -*-
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Loic Dachary <loic@dachary.org>
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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

#ifndef DHT_EXCEPTION_H
#define DHT_EXCEPTION_H

#include <string>

namespace dht
{
  class dht_exception
  {
    public:

      dht_exception(int code, const std::string message) : _code(code), _message(message) {};

      std::string what() const { return _message; }

      int code() const { return _code; }

    protected:

      int _code;

      std::string _message;
  };

} /* end of namespace. */

#endif
