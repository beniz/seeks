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

#include "plugin.h"
#include "mem_utils.h"
#include "interceptor_plugin.h"
#include "action_plugin.h"
#include "filter_plugin.h"
#include "plugin_manager.h" // to get access to the factory.
#include "errlog.h"
#include "urlmatch.h"
#include "loaders.h"
#include "miscutil.h"

namespace sp
{
   plugin::plugin()
     :_interceptor_plugin(NULL),_action_plugin(NULL),_filter_plugin(NULL)
     {	
     }
   
   plugin::plugin(const std::string &config_filename)
     : _config_filename(config_filename),
       _interceptor_plugin(NULL),_action_plugin(NULL),
       _filter_plugin(NULL)
     {
     }
   
   plugin::~plugin()
     {
	if (_interceptor_plugin)
	  delete _interceptor_plugin;
	if (_action_plugin)
	  delete _action_plugin;
	if (_filter_plugin)
	  delete _filter_plugin;

	for (size_t i=0;i<_cgi_dispatchers.size();i++)
	  delete _cgi_dispatchers.at(i);
     
	if (_configuration)
	  delete _configuration;
     }
   
   char* plugin::print() const
     {
	char *res = NULL;
	char *s = NULL;
		
	if (_interceptor_plugin)
	  s = _interceptor_plugin->print();
	miscutil::string_append(&res,s);
	freez(s);
	
	if (_action_plugin)
	  s = _action_plugin->print();
	miscutil::string_append(&res,s);
	freez(s);
	
	if (_filter_plugin)
	  s = _filter_plugin->print();
	miscutil::string_append(&res,s);
	freez(s);
	
	return res;
     }
   
} /* end of namespace. */
