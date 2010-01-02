/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and 
 * does provide several locality sensitive hashing schemes for pattern matching over 
 * continuous and discrete spaces.
 * Copyright (C) 2006,2007 Emmanuel Benazera, juban@free.fr
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
 * \brief This is a hash table bucket, that stores a chain of entries with colliding keys.
 * \author E. Benazera, juban@free.fr
 */

#ifndef BUCKET_H
#define BUCKET_H

#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

namespace lsh
{
  
  /**
   * \brief types of buckets for type detection at creation.
   * @sa createBucket
   */
  enum BucketType {
    BucketT, SearchGroupT, SearchGroupStatsT
  };

  /**
   * \class Bucket
   * \brief hashtable bucket with chaining.
   */
  template<class T> 
    class Bucket
    {
      public:
        Bucket ();

	Bucket (const unsigned long int &key);
	
	/* Bucket (const unsigned long int &key,
	   const int &size); */
	
	Bucket (const unsigned long int &key,
		const T &te);

	~Bucket () {};

	void add (const T &te);
       
        //to be overloaded as needed.
	bool find (const T &te);

	typename std::set<T>::iterator getIterator (const T &te);

	typename std::set<T>::iterator getBeginIterator ();
	
	typename std::set<T>::iterator getEndIterator ();

	typename std::set<T>::const_iterator getBeginIteratorConst () const;

	typename std::set<T>::const_iterator getEndIteratorConst () const;
	
	void remove (typename std::set<T>::iterator vit);

	int removeElement (const T &te);

	bool isEmpty () const { return _bchain.empty (); }

	unsigned int size () const { return _bchain.size (); }

	void reset ();

	std::ostream& print (std::ostream &output) const;
	
	/**
	 * accessors.
	 */
	unsigned long int getKey () const { return _bkey; }
	void setKey (const unsigned long int &bk) { _bkey = bk; }
	
	/**
	 * \brief returns a copy of the bucket chain of element (set).
	 * @return copy set of the bucket chain (set).
	 */
	std::set<T> getBChainC() { return _bchain; }
	
    public:
	unsigned int memSize() const { return sizeof(_bkey) + sizeof(_bchain)
	    + _bchain.size() * sizeof(T);}
	
      protected:
	BucketType _type;
	unsigned long int _bkey;
	std::set<T> _bchain;
	
    };

  template<class T> Bucket<T>::Bucket ()
    : _type(BucketT),_bkey (0)
    {}

  template<class T> Bucket<T>::Bucket (const unsigned long int &key)
    : _type(BucketT),_bkey (key)
    {}

    /* template<class T> Bucket<T>::Bucket (const unsigned long int &key,
				       const int &size)
    : _bkey (key)
    {
      _bchain (size);
      } */

  template<class T> Bucket<T>::Bucket (const unsigned long int &key,
				       const T &te)
    : _type(BucketT),_bkey (key)
    {
      add (te);
    }

  template<class T> void Bucket<T>::add (const T &te)
    {
      _bchain.insert (te);
    }

  template<class T> bool Bucket<T>::find (const T& te)
    {
      if (_bchain.find(te) != _bchain.end())
	return true;
      return false;
    }

  template<class T> typename std::set<T>::iterator Bucket<T>::getIterator (const T &te)
    {
      typename std::set<T>::iterator vit;
      for (vit = _bchain.begin (); vit != _bchain.end (); ++vit)
	if (*vit == te)
	  return vit;
      return _bchain.end ();
    }

  template<class T> typename std::set<T>::iterator Bucket<T>::getBeginIterator ()
    {
      return _bchain.begin ();
    }

  template<class T> typename std::set<T>::iterator Bucket<T>::getEndIterator ()
    {
      return _bchain.end ();
    }

  template<class T> typename std::set<T>::const_iterator Bucket<T>::getBeginIteratorConst () const
    {
      return _bchain.begin ();
    }

  template<class T> typename std::set<T>::const_iterator Bucket<T>::getEndIteratorConst () const
    {
      return _bchain.end ();
    }

  template<class T> void Bucket<T>::remove (typename std::set<T>::iterator vit)
    {
      _bchain.erase (vit);
    }

  template<class T> int Bucket<T>::removeElement (const T &te)
    {
      typename std::set<T>::iterator vit = getIterator (te);
      if (vit == _bchain.end ())
	{
	  std::cout << "[Error]:Bucket::removeElement: Can't find element: "
		    << te << ". Removing operation failed.\n";
	  return 0;
	}
      remove (vit);
      return 1;
    }

  template<class T> std::ostream& Bucket<T>::print (std::ostream &output) const
    {
      output << "********* Bucket #" << _bkey << " **********\n";
      typename std::set<T>::const_iterator vit = _bchain.begin ();
      int i=0;
      while (vit != _bchain.end ())
	{
	  output << i << ": " << (*vit) << std::endl;
	  i++;
	  ++vit;
	}
      output << "*******************************\n";
      return output;
    }
  
  template<class T> void Bucket<T>::reset ()
    {
      _bkey = 0;
      _bchain.clear ();  /* should not be required. */
    }

  template<class T> std::ostream &operator<<(std::ostream &output, const Bucket<T> &bck)
    {
      return bck.print (output);
    }
  
  template<class T> std::ostream &operator<<(std::ostream &output, Bucket<T> *bck)
    {
      return bck->print (output);
    }

  
} /* end of namespace */

#endif
