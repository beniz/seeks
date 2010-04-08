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
#include <assert.h>

using lsh::Random;

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
      
   void FingerTable::setSuccessor(Location* loc)
     {
	_locs[0] = loc;
	
	/**
	 * update successor in vnode.
	 */
	_vnode->setSuccessor(loc);
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
	      * Beware: this should only happen if the finger table is
	      * not yet populated...
	      */
	     if (!loc)
	       continue;
	     
	     if (loc->getDHTKey().between(getVNodeIdKey(), nodeKey))
	       {
		  dkres = loc->getDHTKey();
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
	int status = -1;
	
	_vnode->getPNode()->_l1_client->RPC_getPredecessor(*succ, succ_loc->getNetAddress(),
							   recipientKey,na,
							   succ_pred, na_succ_pred, status);
	/**
	 * TODO: check on RPC status.
	 */
	
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
	     _vnode->setSuccessor(succ_pred_loc);
	  }
	
	/**
	 * notify RPC.
	 */
	_vnode->getPNode()->_l1_client->RPC_notify(*succ, succ_loc->getNetAddress(),
						   getVNodeIdKey(),getVNodeNetAddress(),
						   status);
	
	/**
	 * TODO: check on RPC status.
	 */
	
	return 0;	
     }

   int FingerTable::fix_finger()
     {
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
	int status = -1;
	status = _vnode->find_predecessor(_starts[rindex], dkres, na);
	/**
	 * TODO: check on find_predecessor's status.
	 */
     
	/**
	 * lookup result, add it to the location table if needed.
	 */
	Location* rloc = _vnode->addOrFindToLocationTable(dkres, na);
     
	_locs[rindex] = rloc;
	
	return 0;
     }
   
} /* end of namespace. */
