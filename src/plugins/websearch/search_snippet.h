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

#ifndef SEARCH_SNIPPET_H
#define SEARCH_SNIPPET_H

#include "websearch_configuration.h" // for NSEs.
#include "proxy_dts.h" // for url_spec.

#include <string>
#include <vector>
#include <bitset>
#include <algorithm>
#include <ostream>

using sp::url_spec;

namespace seeks_plugins
{
   class query_context;
   
   enum DOC_TYPE
     {
	UNKNOWN,
	WEBPAGE,
	FORUM,
	FILE_DOC,
	SOFTWARE,
	IMAGE,
	VIDEO,
	AUDIO,
	CODE,
	NEWS,
	REAL_TIME,
	WIKI,
	REJECTED /* user reject, for now by matching a regexp. */
     };
   
   class search_snippet
     {
      public:
	// comparison functions.
	static bool less_url(const search_snippet *s1, const search_snippet *s2)
	  {
	     return std::lexicographical_compare(s1->_url.begin(),s1->_url.end(),
						 s2->_url.begin(),s2->_url.end());
	  };
	
	static bool equal_url(const search_snippet *s1, const search_snippet *s2)
	  {
	     return s1->_url == s2->_url;
	  };
	
	static bool less_seeks_rank(const search_snippet *s1, const search_snippet *s2)
	  {
	     if (s1->_seeks_rank == s2->_seeks_rank)
	       return s1->_rank / static_cast<double>(s1->_engine.count()) 
		 < s2->_rank / static_cast<double>(s2->_engine.count());
	     else
	       return s1->_seeks_rank < s2->_seeks_rank;
	  };

	static bool max_seeks_rank(const search_snippet *s1, const search_snippet *s2)
	  {
	     if (s1->_seeks_rank == s2->_seeks_rank)
	       return s1->_rank / static_cast<double>(s1->_engine.count()) 
		 < s2->_rank / static_cast<double>(s2->_engine.count());  // beware: min rank is still better.
	     else
	       return s1->_seeks_rank > s2->_seeks_rank;  // max seeks rank is better.
	  };

	static bool max_seeks_ir(const search_snippet *s1, const search_snippet *s2)
	  {
	     if (s1->_seeks_ir == s2->_seeks_ir)
	       return search_snippet::max_seeks_rank(s1,s2);
	     else return s1->_seeks_ir > s2->_seeks_ir;
	  };
	
	static bool min_seeks_ir(const search_snippet *s1, const search_snippet *s2)
	  {
	     if (s1->_seeks_ir == s2->_seeks_ir)
	       return search_snippet::less_seeks_rank(s1,s2); // XXX: beware, may not apply to inherited classes.
	     else return s1->_seeks_ir < s2->_seeks_ir;
	  };
		
	// constructors.
      public:
	search_snippet();
	search_snippet(const short &rank);
	
	~search_snippet();
	
	// set_url.
	void set_url(const std::string &url);
	void set_url(const char *url);
	void set_url_no_decode(const std::string &url);
	
	void set_cite(const std::string &cite);
	
	void set_summary(const std::string &summary);
	void set_summary(const char *summary);
	
	// sets a link to the archived url at archive.org (e.g. in case we'no cached link).
	void set_archive_link();

	// sets a link to a sorting of snippets wrt. to their similarity to this snippet.
	virtual void set_similarity_link();

	// sets a back link when similarity is engaged.
	virtual void set_back_similarity_link();
	
	// json output.
	virtual std::string to_json(const bool &thumbs);
	
	// html output for inclusion in search result template page.
	std::string to_html();
	virtual std::string to_html_with_highlight(std::vector<std::string> &words,
						   const std::string &base_url);

	// whether this snippet's engine(s) is(are) enabled.
	// used in result page rendering.
	virtual bool is_se_enabled(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
	
	// static functions.
	// highlights terms within the argument string.
	static void highlight_query(std::vector<std::string> &words,
				    std::string &str);
	
	// highlights the most discriminative terms (for this snippet among all snippets).
	void highlight_discr(std::string &str, const std::string &base_url_str,
			     const std::vector<std::string> &query_words);
	
	// tag snippet, i.e. detect its type if not already done by the parsers.
	void tag();

	// load tagging patterns from files.
	static sp_err load_patterns();

	// match url against tags.
	static bool match_tag(const std::string &url,
			      const std::vector<url_spec*> &patterns);
	
	// delete snippets.
	static void delete_snippets(std::vector<search_snippet*> &snippets);
	
	// merging of snippets (merge s2 into s2, according to certain rules).
	static void merge_snippets(search_snippet *s1, const search_snippet *s2);
	
	// printing output.
	std::ostream& print(std::ostream &output);
	
      public:
	query_context *_qc; // query context the snippet belongs to.
	bool _new; // true before snippet is processed.
	uint32_t _id; // snippet id as hashed url.
	
	std::string _title;
	std::string _url;
	std::string _cite;
	std::string _cached;
	std::string _summary;
	std::string _file_format;
	std::string _date;
	std::string _lang;
	std::string _archive; // a link to archive.org
	std::string _sim_link; // call to similarity sorting.
	bool _sim_back; // whether the back 'button' to similarity is present.
	
	double _rank;  // search engine rank.
	double _seeks_ir; // IR score computed locally.
	double _seeks_rank; // rank computed locally.
	
	std::bitset<NSEs> _engine;  // engines from which it was created (if not directly published).
	enum DOC_TYPE _doc_type;
	
	// type-dependent information.
	std::string _forum_thread_info;

	// cache.
	std::string *_cached_content;
	std::vector<uint32_t> *_features; // temporary set of features, used for fast similarity check between snippets.
	hash_map<uint32_t,float,id_hash_uint> *_features_tfidf; // tf-idf feature set for this snippet.
	hash_map<uint32_t,std::string,id_hash_uint> *_bag_of_words;	
     
	// patterns for snippets tagging (positive patterns for now only).
	static std::vector<url_spec*> _pdf_pos_patterns;
	static std::vector<url_spec*> _file_doc_pos_patterns;
	static std::vector<url_spec*> _audio_pos_patterns;
	static std::vector<url_spec*> _video_pos_patterns;
	static std::vector<url_spec*> _forum_pos_patterns;
	static std::vector<url_spec*> _reject_pos_patterns;
     };
   
} /* end of namespace. */
   
#endif
