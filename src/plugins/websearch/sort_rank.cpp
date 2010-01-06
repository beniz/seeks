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
#include "urlmatch.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <iostream>

#include <assert.h>

using sp::urlmatch;

namespace seeks_plugins
{
   
   void sort_rank::sort_and_merge_snippets(std::vector<search_snippet*> &snippets,
					   std::vector<search_snippet*> &unique_snippets)
     {
	// sort snippets by url.
	std::stable_sort(snippets.begin(),snippets.end(),search_snippet::less_url);
	
	// create a set of unique snippets (by url.).
	std::unique_copy(snippets.begin(),snippets.end(),
			 std::back_inserter(unique_snippets),search_snippet::equal_url);
     }
   
   void sort_rank::sort_merge_and_rank_snippets(query_context *qc, std::vector<search_snippet*> &snippets)
     {
	static double st = 0.9; // similarity threshold.
	
	// initializes the LSH subsystem is we need it and it has not yet
	// been initialized.
	if (websearch::_wconfig->_content_analysis
	    && !qc->_ulsh_ham)
	  {
	     /**
	      * The LSH system based on a Hamming distance has a fixed maximum size
	      * for strings, it is set to 50. Therefore, k should be set accordingly,
	      * that is below 50 and in proportion to the 'fuzziness' necessary for
	      * k-nearest neighbors computation.
	      */
	     qc->_lsh_ham = new LSHSystemHamming(35,5);
	     qc->_ulsh_ham = new LSHUniformHashTableHamming(*qc->_lsh_ham,
							    websearch::_wconfig->_N*3*NSEs);
	  }
	
	std::vector<search_snippet*>::iterator it = snippets.begin();
	search_snippet *c_sp = NULL;
	while(it != snippets.end())
	  {
	     search_snippet *sp = (*it);
	     if (sp->_new)
	       {
		  if ((c_sp = qc->get_cached_snippet(sp->_url.c_str()))!=NULL)
		    {
		       // merging snippets.
		       search_snippet::merge_snippets(c_sp,sp);
		       c_sp->_seeks_rank = c_sp->_engine.count();
		       it = snippets.erase(it);
		       delete sp;
		       sp = NULL;
		       continue;
		    }
		  else if (websearch::_wconfig->_content_analysis)
		    {
		       // grab nearest neighbors out of the LSH uniform hashtable.
		       std::map<double,const std::string,std::greater<double> > mres
			 = qc->_ulsh_ham->getLEltsWithProbabilities(sp->_url,qc->_lsh_ham->_L); // url. we could treat host & path independently...
		       std::map<double,const std::string,std::greater<double> > mres_tmp
			 = qc->_ulsh_ham->getLEltsWithProbabilities(sp->_title,qc->_lsh_ham->_L); // title.
		       std::map<double,const std::string,std::greater<double> >::const_iterator mit = mres_tmp.begin();
		       while(mit!=mres_tmp.end())
			 {
			    mres.insert(std::pair<double,const std::string>((*mit).first,(*mit).second)); // we could do better than this merging...
			    ++mit;
			 }
		       
		       if (!mres.empty())
			 {
			    // iterate results and merge as possible.
			    mit = mres.begin();
			    while(mit!=mres.end())
			      {
				 search_snippet *comp_sp = qc->get_cached_snippet((*mit).second.c_str());
				 if (!comp_sp)
				   comp_sp = qc->get_cached_snippet_title((*mit).second.c_str());
				 assert(comp_sp != NULL);
				 
				 /* std::cout << "url: " << sp->_url << std::endl;
				  std::cout << "url2 (neighbor): " << comp_sp->_url << std::endl; */
				 
				 // Beware: second url (from sp) is the one to be possibly deleted!
				 bool same = content_handler::has_same_content(qc,comp_sp,sp,st);
				 
				 //std::cerr << "[Debug]: same: " << same << std::endl;
				 
				 if (same)
				   {
				      search_snippet::merge_snippets(comp_sp,sp);
				      comp_sp->_seeks_rank = comp_sp->_engine.count();
				      it = snippets.erase(it);
				      delete sp;
				      sp = NULL;
				      break;
				   }
				 
				 ++mit;
			      }
			 } // end if mres empty.
		       if (!sp)
			 continue;
		    }
	     
		  //debug
		  //std::cerr << "new url scanned: " << sp->_url << std::endl;
		  //debug
		  
		  sp->_seeks_rank = sp->_engine.count();
		  sp->_new = false;
		  
		  qc->add_to_unordered_cache(sp);
		  qc->add_to_unordered_cache_title(sp);
		  
		  // lsh.
		  if (websearch::_wconfig->_content_analysis)
		    {
		       qc->_ulsh_ham->add(sp->_url,qc->_lsh_ham->_L);
		       qc->_ulsh_ham->add(sp->_title,qc->_lsh_ham->_L);
		    }
		  
	       } // end if new.
	     
	     ++it;
	  } // end while.
		
        // sort by rank.
        std::stable_sort(snippets.begin(),snippets.end(),
			 search_snippet::max_seeks_rank);
	
	//debug
	/* std::cerr << "[Debug]: sorted result snippets:\n";
	it = snippets.begin();
        while(it!=snippets.end())
	  {
	     (*it)->print(std::cerr);
	     it++;
         i++;
	  } */
	//debug
     }

