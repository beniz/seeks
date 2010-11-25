/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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
  
#include "SuccList.h"
#include "DHTVirtualNode.h"
#include "FingerTable.h"
#include "DHTNode.h"
#include "errlog.h"

#include <iostream>

using sp::errlog;

//#define DEBUG

namespace dht
{
   short SuccList::_max_list_size = 1; // default is successor only, reset by DHTNode constructor.
   
   SuccList::SuccList(DHTVirtualNode *vnode)
     : Stabilizable(), _vnode(vnode), _stable_pass1(false), _stable_pass2(false)
       {
       }
   
   SuccList::~SuccList()
     {
     }

   void SuccList::clear()
     {
	Location *loc = NULL;
	std::list<const DHTKey*>::iterator lit = _succs.begin();
	while(lit!=_succs.end())
	  {
	     loc = _vnode->findLocation(*(*lit));
	     lit = _succs.erase(lit);
	     if (_vnode->not_used_location(loc))
	       _vnode->removeLocation(loc);
	  }
     }

   void SuccList::pop_front()
     {
	if (!_succs.empty())
	  _succs.pop_front();
     }

   void SuccList::set_direct_successor(const DHTKey *succ_key)
     {
	if (_succs.empty())
	  _succs.push_back(succ_key);
	else (*_succs.begin()) = succ_key;
     }
   
   dht_err SuccList::update_successors()
     {
#ifdef DEBUG
	//debug
	std::cerr << "[Debug]:SuccList::update_successors()\n";
	//debug
#endif
	
	/**
	 * RPC call to get successor list.
	 */
	int status = 0;
	DHTKey *dk_succ = _vnode->getSuccessor();
	if (!dk_succ)
	  {
	     // TODO: errlog.
	     // XXX: should never reach here...
	     std::cerr << "[Error]:SuccList::update_successors: this virtual node has no successor:"
	       << *dk_succ << ".Exiting\n";
	     exit(-1);
	  }
	
	Location *loc_succ = _vnode->findLocation(*dk_succ);
	
	std::list<DHTKey> dkres_list;
	std::list<NetAddress> na_list;
	_vnode->getPNode()->getSuccList_cb(*dk_succ,dkres_list,na_list,status);
	if (status == DHT_ERR_UNKNOWN_PEER)
	  _vnode->getPNode()->_l1_client->RPC_getSuccList(*dk_succ, loc_succ->getNetAddress(),
							  dkres_list, na_list, status);
	
	/** 
	 * XXX: we could handle failure, retry, and move to next successor in the list.
	 * For now, this is done in stabilize, in FingerTable.
	 */
	if (status != DHT_ERR_OK)
	  {
#ifdef DEBUG
	     //debug
	     std::cerr << "getSuccList failed: " << status << "\n";
	     //debug
#endif
	     
	     errlog::log_error(LOG_LEVEL_DHT, "getSuccList failed");
	     return status;
	  }
	else
	  {
	     Location *uloc = _vnode->findLocation(*dk_succ);
	     if (uloc)
	       uloc->update_check_time();
	  }
			
	// merge succlist.
     	merge_succ_list(dkres_list,na_list);
     
	return DHT_ERR_OK;
     }
   
