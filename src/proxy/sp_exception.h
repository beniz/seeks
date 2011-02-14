/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, <ebenazer@seeks-project.info>
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

#ifndef SP_EXCEPTION_H
#define SP_EXCEPTION_H

#include "miscutil.h"

using sp::miscutil;

class sp_exception
{
  public:
    sp_exception(int code, const std::string message) : _code(code), _message(message) {};

    std::string what() const
    {
      return _message;
    }

    int code() const
    {
      return _code;
    }

    std::string to_string() const
    {
      return "code = " + sp::miscutil::to_string(_code) + " message = " + _message;
    }

  protected:
    int _code;
    std::string _message;
};

#endif
