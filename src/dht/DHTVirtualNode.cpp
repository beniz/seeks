/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
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

#include "DHTVirtualNode.h"
#include "DHTNode.h"
#include "FingerTable.h"

namespace dht
{
   size_t DHTVirtualNode::_maxSuccsListSize = 15; //default adhoc value. TODO: compute useful value.
   
   DHTVirtualNode::DHTVirtualNode(DHTNode* pnode)
     : _pnode(pnode), _successor(NULL), _predecessor(NULL)
       {
	  /**
	   * We generate a random key as the node's id.
	   */
	  //_idkey = DHTKey::randomKey();
	  _idkey = DHTNode::generate_uniform_key();
	  
	  //std::cout << "virtual node key: " << _idkey << std::endl;
	  
	  /**
	   * create location and registers it to the location table.
	   */
	  Location *lloc = NULL;
	  addToLocationTable(_idkey, getNetAddress(), lloc);
	  
	  /**
	   * finger table.
	   */
	  _fgt = new FingerTable(this, getLocationTable());
       }

   DHTVirtualNode::~DHTVirtualNode()
     {
	if (_successor)
	  delete _successor;
	if (_predecessor)
	  delete _predecessor;
	
	slist<DHTKey*>::iterator sit = _successors.begin();
	while(sit!=_successors.end())
	  {
	     delete (*sit);
	     ++sit;
	  }
	sit = _predecessors.begin();
	while(sit!=_predecessors.end())
	  {
	     delete (*sit);
	     ++sit;
	  }
	
	delete _fgt;
     }
      
   void DHTVirtualNode::notify(const DHTKey& senderKey, const NetAddress& senderAddress)
     {
	if (!_predecessor
	    || senderKey.between(*_predecessor, _idkey))
	  setPredecessor(senderKey, senderAddress);
     }

   void DHTVirtualNode::findClosestPredecessor(const DHTKey& nodeKey,
					       DHTKey& dkres, NetAddress& na,
					       DHTKey& dkres_succ, NetAddress &dkres_succ_na,
					       int& status)
     {
	_fgt->findClosestPredecessor(nodeKey, dkres, na, dkres_succ, dkres_succ_na, status);
     }

   
   /**-- functions using RPCs. --**/
   int DHTVirtualNode::join(const DHTKey& dk_bootstrap,
			    const NetAddress &dk_bootstrap_na,
			    const DHTKey& senderKey,
			    int& status)
     {
	/**
	 * reset predecessor.
	 */
	_predecessor = NULL;
	
	/**
	 * TODO: query the bootstrap node for our successor.
	 */
	DHTKey dkres;
	NetAddress na;
	//status = find_successor(dk_bootstrap, dkres, na);
	_pnode->_l1_client->RPC_joinGetSucc(dk_bootstrap, dk_bootstrap_na,
					    _idkey,_pnode->_l1_na,
					    dkres, na, status);
	
	setSuccessor(dkres);
     
	return status;  /* TODO. */
     }
   
   int DHTVirtualNode::find_successor(const DHTKey& nodeKey,
				      DHTKey& dkres, NetAddress& na)
     {
	DHTKey dk_pred;
	NetAddress na_pred;
	
	/**
	 * find closest predecessor to nodeKey.
	 */
	int status = find_predecessor(nodeKey, dk_pred, na_pred);
	
	/**
	 * TODO: check on find_predecessor's status.
	 */
     
	/**
	 * RPC call to get pred's successor.
	 */
	_pnode->_l1_client->RPC_getSuccessor(dk_pred, na_pred, 
					     getIdKey(), getNetAddress(),
					     dkres, na, status);
	return status;
     }
   
   int DHTVirtualNode::find_predecessor(const DHTKey& nodeKey,
					DHTKey& dkres, NetAddress& na)
     {
	/**
	 * location to iterate (route) among results.
	 */
	Location rloc(_idkey, getNetAddress());
	
	if (!getSuccessor())
	  {
	     /**
	      * TODO: after debugging, write a better handling of this error.
              */
	     std::cerr << "[Error]:DHTNode::find_predecessor: this virtual node has no successor:"
               << nodeKey << ".Exiting\n";
	     exit(-1);
	  }
	
	Location succloc(*getSuccessor(),NetAddress()); // warning: at this point the address is unknown.
			
	while(!nodeKey.between(rloc.getDHTKey(), succloc.getDHTKey()))
          {
	     /**
	      * RPC calls.
	      */
	     int status = -1;
	     
	     const DHTKey recipientKey = rloc.getDHTKey();
	     const NetAddress recipient = rloc.getNetAddress();
	     DHTKey succ_key = succloc.getDHTKey();
	     NetAddress succ_na = succloc.getNetAddress();
	     _pnode->_l1_client->RPC_findClosestPredecessor(recipientKey, recipient, 
							    getIdKey(),getNetAddress(),
							    nodeKey, dkres, na, 
							    succ_key, succ_na, status);
	     
             /**
	      * TODO: check on RPC status.
	      */
	     
	     rloc.setDHTKey(dkres);
	     rloc.setNetAddress(na);
	     succloc.setDHTKey(succ_key);
	     succloc.setNetAddress(succ_na);
	  }
	return 0;	
     }
      