   void SuccList::merge_succ_list(std::list<DHTKey> &dkres_list, std::list<NetAddress> &na_list)
     {
#ifdef DEBUG
	//debug
	std::cerr << "[Debug]:merging succlists\n";
	std::cerr << "current list: ";
	std::list<const DHTKey*>::const_iterator lit = _vnode->_successors._succs.begin();
	while(lit!= _vnode->_successors._succs.end())
	  {
	     std::cerr << *(*lit) << std::endl;
	     ++lit;
	  }
	std::cerr << "list to merge: ";
	std::list<DHTKey>::const_iterator llit = dkres_list.begin();
	while(llit!= dkres_list.end())
	  {
	     std::cerr << (*llit) << std::endl;
	     ++llit;
	  }
	//debug
#endif
	
	_stable_pass2 = _stable_pass1;
	_stable_pass1 = true; // assume we're stable, and check below if it is true.
	
	/**
	 * Remove the last element if the list we got from our successor is longer 
	 * than the requested size. This is because our successor's list contains at
	 * least one more node than what we need.
	 */
	if ((int)dkres_list.size() > SuccList::_max_list_size)
	  {
	     dkres_list.pop_back();
	  }
		
	std::list<DHTKey>::iterator kit = dkres_list.begin();
	std::list<NetAddress>::iterator nit = na_list.begin();
	
	bool prune = false;
	std::list<const DHTKey*>::iterator sit = _vnode->_successors._succs.begin();
	sit++; // skip direct successor.
	const DHTKey *prev_succ_key = _vnode->_successors._succs.front();
	while(kit!=dkres_list.end())
	  {
	     if (sit != _vnode->_successors._succs.end() && *(*sit)<(*kit))
	       {
#ifdef DEBUG
		  std::cerr << "[Debug]:mergelist: <\n";
#endif
		  
		  // node in our list may have died...
		  Location *loc = _vnode->findLocation(*(*sit));
		  
		  //debug
		  assert(loc!=NULL);
		  //debug
		  
		  prev_succ_key = (*sit);
		  
		  // remove successor, and location is it is either dead or unused.
		  sit = _vnode->_successors._succs.erase(sit);
		  if (_vnode->not_used_location(loc))
		    _vnode->removeLocation(loc);
		  else
		    {
		       // RPC-based ping.
		       int status = DHT_ERR_OK;
		       bool dead = _vnode->is_dead(loc->getDHTKey(),loc->getNetAddress(),status);
		       if (dead)
			 _vnode->removeLocation(loc);
		    }
		  _stable_pass1 = false;
	       }
	     else if (*(*sit) == (*kit))
	       {
		  ++kit; ++nit;
		  prev_succ_key = (*sit);
		  ++sit;
	       }
	     else if ((sit == _vnode->_successors._succs.end() && kit != dkres_list.end())
		      || *(*sit)>(*kit))
	       {
#ifdef DEBUG
		  //debug
		  std::cerr << "[Debug]:mergelist: >\n";
		  //debug
#endif
		  
		  /**
		   * A new node may have joined.
		   * First, make sure we're not looping on the circle.
		   */
		  if (_vnode->getIdKey().incl(*prev_succ_key,(*kit)))
		    {
#ifdef DEBUG
		       //debug
		       std::cerr << "[Debug]:loop in merging.\n";
		       //debug
#endif
		       
		       /**
			* remove everything that lies beyond the (*sit) key in the successor list.
			* Though we ping them before complete removal.
			*/
		       int status = DHT_ERR_OK;
		       bool dead = false;
		       Location *loc = NULL;
		       while(sit!=_vnode->_successors._succs.end())
			 {
			    loc = _vnode->findLocation(*(*sit));
			    
			    // test for location usage, _after_ removal from the succlist.
			    sit = _vnode->_successors._succs.erase(sit);
			    if (_vnode->not_used_location(loc))
			      {
				 _vnode->removeLocation(loc);
			      }
			    else
			      {
				 dead = _vnode->is_dead(loc->getDHTKey(),loc->getNetAddress(),status);
				 if (dead)
				   {
				      _vnode->removeLocation(loc);
				   }
			      }
			    _stable_pass1 = false;
			    loc = NULL;
			    status = DHT_ERR_OK;
			    dead = false;
			 }
		    
#ifdef DEBUG
		       //debug
		       std::cerr << "[Debug]:mergelist: cleared remaining list\n";
		       //debug
#endif
		       
		       break; // the successor list is looping around the circle.
		    }
		  
		  // add the new node, list to be pruned out later.
		  // let's ping that node.
		  bool alive = false;
		  if (_vnode->getPNode()->findVNode((*kit)))
		    {
		       /**
			* this is a local virtual node... Either our successor
		        * is not up-to-date, either we're cut off from the world...
			*/
		       alive = true; 
		    }
		  else
		    {
		       int status = DHT_ERR_OK;
		       alive = !_vnode->is_dead((*kit),(*nit),status);
		    }
		  
		  if (alive)
		    {
		       // add to location table.
		       Location *loc = NULL;
		       
#ifdef DEBUG
		       //debug
		       std::cerr << "[Debug]:mergelist: adding to location table: " << (*kit) << std::endl;
		       //debug
#endif
		       
		       loc = _vnode->addOrFindToLocationTable((*kit),(*nit));	
		       
		       //debug
		       assert(loc!=NULL);
		       assert(loc->getDHTKey().count()>0);
		       assert(loc->getDHTKeyRef() == (*kit));
		       //debug
		       
		       // add it to the list, before sit.
		       prev_succ_key = (*sit);
		       _vnode->_successors._succs.insert(sit,&loc->getDHTKeyRef());
		       _stable_pass1 = false;
		       
#ifdef DEBUG
		       //debug
		       std::cerr << "added to successor list: " << loc->getDHTKeyRef() << std::endl;
		       //debug
#endif
		       
		       // don't let the list grow beyond its max allowed size.
		       if ((int)_vnode->_successors.size() > SuccList::_max_list_size)
			 prune = true;
		    }
		  
		  ++kit; ++nit;
	       }
	  }

#ifdef DEBUG
	//debug
	std::cout << "vnode: " << _vnode->getIdKey() << std::endl;
	std::cerr << "succlist (" << _vnode->_successors.size() << ") before pruning: ";
	lit = _vnode->_successors._succs.begin();
	while(lit!= _vnode->_successors._succs.end())
	  {
	     std::cerr << *(*lit) << std::endl;
	     ++lit;
	  }
	//debug
#endif
	
	// prune out the successor list as needed.
	if (prune)
	  {
	     int c = 0;
	     sit = _vnode->_successors._succs.begin();
	     while(sit!=_vnode->_successors._succs.end())
	       {
		  if (c<SuccList::_max_list_size)
		    {
		       ++c;
		       ++sit;
		    }
		  else
		    {
		       // remove location if it is not used in the finger table.
		       Location *loc = _vnode->findLocation(*(*sit));
		       sit = _vnode->_successors._succs.erase(sit);
		       if (_vnode->not_used_location(loc))
			 _vnode->removeLocation(loc);
		    }
	       }
	  }
	
#ifdef DEBUG
	//debug
	/* std::cerr << "vnode: " << _vnode->getIdKey() << std::endl;
	std::cerr << "succlist (" << _vnode->_successors.size() << ") after pruning: ";
	lit = _vnode->_successors._succs.begin();
	while(lit!= _vnode->_successors._succs.end())
	  {
	     std::cerr << *(*lit) << std::endl;
	     ++lit;
	  } */
	//debug
	
	//debug
	std::cerr << "stable1: " << _stable_pass1 << " -- stable2: " << _stable_pass2 << std::endl;
	//debug
#endif
     }
   
