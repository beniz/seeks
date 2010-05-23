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
 
#include "oskmeans.h"
#include "mrf.h"
#include "Random.h"
#include "errlog.h"

#include <algorithm>
#include <iterator>
#include <map>

#include <math.h>

using lsh::mrf;
using lsh::Random;
using sp::errlog;

//#define DEBUG

namespace seeks_plugins
{
   short oskmeans::_niterations = 20;
   float oskmeans::_nu0 = 1.0;
   float oskmeans::_nuf = 0.01;
   
   oskmeans::oskmeans()
     :clustering(),_iterations(0),_lambda(0.0),_rss(0.0),_t(0)
       {
#ifdef DEBUG
	  std::cerr << "[Debug]: oskmeans created: " << _points.size() << " points\n";
#endif
       }

   oskmeans::oskmeans(query_context *qc,
		  const std::vector<search_snippet*> &snippets,
		  const short &K)
     :clustering(qc,snippets,K),_iterations(0),_lambda(0.0),_rss(0.0),_t(0)
       {
#ifdef DEBUG
	  std::cerr << "[Debug]: oskmeans created: " << _points.size() << " points -- " 
	    << _K << " clusters\n";
#endif
	  if (_K > _points.size())
	    _K = _points.size();
       }
      
   oskmeans::~oskmeans()
     {
     }
   
   void oskmeans::initialize()
     {
	_iterations = 0;
	kmeans_pp(); // oskmeans++ initialization.
	//uniform_random_selection();
     }
   
   void oskmeans::uniform_random_selection()
     {
	// debug, randomly select K points among all points...
	size_t npoints = _points.size();
	for (short c=0;c<_K;c++)
	  {
	     short gen_point_pos = (short)Random::genUniformUnsInt32(0,(unsigned long int)npoints-1);
	     
	     // set cluster's centroid.
	     hash_map<uint32_t,float,id_hash_uint> *point_features = get_point_features(gen_point_pos);
	     _clusters[c]._c._features = *point_features;
	  }
     }
      
   void oskmeans::kmeans_pp()
     {
	if (_snippets.empty())
	  return;
	
	size_t npts = _points.size();
	
	bool centroids[npts];
	for (size_t i=0;i<npts;i++)
	  centroids[i] = false;
	
	// grab the first best ranked snippet as the first centroid.
	std::stable_sort(_snippets.begin(),_snippets.end(),
			 search_snippet::max_seeks_rank);
        int sk = 0;
	while(!_snippets.at(sk++)->_features_tfidf)
	  {
	  }
	_clusters[0]._c._features = *_snippets.at(sk-1)->_features_tfidf; 
	uint32_t idc1 = _snippets.at(sk-1)->_id;
	
	sk = 0;
	hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit
	  = _points.begin();
	while(hit!=_points.end())
	  {
	     if ((*hit).first == idc1)
	       {
		  centroids[sk] = true;
		  break;
	       }
	     ++sk;
	     ++hit;
	  }
		
	// use kmeans++ to sample the other centroids.
	for (short c=1;c<_K;c++)
	  {
	     // compute sampling probabilities.
	     double probs[npts];
	     double tmax_sim = 0.0;
	     hit = _points.begin();
	     int k = 0;
	     	   
#ifdef DEBUG
	     //debug
	     std::string urls[npts];
	     //debug
#endif
	     
	     while(hit!=_points.end())
	       {
		  if (centroids[k])
		    {
		       ++hit;
		       probs[k++] = 0.0;
		       continue;
		    }
		  		  
		  double max_sim = 0.0; // max distance.
		  get_closest_cluster((*hit).second,max_sim);
		  
		  search_snippet *sp = _qc->get_cached_snippet((*hit).first);
		  max_sim = (1.0-max_sim)*(1.0-max_sim)*sp->_seeks_rank;
		  
		  probs[k] = max_sim;
		  tmax_sim += max_sim;

#ifdef DEBUG
		  //debug
		  urls[k] = sp->_url;
		  std::cout << "url: " << urls[k] << " -- max_sim: " << max_sim << " -- seeks_rank: " << sp->_seeks_rank << std::endl;
		  //debug
#endif
		  
		  k++;
		  ++hit;
	       }
	     for (size_t j=0;j<npts;j++)  // normalization.
	       probs[j] /= tmax_sim;

#ifdef DEBUG
	     std::cerr << "[Debug]: probs: ";
	     for (size_t j=0;j<npts;j++)
	       std::cerr << urls[j] << ": " << probs[j] << std::endl;
#endif
	     
	     // sample point.
	     //double smpl_prob = Random::genUniformDbl32(0,1);
	     
#ifdef DEBUG
	     std::cerr << "[Debug]: smpl_prob: " << smpl_prob << std::endl;
#endif
	     
	     /*double cprob = 0.0;
	     size_t l = 0;
	     for (l=0;l<npts;l++)
	       {
		  if (!centroids[l])
		    {
		       cprob += probs[l];
		       if (cprob >= smpl_prob)
			 break;
		    }
	       } */
	     
	     // select the point with max value to existing clusters.
	     double max_score = -1.0;
	     int cl = -1;
	     for (size_t l=0;l<npts;l++)
	       if (probs[l] > max_score)
		 {
		    max_score = probs[l];
		    cl = l;
		 }
	     centroids[cl] = true;

#ifdef DEBUG
	     std::cerr << "[Debug]: cluster's centroid #" << c << ": point #" << cl << " -- url: " << urls[cl] << std::endl;
#endif
	     
	     // set cluster's centroid.
	     hash_map<uint32_t,float,id_hash_uint> *point_features = get_point_features(cl);
	     _clusters[c]._c._features = *point_features;
	  }
     }
      
