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

#ifndef CF_H
#define CF_H

#include "plugin.h"
#include "search_snippet.h"

using sp::plugin;

namespace seeks_plugins
{

  class cf : public plugin
  {
    public:
      cf();

      virtual ~cf();

      virtual void start();

      virtual void stop();

      void estimate_ranks(const std::string &query,
                          std::vector<search_snippet*> &snippets);
      
      void get_related_queries(const std::string &query,
			       std::multimap<double,std::string,std::less<double> > &related_queries);
      
      void get_recommended_urls(const std::string &query,
				hash_map<uint32_t,search_snippet*,id_hash_uint> &snippets);
      
    public:
      static plugin *_uc_plugin;
  };

} /* end of namespace. */

#endif
