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
#include "errlog.h"
#include "ocvsurf.h"

using sp::errlog;

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
    for (size_t i=0; i<ns; i++)
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
    std::string **outputs = content_handler::fetch_snippets_content(curls,false,qc); // false: do not use seeks proxy.
    if (!outputs)
      return;

    size_t nurls = curls.size();
    std::vector<img_search_snippet*> sps;
    sps.reserve(nurls);
    std::vector<std::string*> valid_contents;
    valid_contents.reserve(nurls);
    for (size_t i=0; i<nurls; i++)
      {
        if (outputs[i])
          {
            search_snippet *spsp = qc->get_cached_snippet(urls[i]);
            if (!spsp)
              continue; // safe.
            img_search_snippet *sp = static_cast<img_search_snippet*>(spsp);
            sp->_cached_image = outputs[i]; // cache fetched content.
            valid_contents.push_back(sp->_cached_image);
            sps.push_back(sp);
          }
        else
          {
            //std::cerr << "couldn't fetch img: " << curls[i] << std::endl;
          }
      }
    delete[] outputs;

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
        errlog::log_error(LOG_LEVEL_ERROR,"Failed getting referer image: cannot compute image similarity");
        return;
      }
    img_search_snippet *ref_isp = static_cast<img_search_snippet*>(ref_sp);
    if (!ref_isp->_surf_descriptors)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Failed getting referer image descriptors: cannot compute image similarity");
        return;
      }

    // compute scores.
    for (size_t i=0; i<nsps; i++)
      {
        img_search_snippet *sp = static_cast<img_search_snippet*>(sps[i]);
        if (sp->_surf_keypoints)
          {
            /* if (sp->_surf_keypoints->total < 0.5*ref_isp->_surf_keypoints->total) // at least 50% as many feature as reference image.
              continue;

            std::vector<surf_pair> ptpairs;
            ocvsurf::flannFindPairs(ref_isp->_surf_descriptors,
            		  sp->_surf_descriptors,
            		  ptpairs);
            double den = ref_isp->_surf_keypoints->total + sp->_surf_keypoints->total; */
            //sp->_seeks_ir = ptpairs.size() / den;

            //OLD: den /= ref_isp->_surf_keypoints->total;
            /* double num = 1.0;
            sp->_seeks_ir = 1.0;
            for (size_t j=0;j<ptpairs.size();j++)
              {
                 if (ptpairs.at(j)._dist < 0.4)
             num += 1+ptpairs.at(j)._dist;// / (ptpairs.at(j)._dist+0.1); */
            //num += 1.0/(ptpairs.at(j)._dist+0.1) * (ptpairs.at(j)._dist+0.1);
            /* sp->_seeks_ir += 1.0; */
            //else sp->_seeks_ir += 1;// / (ptpairs.at(j)._dist+0.1);
            //OLD: else sp->_seeks_ir += (ptpairs.at(j)._dist+0.1) * (ptpairs.at(j)._dist+0.1);
            /* } */
            //sp->_seeks_ir = num / den;
            /* sp->_seeks_ir = num / static_cast<double>(ref_isp->_surf_keypoints->total); */
            //sp->_seeks_ir = sqrt(sp->_seeks_ir);
            /* if (sp->_seeks_ir > 0.0)
              sp->_seeks_ir = sqrt(1/num) / sqrt(sp->_seeks_ir);// * sp->_seeks_ir); */
            //sp->_seeks_ir = num / den;

            //OLD/ sp->_seeks_ir = num * num * num / (sp->_seeks_ir*sp->_seeks_ir * sp->_seeks_ir);

            //debug
            /* std::cerr << "[Debug]: url: " << sp->_url
              << " -- score: " << sp->_seeks_ir
              << " -- surf keypoints: " << sp->_surf_keypoints->total << std::endl; */
            //debug

            /* if (sp->_seeks_ir < 1e-5)
             sp->_seeks_ir = 0.0; */

            CvMat *points1 = NULL;
            CvMat *points2 = NULL;
            sp->_seeks_ir = ocvsurf::bruteMatch(points1,points2,
                                                ref_isp->_surf_keypoints,ref_isp->_surf_descriptors,
                                                sp->_surf_keypoints,sp->_surf_descriptors,false); // false: no filtering.
            /* if (points1 && points2)
              sp->_seeks_ir = ocvsurf::removeOutliers(points1,points2); */
          }
      }
  }

  void img_content_handler::extract_surf_features_from_snippets(img_query_context *qc,
      const std::vector<std::string*> &img_contents,
      const std::vector<img_search_snippet*> &sps)
  {
    size_t ncontents = img_contents.size();
    for (size_t i=0; i<ncontents; i++)
      {
        ocvsurf::generate_surf_features(img_contents[i],sps.at(i)->_surf_keypoints,
                                        sps.at(i)->_surf_descriptors,sps.at(i));
      }
  }

} /* end of namespace. */