   void oskmeans::clusterize()
     {
	// initialize.
	initialize(); 
	
	if (_snippets.empty())
	  return;
	
	// clustering.
	while(!stopping_criterion())
	  {
#ifdef DEBUG
	     std::cerr << "[Debug]:clusterize, iteration #" << _iterations << std::endl;
#endif
	     
	     for (short c=0;c<_K;c++)
	       {
		  // clear the cluster.
		  _clusters[c].clear();
	       }

	     // clear the garbage cluster.
	     _garbage_cluster.clear();
	     	     	     
	     // iterates points and associate each of them with a cluster.
	     hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit 
	       = _points.begin();
	     while(hit!=_points.end())
	       {
		  float learning_rate = oskmeans::_nu0*pow((oskmeans::_nuf/oskmeans::_nu0),_t/static_cast<float>(_points.size()*oskmeans::_niterations));
		  
#ifdef DEBUG
		  std::cerr << "learning rate: " << learning_rate << std::endl;
#endif
		  
		  // find closest cluster to this point.
		  short cl = assign_cluster((*hit).first,(*hit).second);
		     	     
		  // recomputation of centroids/medoids.
		  if (cl != -1)
		    {
		       float cl_norm = 0.0;
		       recompute_centroid(learning_rate,&_clusters[cl]._c,(*hit).second,cl_norm);
		       normalize_centroid(&_clusters[cl]._c,cl_norm);
		    }
		  		  
		  ++hit;
		  _t++;
	       }
	     
	     // count iteration.
	     _iterations++;
	  }
     }
   
   bool oskmeans::stopping_criterion()
     {
	// max number of iterations.
	if (_iterations >= oskmeans::_niterations)
	  return true;
	else return false;
     }

   short oskmeans::get_closest_cluster(hash_map<uint32_t,float,id_hash_uint> *p,
				       double &max_dist)
     {
	max_dist = 0;
	short close_c = -1;
	for (short c=0;c<_K;c++)
	  {
	     /* if (_clusters[c]._c._features.empty())
	       continue; */
	     
	     float dist = oskmeans::distance_normed_points(*p,_clusters[c]._c._features);
	     
	     if (dist > max_dist)
	       {
		  close_c = c;
		  max_dist = dist;
	       }
	  }
	return close_c;
     }
   
