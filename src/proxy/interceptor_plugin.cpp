/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
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

#include "interceptor_plugin.h"

namespace sp
{
  interceptor_plugin::interceptor_plugin(const std::vector<url_spec*> &pos_patterns,
                                         const std::vector<url_spec*> &neg_patterns,
                                         plugin *parent)
      : plugin_element(pos_patterns, neg_patterns, parent)
  {
  }

  interceptor_plugin::interceptor_plugin(const char *filename,
                                         plugin *parent)
      : plugin_element(filename, parent)
  {
  }

  interceptor_plugin::interceptor_plugin(const std::vector<std::string> &pos_patterns,
                                         const std::vector<std::string> &neg_patterns,
                                         plugin *parent)
      : plugin_element(pos_patterns, neg_patterns, parent)
  {
  }


} /* end of namespace. */
