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

#include "FingerTable.h"
#include "DHTNode.h"
#include "Random.h"
#include "errlog.h"

#include <assert.h>

using lsh::Random;
using sp::errlog;

namespace dht
{
   FingerTable::FingerTable(DHTVirtualNode* vnode, LocationTable* lt)
     : Stabilizable(), _vnode(vnode), _lt(lt)
       {
	  /**
	   * We're filling up the finger table starting keys
	   * by computing KEYNBITS successor keys starting from the virtual node key.
	   */
	  DHTKey dkvnode = _vnode->getIdKey();
	  for (unsigned int i=0; i<KEYNBITS; i++)
	    _starts[i] = dkvnode.successor(i);

	  for (unsigned int i=0; i<KEYNBITS; i++)
	    _locs[i] = NULL;
       }
   
   FingerTable::~FingerTable()
     {
     }
      
   dht_err FingerTable::findClosestPredecessor(const DHTKey& nodeKey,
					       DHTKey& dkres, NetAddress& na,
					       DHTKey& dkres_succ, NetAddress &dkres_succ_na,
					       int& status)
     {
	status = DHT_ERR_OK;
	
	for (int i=KEYNBITS-1; i>=0; i--)
	  {
	     Location* loc = _locs[i];
	     
	     /**
	      * This should only happen when the finger table has
	      * not yet been fully populated...
	      */
	     if (!loc)
	       continue;
	     
	     if (loc->getDHTKey().between(getVNodeIdKey(), nodeKey))
	       {
		  dkres = loc->getDHTKey();
		  
		  //debug
		  assert(dkres.count()>0);
		  //debug
		  
		  na = loc->getNetAddress();
		  dkres_succ = DHTKey();
		  dkres_succ_na = NetAddress();
		  status = DHT_ERR_OK;
		  return DHT_ERR_OK;
	       }     
	  }
	
	/**
	 * otherwise, let's look in the successor list.
	 * XXX: some of those nodes may have been tested already
	 * in the finger table...
	 */
	std::cerr << "[Debug]:looking up closest predecessor in successor list\n";
	_vnode->_successors.findClosestPredecessor(nodeKey,dkres,na,
						   dkres_succ, dkres_succ_na,
						   status);
	if (dkres.count()>0)
	  {
	     std::cerr << "[Debug]:found closest predecessor in successor list: "
	       << dkres << std::endl;
	     
	     status = DHT_ERR_OK;
	     return DHT_ERR_OK;
	  }
	
	/**
	 * otherwise, let's try our successor node.
	 */
	// successor is part of successor list now.
	/* Location *loc = findLocation(*_vnode->getSuccessor());
	if (loc->getDHTKey().between(getVNodeIdKey(), nodeKey))
	  {
	     dkres = loc->getDHTKey();
	     na = loc->getNetAddress();
	     dkres_succ = *(*_vnode->_successors._succs.begin());
	     Location *loc_succ = findLocation(dkres_succ);
	     dkres_succ_na = loc_succ->getNetAddress();
	  } */
		
	// TODO: we should never reach here...
	/**
	 * otherwise return current node's id key, and sets the successor
	 * (saves an rpc later).
	 */
	std::cerr << "[Debug]:closest predecessor is node itself... should not happen\n";
	
	status = DHT_ERR_OK;
	dkres = getVNodeIdKey();
	na = getVNodeNetAddress();
	dkres_succ = *getVNodeSuccessor();
	Location *succ_loc = findLocation(dkres_succ);
	if (!succ_loc)
	  return DHT_ERR_UNKNOWN_PEER_LOCATION;
	dkres_succ_na = succ_loc->getNetAddress();
	
	//debug
	assert(dkres!=getVNodeIdKey()); // exit.
	//debug
	
	return DHT_ERR_OK;
     }
   
