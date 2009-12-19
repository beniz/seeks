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

#include "sort_rank.h"
#include "websearch.h"
#include "content_handler.h"

#include <algorithm>
#include <iterator>
#include <iostream>

namespace seeks_plugins
{
   
   void sort_rank::sort_and_merge_snippets(std::vector<search_snippet*> &snippets,
					   std::vector<search_snippet*> &unique_snippets)
     {
	// sort snippets by url.
	std::sort(snippets.begin(),snippets.end(),search_snippet::less_url);
	
	// create a set of unique snippets (by url.).
	std::unique_copy(snippets.begin(),snippets.end(),
			 std::back_inserter(unique_snippets),search_snippet::equal_url);
     }
   
   void sort_rank::sort_merge_and_rank_snippets(query_context *qc, std::vector<search_snippet*> &snippets)
						//std::vector<search_snippet*> &unique_ranked_snippets)
     {
	static double st = 0.9; // similarity threshold.
	
	// copy original vector.
	//std::copy(snippets.begin(),snippets.end(),std::back_inserter(unique_ranked_snippets));
	
	// sort snippets by url.
	std::sort(snippets.begin(),snippets.end(),
		  search_snippet::less_url);
	
	std::vector<search_snippet*>::iterator it = snippets.begin();
	std::string c_url = "";
	std::string c_title = "";
	search_snippet *c_sp = NULL;
	
	while(it != snippets.end())
	  {
	     search_snippet *sp = (*it);
	     if (sp->_url.compare(c_url) == 0)  // same url as before.
	       {
		  // merging snippets.
		  search_snippet::merge_snippets(c_sp,sp);
		  c_sp->_seeks_rank = c_sp->_engine.count();
		  it = snippets.erase(it);
		  delete sp;
		  sp = NULL;
		  continue;
	       }
	     else if (websearch::_wconfig->_content_analysis
		      && sp->_title.compare(c_title) == 0) // same title as before.
	       {
		  bool same = content_handler::has_same_content(qc,sp->_url,c_sp->_url,st);
		  if (same)
		    {
		       search_snippet::merge_snippets(c_sp,sp);
		       c_sp->_seeks_rank = c_sp->_engine.count();
		       it = snippets.erase(it);
		       delete sp;
		       sp = NULL;
		       continue;
		    }
	       }
	     
	     //debug
	     //std::cerr << "new url scanned: " << sp->_url << std::endl;
	     //debug
	     
	     c_url = sp->_url;
	     c_title = sp->_title;
	     c_sp = sp;
	     c_sp->_seeks_rank = c_sp->_engine.count();
	     
	     ++it;
	  } // end while.
		
	// sort by rank.
	std::sort(snippets.begin(),snippets.end(),
		  search_snippet::max_seeks_rank);
	
	//debug
	/* std::cerr << "[Debug]: sorted result snippets:\n";
	it = snippets.begin();
	while(it!=snippets.end())
	  {
	     (*it)->print(std::cerr);
	     it++;
	  } */
	//debug
     }

   /* advanced sorting and scoring, based on webpages content. */
   /* void sort_rank::retrieve_and_score(query_context *qc)
     {
	// fetch content.
	size_t ncontents = websearch::_wconfig->_N;
	std::vector<std::string> urls;
	for (size_t i=0;i<ncontents;i++)
	  {
	     if (!qc->is_cached(qc->_cached_snippets.at(i)->_url))
	       urls.push_back(qc->_cached_snippets.at(i)->_url);
	  }
	ncontents = urls.size();
	
	char **outputs = content_handler::fetch_snippets_content(qc,ncontents,urls);
		
	// parse content and keep text only.
	std::string *txt_contents = content_handler::parse_snippets_txt_content(ncontents,
										outputs);
	delete[] outputs;
	
	// extract features from fetched text.
	hash_map<const char*,std::vector<uint32_t>*,hash<const char*>,eqstr> features;
	content_handler::extract_features_from_snippets(qc,txt_contents,ncontents,urls,
							features);
	
	// compute score using the extracted features.
	content_handler::feature_based_scoring(qc,features);
     
	// destroy feature sets.
	content_handler::delete_features(features);
     } */
   
} /* end of namespace. */
