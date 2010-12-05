/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009, 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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

#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "proxy_dts.h"

#include <vector>
#include <string>
#include <map>
#include <list>

#include "stl_hash.h"

namespace sp
{
  class plugin;
  class interceptor_plugin;
  class action_plugin;
  class filter_plugin;
  class configuration_spec;

  typedef plugin* maker_ptr();

  /**
   * \brief The plugin manager (PM) registers, starts and stops plugins as needed.
   *        Plugins come in the form of shared libraries. PM scans files in a configurable
   *        repository and tries to load any of the shared libraries it finds.
   *
   *        Loading takes two forms:
   *        - on Linux and FreeBSD, plugins are loaded, created and registered at once,
   *          using a feature from the system's dynamic loader.
   *        - on some other platforms, including OpenBSD and OSX, libraries are loaded
   *          first, then plugins are created using a different mechanism than the one
   *          above (i.e. with an explicit constructor).
   *
   *        The PM serves a certain number of functionalities available throughout Seeks,
   *        such as plugin lookup, plugin information, ...
   */
  class plugin_manager
  {
    public:

      // dynamic library loading, and plugin autoregistration.

      /**
       * \brief loads all plugins, that is lookup shared libraries, load them up,
       *        then create plugin objects and register them.
       */
      static int load_all_plugins();

      /**
       * \brief destroys all plugin objects and unload shared libraries.
       */
      static int close_all_plugins();

      /**
       * \brief creates the plugin objects.
       */
      static int instanciate_plugins();

      /**
       * \brief registers a plugin and its CGI functions.
       * @param p plugin object.
       */
      static void register_plugin(plugin *p);

      /**
       * \brief locates a CGI resource among the registered plugins, if any
       *        (resource or plugin).
       * @param path CGI resource path.
       * @return CGI dispatcher for this resource if it exists, NULL otherwise.
       * @see cgi main mechanism in Seeks proxy.
       */
      static cgi_dispatcher* find_plugin_cgi_dispatcher(const char *path);

      /**
       * \brief determines which plugins are activated by a client request.
       *        Activated plugins are added to the client request state object.
       * @param csp HTTP client requeset state.
       * @param http HTTP request.
       */
      static void get_url_plugins(client_state *csp, http_request *http);

      /**
       * \brief finds a plugin with its name.
       * @param name plugin name.
       * @return plugin if it exists under the requested name, NULL otherwise.
       */
      static plugin* get_plugin(const std::string &name);

    public:
      static std::vector<plugin*> _plugins;  /**< set of plugins. */
      static std::vector<interceptor_plugin*> _ref_interceptor_plugins; /**< registered interceptor plugins. */
      static std::vector<action_plugin*> _ref_action_plugins; /**< registered action plugins. */
      static std::vector<filter_plugin*> _ref_filter_plugins; /**< registered filter plugins. */

      static hash_map<const char*,cgi_dispatcher*,hash<const char*>,eqstr> _cgi_dispatchers; /**< registered CGI dispatchers, key is resource. */

      static std::string _config_html_template; /**< configuration HTML page. */

      static std::string _plugin_repository; /**< plugin repository. */

    public:
      static std::map<std::string,maker_ptr*,std::less<std::string> > _factory; /**< factory of plugins. */

    private:
      static std::list<void*> _dl_list; /**< list of opened dynamic (shared) libraries. */
      static std::map<std::string,configuration_spec*,std::less<std::string> > _configurations; /**< plugin configuration objects. */
  };

} /* end of namespace. */

#endif
