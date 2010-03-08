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
   
   class plugin_manager
     {
      public:
	
	// dynamic library loading, and plugin autoregistration.
	static int load_all_plugins();
	static int close_all_plugins();
	
	// creates the plugin objects.
	static int instanciate_plugins();

	// registers a plugin and its CGI functions.
	static void register_plugin(plugin *p);
	static cgi_dispatcher* find_plugin_cgi_dispatcher(const char *path);
	
	// determines which plugins are activated by a client request.
	static void get_url_plugins(client_state *csp, http_request *http);
	
      public:
	static std::vector<plugin*> _plugins;
	static std::vector<interceptor_plugin*> _ref_interceptor_plugins; // referenced interceptor plugins.
	static std::vector<action_plugin*> _ref_action_plugins; // referenced action plugins.
	static std::vector<filter_plugin*> _ref_filter_plugins; // referenced filter plugins.

	static hash_map<const char*,cgi_dispatcher*,hash<const char*>,eqstr> _cgi_dispatchers;
	
	// configuration page.
	static std::string _config_html_template;
	
	static std::string _plugin_repository; /**< plugin repository. */

      public:
	static std::map<std::string,maker_ptr*,std::less<std::string> > _factory; // factory of plugins.
	
      private:
	static std::list<void*> _dl_list; // list of opened dynamic libs.
	static std::map<std::string,configuration_spec*,std::less<std::string> > _configurations; // plugin configuration objects.
     };
   
} /* end of namespace. */

#endif
