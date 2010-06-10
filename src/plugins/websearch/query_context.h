/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009, 2010 Emmanuel Benazera, juban@free.fr
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

typedef pthread_mutex_t sp_mutex_t;

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
	 * \brief perform expansion.
	 */
	sp_err expand(client_state *csp,
		      http_response *rsp,
		      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
		      const int &page_start, const int &page_end,
		      const std::bitset<NSEs> &se_enabled);
		
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
					       std::string &query, std::string &url_enc_query);
	
	/**
	 * \brief synchronizes qc's parameters with parameters.
	 */
	void update_parameters(hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
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
	 * @param lctitle lower-case title.
	 */
	search_snippet* get_cached_snippet_title(const char *lctitle);

	/**
	 * \brief detects whether a query contain a language command and 
	 *        fills up the language parameter.
	 * @return true if it does, false otherwise.
	 */
	static bool has_query_lang(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
				   std::string &qlang);
	
	/**
	 * \brief detect query language from the query special keywords, :en, :fr, ...
	 * @return true if the language could be detected that way, false otherwise.
	 */
	bool detect_query_lang(hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	/**
	 * \brief detect query language, using http headers.
	 */
	static std::string detect_query_lang_http(const std::list<const char*> &http_headers);
	
	/**
	 * \brief grab useful HTTP headers from the client.
	 */
	void grab_useful_headers(const std::list<const char*> &http_headers);

	/**
	 * \brief conversion to forced region, from language.
	 */
	static std::string lang_forced_region(const std::string &auto_lang);
	
	/**
	 * \brief force regions.
	 */
	static void in_query_command_forced_region(std::string &auto_lang,
						   std::string &region_lang);
	
	/**
	 * \brief generates the HTTP language header.
	 */
	std::string generate_lang_http_header() const;
	
	/**
	 * \brief fill up activated search engines from parameters or configuration.
	 */
	static void fillup_engines(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
				   std::bitset<NSEs> &engines);
	
      public:
	std::string _query;
	std::string _url_enc_query;
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
	std::string _auto_lang; // lang, e.g. en.
	std::string _auto_lang_reg; // lang-region, e.g. en-US.
	
	/* other HTTP headers, useful when interrogating search engines. */
	std::list<const char*> _useful_http_headers;
		
	/* in-query command. */
	std::string _in_query_command;
     
	/* query tokenizing and hashing delimiters. */
	static std::string _query_delims;
     
	/* mutex for threaded work on the context. */
	sp_mutex_t _qc_mutex;
     
	/* search engines used in this context. */
	std::bitset<NSEs> _engines;
     };
      
} /* end of namespace. */

#endif