   int DHTVirtualNode::stabilize()
     {
	/**
	 * stabilize is in FingerTable (because of the stabilization alg).
	 */
	return _fgt->stabilize();
     }
   
   /**
    * accessors.
    */
   void DHTVirtualNode::setSuccessor(const DHTKey &dk)
     {
	if (_successor)
	  delete _successor;
	_successor = new DHTKey(dk);
     }
   
   //DEPRECATED   
   void DHTVirtualNode::setSuccessor(const DHTKey& dk, const NetAddress& na)
     {
	if (*_successor == dk)
	  {
	     /**
	      * lookup for the corresponding location.
	      */
	     Location* loc = findLocation(dk);
	     //debug
	     if (!loc)
	       {
		  std::cout << "[Error]:DHTVirtualNode::setSuccessor: successor node isn't in location table! Exiting.\n";
		  exit(-1);
	       }
	     //debug
	     
	     /**
	      * updates the address (just in case we're talking to
	      * another node with the same key, or that com port has changed).
	      */
	     loc->update(na);
	  }
	else
	  {
	     Location* loc = findLocation(dk);
	     if (!loc)
	       {
		  /**
		   * create/add location to table.
		   */
		  addToLocationTable(dk, na, loc);
	       }
	     setSuccessor(loc->getDHTKey());
	  }
     }

   void DHTVirtualNode::setSuccessor(Location* loc)
     {
	if (*_successor != loc->getDHTKey())
	  {
	     setSuccessor(loc->getDHTKey());
	  }
	/**
	 * TODO: udpate location ?
	 */
     }

   void DHTVirtualNode::setPredecessor(const DHTKey &dk)
     {
	if (_predecessor)
	  delete _predecessor;
	_predecessor = new DHTKey(dk);
     }

   //DEPRECATED
   void DHTVirtualNode::setPredecessor(const DHTKey& dk, const NetAddress& na)
     {
	if (*_predecessor == dk)
	  {
	     /**
	      * lookup for the corresponding location.
              */
	     Location* loc = findLocation(dk);
	     //debug
	     if (!loc)
	       {
		  std::cout << "[Error]:DHTVirtualNode::setPredecessor: predecessor node isn't in location table! Exiting.\n";
                  exit(-1);
	       }
	     //debug
	     /**
	      * updates the address (just in case we're talking to
              * another node with the same key, or that com port has changed).
	      */
	     loc->update(na);
	  }
	else
	  {
	     Location* loc = findLocation(dk);
	     if (!loc)
	       {
		  /**
		   * create/add location to table.
                   */
		  addToLocationTable(dk, na, loc);
	       }
	     setPredecessor(loc->getDHTKey());
	     loc->update(na);
	  }
     }

   void DHTVirtualNode::setPredecessor(Location* loc)
     {
	if (*_predecessor != loc->getDHTKey())
	  setPredecessor(loc->getDHTKey());
     }
   

   /**
    * function channeling to pnode (DHTNode) functions.
    */
   
   LocationTable* DHTVirtualNode::getLocationTable() const
     {
	return _pnode->getLocationTable();
     }
   
   Location* DHTVirtualNode::findLocation(const DHTKey& dk) const
     {
	return getLocationTable()->findLocation(dk);
     }
   
   void DHTVirtualNode::addToLocationTable(const DHTKey& dk, const NetAddress& na,
					   Location* loc) const
     {
	getLocationTable()->addToLocationTable(dk, na, loc);
     }

   NetAddress DHTVirtualNode::getNetAddress() const
     {
	return _pnode->getNetAddress();
     }
   
   Location* DHTVirtualNode::addOrFindToLocationTable(const DHTKey& key, const NetAddress& na)
     {
	return getLocationTable()->addOrFindToLocationTable(key, na);
     }
   
   
} /* end of namespace. */
