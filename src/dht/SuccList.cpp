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
#include "Location.h"
#include "errlog.h"

#include <iostream>

using sp::errlog;

namespace dht
{
   SuccList::SuccList(DHTVirtualNode *vnode)
     : Stabilizable(), _vnode(vnode)
       {
       }
   
   SuccList::~SuccList()
     {
     }

   void SuccList::clear()
     {
	// TODO: beware, should remove keys from LocationTable as well.
	_succs.clear();
     }
      
   dht_err SuccList::update_successors()
     {
	//debug
	std::cerr << "[Debug]:SuccList::update_successors()\n";
	//debug
	
	/**
	 * RPC call to get successor list.
	 */
	int status = 0;
	DHTKey *dk_succ = _vnode->getSuccessor();
	if (!dk_succ)
	  {
	     // TODO: errlog.
	     std::cerr << "[Error]:SuccList::update_successors: this virtual node has no successor:"
	       << *dk_succ << ".Exiting\n";
	     exit(-1);
	  }
	
	Location *loc_succ = _vnode->findLocation(*dk_succ);
	
	std::list<DHTKey> dkres_list;
	std::list<NetAddress> na_list;
	dht_err loc_err = _vnode->getPNode()->getSuccList_cb(*dk_succ,dkres_list,na_list,status);
	if (loc_err == DHT_ERR_UNKNOWN_PEER)
	  _vnode->getPNode()->_l1_client->RPC_getSuccList(*dk_succ, loc_succ->getNetAddress(),
							  _vnode->getIdKey(), _vnode->getNetAddress(),
							  dkres_list, na_list, status);
	
	// TODO: handle failure, retry, and move to next successor in the list.
	
	if ((dht_err)status != DHT_ERR_OK)
	  {
	     //debug
	     std::cerr << "getSuccList failed: " << status << "\n";
	     //debug
	     
	     errlog::log_error(LOG_LEVEL_DHT, "getSuccList failed\n");
	     return (dht_err)status;
	  }
		
	// merge succlist.
     	merge_succ_list(dkres_list,na_list);
     
	return DHT_ERR_OK;
     }
   
   void SuccList::merge_succ_list(std::list<DHTKey> &dkres_list, std::list<NetAddress> &na_list)
     {
	std::list<DHTKey>::iterator kit = dkres_list.begin();
	std::list<NetAddress>::iterator nit = na_list.begin();
	
	// first pass.
	if (_vnode->_successors._succs.empty())
	  {
	     while(kit!=dkres_list.end())
	       {
		  Location *loc = _vnode->addOrFindToLocationTable((*kit),(*nit));
		  _vnode->_successors._succs.push_back(&loc->getDHTKeyRef());
		  ++kit; ++nit;
	       }
	     return;
	  }
		
	bool prune = false;
	std::list<const DHTKey*>::iterator sit = _vnode->_successors._succs.begin();
	while(kit!=dkres_list.end() && sit!=_vnode->_successors._succs.end())
	  {
	     if (*(*sit)<(*kit))
	       {
		  // node is our list may have died...
		  
		  // ping sit.
		  Location *loc = _vnode->findLocation(*(*sit));
		  
		  //debug
		  assert(loc!=NULL);
		  //debug
		  
		  bool dead = true;
		  if (_vnode->getPNode()->findVNode(*(*sit)))
		    {
		       // this is a local virtual node... Either our successor
		       // is not up-to-date, either we're cut off from the world...
		       // XXX: what to do ?
		       dead = false;
		    }
		  else 
		    {
		       // let's ping that node.
		       int status = DHT_ERR_OK;
		       dht_err err = _vnode->getPNode()->_l1_client->RPC_ping(*(*sit),loc->getNetAddress(),
									      _vnode->getIdKey(),_vnode->getNetAddress(),
									      status);
		       if (err == DHT_ERR_OK && (dht_err) status == DHT_ERR_OK)
			 dead = false;
		    }
		  		  
		  // if dead, remove node and location.
		  if (dead)
		    {
		       sit = _vnode->_successors._succs.erase(sit);
		       _vnode->removeLocation(loc);
		    }
		  else // move to next sit node
		    ++sit;
	       }
	     else if (*(*sit)>(*kit))
	       {
		  // a new node may have joined.
		  // add the new node, list to be pruned out later.
		  // let's ping that node.
		  bool alive = false;
		  if (_vnode->getPNode()->findVNode((*kit)))
		    {
		       // this is a local virtual node... Either our successor
		       // is not up-to-date, either we're cut off from the world...
		       alive = true; 
		    }
		  else
		    {
		       int status = DHT_ERR_OK;
		       dht_err err = _vnode->getPNode()->_l1_client->RPC_ping((*kit),(*nit),
									      _vnode->getIdKey(),_vnode->getNetAddress(),
									      status);
		       if (err == DHT_ERR_OK && (dht_err) status == DHT_ERR_OK)
			 alive = true;
		    }
		  
		  if (alive)
		    {
		       // add to location table.
		       Location *loc = NULL;
		       _vnode->addToLocationTable((*kit),(*nit),loc);
		    
		       // add it to the list.
		       _vnode->_successors._succs.insert(sit,&loc->getDHTKeyRef());
		       prune = true;
		    }
		  
		  ++kit; ++nit;
	       }
	     else if (*(*sit) == (*kit))
	       {
		  ++kit; ++nit;
		  ++sit;
	       }
	  }
	
	// prune out the successor list as needed.
	if (prune)
	  {
	     int ssize = _vnode->_successors._succs.size();
	     if (ssize <= DHTNode::_dht_config->_nvnodes)
	       return;
	     
	     int c = 0;
	     sit = _vnode->_successors._succs.begin();
	     while(sit!=_vnode->_successors._succs.end())
	       {
		  if (c<DHTNode::_dht_config->_nvnodes)
		    {
		       ++c;
		       ++sit;
		    }
		  else
		    {
		       // remove location if it is not used in the finger table.
		       Location *loc = _vnode->findLocation(*(*sit));
		       if (_vnode->getFingerTable()->has_key(-1,loc))
			 {
			    _vnode->removeLocation(loc);
			 }
		       sit = _vnode->_successors._succs.erase(sit);
		    }
	       }
	  }
	
	//debug
	assert(_vnode->_successors._succs.size() == DHTNode::_dht_config->_nvnodes);
	//debug
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

   dht_err SuccList::findClosestPredecessor(const DHTKey &nodeKey,
					    DHTKey &dkres, NetAddress &na,
					    DHTKey &dkres_succ, NetAddress &dkres_succ_na,
					    int &status)
     {
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
	       }
	     sit2 = sit;
	  }
	return DHT_ERR_OK;
     }
         
} /* end of namespace. */
