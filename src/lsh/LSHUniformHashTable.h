/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2006 Emmanuel Benazera, juban@free.fr
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * \file$Id:
 * \brief This is a dual key uniform hashtable, where collision is resolved by chaining colliding entries.
 * \author E. Benazera, juban@free.fr
 */

#ifndef LSHUNIFORMHASHTABLE_H
#define LSHUNIFORMHASHTABLE_H

#include "Bucket.h"
#include "BucketOperations.h"
#include "LSHSystem.h"
#include <queue>
#include <vector>
#include <ostream>

namespace lsh
{

  /**
   * \class LSHUniformHashTable
   * \brief Dual key uniform hash table with collision resolved by chaining.
   */
  template<class T>
  class LSHUniformHashTable
  {
    public:
      /**
       * \brief Hashtable constructor with default size.
       */
      LSHUniformHashTable ();

      /**
       * \brief Hashtable constructor.
       * @param uhsize maximum hashtable size.
       */
      LSHUniformHashTable (const unsigned long int &uhsize);

      /**
       * \brief Hashtable destructor.
       */
      virtual ~LSHUniformHashTable ();

      /**
       * Bucket creation: this must be overloaded for using other subclasses
       * of Bucket as low-level containers of the uniform hashtable.
       */
      virtual Bucket<T>* createBucket(const unsigned long int &ckey,
                                      const T &te);

      /**
       * Main operations: insertion, removal, ...
       */

      /**
       * \brief add an element to the locality sensitive hashtable:
       *        - the element is added L times,
       *        - its key is computed based on K sampling functions.
       *        L is the constant parameter that is defined in the metric-associated classes.
       * @sa LSHSystemHamming, LSHSystemEucl
       *
       * @param te element to be added,
       * @param L number of times the element is added.
       * @return the mean code returned by the L adding operations.
       */
      int add (const T &te,
               const unsigned int L);

      /**
       * \brief add an element to the locality sensitive hashtable, based on its main & control keys.
       * @param mkey main hashed key,
       * @param ckey control hashed key,
       * @param te element to be added.
       * @return 3 if a new entry has been added to the table, 2 if a new bucket has been added
       *         to an existing entry, 1 if an element has been added to an existing bucket of
       *         an existing entry.
       */
      int add (const unsigned long int &mkey,
               const unsigned long int &ckey,
               const T &te);

      /**
       * \brief removes an element from the locality sensitive hashtable:
       *        - the element is removed L times.
       * @param te element to be removed.
       * @param L number of time the element is removed.
       * @return the mean code returned by the L removal operations.
       */
      int remove (const T &te,
                  const unsigned int L);

      /**
       * \brief remove an element from the locality sensitive hashtable, based on its main & control
       *        keys.
       * @param mkey main hased key.
       * @param ckey control hashed key.
       * @param te element to be removed.
       * @return L if all elements have been correctly removed, a number between 0 and L otherwise.
       */
      int remove (const unsigned long int &mkey,
                  const unsigned long int &ckey,
                  const T &te);

      std::set<T> getLElts (const T &te, const unsigned int& L);

      std::map<double,const T,std::greater<double> > getLEltsWithProbabilities (const T &te,
          const unsigned int &L);

      std::vector<Bucket<T>*> getL (const T &te,
                                    const unsigned int L);

      Bucket<T>* get (const unsigned long int &key,
                      const unsigned long int &ckey);

      typename std::vector<Bucket<T>*>::const_iterator getConstIterator (const unsigned long int &key,
          const unsigned long int &ckey);

      typename std::vector<Bucket<T>*>::iterator getIterator (const unsigned long int &key,
          const unsigned long int &ckey);

      std::vector<Bucket<T>*>* getBuckets (const unsigned long int &key) const;

      Bucket<T>* find (const unsigned long int &ckey,
                       const std::vector<Bucket<T>*> &vbck);

      typename std::vector<Bucket<T>*>::const_iterator findConstIterator (const unsigned long int &ckey,
          const std::vector<Bucket<T>*> &vbck);

      typename std::vector<Bucket<T>*>::iterator findIterator (const unsigned long int &ckey,
          std::vector<Bucket<T>*> &vbck);

      /**
       * Index management.
       */
      void removeIndex (const unsigned long int &key);

      std::vector<unsigned long int>::iterator getIndexBeginIterator ()
      {
        return _filled.begin ();
      }

      std::vector<unsigned long int>::iterator getIndexEndIterator ()
      {
        return _filled.end ();
      }

      unsigned int filledSize () const
      {
        return _filled.size ();
      }

      /**
       * Memory management.
       */
      Bucket<T>* getNextAllocatedBucket ();

      void freeUnusedAllocatedBuckets ();

      /**
       * Counting.
       */
      int countBuckets() const;

      double meanBucketsPerBin() const;