   bool SuccList::has_key(const DHTKey &key) const
     {
	std::list<const DHTKey*>::const_iterator sit = _succs.begin();
	while(sit!=_succs.end())
	  {
	     if (*(*sit) == key)
	       return true;
	     ++sit;
	  }
	return false;
     }
   
   void SuccList::removeKey(const DHTKey &key)
     {
	std::list<const DHTKey*>::iterator sit = _succs.begin();
	while(sit!=_succs.end())
	  {
	     if (*(*sit) == key)
	       sit = _succs.erase(sit);
	     else ++sit;
	  }
     }
      
   void SuccList::findClosestPredecessor(const DHTKey &nodeKey,
					    DHTKey &dkres, NetAddress &na,
					    DHTKey &dkres_succ, NetAddress &dkres_succ_na,
					    int &status)
     {
       status =  DHT_ERR_OK;
	std::list<const DHTKey*>::const_iterator sit = _succs.end();
	std::list<const DHTKey*>::const_iterator sit2 = sit;
	while(sit!=_succs.begin())
	  {
	     --sit;
	     Location *loc = _vnode->findLocation(*(*sit));
	     
	     /**
	      * XXX: this should never happen.
	      */
	     if (!loc)
	       continue;
	     
#ifdef DEBUG
	     //debug
	     std::cerr << "[Debug]: in succlist findclosestpredecessor: " << loc->getDHTKey() << std::endl;
	     std::cerr << "? in [vnode key=" << _vnode->getIdKey() << ", nodekey=" << nodeKey << std::endl;
	     //debug
#endif
	     
	     if (loc->getDHTKey().between(_vnode->getIdKey(),nodeKey))
	       {
		  dkres = loc->getDHTKey();
		  na = loc->getNetAddress();
		  if (sit2 != _succs.end())
		    {
		       Location *loc2 = _vnode->findLocation(*(*sit2));
		       dkres_succ = loc2->getDHTKey();
		       dkres_succ_na = loc2->getNetAddress();
		    }
		  return;
	       }
	     sit2 = sit;
	  }
     }

   bool SuccList::isStable()
     {
	return (_stable_pass1 && _stable_pass2);
     }
      
} /* end of namespace. */
