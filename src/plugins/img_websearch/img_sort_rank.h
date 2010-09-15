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

#ifndef IMG_SORT_RANK_H
#define IMG_SORT_RANK_H

#include "img_query_context.h"
#include "img_search_snippet.h"

namespace seeks_plugins
{
   
   class img_sort_rank
     {
      public:
	static void sort_rank_and_merge_snippets(img_query_context *qc,
						 std::vector<search_snippet*> &snippets);
     
#ifdef FEATURE_OPENCV2
	static void score_and_sort_by_similarity(img_query_context *qc, const char *id_str,
						 img_search_snippet *&ref_sp,
						 std::vector<search_snippet*> &sorted_snippets,
						 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);
#endif
     };
   
} /* end of namespace. */

#endif