      /**
       * Printing.
       */
      std::ostream& printSlot (std::ostream &output, std::vector<Bucket<T>*> *slot);

      std::ostream& print (std::ostream &output);

      /**
       * Key management, virtual functions.
       */
      virtual void LcomputeMKey (T te,
                                 unsigned long int *Lmkeys)
      {
        std::cout << "[Warning]:Using undefined virtual root function LSHUniformHashTable::LcomputeMKey !\n";
      }

      virtual void LcomputeCKey (T te,
                                 unsigned long int *Lckeys)
      {
        std::cout << "[Warning]:Using undefined virtual root function LSHUniformHashTable::LcomputeCKey !\n";
      }

      virtual void LcomputeMCKey (T te,
                                  unsigned long int *Lmkeys,
                                  unsigned long int *Lckeys)
      {
        std::cout << "[Warning]:Using undefined virtual root function LSHUniformHashTable::LcomputeMCKey !\n";
      }

    protected:
      /**
       * Hashtable size. This number is a constant for each instance.
       */
      const unsigned long int _uhsize;

    public:
      static const unsigned int _default_uhsize = 100;

    protected:
      /**
       * uhsize maximum allocated buckets as the main table.
       */
      std::vector<Bucket<T>*> **_table;

      /**
       * filled indexes.
       */
      std::vector<unsigned long int> _filled;

      /**
       * Already allocated buckets, here for reuse.
       */
      std::queue<Bucket<T>*> _aallb;
  };

  template<class T> LSHUniformHashTable<T>::LSHUniformHashTable ()
      : _uhsize (LSHUniformHashTable::_default_uhsize)
  {
    _table = new std::vector<Bucket<T>*>*[_uhsize];
    for (unsigned long int l=0; l<_uhsize; l++)
      _table[l] = NULL;
  }

  template<class T> LSHUniformHashTable<T>::LSHUniformHashTable (const unsigned long int &uhsize)
      : _uhsize (uhsize)
  {
    _table = new std::vector<Bucket<T>*>*[_uhsize];
    for (unsigned long int l=0; l<uhsize; l++)
      _table[l] = NULL;
  }

  template<class T> LSHUniformHashTable<T>::~LSHUniformHashTable ()
  {
    /**
     * Clear buckets.
     */
    std::vector<unsigned long int>::iterator iit = getIndexBeginIterator ();
    while (iit != getIndexEndIterator ())
      {
        std::vector<Bucket<T>*> *vcbk = _table[(*iit)];
        for (unsigned int i=0; i<vcbk->size (); i++)
          delete (*vcbk)[i];
        delete vcbk;
        ++iit;
      }

    delete []_table;

    freeUnusedAllocatedBuckets ();
  }

  template<class T> Bucket<T>* LSHUniformHashTable<T>::createBucket(const unsigned long int &ckey,
      const T &te)
  {
    return new Bucket<T>(ckey,te);
  }

  template<class T> int LSHUniformHashTable<T>::add (const T &te,
      const unsigned int L)
  {
    unsigned long int Lmkeys[L];
    unsigned long int Lckeys[L];

    LcomputeMCKey (te, Lmkeys, Lckeys);

    int r = 0;
    for (unsigned int l=0; l<L; l++)
      {

        //debug
        /* std::cout << "[Debug]:LSHUniformHashTable:add: adding ("
        << Lmkeys[l] << ", " << Lckeys[l] << ")\n"; */
        //std::cout << "table size: " << _filled.size () << std::endl;
        //debug

        r += add (Lmkeys[l], Lckeys[l], te);
      }

    return static_cast<int> (r / L);
  }

  template<class T> int LSHUniformHashTable<T>::add (const unsigned long int &mkey,
      const unsigned long int &ckey,
      const T &te)
  {
    /**
     * Look up for an existing instance. If nothing is found, create a vector and a bucket.
     */
    Bucket<T> *bck = NULL;
    std::vector<Bucket<T>*> *vbck = NULL;

    if ((vbck = _table[mkey]) == NULL)
      {
        if ((bck = getNextAllocatedBucket ()) == NULL)
          bck = createBucket(ckey,te);
        else
          {
            bck->setKey (ckey);
            bck->add (te);
          }

        vbck = new std::vector<Bucket<T>*> (1);
        (*vbck)[0] = bck;
        _table[mkey] = vbck;
        _filled.push_back (mkey);
        return 3;
      }

    /**
     * Else, look up for the control key. If nothing is found, create a bucket.
     */
    if ((bck = find (ckey, *vbck)) == NULL)
      {
        if ((bck = getNextAllocatedBucket ()) == NULL)
          bck = createBucket(ckey,te);
        else
          {
            bck->setKey (ckey);
            bck->add (te);
          }

        /**
         * if the vector is empty, then put back this slot into the index
         * of filled slots, since we're adding a bucket to it.
         */
        if (vbck->empty ())
          _filled.push_back (mkey);

        vbck->push_back (bck);
        return 2;
      }

    /**
     * Else, if the element is not already in the bucket, add it up.
     */
    //typename std::vector<T>::iterator vit;
    /* if ((vit = bck->getIterator (te)) == bck->getEndIterator ()) */
    bck->add (te);
    return 1;
  }