   int FingerTable::stabilize()
     {
	static int retries = 3;
	int ret = 0;
	
	//debug
	std::cerr << "[Debug]:FingerTable::stabilize()\n";
	//debug
	
	DHTKey recipientKey = _vnode->getIdKey();
	NetAddress na = getVNodeNetAddress();
	
	/**
	 * get successor's predecessor.
	 */
	DHTKey* succ;
	if ((succ = _vnode->getSuccessor()) == NULL)
          {
	     /**
	      * TODO: after debugging, write a better handling of this error.
              */
	     std::cerr << "[Error]:DHTNode::stabilize: this virtual node has no successor: "
	       << recipientKey << ". Exiting\n";
	     exit(-1);
	  }
	
	/**
	 * lookup for the peer.
	 */
	Location* succ_loc = findLocation(*succ);
	
        /**
	 * RPC call to get the predecessor.
	 */
	DHTKey succ_pred;
	NetAddress na_succ_pred;
	
	int status = 0;
	dht_err err = DHT_ERR_RETRY;
	std::vector<Location*> dead_locs;
	while (err != DHT_ERR_OK) // while over all successors in the list. If all fail, then rejoin.
	  {
	     err = _vnode->getPNode()->getPredecessor_cb(succ_loc->getDHTKey(), succ_pred, na_succ_pred, status);
	     if (err == DHT_ERR_UNKNOWN_PEER)
	       err = _vnode->getPNode()->_l1_client->RPC_getPredecessor(succ_loc->getDHTKey(), succ_loc->getNetAddress(),
									recipientKey,na,
									succ_pred, na_succ_pred, status);
	     //debug
	     std::cerr << "[Debug]: predecessor call: err=" << err << " -- status=" << status << std::endl;
	     //debug
	     	     
	     /**
	      * handle successor failure, retry, then move down the successor list.
	      */
	     if (err != DHT_ERR_OK
		 && (err == DHT_ERR_CALL || err == DHT_ERR_COM_TIMEOUT || err == DHT_ERR_UNKNOWN_PEER
		     || ret == retries)) // node is not dead, but predecessor call has failed 'retries' times.
	       {
		  /**
		   * our successor is not responding.
		   * let's ping it, if it seems dead, let's move to the next successor on our list.
		   */
		  bool dead = _vnode->is_dead(*succ,succ_loc->getNetAddress(),
					      status);
		  
		  if (dead || ret == retries)
		    {
		       //debug
		       std::cerr << "[Debug]:dead successor detected\n";
		       //debug
		       
		       // get a new successor.
		       _vnode->_successors.pop_front(); // removes our current successor.
		       
		       std::list<const DHTKey*>::const_iterator kit
			 = _vnode->_successors.empty() ? _vnode->_successors.end()
			   : _vnode->getFirstSuccessor();
		       
		       /**
			* if we're at the end, game over, we will need to rejoin the network
			* (we're probably cut off because of network failure, or some weird configuration).
			*/
		       if (kit == _vnode->_successors.end())
			 break; // we're out of potential successors.
		       		       
		       //debug
		       std::cerr << "[Debug]:trying successor: " << *(*kit) << std::endl;
		       //debug
		       
		       succ = const_cast<DHTKey*>((*kit));
		       
		       // mark dead nodes for tentative removal from location table.
		       dead_locs.push_back(succ_loc);
		       
		       // sets a new successor.
		       succ_loc = findLocation(*succ);
		       
		       // replaces the successor and the head of the successor list as well.
		       _vnode->setSuccessor(succ_loc->getDHTKeyRef(),succ_loc->getNetAddress());
		       
		       // resets the error.
		       err = status = DHT_ERR_RETRY;
		    }
		  else 
		    {
		       // let's retry the getpredecessor call.
		       ret++;
		    }
	       }
	     	     	     
	     /**
	      * Let's loop if we did change our successor.
	      */
	     if (err == DHT_ERR_RETRY)
	       {
		  //debug
		  std::cerr << "[Debug]: trying to call the (new) successor, one more loop.\n";
		  //debug
		  
		  continue;
	       }
	     
	     /**
	      * check on RPC status.
	      */
	     if ((dht_err)status == DHT_ERR_NO_PREDECESSOR_FOUND) // our successor has an unset predecessor.
	       {
		  //debug
		  std::cerr << "[Debug]:stabilize: our successor has no predecessor set.\n";
		  //debug
		  
		  break;
	       }
	     else if ((dht_err)status == DHT_ERR_OK)
	       {
		  /**
		   * beware, the predecessor may be a dead node.
		   * this may happen when our successor has failed and was replaced by its
		   * successor in the list. Asked for its predecessor, this new successor may
		   * with high probability return our old dead successor.
		   * The check below ensures we detect this case.
		   */
		  Location *pred_loc = _vnode->findLocation(succ_pred);
		  if (pred_loc)
		    {
		       for (size_t i=0;i<dead_locs.size();i++)
			 if (pred_loc == dead_locs.at(i)) // comparing ptr.
			   {
			      status = DHT_ERR_NO_PREDECESSOR_FOUND;
			      succ_pred.reset(); // clears every bit.
			      na_succ_pred = NetAddress();
			      break;
			   }
		    }
		  break;
	       }
	     else 
	       {
		  // other errors: our successor has replied, but with a signaled error.
		  err = status;
		  break;
	       }
	     
	     status = 0;
	  }
	
	// clear dead nodes.
	for (size_t i=0;i<dead_locs.size();i++)
	  {
	     Location *dead_loc = dead_locs.at(i);
	     
	     //debug
	     assert(!_vnode->_successors.has_key(dead_loc->getDHTKey()));
	     assert(*_vnode->getSuccessor() != dead_loc->getDHTKey());
	     //debug
	     
	     _vnode->removeLocation(dead_loc);
	  }
	dead_locs.clear();
	
	/**
	 * total failure, we're very much probably cut off from the network.
	 * let's try to rejoin.
	 * TODO: if join fails, let's fall back into a trying mode, in which
	 * we try a join every x minutes.
	 */
	//TODO: rejoin + wrong test here.
	/* if (_vnode->_successors.empty())
	  {
	     std::cerr << "[Debug]: no more successors... Should try to rejoin the overlay network\n";
	     exit(0);
	  } */
		
	if ((dht_err)status != DHT_ERR_NO_PREDECESSOR_FOUND && (dht_err)status != DHT_ERR_OK)
	  {
	     //debug
	     std::cerr << "[Debug]:FingerTable::stabilize: failed return from getPredecessor\n";
	     //debug
	     
	     errlog::log_error(LOG_LEVEL_DHT, "FingerTable::stabilize: failed return from getPredecessor, %u",
			       status);
	     //return (dht_err)status;
	  
	     std::cerr << "[Debug]: no more successors to try... Should try to rejoin the overlay network\n";
	     exit(0);
	  }
	else if ((dht_err)status == DHT_ERR_OK)
	  {
	     /**
	      * look up succ_pred, add it to the location table if needed.
	      * -> because succ_pred should be in one of the succlist/predlist anyways.
	      */
	     Location* succ_pred_loc = _vnode->addOrFindToLocationTable(succ_pred, na_succ_pred);
	     
	     /**
	      * key check: if a node has taken place in between us and our
	      * successor.
	      */
	     if (succ_pred.between(recipientKey, *succ))
	       {
		  _vnode->setSuccessor(succ_pred_loc->getDHTKeyRef(),succ_pred_loc->getNetAddress());
		  _vnode->update_successor_list_head();
	       }
	  }
	
	/**
	 * notify RPC.
	 */
	err = _vnode->getPNode()->notify_cb(succ_loc->getDHTKey(), getVNodeIdKey(), getVNodeNetAddress(), status);
	if (err == DHT_ERR_UNKNOWN_PEER)
	  _vnode->getPNode()->_l1_client->RPC_notify(succ_loc->getDHTKey(), succ_loc->getNetAddress(),
						     getVNodeIdKey(),getVNodeNetAddress(),
						     status);
	// TODO: handle successor failure, retry, then move down the successor list.
	/**
	 * check on RPC status.
	 */
	if ((dht_err)status != DHT_ERR_OK)
	  {
	     errlog::log_error(LOG_LEVEL_DHT, "FingerTable::stabilize: failed notify call");
	     return (dht_err)status;
	  }
		
	return status;	
     }

