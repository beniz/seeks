
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

#ifdef FEATURE_OPENCV2
#include "img_content_handler.h"
#include "ocvsurf.h"
#endif

namespace seeks_plugins
{
   
   void img_sort_rank::sort_rank_and_merge_snippets(img_query_context *qc,
						    std::vector<search_snippet*> &snippets)
     {
#ifdef FEATURE_OPENCV2
	static double st = 0.47; // similarity threshold.
	if (img_websearch_configuration::_img_wconfig->_img_content_analysis)
	  {
	     // fill up the cache first.
	     qc->update_unordered_cache();
	     
	     // fetch all image thumbnails and compute SURF features.
	     // XXX: time consuming.
	     img_content_handler::fetch_all_img_snippets_and_features(qc);
	  
	     // clear unordered cache.
	     qc->_unordered_snippets.clear();
	  }
#endif
	
	std::vector<search_snippet*>::iterator it = snippets.begin();
	img_search_snippet *c_sp = NULL;
	while(it != snippets.end())
	  {
	     img_search_snippet *sp = static_cast<img_search_snippet*>((*it));
	     if (sp->_new)
	       {
		  if ((c_sp = static_cast<img_search_snippet*>(qc->get_cached_snippet(sp->_id)))!=NULL)
		    {
		       // merge image snippets.
		       std::cerr << "merging into " << c_sp->_url << std::endl;
		       img_search_snippet::merge_img_snippets(c_sp,sp);
		       c_sp->_seeks_rank = c_sp->_img_engine.count();
		       it = snippets.erase(it);
		       delete sp;
		       sp = NULL;
		       continue;
		    }
#ifdef FEATURE_OPENCV2
		  if (img_websearch_configuration::_img_wconfig->_img_content_analysis) // compare to other snippets to detect identical images. 
		    {
		       // check on similarity w.r.t. other thumbnails.
		       if (sp->_surf_keypoints)
			 {
			    hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator sit = qc->_unordered_snippets.begin();
			    while(sit!=qc->_unordered_snippets.end())
			      {
				 if ((*sit).second == (*it))
				   {
				      ++sit;
				      continue;
				   }
				 
				 img_search_snippet *ssp = static_cast<img_search_snippet*>((*sit).second);
				 if (!ssp->_surf_keypoints)
				   {
				      ++sit;
				      continue;
				   }
				 				 
				 std::vector<surf_pair> ptpairs;
				 ocvsurf::flannFindPairs(sp->_surf_descriptors,ssp->_surf_descriptors,
							 ptpairs);
				 double den = sp->_surf_keypoints->total + ssp->_surf_keypoints->total;
				 size_t npt = ptpairs.size();
				 int np = 0;
				 for (size_t j=0;j<npt;j++)
				   {
				      if (ptpairs.at(j)._dist < 0.6)
					np++;
				   }
				 double score = 2.0 * np / den;
				 
				 if (score > st) // over threshold, that is > ~0.5
				   {
				      // merge current image snippet and delete it.
				      std::cerr << "merging (" << score << ") " << sp->_url << " into " << ssp->_url << std::endl;
				      img_search_snippet::merge_img_snippets(ssp,sp);
				      ssp->_seeks_rank = ssp->_img_engine.count();
				      it = snippets.erase(it);
				      //qc->remove_from_unordered_cache(sp->_id);
				      delete sp;
				      sp = NULL;
				      break;
				   }
				 else ++sit;
			      }
			 }
		    }
#endif
		  if (!sp)
		    continue;
		  
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

#ifdef FEATURE_OPENCV2
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
	std::sort(sorted_snippets.begin(),sorted_snippets.end(),search_snippet::max_seeks_ir);
     }
#endif
   
} /* end of namespace. */