  template<class T> int LSHUniformHashTable<T>::remove (const T &te,
      const unsigned int L)
  {
    unsigned long int Lmkeys[L];
    unsigned long int Lckeys[L];

    LcomputeMCKey (te, Lmkeys, Lckeys);

    int r = 0;
    for (unsigned int l=0; l<L; l++)
      {
        r += remove (Lmkeys[l], Lckeys[l], te);

        //debug
        /* std::cout << "[Debug]:LSHUniformHashTable:add: removing ("
        << Lmkeys[l] << ", " << Lckeys[l] << ")\n";
        std::cout << "table size: " << _filled.size () << std::endl; */
        //debug
      }

    return r / L;
  }

  template<class T> int LSHUniformHashTable<T>::remove (const unsigned long int &mkey,
      const unsigned long int &ckey,
      const T &te)
  {
    /**
     * Look up for the bucket.
     */
    Bucket<T> *bck;
    if ((bck = get (mkey, ckey)) == NULL)
      {
        std::cout << "[Error]:LSHUniformHashTable::remove: Can't find bucket ("
                  << mkey << ", " << ckey << "). Removing operation failed.\n";
        return 0;
      }

    int r = bck->removeElement (te);

    /**
     * Remove the bucket if empty.
     * Don't deallocate the bucket. Keep it for later.
     * Don't deallocate the vector, leave it in place for later use.
     */
    if (bck->isEmpty ())
      {
        typename std::vector<Bucket<T>*>::iterator vit = getIterator (mkey, ckey);
        std::vector<Bucket<T>*> *vbck = getBuckets (mkey);
        vbck->erase (vit);

        bck->reset ();
        _aallb.push (bck);

        if (vbck->empty ())
          {
            removeIndex (mkey);
            /* delete vbck; */  /* TODO: could keep it and reuse it instead. */
          }
      }
    return r;
  }

  template<class T> void LSHUniformHashTable<T>::removeIndex (const unsigned long int &key)
  {
    std::vector<unsigned long int>::iterator vit = _filled.begin ();
    while (vit != _filled.end ())
      {
        if ((*vit) == key)
          {
            break;
          }
        ++vit;
      }
    _filled.erase (vit);
  }

  template<class T> std::vector<Bucket<T>*> LSHUniformHashTable<T>::getL (const T &te,
      const unsigned int L)
  {
    std::vector<Bucket<T>*> r;

    unsigned long int Lmkeys[L];
    unsigned long int Lckeys[L];

    LcomputeMCKey (te, Lmkeys, Lckeys);

    for (unsigned int l=0; l<L; l++)
      {
        Bucket<T> *bck;

        if ((bck = get (Lmkeys[l], Lckeys[l])) != NULL)
          {

            //debug
            /* std::cout << "getting: mkey: " << Lmkeys[l] << " -- ckey: " << Lckeys[l] << std::endl;
               std::cout << bck << std::endl; */
            //debug

            r.push_back (bck);
          }
      }

    return r;
  }

  template<class T> std::set<T> LSHUniformHashTable<T>::getLElts (const T &te,
      const unsigned int& L)
  {
    std::vector<Bucket<T>*> v_res = getL (te, L);
    std::set<T> s_res;
    BucketOperations<T>::Lunion (v_res, s_res);
    return s_res;
  }

  template<class T> std::map<double,const T,std::greater<double> > LSHUniformHashTable<T>::getLEltsWithProbabilities (const T &te,
      const unsigned int &L)
  {
    std::vector<Bucket<T>*> v_res = getL (te, L);
    std::map<double, const T, std::greater<double> > m_res;
    BucketOperations<T>::LunionWithProbabilities (v_res, m_res);
    return m_res;
  }

  template<class T> Bucket<T>* LSHUniformHashTable<T>::get (const unsigned long int &key,
      const unsigned long int &ckey)
  {
    std::vector<Bucket<T>*> *vbck = NULL;
    if ((vbck = getBuckets (key)) == NULL)
      {
        return NULL;
      }

    return find (ckey, *vbck);
  }

  template<class T> typename std::vector<Bucket<T>*>::const_iterator LSHUniformHashTable<T>::getConstIterator (const unsigned long int &key,
      const unsigned long int &ckey)
  {
    std::vector<Bucket<T>*> *vbck = NULL;
    if ((vbck = getBuckets (key)) == NULL)
      {
        std::cout << "[Error]:LSHUniformHashTable<T>::getConstIterator: can't find key: "
                  << ckey << ".Exiting\n";
        exit (-1);
      }

    return findConstIterator (ckey, *vbck);
  }

