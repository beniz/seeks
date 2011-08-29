/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#ifndef NO_TRACKING_H
#define NO_TRACKING_H

#define NO_PERL

#include "plugin.h"
#include "interceptor_plugin.h"

using namespace sp;

namespace seeks_plugins
{

  class no_tracking : public plugin
  {
    public:
      no_tracking();

      ~no_tracking() {};

      virtual void start() {};

      virtual void stop() {};
  };

  class no_tracking_element : public interceptor_plugin
  {
    public:
      no_tracking_element(plugin *parent);

      ~no_tracking_element() {};

      virtual http_response* plugin_response(client_state *csp);

    private:
      void set_last_modified_random(client_state *csp);

      void remove_if_none_match(client_state *csp);

      static std::string _lm_filename; /**< URL interception patterns for treating the last-modified header. */
  };

} /* end of namespace. */

#endif
