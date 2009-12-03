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

#ifndef SEARCH_SNIPPET_H
#define SEARCH_SNIPPET_H

#include "websearch_configuration.h"

#include <string>
#include <vector>
#include <bitset>
#include <algorithm>
#include <ostream>

namespace seeks_plugins
{
   
   enum DOC_TYPE
     {
	UNKNOWN,
	WEBPAGE,
	FORUM,
	FILE_DOC,
	SOFTWARE,
	VIDEO,
	CODE,
	NEWS,
	REAL_TIME
     };
   
   class search_snippet
     {
      public:
	// comparison functions.
	static bool less_url(const search_snippet *s1, const search_snippet *s2)
	  {
	     return std::lexicographical_compare(s1->_url.begin(),s1->_url.end(),
						 s2->_url.begin(),s2->_url.end());
	  }
	
	static bool equal_url(const search_snippet *s1, const search_snippet *s2)
	  {
	     return s1->_url == s2->_url;
	  }
	
	static bool less_seeks_rank(const search_snippet *s1, const search_snippet *s2)
	  {
	     if (s1->_seeks_rank == s2->_seeks_rank)
	       return s1->_rank < s2->_rank;
	     else
	       return s1->_seeks_rank < s2->_seeks_rank;
	  }

	static bool max_seeks_rank(const search_snippet *s1, const search_snippet *s2)
	  {
	     if (s1->_seeks_rank == s2->_seeks_rank)
	       return s1->_rank < s2->_rank;  // beware: min rank is still better.
	     else
	       return s1->_seeks_rank > s2->_seeks_rank;  // max seeks rank is better.
	  }
	
	// constructors.
      public:
	search_snippet();
	search_snippet(const short &rank);
	
	~search_snippet();
	
	// set_url with url preprocessing for later comparison.
	void set_url(const std::string &url);
	void set_url(const char *url);
	void set_summary(const std::string &summary);
	void set_summary(const char *summary);
	
	// xml output.
	
	// html output for inclusion in search result template page.
	std::string to_html() const;
	std::string to_html_with_highlight(std::vector<std::string> &words) const;

	// static functions.
	// highlights terms within the argument string.
	static void highlight_query(std::vector<std::string> &words,
				    std::string &str);
	
	// delete snippets.
	static void delete_snippets(std::vector<search_snippet*> &snippets);
	
	// merging of snippets (merge s2 into s2, according to certain rules).
	static void merge_snippets(search_snippet *s1, const search_snippet *s2);
	
	// printing output.
	std::ostream& print(std::ostream &output);
	
      public:
	std::string _title;
	std::string _url;
	std::string _cite;
	std::string _cached;
	std::string _summary;
	std::string _file_format;
	std::string _date;
	std::string _lang;
	
	short _rank;  // search engine rank.
	double _seeks_ir; // IR score computed locally.
	short _seeks_rank; // rank computed locally.
	
	std::bitset<NSEs> _engine;  // engines from which it was created (if not directly published).
	enum DOC_TYPE _doc_type;
	
	// type-dependent information.
	std::string _forum_thread_info;
     };
   
} /* end of namespace. */
   
#endif