   void sort_rank::score_and_sort_by_similarity(query_context *qc, const char *url)
     {
	search_snippet *usp = qc->get_cached_snippet(url);
	usp->set_back_similarity_link();
	
	// grab all urls content that might be needed. 
	// XXX: should look into solutions for lowering this number.
	// for now, grab content for urls on the first 3 pages.
	size_t ncontents = websearch::_wconfig->_N * 3;
	std::vector<std::string> urls;
	std::vector<search_snippet*> snippets;
	size_t nurls = std::min(qc->_cached_snippets.size(),ncontents);
	bool url_added = false;
	for (size_t i=0;i<nurls;i++)
	  {
	     search_snippet *sp = qc->_cached_snippets.at(i);
	     
	     if (!sp->_cached_content)
	       {
		  if (sp == usp)
		    {
		       url_added = true;
		    }
		  urls.push_back(sp->_url);
		  snippets.push_back(sp);
	       }
	  }
	if (!url_added)
	  {
	     urls.push_back(url);
	     snippets.push_back(usp);
	  }
	
	nurls = urls.size();
	char **outputs = content_handler::fetch_snippets_content(qc,urls);
	for (size_t i=0;i<nurls;i++)	
	  if (outputs[i])
	    {
	       search_snippet *sp = qc->get_cached_snippet(urls[i].c_str());
	       sp->_cached_content = outputs[i];
	    }
		
	// parse fetched content.
	std::string *txt_contents = content_handler::parse_snippets_txt_content(nurls,outputs);
	free(outputs); // contents still lives as cache within snippets.
	
	// compute features.
	std::vector<search_snippet*> sps;
	std::vector<std::string> valid_contents;
	for (size_t i=0;i<nurls;i++)
	  if (!txt_contents[i].empty())
	    {
	       valid_contents.push_back(txt_contents[i]);
	       sps.push_back(snippets.at(i));
	    }
	content_handler::extract_features_from_snippets(qc,&valid_contents.at(0),sps.size(),&sps.at(0));    
	
	// run similarity analysis and compute scores.
	content_handler::feature_based_similarity_scoring(qc,qc->_cached_snippets.size(),&qc->_cached_snippets.at(0),url);
	
	// sort snippets according to computed scores.
	std::stable_sort(qc->_cached_snippets.begin(),qc->_cached_snippets.end(),search_snippet::max_seeks_ir);

	// reset scores.
	for (size_t i=0;i<sps.size();i++)
	  sps[i]->_seeks_ir = 0;
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
