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

#ifndef PLUGIN_H
#define PLUGIN_H

#include "plugin_manager.h"
#include "proxy_dts.h"
#include "configuration_spec.h"

#include <string>

namespace sp
{
   class interceptor_plugin;
   class action_plugin;
   class filter_plugin;
   
   //-> into plugin_element.
  /**
   * !! Plugin auto-registration requires an inside class C style
   *    function, here called maker():
   * extern "C" {
   *  plugin* maker() 
   * {
   *  return new your_plugin;
   * }
   */
   
   class plugin
     {
      public:
	// constructor, used in maker.
	plugin(); 
	
	plugin(const std::string &config_filename);
	
	// destructor.
	virtual ~plugin();
	
	char* print() const;
	
	/* start/stop plugin. */
	virtual void  start() {};
	
	virtual void stop() {};
	
	/* accessors. */
	std::string get_name() const { return _name; };
	const char* get_name_cstr() const { return _name.c_str(); };
	std::string get_description() const { return _description; };
	const char* get_description_cstr() const { return _description.c_str(); };
	std::string get_version_major() const { return _version_major; };
	std::string get_version_minor() const { return _version_minor; };

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
	std::vector<cgi_dispatcher*> _cgi_dispatchers;
	
      public:
	// interception, parsing & filtering.
	interceptor_plugin *_interceptor_plugin;
	action_plugin *_action_plugin;
	filter_plugin *_filter_plugin;
     };

} /* end of namespace. */

#endif