   int FingerTable::fix_finger()
     {
	// TODO: seed.
	unsigned long int rindex = Random::genUniformUnsInt32(1, KEYNBITS-1);
	
	//debug
	std::cerr << "[Debug]:FingerTable::fix_finger: " << rindex << std::endl;
	//debug
	
	//debug
	assert(rindex > 0);
	assert(rindex < KEYNBITS);
	//debug
     
	/**
	 * find_successor call.
	 */
	DHTKey dkres;
	NetAddress na;
	dht_err status = DHT_ERR_OK;
	
	//debug
	//std::cerr << "[Debug]:finger nodekey: _starts[" << rindex << "]: " << _starts[rindex] << std::endl;
	//debug
	
	status = _vnode->find_successor(_starts[rindex], dkres, na);
	if (status != DHT_ERR_OK)
	  {
	     //debug
	     std::cerr << "[Debug]:fix_fingers: error finding successor.\n";
	     //debug
	     
	     errlog::log_error(LOG_LEVEL_DHT, "Error finding successor when fixing finger.\n");
	     return status;
	  }
	
	//debug
	assert(dkres.count()>0);
	//debug
	
	/**
	 * lookup result, add it to the location table if needed.
	 */
	Location* rloc = _vnode->addOrFindToLocationTable(dkres, na);
	Location *curr_loc = _locs[rindex];
	if (curr_loc && rloc != curr_loc)
	  {
	     // remove location if it not used in other lists.
	     if (!_vnode->_successors.has_key(curr_loc->getDHTKey()))
	       _vnode->removeLocation(curr_loc);
	  }
		
	_locs[rindex] = rloc;
	
	//debug
	print(std::cout);
	//debug
	
	return 0;
     }

