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

#ifndef SORT_RANK_H
#define SORT_RANK_H

#include "search_snippet.h"
#include "clustering.h" // cluster.

namespace seeks_plugins
{
  class sort_rank
  {
    public:


      // merge snippets based on the target url.
      static void sort_and_merge_snippets(std::vector<search_snippet*> &snippets,
                                          std::vector<search_snippet*> &unique_snippets);

      static void sort_merge_and_rank_snippets(query_context *qc, std::vector<search_snippet*> &snippets,
          const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static void score_and_sort_by_similarity(query_context *qc, const char *id_str,
          const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
          search_snippet *&ref_sp,
          std::vector<search_snippet*> &sorted_snippets) throw (sp_exception);

      static void group_by_types(query_context *qc, cluster *&clusters, short &K);

#if defined(PROTOBUF) && defined(TC)
      static void personalize(query_context *qc) throw (sp_exception);
      static void personalized_rank_snippets(query_context *qc,
                                             std::vector<search_snippet*> &snippets) throw (sp_exception);
      static void get_related_queries(query_context *qc) throw (sp_exception);
      static void get_recommended_urls(query_context *qc) throw (sp_exception);
#endif
  };

} /* end of namespace. */

#endif
