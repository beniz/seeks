/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2007 Emmanuel Benazera, juban@free.fr
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

/**
 * \file$Id:
 * \brief static operations for hashtable buckets.
 * \author E. Benazera, juban@free.fr
 */

#ifndef BUCKETOPERATIONS_H
#define BUCKETOPERATIONS_H

#include "Bucket.h"

namespace lsh
{

  /**
   * \class BucketOperations
   * \brief static methods for the different types of hashtable buckets.
   */
  template<class T>
  class BucketOperations
  {
    public:

      static void unionB (const Bucket<T> &b1, const Bucket<T> &b2,
                          std::set<T> &res);

      static void unionB (const Bucket<T> &b, const std::set<T> &s,
                          std::set<T> &res);

      static void Lunion (const std::vector<Bucket<T>*> &vec,
                          std::set<T> &res);

      static void LunionWithProbabilities (const std::vector<Bucket<T>*> &vec,
                                           std::map<double, const T, std::greater<double> > &probs);


  };

  template<class T> void BucketOperations<T>::LunionWithProbabilities (const std::vector<Bucket<T>*> &vec,
      std::map<double, const T, std::greater<double> > &probs)
  {
    double total_elts = 0;
    std::map<const T,double> probs_tmp;
    for (unsigned int i=0; i<vec.size (); i++)
      {
        Bucket<T> *bck = vec[i];
        typename std::set<T>::const_iterator bit = bck->getBeginIterator ();
        while (bit != bck->getEndIterator ())
          {
            total_elts++;

            bool eq = false;
            typename std::map<const T,double>::iterator mit = probs_tmp.begin();
            while (mit!=probs_tmp.end())
              {
                //BEWARE: operator must be overloaded in class T.
                if ((*mit).first == (*bit))
                  {
                    eq = true;
                    break;
                  }
                ++mit;
              }

            if (eq)
              (*mit).second++;
            else probs_tmp.insert (std::pair<const T,double> (*bit,1.0));
            ++bit;
          }
      }

    /**
     * normalize probabilities and inverse the mapping.
     */
    typename std::map<const T,double>::iterator mit = probs_tmp.begin ();
    while (mit != probs_tmp.end ())
      {
        probs.insert(std::pair<double,const T>((*mit).second / total_elts,
                                               (*mit).first));
        ++mit;
      }
  }

  template<class T> void BucketOperations<T>::unionB (const Bucket<T> &b1, const Bucket<T> &b2,
      std::set<T> &res)
  {
    std::set_union (b1.getBeginIterator (), b1.getEndIterator (),
                    b2.getBeginIterator (), b2.getEndIterator (),
                    std::insert_iterator<std::set<T> > (res, res.begin ()),
                    std::less<T> ());
  }

  template<class T> void BucketOperations<T>::unionB (const Bucket<T> &b, const std::set<T> &s,
      std::set<T> &res)
  {
    std::set_union (b.getBeginIteratorConst (), b.getEndIteratorConst (),
                    s.begin (), s.end (),
                    std::insert_iterator<std::set<T> > (res, res.begin ()),
                    std::less<T> ());
  }

  template<class T> void BucketOperations<T>::Lunion (const std::vector<Bucket<T>*> &vec,
      std::set<T> &res)
  {
    if (vec.empty ())
      return;
    if (vec.size () == 1)
      {
        res = vec[0]->getBChainC();
        return;
      }

    std::set<T> sres;
    for (unsigned int i=0; i<vec.size (); i++)
      {
        BucketOperations<T>::unionB (*vec[i], res, res);
      }
  }

} /* end of namespace. */

#endif