   bool FingerTable::has_key(const int &index, Location *loc) const
     {
	for (int i=0;i<KEYNBITS;i++)
	  {
	     if (i != index)
	       {
		  if (_locs[i] == loc)
		    return true;
	       }
	  }
	return false;
     }
      
   void FingerTable::removeLocation(Location *loc)
     {
	for (int i=0;i<KEYNBITS;i++)
	  {
	     if (_locs[i] == loc)
	       {
		  _locs[i] = NULL;
	       }
	  }
     }
      
   void FingerTable::print(std::ostream &out) const
     {
	out << "   ftable: " << _vnode->getIdKey() << std::endl;
	if (_vnode->getSuccessor())
	  out << "successor: " << *_vnode->getSuccessor() << std::endl;
	if (_vnode->getPredecessor())
	  out << "predecessor: " << *_vnode->getPredecessor() << std::endl;
	out << "successor list (" << _vnode->_successors.size() << "): ";
	std::list<const DHTKey*>::const_iterator lit = _vnode->_successors._succs.begin();
	while(lit!=_vnode->_successors._succs.end())
	  {
	     out << *(*lit) << std::endl;
	     ++lit;
	  }
	/* for (int i=0;i<KEYNBITS;i++)
	  {
	     if (_locs[i])
	       {
		  out << "---------------------------------------------------------------\n";
		  out << "starts: " << i << ": " << _starts[i] << std::endl;;
		  out << "           " << _locs[i]->getDHTKey() << std::endl;
		  out << _locs[i]->getNetAddress() << std::endl;
	       }
	  } */
     }
   
} /* end of namespace. */
