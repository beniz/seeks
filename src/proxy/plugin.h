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

#ifndef PLUGIN_H
#define PLUGIN_H

#include "plugin_manager.h"
#include "proxy_dts.h"
#include "configuration_spec.h"

#if defined(PROTOBUF) && defined(TC)
#include "db_record.h"
#endif

#include <string>

namespace sp
{
  class interceptor_plugin;
  class action_plugin;
  class filter_plugin;

  /**
  * !! Plugin auto-registration requires an inside class C style
  *    function, here called maker():
  * extern "C" {
  *  plugin* maker()
  * {
  *  return new your_plugin;
  * }
  */

  /**
   * \brief main plugin class. A plugin is a shared library with either or both:
   *        - internal plugins such as URI interceptors or content filters.
   *        - a set of CGI callbacks for performing certain informations.
   * e.g. websearch and httpserv are two plugins, the first implementing a metasearch
   * engine, the second a lightweight HTTP web server based on libevent.
   *
   * Plugins form the modular part of the Seeks platform. Plugins are themselves
   * modular throught plugin_element that can be specialized in different manner
   * (e.g. URI interceptors, ...).
   *
   * The way it works is that the plugin_element are activated by a set of regexp
   * patterns. This allows a plugin to respond differently to a series of different
   * intput / outputs, on top of the Seeks platform.
   */
  class plugin
  {
    public:
      /**
       * \brief constructor, also called in automatic maker function
       *        in auto-registration of plugin dynamic libraries.
       */
      plugin();

      /**
       * \brief constructor.
       * @param config_filename configuration filename.
       */
      plugin(const std::string &config_filename);

      /**
       * \brief destructor.
       */
      virtual ~plugin();

      /**
       * \brief builds a string of plugin element's printing functions
       *        output.
       * @return a string, user is responsible for freeing it.
       */
      std::string print() const;

      /**
       * \brief actions taken when starting plugin.
       */
      virtual void  start() {};

      /**
       * \brief actions taken when stopping plugin.
       */
      virtual void stop() {};

      /* accessors. */

      /**
       * \brief get plugin name.
       * @return plugin name.
       */
      std::string get_name() const
      {
        return _name;
      };

      /**
       * \brief get plugin name in a C-string.
       * @return plugin name.
       */
      const char* get_name_cstr() const
      {
        return _name.c_str();
      };

      /**
       * \brief get plugin description.
       * @return plugin description.
       */
      std::string get_description() const
      {
        return _description;
      };

      /**
       * \brief get plugin description in a C-string.
       * @return plugin description.
       */
      const char* get_description_cstr() const
      {
        return _description.c_str();
      };

      /**
       * \brief get plugin version major label.
       * @return plugin version major.
       */
      std::string get_version_major() const
      {
        return _version_major;
      };

      /**
       * \brief get plugin version minor label.
       * @return plugin version minor.
       */
      std::string get_version_minor() const
      {
        return _version_minor;
      };

#if defined(PROTOBUF) && defined(TC)
      /* user db related, when needed for a plugin to store user data. */
      virtual db_record* create_db_record()
      {
        return NULL;
      };
#endif

    protected:
      std::string _name; /**< plugin name. */
      std::string _description; /**< plugin description. */

      // versioning.
      std::string _version_major;  /**< plugin version major number. */
      std::string _version_minor;  /**< plugin version minor number. */

    public:
      // configuration.
      std::string _config_filename; /**< local plugin configuration page. */
      configuration_spec *_configuration; /**< configuration object. */

      // CGI calls.
    public:
      std::vector<cgi_dispatcher*> _cgi_dispatchers; /**< list of plugin's CGI dispatchers. */

    public:
      // interception, parsing & filtering.
      interceptor_plugin *_interceptor_plugin; /**< internal interceptor plugin (interception from proxy). */
      action_plugin *_action_plugin;
      filter_plugin *_filter_plugin;  /**< internal filtering plugin. */
  };

} /* end of namespace. */

#endif
