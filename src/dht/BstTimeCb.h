/**
 * This file is part of the Seeks project.
 * Copyright (C) 2006, 2010 Emmanuel Benazera, juban@free.fr
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

#ifndef BSTTIMECB_H
#define BSTTIMECB_H

#include "Bst.h"
#include "callback.h"
#include <time.h>  //GNU unix, TODO: win32.
#include <pthread.h>
#include <iostream>

namespace dht
{
#ifndef timespecOPS
#define timespecOPS
   
   /**
    * operators to timespec.
    */
   inline bool operator==(const timespec& tva, const timespec& tvb)
     {
	return tva.tv_sec == tvb.tv_sec && tva.tv_nsec == tvb.tv_nsec;
     }
   
   inline bool operator!=(const timespec& tva, const timespec& tvb)
     {
	return tva.tv_sec != tvb.tv_sec || tva.tv_nsec != tvb.tv_nsec;
     }
   
   inline bool operator<(const timespec& tva, const timespec& tvb)
     {
	return tva.tv_sec < tvb.tv_sec && tva.tv_nsec < tvb.tv_nsec;
     }
   
   inline bool operator>(const timespec& tva, const timespec& tvb)
     {
	return tva.tv_sec > tvb.tv_sec && tva.tv_nsec > tvb.tv_nsec;
     }
   
   inline bool operator<=(const timespec& tva, const timespec& tvb)
     {
	return (tva.tv_sec < tvb.tv_sec) 
	  || (tva.tv_sec == tvb.tv_sec && tva.tv_nsec <= tvb.tv_nsec);
     }
   
   inline bool operator>=(const timespec& tva, const timespec& tvb)
     {
	return (tva.tv_sec > tvb.tv_sec) 
	  || (tva.tv_sec == tvb.tv_sec && tva.tv_nsec >= tvb.tv_nsec);
     }
  
   /* printing timespec. */
   std::ostream& printTv (std::ostream& output, const timespec& tv);
   
   std::ostream& operator<<(std::ostream &output, const timespec& tv);
#endif

   /**
    * \class BstTimeCb
    * \brief binary search tree with time dates as keys and callbacks as values.
    */
   class BstTimeCb : public Bst<timespec, callback<int>* >
     {
      public:
	BstTimeCb(const timespec&, callback<int>* cb);
	
	BstTimeCb(const timespec&, callback<int>* cb,
		  BstTimeCb* rnode, BstTimeCb* lnode, BstTimeCb* fnode);  //USELESS ?

	~BstTimeCb();
	
	/**
	 * \brief execute the node's callback.
	 * UNUSED for now.
	 */
	int execute();
	
	static void setMaxVal(const timespec& val);
	static timespec getMaxVal() 
	  {
	     return BstTimeCb::_maxval; 
	  }
       
	static void updateMaxVal(BstTimeCb* b);

	static std::ostream& printBstTimeCb(std::ostream& output, BstTimeCb* bt);
	
      private:
	/**
	 * max value in the tree, for fast checking.
	 * Beware: initialized with the default constructor...
	 */
	static timespec _maxval;
     };

#ifndef TIMECHECKLOOP
#define TIMECHECKLOOP
   void* timecheck_loop(void* bsttimecb);
#endif
   
   /**
    * \class BstTimeCbTree
    * \brief this class is for managing a binary search tree with time dates as keys
    *        and callbacks as values.
    */
   class BstTimeCbTree
     {
      public:
	BstTimeCbTree();
	
	BstTimeCbTree(const timespec&, callback<int>* cb);
	
	~BstTimeCbTree();
	
	/**
	 * \brief checks on current time and executes all callbacks whose date is
	 *        below or equal to the current time.
	 */
	void timecheck();
	
	/**
	 * \brief starts a threaded loop over timecheck().
	 */
	void start_threaded_timecheck_loop();
	
	/**
	 * insert.
	 */
	void insert(const timespec& tv, callback<int>* cb);
	
	/**
	 * accessors.
	 */
	pthread_t& get_tcl_thread() 
	  {
	     return _tcl_thread; 
	  }
	
      protected:
	/**
	 * bst root.
	 */
	BstTimeCb* _bstcbtree;
	
	/**
	 * timecheck loop thread id.
	 */
	pthread_t _tcl_thread;

	/**
	 * pthread mutex on the tree.
	 */
	//pthread_mutex_t _cbtree_mutex;
     };
      
} /* end of namespace. */

#endif
