/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
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

#include "sort_rank.h"
#include "websearch.h"
#include "content_handler.h"
#include "urlmatch.h"
#include "miscutil.h"
#include "cf.h"

#include <ctype.h>
#include <algorithm>
#include <iterator>
#include <map>
#include <iostream>

#include <assert.h>

using sp::urlmatch;
using sp::miscutil;

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
   
   void sort_rank::sort_merge_and_rank_snippets(query_context *qc, std::vector<search_snippet*> &snippets,
						const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	static double st = 0.9; // similarity threshold.
	
	bool content_analysis = websearch::_wconfig->_content_analysis;
	const char *ca = miscutil::lookup(parameters,"content_analysis");
	if (ca && strcasecmp(ca,"on") == 0)
	  content_analysis = true;
	
	const char *cache_check = miscutil::lookup(parameters,"ccheck");
	bool ccheck = true;
	if (cache_check && strcasecmp(cache_check,"no") == 0)
	  ccheck = false;
	
	// initializes the LSH subsystem is we need it and it has not yet
	// been initialized.
	if (content_analysis && !qc->_ulsh_ham)
	  {
	     /**
	      * The LSH system based on a Hamming distance has a fixed maximum size
	      * for strings, it is set to 100. Therefore, k should be set accordingly,
	      * that is below 100 and in proportion to the 'fuzziness' necessary for
	      * k-nearest neighbors computation.
	      */
	     qc->_lsh_ham = new LSHSystemHamming(55,5);
	     qc->_ulsh_ham = new LSHUniformHashTableHamming(*qc->_lsh_ham,
							    websearch::_wconfig->_Nr*3*NSEs);
	  }
	
	std::vector<search_snippet*>::iterator it = snippets.begin();
	search_snippet *c_sp = NULL;
	while(it != snippets.end())
	  {
	     search_snippet *sp = (*it);
	     
	     if (!ccheck && sp->_doc_type == TWEET)
	       sp->_meta_rank = -1; // reset the rank because it includes retweets.
	     
	     if (sp->_new)
	       {
		  if ((c_sp = qc->get_cached_snippet(sp->_id))!=NULL
		      && c_sp->_doc_type == sp->_doc_type)
		    {
		       // merging snippets.
		       search_snippet::merge_snippets(c_sp,sp);
		       it = snippets.erase(it);
		       delete sp;
		       sp = NULL;
		       continue;
		    }
		  else if (content_analysis)
		    {
		       // grab nearest neighbors out of the LSH uniform hashtable.
		       std::string surl = urlmatch::strip_url(sp->_url);
		       std::map<double,const std::string,std::greater<double> > mres
			 = qc->_ulsh_ham->getLEltsWithProbabilities(surl,qc->_lsh_ham->_Ld); // url. we could treat host & path independently...
		       std::string lctitle = sp->_title;
		       std::transform(lctitle.begin(),lctitle.end(),lctitle.begin(),tolower);
		       std::map<double,const std::string,std::greater<double> > mres_tmp
			 = qc->_ulsh_ham->getLEltsWithProbabilities(lctitle,qc->_lsh_ham->_Ld); // title.
		       		       
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
				 search_snippet *comp_sp = qc->get_cached_snippet((*mit).second);
				 if (!comp_sp)
				   comp_sp = qc->get_cached_snippet_title((*mit).second.c_str());
				 assert(comp_sp != NULL);
				 	     
				 // Beware: second url (from sp) is the one to be possibly deleted!
				 bool same = content_handler::has_same_content(qc,comp_sp,sp,st);
				 				 
				 if (same)
				   {
				      search_snippet::merge_snippets(comp_sp,sp);
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
		  
		  sp->_meta_rank = sp->_engine.count();
		  sp->_new = false;
		  
		  qc->add_to_unordered_cache(sp);
		  qc->add_to_unordered_cache_title(sp);
		  
		  // lsh.
		  if (content_analysis)
		    {
		       std::string surl = urlmatch::strip_url(sp->_url);
		       qc->_ulsh_ham->add(surl,qc->_lsh_ham->_Ld);
		       std::string lctitle = sp->_title;
		       std::transform(lctitle.begin(),lctitle.end(),lctitle.begin(),tolower);
		       qc->_ulsh_ham->add(lctitle,qc->_lsh_ham->_Ld);
		    }
		  
	       } // end if new.
	     
	     ++it;
	  } // end while.
		
        // sort by rank.
        std::stable_sort(snippets.begin(),snippets.end(),
			 search_snippet::max_meta_rank);
	
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

   void sort_rank::score_and_sort_by_similarity(query_context *qc, const char *id_str,
						const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
						search_snippet *&ref_sp,
						std::vector<search_snippet*> &sorted_snippets)
     {
	uint32_t id = (uint32_t)strtod(id_str,NULL);
	
	ref_sp = qc->get_cached_snippet(id);
	
	if (!ref_sp) // this should not happen, unless someone is forcing an url onto a Seeks node.
	  return;
	
	ref_sp->set_back_similarity_link(parameters);
	
	bool content_analysis = websearch::_wconfig->_content_analysis;
	const char *ca = miscutil::lookup(parameters,"content_analysis");
	if (ca && strcasecmp(ca,"on") == 0)
	  content_analysis = true;
	
	if (content_analysis)
	  content_handler::fetch_all_snippets_content_and_features(qc);
	else content_handler::fetch_all_snippets_summary_and_features(qc);
	
	// run similarity analysis and compute scores.
	content_handler::feature_based_similarity_scoring(qc,sorted_snippets.size(),
							  &sorted_snippets.at(0),ref_sp);
	
	// sort snippets according to computed scores.
	std::stable_sort(sorted_snippets.begin(),sorted_snippets.end(),search_snippet::max_seeks_ir);
     }

   void sort_rank::group_by_types(query_context *qc, cluster *&clusters, short &K)
     {
	/**
	 * grouping is done per file type, the most common being:
	 * 0 - html / html aka webpages (but no wikis)
	 * 1 - wiki
	 * 2 - pdf
	 * 3 - .doc, .pps, & so on aka other types of documents.
	 * 4 - forums
	 * 5 - video file
	 * 6 - audio file
	 * So for now, K is set to 7.
	 */
	K = 7;
	clusters = new cluster[K];
	
	size_t nsnippets = qc->_cached_snippets.size();
	for (size_t i=0;i<nsnippets;i++)
	  {
	     search_snippet *se = qc->_cached_snippets.at(i);
	     
	     if (se->_doc_type == WEBPAGE)
	       {
		  clusters[0].add_point(se->_id,NULL);
		  clusters[0]._label = "Webpages";  // TODO: languages...
	       }
	     else if (se->_doc_type == WIKI)
	       {
		  clusters[1].add_point(se->_id,NULL);
		  clusters[1]._label = "Wikis";
	       }
	     else if (se->_doc_type == FILE_DOC
		      && se->_file_format == "pdf")
	       {
		  clusters[2].add_point(se->_id,NULL);
		  clusters[2]._label = "PDFs";
	       }
	     else if (se->_doc_type == FILE_DOC)
	       {
		  clusters[3].add_point(se->_id,NULL);
		  clusters[3]._label = "Other files";
	       }
	     else if (se->_doc_type == FORUM)
	       {
		  clusters[4].add_point(se->_id,NULL);
		  clusters[4]._label = "Forums";
	       }
	     else if (se->_doc_type == VIDEO
		     || se->_doc_type == VIDEO_THUMB)
	       {
		  clusters[5].add_point(se->_id,NULL);
		  clusters[5]._label = "Videos";
	       }
	     else if (se->_doc_type == AUDIO)
	       {
		  clusters[6].add_point(se->_id,NULL);
		  clusters[6]._label = "Audio";
	       }
	  }
	
	// sort groups by decreasing sizes.
	std::stable_sort(clusters,clusters+K,cluster::max_size_cluster);
     }

   void sort_rank::personalized_rank_snippets(query_context *qc, std::vector<search_snippet*> &snippets,
					      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	if (!websearch::_cf_plugin)
	  return;
	std::cerr << "computing personalized ranks...\n";
	static_cast<cf*>(websearch::_cf_plugin)->estimate_ranks(qc->_query,snippets);
	std::stable_sort(snippets.begin(),snippets.end(),
			 search_snippet::max_seeks_rank);
     }
      
} /* end of namespace. */
