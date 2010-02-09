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
 
#include "clustering.h"
#include "errlog.h"

using sp::errlog;

namespace seeks_plugins
{
   
   /*- centroid. -*/
   centroid::centroid()
     {
     }
      
   /*- cluster. -*/
   cluster::cluster()
     :_rank(0.0)
     {
     }
      
   void cluster::add_point(const uint32_t &id,
			   hash_map<uint32_t,float,id_hash_uint> *p)
     {
	hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::iterator hit;
	if ((hit=_cpoints.find(id))!=_cpoints.end())
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "Trying to add a snippet multiple times to the same cluster");
	  }
	else _cpoints.insert(std::pair<uint32_t,hash_map<uint32_t,float,id_hash_uint>*>(id,p));
     }
   
   void cluster::compute_rank(const query_context *qc)
     {
	_rank = 0.0;
	hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit
	  = _cpoints.begin();
	while(hit!=_cpoints.end())
	  {
	     search_snippet *sp = qc->get_cached_snippet((*hit).first);
	     _rank += sp->_seeks_rank; // summing up the seeks rank yields a cluster rank (we could use the mean instead).
	     ++hit;
	  }
     }
      
   /*- clustering. -*/
   clustering::clustering()
     :_qc(NULL),_K(0),_clusters(NULL),_cluster_labels(NULL)
       {
       }
   
   clustering::clustering(query_context *qc,
			  const std::vector<search_snippet*> &snippets,
			  const short &K)
     :_qc(qc),_K(K),_snippets(snippets)
     {
	_clusters = new cluster[_K];
	_cluster_labels = new std::vector<std::string>[_K];
	
	// setup points and dimensions.
	size_t nsp = _snippets.size();
	for (size_t s=0;s<nsp;s++)
	  {
	     search_snippet *sp = _snippets.at(s);
	     if (sp->_features_tfidf)
	       _points.insert(std::pair<uint32_t,hash_map<uint32_t,float,id_hash_uint>*>(sp->_id,sp->_features_tfidf));
	  }
     }
      
   clustering::~clustering()
     {
	if (_clusters)
	  delete[] _clusters;
	if (_cluster_labels)
	  delete[] _cluster_labels;
     };
   
   void clustering::post_processing()
     {
	// rank snippets within clusters.
	rank_clusters_elements();
	
	// rank clusters.
	compute_clusters_rank();
     
	// sort clusters.
	std::stable_sort(_clusters,_clusters+_K,cluster::max_rank_cluster);
     }
      
   // default ranking is as computed by seeks on the main list of results.
   void clustering::rank_elements(cluster &cl)
     {
	hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::iterator hit
	  = cl._cpoints.begin();
	while(hit!=cl._cpoints.end())
	  {
	     search_snippet *sp = _qc->get_cached_snippet((*hit).first);
	     sp->_seeks_ir = sp->_seeks_rank;
	     ++hit;
	  }
     }
   
   void clustering::rank_clusters_elements()
     {
	for (short c=0;c<_K;c++)
	  rank_elements(_clusters[c]);
     }
   
   void clustering::compute_clusters_rank()
     {
	for (short c=0;c<_K;c++)
	  _clusters[c].compute_rank(_qc);
     }
   
   hash_map<uint32_t,float,id_hash_uint>* clustering::get_point_features(const short &np)
     {
	short p = 0;
	hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit
	  = _points.begin();
	while(hit!=_points.end())
	  {
	     if (p == np)
	       return (*hit).second;
	     p++;
	     ++hit;
	  }
	return NULL;
     }
   
} /* end of namespace. */
