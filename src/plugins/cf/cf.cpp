/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "cf.h"
#include "cf_configuration.h"
#include "rank_estimators.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "plugin_manager.h"

#include <sys/stat.h>
#include <iostream>

using sp::seeks_proxy;
using sp::proxy_configuration;
using sp::plugin_manager;

namespace seeks_plugins
{
   
   plugin* cf::_uc_plugin = NULL;
      
   cf::cf()
     :plugin()
       {
	  _name = "cf";
	  _version_major = "0";
	  _version_minor = "1";
	  
	  // configuration.
	  if (seeks_proxy::_datadir.empty())
	    _config_filename = plugin_manager::_plugin_repository + "cf/cf-config";
	  else
	    _config_filename = seeks_proxy::_datadir + "/plugins/cf/cf-config";
       
#ifdef SEEKS_CONFIGDIR
	  struct stat stFileInfo;
	  if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
	    {
	       _config_filename = SEEKS_CONFIGDIR "/cf-config";
	    }
#endif
	  
	  if (cf_configuration::_config == NULL)
	    cf_configuration::_config = new cf_configuration(_config_filename);
	  _configuration = cf_configuration::_config;
       }
   
   cf::~cf()
     {
     }
      
   void cf::start()
     {
	//TODO: check on user_db.
     
	// look for dependent plugins.
	cf::_uc_plugin = plugin_manager::get_plugin("uri-capture");
     }
   
   void cf::stop()
     {
     }

   void cf::estimate_ranks(const std::string &query,
			   std::vector<search_snippet*> &snippets)
     {
	simple_re sre; // estimator.
	sre.estimate_ranks(query,snippets);
     }
   
#if defined(ON_OPENBSD) || defined(ON_OSX)
   extern "C"
     {
	plugin* maker()
	  {
	     return new cf;
	  }
     }
#else
   plugin* makercf()
     {
	return new cf;
     }
   class proxy_autor_cf
     {
      public:
	proxy_autor_cf()
	  {
	     plugin_manager::_factory["cf"] = makercf; // beware: default plugin shell with no name.
	  }
     };
   proxy_autor_cf _p; // one instance, instanciated when dl-opening.
#endif
   
   
} /* end of namespace. */
