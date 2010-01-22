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
 
#ifndef SORT_RANK_H
#define SORT_RANK_H

#include "search_snippet.h"

namespace seeks_plugins
{
   class sort_rank
     {
      public:
	
	
	// merge snippets based on the target url.
	static void sort_and_merge_snippets(std::vector<search_snippet*> &snippets,
					    std::vector<search_snippet*> &unique_snippets);
	
	static void sort_merge_and_rank_snippets(query_context *qc, std::vector<search_snippet*> &snippets);
	
	static void score_and_sort_by_similarity(query_context *qc, const char *id_str,
						 search_snippet *&ref_sp,
						 std::vector<search_snippet*> &sorted_snippets);
	
	// advanced sorting, based on webpages content.
	//static void retrieve_and_score(query_context *qc);
	
	
     };
   
} /* end of namespace. */

#endif
