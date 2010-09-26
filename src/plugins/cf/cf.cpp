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
#include "rank_estimators.h"
#include "plugin_manager.h"

using sp::plugin_manager;

namespace seeks_plugins
{
   
   cf::cf()
     :plugin()
       {
	  _name = "cf";
	  _version_major = "0";
	  _version_minor = "1";
	  
	  //TODO: configuration.
       }
   
   cf::~cf()
     {
     }
      
   void cf::start()
     {
	//TODO: check on user_db.
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
	     return new query_capture;
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
