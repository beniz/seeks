/**
 *  This file is part of the Seeks project.
 *  Copyright (C) 2006, 2010 Emmanuel Benazera, juban@free.fr
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

#include "BstTimeCb.h"
#include <time.h>
#include <unistd.h>
#include <iostream>

namespace dht
{
   /* printing timeval. */
   std::ostream& printTv (std::ostream& output, const timespec& tv)
     {
	output << "(" << tv.tv_sec << "," << tv.tv_nsec << ")";
	return output;
     }
   
   std::ostream& operator<<(std::ostream &output, const timespec& tv)
     {	
	return printTv(output, tv);
     }
   
   /*-- BstTimeCb --*/
   timespec BstTimeCb::_maxval;
   
   BstTimeCb::BstTimeCb(const timespec& tv, callback<int>* cb)
     : Bst<timespec, callback<int>* >(tv, cb)
       {  
         BstTimeCb::setMaxVal(tv);
       }
   
   BstTimeCb::BstTimeCb(const timespec& tv, callback<int>* cb,
			BstTimeCb* rnode, BstTimeCb* lnode, BstTimeCb* fnode)
     : Bst<timespec, callback<int>* >(tv, cb, rnode, lnode, fnode)
       {
         BstTimeCb::setMaxVal(tv);
       }

   BstTimeCb::~BstTimeCb()
     {
       if (_ptR)
         {
           delete _ptR;
           _ptR = NULL;
         }
       if (_ptL)
         {
           delete _ptL;
           _ptL= NULL;
         }
       if (_ptF)
         {
           delete _ptF;
           _ptF = NULL;
         }
	for (unsigned int i=0; i<_data.size(); i++)
	  delete _data[i];
     }
   
   //static members
   void BstTimeCb::setMaxVal(const timespec& val)
       {
	  if (val > BstTimeCb::_maxval)
	    BstTimeCb::_maxval = val;
       }
   
   /**
    * TODO: we could be smarter than that, and only update the tree
    * max val when the deepest node on the right side of the tree is
    * deleted.
    */
   void BstTimeCb::updateMaxVal(BstTimeCb* b)
     {
	BstTimeCb* temp = b;
	while(temp->getRightChild())
	  temp = static_cast<BstTimeCb*>(temp->getRightChild());
	BstTimeCb::setMaxVal(temp->getValue());
     }
   
   int BstTimeCb::execute()
     {	
	int result = 0;
	std::vector<callback<int>* >::iterator sit = _data.begin();
	while (sit != _data.end())
	  {
	     result += (*(*sit))();
	     sit++;
	  }
	return result;
     }

   std::ostream& BstTimeCb::printBstTimeCb(std::ostream& output, BstTimeCb* bt)
     {
	output << "Max value: " << BstTimeCb::_maxval << std::endl;
	return BstTimeCb::printBst(output, bt);
     }
   
   
   /*-- BstTimeCbTree --*/
   // TODO: mutex def goes elsewhere.
   pthread_mutex_t _cbtree_mutex = PTHREAD_MUTEX_INITIALIZER;
   
   BstTimeCbTree::BstTimeCbTree()
     :_bstcbtree(NULL),_abort(false)
     {
       mutex_init(&_cbtree_mutex);
       mutex_init(&_run_mutex);
       start_threaded_timecheck_loop();
     }
   
   BstTimeCbTree::BstTimeCbTree(const timespec& tv, callback<int>* cb)
     {
       mutex_init(&_cbtree_mutex);
       mutex_init(&_run_mutex);
       _bstcbtree = new BstTimeCb(tv, cb);
       start_threaded_timecheck_loop();
     }

   BstTimeCbTree::~BstTimeCbTree()
     {
       stop_threaded_timecheck_loop();
       if (_bstcbtree)
         delete _bstcbtree;
     }
      
   void BstTimeCbTree::insert(const timespec& tv, callback<int>* cb)
     {
	//if (_bstcbtree)
	_bstcbtree = static_cast<BstTimeCb*>(BstTimeCb::insert(_bstcbtree, tv, cb));
	//else _bstcbtree = new BstTimeCb(tv, cb);
     }
   
   void BstTimeCbTree::timecheck()
     {
	mutex_lock(&_cbtree_mutex);
	
	/**
	 * get the current clock.
	 */
	timespec tnow;
	clock_gettime(CLOCK_REALTIME, &tnow);

	//debug
	//std::cout << "[Debug]:BstTimeCb::timecheck: " << tnow << std::endl;
	/* std::cout << "[Debug]:BstTimeCb::timecheck: bstcbtree: " << _bstcbtree << std::endl; */
	//debug
	
	/**
	 * checks if there nothing to be done.
	 */
	if (!_bstcbtree || BstTimeCb::getMaxVal() > tnow) //TODO: test Max value.
	  {
	     pthread_mutex_unlock(&_cbtree_mutex);
	     return;
	  }
	
	/**
	 * grab all tree nodes whose keys are below the current clock.
	 */
	std::vector<Bst<timespec, callback<int>* >*> allnodes;
	BstTimeCb* btc_upnode = static_cast<BstTimeCb*>(_bstcbtree->getAllNodesBelow(tnow, allnodes));

	//std::cout << "tree size: " << _bstcbtree->size() << std::endl;
	//std::cout << "allnodes size: " << allnodes.size() << std::endl;
	
	//debug
	if (!btc_upnode)
	  {
	     /* std::cout << "[Debug]:BstTimeCb::timecheck: no callbacks found below date: "
	      << tnow << std::endl; */
	     pthread_mutex_unlock(&_cbtree_mutex);
	     return;
	  }
	//debug
	
	/**
	 * execute callbacks.
	 */
	//debug
	/* std::cout << "[Debug]:BstTimeCb::timecheck: " << tnow << std::endl;
	 std::cout << "[Debug]:BstTimeCb::timecheck: number of callbacks to execute: "
	 << allnodes.size () << std::endl; */
	//debug
	
	for (unsigned int i=0; i<allnodes.size(); i++)
	  static_cast<BstTimeCb*>(allnodes[i])->execute();
	
	//debug
	/* std::cout << "[Debug]:BstTimeCb::timecheck: tree before removal:\n";
	 BstTimeCb::printBst(std::cout, _bstcbtree); */
	//debug
	
	for (unsigned int i=0; i<allnodes.size(); i++)
	  {
	     /**
	      * since removal doesn't conserve pointers, we need to look values up
	      * again.
	      */
	     bool root = false;
	     BstTimeCb* bst_upnode_n = static_cast<BstTimeCb*>(BstTimeCb::remove(allnodes[i]->getValue(), _bstcbtree, root));
	     if (root)
	       _bstcbtree = bst_upnode_n;
	  }

	if (_bstcbtree)
	  BstTimeCb::updateMaxVal(_bstcbtree);
	else BstTimeCb::setMaxVal(timespec()); 
	
	/**
	 * If we need a timeout for other checks within the same loop,
	 * then we should compute it here by looking at the tree's root key.
	 */

	//debug
	//BstTimeCb::printBst(std::cout, _bstcbtree);
	//debug
	
	mutex_unlock(&_cbtree_mutex);
     }

   void* timecheck_loop(void* bsttimecb)
     {
	BstTimeCbTree* btb = static_cast<BstTimeCbTree*>(bsttimecb);
	for (;;)
	  {
            mutex_lock(&btb->_run_mutex);
            if (btb->_abort)
              {
                mutex_unlock(&btb->_run_mutex);
                
                //debug                                                                
                std::cerr << "[Debug]:BstTimeCb::timecheck_loop: stopped thread with id:"
                          << btb->_tcl_thread << std::endl;
                //debug
              
                pthread_exit(NULL);
              }
            mutex_unlock(&btb->_run_mutex);
            
            btb->timecheck();
            sleep(5);
	  }
	return NULL;
     }
   
   // TODO: mutex it.
   // TODO: reuse an existing thread or use another mechanism (?).
   void BstTimeCbTree::start_threaded_timecheck_loop()
     {
	/**
	 * Thread creation: TODO: on win32.
	 */
	pthread_attr_t tcl_attr;
	pthread_attr_init(&tcl_attr);
	//pthread_attr_setdetachstate(&tcl_attr, PTHREAD_CREATE_DETACHED);
	int tcl_thr_res = pthread_create(&_tcl_thread, &tcl_attr, 
                                         &timecheck_loop, 
                                         (void*)this);
	
	/**
	 * TODO: checks that tcl_thr_res is 0, and deals with errors.
	 */
	
	pthread_attr_destroy(&tcl_attr);

	//debug
	std::cout << "[Debug]:BstTimeCb::start_threaded_timecheck_loop: started with id:"
	  << _tcl_thread << std::endl;
	//debug
	
	return;
     }

   void BstTimeCbTree::stop_threaded_timecheck_loop()
    {
      mutex_lock(&_run_mutex);
      _abort = true;
      mutex_unlock(&_run_mutex);
      pthread_join(_tcl_thread,NULL);
    }
   
} /* end of namespace. */
