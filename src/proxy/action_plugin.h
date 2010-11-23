/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef ACTION_PLUGIN_H
#define ACTION_PLUGIN_H

#include "plugin.h"

#include <string>

namespace sp
{
  class action_plugin : public plugin_element
  {
    public:
      action_plugin(const std::vector<url_spec*> &patterns,
                    plugin *parent);

      virtual http_response* plugin_response(client_state *csp)
      {
        return NULL;
      };

      virtual std::string print()
      {
        return "";
      }; // virtual

    private:
      std::string _action_file; /**< file with action description. */
  };

} /* end of namespace. */

#endif
