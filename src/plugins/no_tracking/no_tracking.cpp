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

#include "no_tracking.h"
#include "proxy_dts.h"
#include "seeks_proxy.h"
#include <iostream>

namespace seeks_plugins
{
  /*- no_tracking -*/
  no_tracking::no_tracking()
    : plugin()
  {
    _name = "no-tracking";
    _version_major = "0";
    _version_minor = "1";
    _config_filename = "";
    _configuration = NULL;
    _interceptor_plugin = new no_tracking_element(this);
  }

  /* no_tracking_element -*/
  std::string no_tracking_element::_lm_filename = "no_tracking/lm-patterns";

  no_tracking_element::no_tracking_element(plugin *parent)
    : interceptor_plugin((seeks_proxy::_datadir.empty() ? std::string(plugin_manager::_plugin_repository
                          + no_tracking_element::_lm_filename).c_str()
                          : std::string(seeks_proxy::_datadir + "/plugins/" + no_tracking_element::_lm_filename).c_str()),
                         parent)
  {
  }

  http_response* no_tracking_element::plugin_response(client_state *csp)
  {
    set_last_modified_random(csp);
    remove_if_none_match(csp);
    return NULL;
  }

  /**
   * avoids resending a tracking ETAG.
   */
  void no_tracking_element::remove_if_none_match(client_state *csp)
  {
    csp->_action._flags |= ACTION_CRUNCH_IF_NONE_MATCH;
  }

  void no_tracking_element::set_last_modified_random(client_state *csp)
  {
    csp->_action._flags |= ACTION_OVERWRITE_LAST_MODIFIED;
    csp->_action._string[ACTION_STRING_LAST_MODIFIED] = strdup("randomize");
  }

  /* plugin registration */
  extern "C"
  {
    plugin* maker()
    {
      return new no_tracking;
    }
  }

} /* end of namespace. */