   short oskmeans::assign_cluster(const uint32_t &id,
				  hash_map<uint32_t,float,id_hash_uint> *p)
     {
	// find closest cluster to p.
	double max_dist = 0.0;
	short close_c = get_closest_cluster(p,max_dist);
	
	// assign p to cluster.
#ifdef DEBUG
	std::cerr << "[Debug]: assigning " << id << " to cluster " << close_c << std::endl;
#endif
	
	if (close_c == -1)
	  _garbage_cluster.add_point(id,p);
	else _clusters[close_c].add_point(id,p);
	return close_c;
     }
   
   // static.
   float oskmeans::distance(const hash_map<uint32_t,float,id_hash_uint> &p1, const hash_map<uint32_t,float,id_hash_uint> &p2)
     {
	double dist = oskmeans::distance_normed_points(p1,p2);
	
	if (dist == 0.0)
	  return dist;
	
	return dist / (oskmeans::enorm(p1)*oskmeans::enorm(p2));
     }
   
   float oskmeans::distance_normed_points(const hash_map<uint32_t,float,id_hash_uint> &p1,
					  const hash_map<uint32_t,float,id_hash_uint> &p2)
     {
	float dist = 0.0;
	hash_map<uint32_t,float,id_hash_uint>::const_iterator hit = p1.begin();
	hash_map<uint32_t,float,id_hash_uint>::const_iterator hit2;
	while(hit!=p1.end())
	  {
	     if ((hit2=p2.find((*hit).first))!=p2.end())
	       {
		  dist += (*hit).second * (*hit2).second;
	       }
	     ++hit;
	  }
	
	return dist;
     }
   
   // static.
   float oskmeans::enorm(const hash_map<uint32_t,float,id_hash_uint> &p)
     {
	float enorm = 0.0;
	hash_map<uint32_t,float,id_hash_uint>::const_iterator hit = p.begin();
	while(hit!=p.end())
	  {
	     enorm += (*hit).second*(*hit).second;
	     ++hit;
	  }
	return sqrt(enorm);
     }
         
   void oskmeans::recompute_centroid(const float &learning_rate,
				     centroid *c,
				     hash_map<uint32_t,float,id_hash_uint> *p,
				     float &cl_norm)
     {
	hash_map<uint32_t,float,id_hash_uint>::const_iterator phit = p->begin();
	hash_map<uint32_t,float,id_hash_uint>::iterator chit;
	while(phit!=p->end())
	  {
	     float update = learning_rate * (*phit).second;
	     if ((chit=c->_features.find((*phit).first))!=c->_features.end())
	       {
		  (*chit).second += update;
		  cl_norm += (*chit).second;
	       }
	     else 
	       {
		  c->_features.insert(std::pair<uint32_t,float>((*phit).first,update));
		  cl_norm += update;
	       }
	     ++phit;
	  }
     }
      
   void oskmeans::normalize_centroid(centroid *c,
				     const float &cl_norm)
     {
	// normalize.
	hash_map<uint32_t,float,id_hash_uint>::iterator chit = c->_features.begin();
	while(chit!=c->_features.end())
	  {
	     (*chit).second /= cl_norm;
	     ++chit;
	  }
     }

   void oskmeans::rank_elements(cluster &cl)
     {
	hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::iterator hit
	  = cl._cpoints.begin();
	while(hit!=cl._cpoints.end())
	  {
	     float dist = oskmeans::distance_normed_points(*(*hit).second,cl._c._features);
	     search_snippet *sp = _qc->get_cached_snippet((*hit).first);
	     sp->_seeks_ir = static_cast<double>(dist);
	     ++hit;
	  }
     }
      
} /* end of namespace. */
