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

#ifndef IMG_SEARCH_SNIPPET_H
#define IMG_SEARCH_SNIPPET_H

#include "config.h"

#ifdef FEATURE_OPENCV2
#undef HAVE_CONFIG_H
#include <cv.h> // OpenCV.
#include <highgui.h>
#define HAVE_CONFIG_H
#endif

#include "search_snippet.h" // from websearch plugin.
#include "img_websearch_configuration.h"

namespace seeks_plugins
{

  class img_search_snippet : public search_snippet
  {
    public:
      static bool max_seeks_rank(search_snippet *s1, search_snippet *s2)
      {
        img_search_snippet *is1 = static_cast<img_search_snippet*>(s1);
        img_search_snippet *is2 = static_cast<img_search_snippet*>(s2);
        if (is1->_seeks_rank == is2->_seeks_rank)
          return is1->_rank / static_cast<double>(is1->_img_engine.count())
                 < is2->_rank / static_cast<double>(is2->_img_engine.count());  // beware: min rank is still better.
        else
          return is1->_seeks_rank > is2->_seeks_rank;  // max seeks rank is better.
      };

      img_search_snippet();
      img_search_snippet(const short &rank);

      ~img_search_snippet();

      virtual std::string to_html_with_highlight(std::vector<std::string> &words,
          const std::string &base_url,
          const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      virtual std::string to_json(const bool &thumbs,
                                  const std::vector<std::string> &query_words);

      virtual bool is_se_enabled(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      virtual void set_similarity_link(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      virtual void set_back_similarity_link(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static void merge_img_snippets(img_search_snippet *s1,
                                     const img_search_snippet *s2);

      // variables.
      std::bitset<IMG_NSEs> _img_engine;

#ifdef FEATURE_OPENCV2
      // OpenCV feature format.
      CvSeq *_surf_keypoints;
      CvSeq *_surf_descriptors;
      CvMemStorage *_surf_storage;
#endif

      // cached image.
      std::string *_cached_image;
  };

} /* end of namespace. */

#endif
