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
	 * otherwise return current node's id key, and sets the successor
	 * (saves an rpc later).
	 */
	status = DHT_ERR_OK;
	dkres = getVNodeIdKey();
	na = getVNodeNetAddress();
	dkres_succ = *getVNodeSuccessor();
	Location *succ_loc = findLocation(dkres_succ);
	if (!succ_loc)
	  return DHT_ERR_UNKNOWN_PEER_LOCATION;
	dkres_succ_na = succ_loc->getNetAddress();
	return DHT_ERR_OK;
     }
   
   int FingerTable::stabilize()
     {
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
	
	dht_err loc_err = _vnode->getPNode()->getPredecessor_cb(*succ, succ_pred, na_succ_pred, status);
	if (loc_err == DHT_ERR_UNKNOWN_PEER)
	  _vnode->getPNode()->_l1_client->RPC_getPredecessor(*succ, succ_loc->getNetAddress(),
							     recipientKey,na,
							     succ_pred, na_succ_pred, status);
	// TODO: handle successor failure, retry, then move down the successor list.
	/**
	 * check on RPC status.
	 */
	if ((dht_err)status == DHT_ERR_NO_PREDECESSOR_FOUND) // our successor has an unset predecessor.
	  {
	  }
	else if ((dht_err)status != DHT_ERR_OK)
	  {
	     //debug
	     std::cerr << "[Debug]:FingerTable::stabilize: failed return from getPredecessor\n";
	     //debug
	  
	     errlog::log_error(LOG_LEVEL_DHT, "FingerTable::stabilize: failed return from getPredecessor");
	     return (dht_err)status;
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
	       }
	  }
	
	/**
	 * notify RPC.
	 */
	loc_err = _vnode->getPNode()->notify_cb(*succ, getVNodeIdKey(), getVNodeNetAddress(), status);
	if (loc_err == DHT_ERR_UNKNOWN_PEER)
	  _vnode->getPNode()->_l1_client->RPC_notify(*succ, succ_loc->getNetAddress(),
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
	//debug
	std::cerr << "[Debug]:FingerTable::fix_finger()\n";
	//debug
	
	// TODO: seed.
	unsigned long int rindex = Random::genUniformUnsInt32(1, KEYNBITS-1);
		
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
	
	assert(dkres.count()>0);
	
	/**
	 * lookup result, add it to the location table if needed.
	 */
	Location* rloc = _vnode->addOrFindToLocationTable(dkres, na);
     
	_locs[rindex] = rloc;
	
	//debug
	//print(std::cout);
	//debug
	
	return 0;
     }

   void FingerTable::print(std::ostream &out) const
     {
	out << "   ftable: " << _vnode->getIdKey() << std::endl;
	if (_vnode->getSuccessor())
	  out << "successor: " << *_vnode->getSuccessor() << std::endl;
	if (_vnode->getPredecessor())
	  out << "predecessor: " << *_vnode->getPredecessor() << std::endl;
	for (int i=0;i<KEYNBITS;i++)
	  {
	     if (_locs[i])
	       {
		  out << "---------------------------------------------------------------\n";
		  out << "starts: " << i << ": " << _starts[i] << std::endl;;
		  out << "           " << _locs[i]->getDHTKey() << std::endl;
		  out << _locs[i]->getNetAddress() << std::endl;
	       }
	  }
     }
   
} /* end of namespace. */