  template<class T> typename std::vector<Bucket<T>*>::iterator LSHUniformHashTable<T>::getIterator (const unsigned long int &key,
      const unsigned long int &ckey)
  {
    std::vector<Bucket<T>*> *vbck = NULL;
    if ((vbck = getBuckets (key)) == NULL)
      {
        std::cout << "[Error]:LSHUniformHashTable<T>::getIterator: can't find key: "
                  << ckey << ".Exiting\n";
        exit (-1);
      }

    return findIterator (ckey, *vbck);
  }

  template<class T> std::vector<Bucket<T>*>* LSHUniformHashTable<T>::getBuckets (const unsigned long int &key) const
  {
    if (key < _uhsize)
      return _table[key];
    else
      {
        std::cout << "[Error]:LSHUniformHashTable::getBuckets: " << key
                  << " is beyond the table size: " << _uhsize
                  << ". Returning NULL pointer" << std::endl;
        return NULL;
      }
  }

  template<class T> Bucket<T>* LSHUniformHashTable<T>::find (const unsigned long int &ckey,
      const std::vector<Bucket<T>*> &vbck)
  {
    typename std::vector<Bucket<T>*>::const_iterator vit;
    for (vit=vbck.begin (); vit != vbck.end (); ++vit)
      {
        if ((*vit)->getKey () == ckey)
          return (*vit);
      }
    return NULL;
  }

  template<class T> typename std::vector<Bucket<T>*>::const_iterator LSHUniformHashTable<T>::findConstIterator (const unsigned long int &ckey,
      const std::vector<Bucket<T>*> &vbck)
  {
    typename std::vector<Bucket<T>*>::const_iterator vit;
    while (vit != vbck.end ())
      {
        if ((*vit)->getKey () == ckey)
          return vit;
        ++vit;
      }
    return vbck.end ();
  }

  template<class T> typename std::vector<Bucket<T>*>::iterator LSHUniformHashTable<T>::findIterator (const unsigned long int &ckey,
      std::vector<Bucket<T>*> &vbck)
  {
    typename std::vector<Bucket<T>*>::iterator vit = vbck.begin ();
    while (vit != vbck.end ())
      {
        if ((*vit)->getKey () == ckey)
          return vit;
        ++vit;
      }
    return vbck.end ();
  }

  template<class T> Bucket<T>* LSHUniformHashTable<T>::getNextAllocatedBucket ()
  {
    if (_aallb.empty ())
      return NULL;
    Bucket<T>* bck = _aallb.front ();
    _aallb.pop ();
    return bck;
  }

  template<class T> void LSHUniformHashTable<T>::freeUnusedAllocatedBuckets ()
  {
    while (! _aallb.empty ())
      {
        Bucket<T> *bck = _aallb.front ();
        delete bck;
        _aallb.pop ();
      }
  }

  template<class T> int LSHUniformHashTable<T>::countBuckets() const
  {
    int cbuckets = 0;
    std::vector<unsigned long int>::const_iterator fit = _filled.begin();
    while (fit!=_filled.end())
      {
        unsigned long int mkey = (*fit);
        std::vector<Bucket<T>*>* buckets = getBuckets(mkey);
        cbuckets += buckets->size();
        fit++;
      }
    return cbuckets;
  }

  template<class T> double LSHUniformHashTable<T>::meanBucketsPerBin() const
  {
    int cbuckets = countBuckets();
    unsigned int filled = filledSize();
    return static_cast<double>(cbuckets)/static_cast<double>(filled);
  }

  template<class T> std::ostream& LSHUniformHashTable<T>::printSlot (std::ostream &output,
      std::vector<Bucket<T>*> *slot)
  {
    for (unsigned int i=0; i<slot->size (); i++)
      {
        output << (*slot)[i];// << std::endl;
      }
    return output;
  }

  template<class T> std::ostream& LSHUniformHashTable<T>::print (std::ostream &output)
  {
    output << "************* uhash (" << _filled.size () << ") ****************\n";
    std::vector<unsigned long int>::iterator iit = getIndexBeginIterator ();
    int i = 0;
    while (iit != getIndexEndIterator ())
      {
        output << i << ": ";
        printSlot (output, _table[(*iit)]);
        ++iit;
        i++;
      }
    return output;
  }

  template<class T> std::ostream &operator<<(std::ostream &output, const LSHUniformHashTable<T> &uhash)
  {
    return uhash.print (output);
  }

  template<class T> std::ostream &operator<<(std::ostream &output, const LSHUniformHashTable<T> *uhash)
  {
    return uhash->print (output);
  }

} /* end of namespace */

#endif
