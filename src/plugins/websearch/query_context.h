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
#include "LSHUniformHashTableHamming.h" // for regrouping urls, titles and other text snippets.
#include "stl_hash.h"

#include <time.h>

using sp::sweepable;
using sp::sp_err;
using sp::client_state;
using sp::http_response;
using lsh::LSHSystemHamming;
using lsh::LSHUniformHashTableHamming;

namespace seeks_plugins
{   
   class query_context : public sweepable
     {
      public:
	/**
	 * \brief Dummy constructor. Testing purposes.
	 */
	query_context();
	
	/**
	 * \brief Constructor.
	 */
	query_context(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
		      const std::list<const char*> &http_headers);
	
	/**
	 * \brief destructor.
	 */
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
	 * sorts query words so that this context is query word order independent.
	 */
	static std::string sort_query(const std::string &query);
	
	/**
	 * \brief query hashing, based on mrf in lsh.
	 * grabs query from parameters, stores it into query and hashes it.
	 * the 32 bit hash is returned.
	 */
	static uint32_t hash_query_for_context(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					       std::string &query);
	
	/**
	 * \brief adds a snippet to the unordered cache set.
	 */
	void add_to_unordered_cache(search_snippet *sr);
	
	/**
	 * \brief updates unordered cached snippets. This set is for fast access
	 * and update to the snippets in the cache.
	 */
	void update_unordered_cache();
	
	/**
	 * \brief finds and updates a search snippet's seeks rank.
	 */
	void update_snippet_seeks_rank(const uint32_t &id,
				       const double &rank);

	/**
	 * \brief returns a cached snippet if it knows it, NULL otherwise.
	 */
	search_snippet* get_cached_snippet(const std::string &url) const;
	
	/**
	 * \brief returns a cached snippet if it knows it, NULL otherwise.
	 */
	search_snippet* get_cached_snippet(const uint32_t &id)  const;
	
	/**
	 * \brief adds a snippet to the unordered cache set.
	 */
	void add_to_unordered_cache_title(search_snippet *sr);
	
	/**
	 * \brief returns a cached snippet if it knows it, NULL otherwise.
	 */
	search_snippet* get_cached_snippet_title(const char *title);

	/**
	 * \brief detect query language, using http headers.
	 */
	static std::string detect_query_lang_http(const std::list<const char*> &http_headers);
	
	/**
	 * \brief grab useful HTTP headers from the client.
	 */
	void grab_useful_headers(const std::list<const char*> &http_headers);
	
      public:
	std::string _query;
	uint32_t _query_hash;
	uint32_t _page_expansion; // expansion as fetched pages from the search engines.
		
	/* cache. */
	std::vector<search_snippet*> _cached_snippets;
	hash_map<uint32_t,search_snippet*,id_hash_uint> _unordered_snippets; // cached snippets ptr, key is the hashed url = snippet id.
	hash_map<const char*,search_snippet*,hash<const char*>,eqstr> _unordered_snippets_title; // cached snippets ptr, title is the key.
	hash_map<const char*,const char*,hash<const char*>,eqstr> _cached_urls; // cached content, url is the key.
		
	/* timer. */
	time_t _creation_time;
	time_t _last_time_of_use;

	/* others. */
	hash_map<int,std::string> _cuil_pages; // hack to grab the next pages for a search query
	                                       // on cuil. No other way that I know but to grab
					       // them from the webpage and store them here.

	std::vector<std::string> _suggestions; // suggered related queries.

	/* LSH subsystem for regrouping textual elements. */
	LSHSystemHamming *_lsh_ham;
	LSHUniformHashTableHamming *_ulsh_ham;

	/* locking. */
	bool _lock;

	/* tfidf feature computation flag. */
	bool _compute_tfidf_features;

	/* automatic language detection. */
	std::string _auto_lang;

	/* other HTTP headers, useful when interrogating search engines. */
	std::list<const char*> _useful_http_headers;
     };
      
} /* end of namespace. */

#endif
