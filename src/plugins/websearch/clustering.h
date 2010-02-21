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
  
#ifndef CLUSTERING_H
#define CLUSTERING_H

#include "stl_hash.h"
#include "search_snippet.h"
#include "query_context.h"

namespace seeks_plugins
{
   class centroid
     {
      public:
	centroid();
	
	~centroid()
	  {};

	void clear()
	  {
	     _features.clear();
	  };
	
	hash_map<uint32_t,float,id_hash_uint> _features;
     };
      
   class cluster
     {
      public:
	static bool max_rank_cluster(const cluster &c1, const cluster &c2)
	  {
	     if (c1._rank > c2._rank)
	       return true;
	     else return false;
	  };

	static bool max_size_cluster(const cluster &c1, const cluster &c2)
	  {
	     if (c1._cpoints.size() > c2._cpoints.size())
	       return true;
	     else return false;
	  };
		
      public:
	cluster();
	
	~cluster()
	  {};

	void clear()
	  {
	     _cpoints.clear();
	  }
		
	void add_point(const uint32_t &id,
		       hash_map<uint32_t,float,id_hash_uint> *p);
	
	void compute_rank(const query_context *qc);
	
	void compute_label(const query_context *qc);
	
	centroid _c; /**< cluster's centroid. */
	hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint> _cpoints; /**< points associated to this cluster. */
	double _rank; /**< cluster's rank among clusters. */
	std::string _label; /**< cluster's label. */
     };
      
   class clustering
     {
      public:
	clustering();
	
	clustering(query_context *qc,
		   const std::vector<search_snippet*> &snippets,
		   const short &K);
	
	virtual ~clustering();
	
	virtual void initialize() {};
	
	virtual void clusterize() {};
	
	virtual void rank_elements(cluster &cl);
	
	void post_processing();
	
	void rank_clusters_elements();
	
	void compute_clusters_rank();

	void compute_cluster_labels();
	
      protected:
	hash_map<uint32_t,float,id_hash_uint>* get_point_features(const short &np);
	
      public:
	query_context *_qc;
	
	hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint> _points; /**< key is the snippet's id. */

	short _K; /**< number of clusters. */
     
	cluster *_clusters;

	std::vector<std::string> *_cluster_labels;
	
	cluster _garbage_cluster; /**< everything not attached elsewhere. */
     
	std::vector<search_snippet*> _snippets;
     };
   
} /* end of namespace. */

#endif
