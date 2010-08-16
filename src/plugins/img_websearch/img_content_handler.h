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

#ifndef IMG_CONTENT_HANDLER_H
#define IMG_CONTENT_HANDLER_H

#include "img_query_context.h"
#include "img_search_snippet.h"

#include <vector>
#include <string>

namespace seeks_plugins
{
   
   class img_content_handler
     {
      public:
	static void fetch_all_img_snippets_and_features(img_query_context *qc);
	
	static void extract_surf_features_from_snippets(img_query_context *qc,
							const std::vector<std::string*> &img_contents,
							const std::vector<img_search_snippet*> &sps);
	
	static void feature_based_similarity_scoring(img_query_context *qc,
						     const size_t &nsps,
						     search_snippet **sps,
						     search_snippet *ref_sp);
     };
      
} /* end of namespace. */

#endif
  
