/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 **/
 
#ifndef QUERY_CONTEXT_H
#define QUERY_CONTEXT_H

#include "sweeper.h"
#include "proxy_dts.h"
#include "search_snippet.h"

#include <time.h>

using sp::sweepable;
using sp::sp_err;
using sp::client_state;
using sp::http_response;

namespace seeks_plugins
{   
   class query_context : public sweepable
     {
      public:
	query_context(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	virtual ~query_context();
     
	/**
	 * \brief generates search result snippets, either from querying the
	 * search engines, or by looking up the current query context's cache.
	 */
	sp_err generate(client_state *csp,
			http_response *rsp,
			const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	/**
	 * \brief regenerates search result snippets as necessary.
	 */
	sp_err regenerate(client_state *csp,
			  http_response *rsp,
			  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	/**
	 * virtual call to evaluate the sweeping condition.
	 */
	virtual bool sweep_me();

	/**
	 * registers context with the websearch plugin.
	 */
	void register_qc();

	/**
	 * unregisters from the websearch plugin.
	 */
	void unregister();
	
	/**
	 * update last time of use.
	 */
	void update_last_time();
	
	/**
	 * \brief query hashing, based on mrf in lsh.
	 * grabs query from parameters, stores it into query and hashes it.
	 * the 32 bit hash is returned.
	 */
	static uint32_t hash_query_for_context(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					       std::string &query);
	
	/**
	 * \brief cache a url's content for fast access by user.
	 * Ties are reduced by keeping the content with largest size.
	 */
	void cache_url(const std::string &url, const std::string &url_content);
	
	/**
	 * \brief checks whether a url is already cached.
	 */
	bool is_cached(const std::string &url);

	/**
	 * \brief updates unordered cached snippets. This set is for fast access
	 * and update to the snippets in the cache.
	 */
	void update_unordered_cache();
	
	/**
	 * \brief finds and updates a search snippet's seeks rank.
	 */
	void update_snippet_seeks_rank(const char *url,
				       const double &rank);
	
      public:
	std::string _query;
	uint32_t _query_hash;
	uint32_t _page_expansion; // expansion as fetched pages from the search engines.
		
	/* cache. */
	std::vector<search_snippet*> _cached_snippets;
	hash_map<const char*,search_snippet*,hash<const char*>,eqstr> _unordered_snippets; // cached snippets ptr, url is the key.
	hash_map<const char*,std::string,hash<const char*>,eqstr> _cached_urls; // cached content, url is the key.
	
	/* timer. */
	time_t _creation_time;
	time_t _last_time_of_use;

	/* others. */
	hash_map<int,std::string> _cuil_pages; // hack to grab the next pages for a search query
	                                       // on cuil. No other way that I know but to grab
					       // them from the webpage and store them here.

	std::vector<std::string> _suggestions; // suggered related queries.
     };
      
} /* end of namespace. */

#endif
