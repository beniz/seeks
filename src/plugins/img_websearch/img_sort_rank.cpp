/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "img_sort_rank.h"
#include "img_content_handler.h"

namespace seeks_plugins
{
   
   void img_sort_rank::sort_rank_and_merge_snippets(img_query_context *qc,
						    std::vector<search_snippet*> &snippets)
     {
	static double st = 0.9; // similarity threshold.
	
	std::vector<search_snippet*>::iterator it = snippets.begin();
	img_search_snippet *c_sp = NULL;
	while(it != snippets.end())
	  {
	     img_search_snippet *sp = static_cast<img_search_snippet*>((*it));
	     if (sp->_new)
	       {
		  if ((c_sp = static_cast<img_search_snippet*>(qc->get_cached_snippet(sp->_id)))!=NULL)
		    {
		       //TODO: merge image snippets.
		    }
		  
		  sp->_seeks_rank = sp->_img_engine.count();
		  sp->_new = false;
		  
		  qc->add_to_unordered_cache(sp);
		  qc->add_to_unordered_cache_title(sp);
	       } // end if new.
	     ++it;
	  }
	
	// sort by rank.
	std::stable_sort(snippets.begin(),snippets.end(),
			 img_search_snippet::max_seeks_rank);
     }

   void img_sort_rank::score_and_sort_by_similarity(img_query_context *qc, const char *id_str,
						    img_search_snippet *&ref_sp,
						    std::vector<search_snippet*> &sorted_snippets)
     {
	uint32_t id = (uint32_t)strtod(id_str,NULL);
	
	ref_sp = static_cast<img_search_snippet*>(qc->get_cached_snippet(id));
	
	if (!ref_sp) // this should not happen, unless someone is forcing an url onto a Seeks node.
	  return;
	
	ref_sp->set_back_similarity_link();
	
	img_content_handler::fetch_all_img_snippets_and_features(qc);
	
	// run similarity analysis and compute scores.
	img_content_handler::feature_based_similarity_scoring(qc,sorted_snippets.size(),
							      &sorted_snippets.at(0),ref_sp);
	
	// sort snippets according to computed scores.
	std::stable_sort(sorted_snippets.begin(),sorted_snippets.end(),search_snippet::max_seeks_ir);
     }
   
} /* end of namespace. */
