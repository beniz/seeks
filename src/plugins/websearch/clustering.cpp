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
#include "seeks_proxy.h"
#include "errlog.h"
#include "miscutil.h"
#include "lsh_configuration.h"

using sp::errlog;
using sp::miscutil;
using sp::seeks_proxy;
using lsh::lsh_configuration;
using lsh::stopwordlist;

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
    while (hit!=_cpoints.end())
      {
        search_snippet *sp = qc->get_cached_snippet((*hit).first);
        _rank += sp->_seeks_rank; // summing up the seeks rank yields a cluster rank (we could use the mean instead).
        ++hit;
      }
  }

  void cluster::compute_label(const query_context *qc)
  {
    // compute total tf-idf weight for features of docs belonging to this cluster.
    hash_map<uint32_t,float,id_hash_uint> f_totals;
    hash_map<uint32_t,float,id_hash_uint>::iterator fhit,hit2;
    hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit
    = _cpoints.begin();
    while (hit!=_cpoints.end())
      {
        hit2 = (*hit).second->begin();
        while (hit2!=(*hit).second->end())
          {
            if ((fhit=f_totals.find((*hit2).first))!=f_totals.end())
              {
                (*fhit).second += (*hit2).second;
              }
            else f_totals.insert(std::pair<uint32_t,float>((*hit2).first,(*hit2).second));
            ++hit2;
          }
        ++hit;
      }

    // grab features with the highest tf-idf weight.
    std::map<float,uint32_t,std::greater<float> > f_mtotals;
    fhit = f_totals.begin();
    while (fhit!=f_totals.end())
      {
        f_mtotals.insert(std::pair<float,uint32_t>((*fhit).second,(*fhit).first));
        ++fhit;
      }
    f_totals.clear();

    // we need query words and a stopword list for rejecting labels.
    std::vector<std::string> words;
    miscutil::tokenize(qc->_query,words," ");
    size_t nwords = words.size();
    stopwordlist *swl = seeks_proxy::_lsh_config->get_wordlist(qc->_auto_lang);

    // turn features into word labels.
    int k=0;
    int KW = 2; // number of words per label. TODO: use weights for less/more words...
    std::map<float,uint32_t,std::greater<float> >::iterator mit = f_mtotals.begin();
    while (mit!=f_mtotals.end())
      {
        bool found = false;
        if (k>KW)
          break;
        else
          {
            hit = _cpoints.begin();
            while (hit!=_cpoints.end())
              {
                uint32_t id = (*hit).first;
                search_snippet *sp = qc->get_cached_snippet(id);

                hash_map<uint32_t,std::string,id_hash_uint>::const_iterator bit;
                if ((bit=sp->_bag_of_words->find((*mit).second))!=sp->_bag_of_words->end())
                  {
                    // two checks needed: whether the word already belongs to the query
                    // (can happen after successive use of cluster label queries);
                    // whether the word belongs to the english stop word list (because
                    // whatever the query language, some english results sometimes gets in).
                    bool reject = false;
                    for (size_t i=0; i<nwords; i++)
                      {
                        if (words.at(i) == (*bit).second) // check against query word.
                          {
                            reject = true;
                            break;
                          }
                      }
                    if (!reject)
                      {
                        reject = swl->has_word((*bit).second); // check against the english stopword list.
                      }

                    if (reject)
                      {
                        ++hit;
                        continue;
                      }

                    /* std::cerr << "adding to label: " << (*bit).second
                      << " --> " << (*mit).first << std::endl; */

                    if (!_label.empty())
                      _label += " ";
                    _label += (*bit).second;
                    found = true;
                    break;
                  }
                ++hit;
              }
          }
        ++mit;
        if (found)
          k++;
      }

    //std::cerr << "label: " << _label << std::endl;
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
    for (size_t s=0; s<nsp; s++)
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

    // compute labels.
    compute_cluster_labels();
  }

  // default ranking is as computed by seeks on the main list of results.
  void clustering::rank_elements(cluster &cl)
  {
    hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::iterator hit
    = cl._cpoints.begin();
    while (hit!=cl._cpoints.end())
      {
        search_snippet *sp = _qc->get_cached_snippet((*hit).first);
        sp->_seeks_ir = sp->_seeks_rank;
        ++hit;
      }
  }

  void clustering::rank_clusters_elements()
  {
    for (short c=0; c<_K; c++)
      rank_elements(_clusters[c]);
  }

  void clustering::compute_clusters_rank()
  {
    for (short c=0; c<_K; c++)
      _clusters[c].compute_rank(_qc);
  }

  void clustering::compute_cluster_labels()
  {
    for (short c=0; c<_K; c++)
      _clusters[c].compute_label(_qc);
  }

  hash_map<uint32_t,float,id_hash_uint>* clustering::get_point_features(const short &np)
  {
    short p = 0;
    hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit
    = _points.begin();
    while (hit!=_points.end())
      {
        if (p == np)
          return (*hit).second;
        p++;
        ++hit;
      }
    return NULL;
  }

} /* end of namespace. */
