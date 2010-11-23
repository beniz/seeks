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

#ifndef OSKMEANS_H
#define OSKMEANS_H

#include "clustering.h"

namespace seeks_plugins
{
  class oskmeans : public clustering
  {
    public:
      oskmeans();

      oskmeans(query_context *qc,
               const std::vector<search_snippet*> &snippets,
               const short &K);

      ~oskmeans();

      virtual void initialize();

      virtual void clusterize();

      virtual void rank_elements(cluster &cl);

      void uniform_random_selection();

      void kmeans_pp();

      bool stopping_criterion();

      /* double rss();
       double rad_c(const short &c); */

      short get_closest_cluster(hash_map<uint32_t,float,id_hash_uint> *p,
                                double &min_dist);

      short assign_cluster(const uint32_t &id,
                           hash_map<uint32_t,float,id_hash_uint> *p);

      static float distance(const hash_map<uint32_t,float,id_hash_uint> &p1,
                            const hash_map<uint32_t,float,id_hash_uint> &p2);

      static float distance_normed_points(const hash_map<uint32_t,float,id_hash_uint> &p1,
                                          const hash_map<uint32_t,float,id_hash_uint> &p2);

      static float enorm(const hash_map<uint32_t,float,id_hash_uint> &p);

      void recompute_centroid(const float &learning_rate,
                              centroid *c,
                              hash_map<uint32_t,float,id_hash_uint> *p,
                              float &cl_norm);

      void normalize_centroid(centroid *c,
                              const float &cl_norm);

    private:
      short _iterations; /**< number of clustering iterations, for control. */
      double _lambda; /**< weight control on cluster numbers. */
      double _rss; /**< error control. */
      double _t;

      static short _niterations;
      static float _nu0;
      static float _nuf;
  };

} /* end of namespace. */

#endif
