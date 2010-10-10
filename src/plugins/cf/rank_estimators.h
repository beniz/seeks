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
  
#ifndef RANK_ESTIMATORS_H
#define RANK_ESTIMATORS_H

#include "search_snippet.h"
#include "db_query_record.h"

namespace seeks_plugins
{
   
   class rank_estimator
     {
      public:
	rank_estimator() {};
	
	virtual ~rank_estimator() {};
	
	virtual void estimate_ranks(const std::string &query,
				    std::vector<search_snippet*> &snippets) {};
	
	void fetch_user_db_record(const std::string &query,
				  std::vector<db_record*> &records);
     };
      
   class simple_re : public rank_estimator
     {
      public:
	simple_re();
	
	virtual ~simple_re();
	
	void extract_queries(const std::vector<db_record*> &records,
			     hash_map<const char*,query_data*,hash<const char*>,eqstr> &qdata);
	
	virtual void estimate_ranks(const std::string &query,
				    std::vector<search_snippet*> &snippets);
	
	float estimate_rank(search_snippet *s, const int &ns,
			    const query_data *qd,
			    const float &total_hits,
			    const std::string &surl,
			    const std::string &host, const std::string &path);
     
	float estimate_prior(const std::string &surl,
			     const std::string &host,
			     const uint64_t &nuri,
			     bool &personalized);
     };
      
} /* end of namespace. */

#endif
