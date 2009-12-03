/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 **/

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
