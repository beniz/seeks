/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Loic Dachary <loic@dachary.org>
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

#ifndef XML_RENDERER_PRIVATE_H
#define XML_RENDERER_PRIVATE_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include "websearch.h"
#include "clustering.h"

namespace xml_renderer_private
{
  sp_err collect_xml_results(std::list<std::string> &results,
			     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
			     const query_context *qc,
			     const double &qtime,
			     const bool &img,
			     xmlNodePtr parent);

}

#endif // JSON_RENDERER_PRIVATE_H
