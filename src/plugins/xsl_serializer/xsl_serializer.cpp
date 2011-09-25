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

#include "xsl_serializer.h"
#include "seeks_proxy.h"
#include "miscutil.h"

#include <string.h>
#include <iostream>

namespace seeks_plugins
{

  xsl_serializer::xsl_serializer()
    :plugin()
  {
    _name = "xsl-serializer";
    _version_major = "0";
    _version_minor = "1";

  }

  xsl_serializer::~xsl_serializer()
  {
  }

  xsl_serializer::get_document() {
    doc=xmlNewDoc("1.0");
    return doc;
  }

  /* plugin registration. */
  extern "C"
  {
    plugin* maker()
    {
      return new xsl_serializer;
    }
  }

} /* end of namespace */
