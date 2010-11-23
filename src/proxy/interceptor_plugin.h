/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, ebenazer@seeks-project.info
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

#ifndef INTERCEPTOR_PLUGIN_H
#define INTERCEPTOR_PLUGIN_H

#include "plugin_element.h"

#include <string>
#include <vector>

namespace sp
{
  /**
   * \brief interceptor plugin reacts positively to a given set of patterns,
   *        when they match a URL requested to the proxy.
   *
   *        In response the interceptor plugin can trigger any kind of response.
   *
   *        Example of interceptor plugin includes the 'blocker' plugin that
   *        can block ads or any other pattern set by the user(s).
   *        The 'websearch' plugin uses an interceptor plugin to capture search
   *        queries to several search engines and to channel them to Seeks instead.
   */
  class interceptor_plugin : public plugin_element
  {
    public:
      /**
       * \brief constructor.
       * @param pos_patterns are patterns (e.g. URI patterns) that activate the plugin.
       * @param neg_patterns are patterns the plugin should not be activated by
       *        (default is everything but what is in the positive patterns set, but
       *        negative patterns are sometimes useful).
       * @param parent the plugin object the interceptor belongs to.
       */
      interceptor_plugin(const std::vector<url_spec*> &pos_patterns,
                         const std::vector<url_spec*> &neg_patterns,
                         plugin *parent);

      /**
       * \brief constructor.
       * @param pattern_filename the the pattern file, see plugin_element class description
       *        for pattern file format.
       * @param parent the plugin object the interceptor belongs to.
       */
      interceptor_plugin(const char *filename,
                         plugin *parent);

      /**
       * \brief constructor.
       * @param pos_patterns are compiled patterns (e.g. URI patterns) that activate the plugin.
       * @param neg_patterns are compiled patterns the plugin should not be activated by
       *        (default is everything but what is in the positive patterns set, but
       *        negative patterns are sometimes useful).
       * @param parent the plugin object the interceptor belongs to.
       */
      interceptor_plugin(const std::vector<std::string> &pos_patterns,
                         const std::vector<std::string> &neg_patterns,
                         plugin *parent);

      /**
       * \brief plugin response to interception.
       * @param csp the client HTTP request state.
       * @return an HTTP response, NULL is no response needs to be given.
       */
      virtual http_response* plugin_response(client_state *csp)
      {
        return NULL;
      };

      /**
       * \brief prints out plugin information to a string.
       * @return string that contains plugin information.
       */
      virtual std::string print()
      {
        return "";
      }; // virtual
  };

} /* end of namespace. */

#endif
