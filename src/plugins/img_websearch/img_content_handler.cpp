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

#include "img_content_handler.h"
#include "content_handler.h"
#include "ocvsurf.h"

namespace seeks_plugins
{
   
   void img_content_handler::fetch_all_img_snippets_and_features(img_query_context *qc)
     {
	// prepare urls to fetch.
	size_t ns = qc->_cached_snippets.size();
	std::vector<std::string> urls;
	urls.reserve(ns);
	std::vector<std::string> curls;
	curls.reserve(ns);
	std::vector<img_search_snippet*> snippets;
	snippets.reserve(ns);
	for (size_t i=0;i<ns;i++)
	  {
	     img_search_snippet *sp = static_cast<img_search_snippet*>(qc->_cached_snippets.at(i));
	     if (sp->_cached_image)
	       {
		  //std::cerr << "already have a cached image for " << sp->_cached << std::endl;
		  continue; // already in cache.
	       }
	     urls.push_back(sp->_url);
	     curls.push_back(sp->_cached); // fetch smaller, cached image, usually a thumbnail.
	     snippets.push_back(sp);
	  }
	
	// fetch content.
	std::string **outputs = content_handler::fetch_snippets_content(curls,true,qc); // true: use seeks proxy.
	if (!outputs)
	  return;
	
	size_t nurls = curls.size();
	std::vector<img_search_snippet*> sps;
	sps.reserve(nurls);
	std::vector<std::string*> valid_contents;
	valid_contents.reserve(nurls);
	for (size_t i=0;i<nurls;i++)
	  {
	     if (outputs[i])
	       {
		  img_search_snippet *sp = static_cast<img_search_snippet*>(qc->get_cached_snippet(urls[i]));
		  sp->_cached_image = outputs[i]; // cache fetched content.
		  valid_contents.push_back(sp->_cached_image);
		  sps.push_back(sp);
	       }
	     else
	       {
		  //std::cerr << "couldn't fetch img: " << curls[i] << std::endl;
	       }
	  }
		
	// compute SURF features.
	img_content_handler::extract_surf_features_from_snippets(qc,valid_contents,sps);
     }
     
   void img_content_handler::feature_based_similarity_scoring(img_query_context *qc,
							      const size_t &nsps,
							      search_snippet **sps,
							      search_snippet *ref_sp)
     {
	if (!ref_sp)
	  {
	     std::cerr << "no ref_sp!\n";
	     return; // we should never reach here.
	  }
	img_search_snippet *ref_isp = static_cast<img_search_snippet*>(ref_sp);	
	
	// compute scores.
	for (size_t i=0;i<nsps;i++)
	  {
	     img_search_snippet *sp = static_cast<img_search_snippet*>(sps[i]);
	     if (sp->_surf_keypoints)
	       {
		  std::vector<int> ptpairs;
		  ocvsurf::flannFindPairs(ref_isp->_surf_descriptors,
					  sp->_surf_descriptors,
					  ptpairs);
		  double den = ref_isp->_surf_keypoints->total + sp->_surf_keypoints->total;
		  sp->_seeks_ir = ptpairs.size() / den;
		  
		  //debug
		  /* std::cerr << "[Debug]: url: " << sp->_url
		    << " -- score: " << sp->_seeks_ir << std::endl; */
		  //debug
	       }
	  }
     }
      
   void img_content_handler::extract_surf_features_from_snippets(img_query_context *qc,
								 const std::vector<std::string*> &img_contents,
								 const std::vector<img_search_snippet*> &sps)
     {
	size_t ncontents = img_contents.size();
	for (size_t i=0;i<ncontents;i++)
	  {
	     ocvsurf::generate_surf_features(img_contents[i],sps.at(i)->_surf_keypoints,
					     sps.at(i)->_surf_descriptors,sps.at(i));
	  }
     }
   
} /* end of namespace. */
