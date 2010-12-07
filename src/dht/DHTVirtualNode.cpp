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
#include "RouteIterator.h"
#include "errlog.h"

#include <math.h>

using sp::errlog;

#define WITH_RANDOM_KEY
//#define DEBUG

namespace dht
{
   DHTVirtualNode::DHTVirtualNode(DHTNode* pnode)
     : _pnode(pnode),_successor(NULL),_predecessor(NULL),
       _successors(this),_nnodes(0),_nnvnodes(0),_connected(false)
       {
	  /**
	   * We generate a random key as the node's id.
	   */
#ifdef WITH_RANDOM_KEY
	  _idkey = DHTKey::randomKey();
#else
	  _idkey = DHTNode::generate_uniform_key();
#endif
	  
	  errlog::log_error(LOG_LEVEL_DHT, "Generated virtual node %s", _idkey.to_rstring().c_str());
	  
	  /**
	   * initialize other structures.
	   */
	  init_vnode();
       }

   DHTVirtualNode::DHTVirtualNode(DHTNode *pnode, const DHTKey &idkey, LocationTable *lt)
     : _lt(lt), _pnode(pnode),_successor(NULL),_predecessor(NULL),
       _successors(this),_nnodes(0),_nnvnodes(0),_connected(false)
     {
	/**
	 * no need to generate the virtual node key, we have it from
	 * persistent data.
	 */
	_idkey = idkey;
	
	/**
	 * initialize other structures.
	 */
	init_vnode();
     }
      
   DHTVirtualNode::~DHTVirtualNode()
     {
       mutex_lock(&_succ_mutex);
	if (_successor)
	  delete _successor;
	_successor = NULL;
	mutex_unlock(&_succ_mutex);
	
	mutex_lock(&_pred_mutex);
	if (_predecessor)
	  delete _predecessor;
	_predecessor = NULL;
	mutex_unlock(&_pred_mutex);
	
	_successors.clear();
	
	delete _fgt;
	_fgt = NULL;
     
        delete _lt;
	_lt = NULL;
     }
   
   void DHTVirtualNode::init_vnode()
     {
	/**
	 * create location table. TODO: read from serialized table.
	 */
	_lt = new LocationTable();
	
	/**
	 * create location and registers it to the location table.
	 */
	Location *lloc = NULL;
	addToLocationTable(_idkey, getNetAddress(), lloc);
	
	/**
	 * finger table.
	 */
	_fgt = new FingerTable(this,_lt);
	
	/**
	 * Initializes mutexes.
	 */
	mutex_init(&_pred_mutex);
	mutex_init(&_succ_mutex);
     }
   
   dht_err DHTVirtualNode::notify(const DHTKey& senderKey, const NetAddress& senderAddress)
     {
       bool reset_pred = false;
       DHTKey pred = getPredecessorS();
       if (pred.count() == 0)
	 reset_pred = true;	 
       	else if (pred != senderKey) // our predecessor may have changed.
	  {
	     /**
	      * If we are the new successor of a node, because some other node did 
	      * fail in between us, then we need to make sure that this dead node is
	      * not our predecessor. If it is, then we do accept the change of predecessor.
	      *
	      * XXX: this needs for a ping seems to not be mentionned in the original Chord
	      * protocol. However, this breaks our golden rule that no node should be forced
	      * to make network operations on the behalf of others. (This rule is only broken
	      * on 'join' operations). Solution would be to ping our predecessor after a certain
	      * time has passed.
	      */
	     // TODO: slow down the time between two pings by looking at a location 'alive' flag.
	     Location *pred_loc = findLocation(pred);
	     int status = 0;
	     bool dead = is_dead(pred_loc->getDHTKey(),pred_loc->getNetAddress(),status);
	     if (dead)
	       reset_pred = true;
	     else if (senderKey.between(pred, _idkey))
	       reset_pred = true;
	  }
       
	if (reset_pred)
	  {
	     Location *old_pred_loc = NULL;
	     old_pred_loc = findLocation(pred);
	     setPredecessor(senderKey, senderAddress);
	  
	     //TODO: move keys (+ catch errors ?)
	     if (old_pred_loc)
	       {
		  short start_replication_radius = 0;
		  if (old_pred_loc->getDHTKey() > senderKey) // our old predecessor did leave or fail.
		    {
		       // some replicated keys we host are ours now.
		       replication_host_keys(old_pred_loc->getDHTKey()); //TODO: old_pred_loc key unnecessary here.
		    }
		  else if (old_pred_loc->getDHTKey() < senderKey) // our new predecessor did join the circle.
		    {
		       // some of our keys should now belong to our predecessor.
		       replication_move_keys_backward(old_pred_loc->getDHTKey(),
						      senderKey,senderAddress);
		       start_replication_radius = 1;
		    }
		  
		  //TODO: forward replication.
		  replication_trickle_forward(old_pred_loc->getDHTKey(),start_replication_radius);
		  
		  // remove old_pred_loc from location table.
		  if (!_successors.has_key(old_pred_loc->getDHTKey())) // XXX: prevents rare case in which our predecessor is also our successor (two-nodes ring).
		    removeLocation(old_pred_loc);
	       }
	  }
	return DHT_ERR_OK;
     }
   
   void DHTVirtualNode::findClosestPredecessor(const DHTKey& nodeKey,
						  DHTKey& dkres, NetAddress& na,
						  DHTKey& dkres_succ, NetAddress &dkres_succ_na,
						  int& status)
     {
	_fgt->findClosestPredecessor(nodeKey, dkres, na, dkres_succ, dkres_succ_na, status);
     }

   void DHTVirtualNode::ping()
     {
     }
      
   /**-- functions using RPCs. --**/
   void DHTVirtualNode::join(const DHTKey& dk_bootstrap,
				const NetAddress &dk_bootstrap_na,
				const DHTKey& senderKey,
				int& status)
     {
	/**
	 * reset predecessor.
	 */
       mutex_lock(&_pred_mutex);
	_predecessor = NULL;
	mutex_unlock(&_pred_mutex);
	
	/**
	 * query the bootstrap node for our successor.
	 */
	DHTKey dkres;
	NetAddress na;
	_pnode->_l1_client->RPC_joinGetSucc(dk_bootstrap, dk_bootstrap_na,
							  senderKey,
							  dkres, na, status);
	
	if (status != DHT_ERR_OK)
	     return;
        _connected = true;       
	
        setSuccessor(dkres,na);
        update_successor_list_head();
     }
      
   dht_err DHTVirtualNode::find_successor(const DHTKey& nodeKey,
					  DHTKey& dkres, NetAddress& na)
     {
	DHTKey dk_pred;
	NetAddress na_pred;
	
	/**
	 * find closest predecessor to nodeKey.
	 */
	dht_err dht_status = find_predecessor(nodeKey, dk_pred, na_pred);
	
	/**
	 * check on find_predecessor's status.
	 */
	if (dht_status != DHT_ERR_OK)
	  {
#ifdef DEBUG
	     //debug
	     std::cerr << "find_successor failed on getting predecessor\n";
	     //debug
#endif
	     
	     errlog::log_error(LOG_LEVEL_DHT, "find_successor failed on getting predecessor");
	     return dht_status;
	  }
	
	/**
	 * RPC call to get pred's successor.
	 * we check among local virtual nodes first.
	 */
	int status = 0;
	_pnode->getSuccessor_cb(dk_pred,dkres,na,status);
	if (status == DHT_ERR_UNKNOWN_PEER)
	  _pnode->_l1_client->RPC_getSuccessor(dk_pred, na_pred, 
					       dkres, na, status);
	if (status == DHT_ERR_OK)
	  {
	     Location *uloc = findLocation(dk_pred);
	     if (uloc)
	       uloc->update_check_time();
	  }
	
	return status;
     }
   
   dht_err DHTVirtualNode::find_predecessor(const DHTKey& nodeKey,
					    DHTKey& dkres, NetAddress& na)
     {
	static short retries = 2;
	int ret = 0;
		
	/**
	 * Default result is itself.
	 */
	dkres = getIdKey();
	na = getNetAddress();
	
	/**
	 * location to iterate (route) among results.
	 */
	Location rloc(_idkey, getNetAddress());
	
	DHTKey succ = getSuccessorS();
	if (!succ.count())
	  {
	    /**
	      * TODO: after debugging, write a better handling of this error.
              */
	     // this should not happen though...
	     // TODO: errlog.
	     std::cerr << "[Error]:DHTNode::find_predecessor: this virtual node has no successor:"
               << nodeKey << ".Exiting\n";
	     exit(-1);
	  }
	
	Location succloc(succ,NetAddress()); // warning: at this point the address is not needed.
	RouteIterator rit;
	rit._hops.push_back(new Location(rloc.getDHTKey(),rloc.getNetAddress()));
	
	short nhops = 0;
	while(!nodeKey.between(rloc.getDHTKey(), succloc.getDHTKey())
	      && nodeKey != succloc.getDHTKey()) // equality: host node is the node with the same key as the looked up key.
	  {
#ifdef DEBUG
	     //debug
	     std::cerr << "[Debug]:find_predecessor: passed between test: nodekey "
	       << nodeKey << " not between " << rloc.getDHTKey() << " and " << succloc.getDHTKey() << std::endl;
	     //debug
#endif
	     
	     if (nhops > dht_configuration::_dht_config->_max_hops)
	       {
#ifdef DEBUG
		  //debug
		 std::cerr << "[Debug]:reached the maximum number of " << dht_configuration::_dht_config->_max_hops << " hops\n";
		  //debug
#endif	  
		 errlog::log_error(LOG_LEVEL_DHT, "reached the maximum number of %u hops", dht_configuration::_dht_config->_max_hops);
		  dkres = DHTKey();
		  na = NetAddress();
		  return DHT_ERR_MAXHOPS;
	       }
	     	     
	     /**
	      * RPC calls.
	      */
	     int status = -1;
	     const DHTKey recipientKey = rloc.getDHTKey();
	     const NetAddress recipient = rloc.getNetAddress();
	     DHTKey succ_key = DHTKey();
	     NetAddress succ_na = NetAddress();
	     dkres = DHTKey();
	     na = NetAddress();
	     
	     /**
	      * we make a local call to virtual nodes first, and a remote call if needed.
	      */
	     _pnode->findClosestPredecessor_cb(recipientKey,
							     nodeKey, dkres, na,
							     succ_key, succ_na, status);
	     if (status == DHT_ERR_UNKNOWN_PEER)
	       _pnode->_l1_client->RPC_findClosestPredecessor(recipientKey, recipient, 
								    nodeKey, dkres, na, 
								    succ_key, succ_na, status);
	     
             /**
	      * If the call has failed, then our current findPredecessor function
	      * has undershot the looked up key. In general this means the call
	      * has failed and should be retried.
	      */
	     if (status != DHT_ERR_OK)
	       {
		  if (ret < retries && (status == DHT_ERR_CALL 
					|| status == DHT_ERR_COM_TIMEOUT)) // failed call, remote node does not respond.
		    {
#ifdef DEBUG
		       //debug
		       std::cerr << "[Debug]:error while finding predecessor, will try to undershoot to find a new route\n";
		       //debug
#endif
		       
		       // let's undershoot by finding the closest predecessor to the 
		       // dead node.
		       std::vector<Location*>::iterator rtit = rit._hops.end();
		       while(status != DHT_ERR_OK && rtit!=rit._hops.begin())
			 {
			    --rtit;
			    
			    Location *past_loc = (*rtit);
			    
			    _pnode->findClosestPredecessor_cb(past_loc->getDHTKey(),
								    recipientKey,dkres,na,
								    succ_key,succ_na,status);
			    if (status == DHT_ERR_UNKNOWN_PEER)
			      _pnode->_l1_client->RPC_findClosestPredecessor(past_loc->getDHTKey(),
										   past_loc->getNetAddress(),
										   recipientKey,dkres,na,
										   succ_key,succ_na,status);
			 }
		    
		       if (status != DHT_ERR_OK)
			 {
			    // weird, undershooting did fail.
			    errlog::log_error(LOG_LEVEL_DHT, "Tentative overshooting did fail in find_predecessor");
			    return status;
			 }
		       else
			 {
			    Location *uloc = findLocation((*rtit)->getDHTKey());
			    rtit++;
			    rit.erase_from(rtit);
			    if (uloc)
			      uloc->update_check_time();
			 }
		       ret++; // we have a limited number of such routing trials.
		    }
	       } /* end if !DHT_ERR_OK */
	     else
	       {
		  Location *uloc = findLocation(recipientKey);
		  if (uloc)
		    uloc->update_check_time();
	       }
	     	     	     
	     /**
	      * check on rpc status.
	      */
	     if (status != DHT_ERR_OK)
	       {
#ifdef DEBUG
		  //debug
		  std::cerr << "[Debug]:DHTVirtualNode::find_predecessor: failed.\n";
		  //debug
#endif
		  
		  return status;
	       }
	     
#ifdef DEBUG
	     //debug
	     std::cerr << "[Debug]:find_predecessor: dkres: " << dkres << std::endl;
	     //debug
	     
	     //debug
	     assert(dkres.count()>0);
	     assert(dkres != rloc.getDHTKey());
	     //debug
#endif
	     
	     rloc.setDHTKey(dkres);
	     rloc.setNetAddress(na);
	     rit._hops.push_back(new Location(rloc.getDHTKey(),rloc.getNetAddress()));
	     
	     if (succ_key.count() > 0
		 && status == DHT_ERR_OK)
	       {
	       }
	     else
	       {
		  /**
		   * In general we need to ask rloc for its successor.
		   */
		  _pnode->getSuccessor_cb(dkres,succ_key,succ_na,status);
		  if (status == DHT_ERR_UNKNOWN_PEER)
		    _pnode->_l1_client->RPC_getSuccessor(dkres, na,
							 succ_key, succ_na, status);
		  
		  if (status != DHT_ERR_OK)
		    {
#ifdef DEBUG
		       //debug
		       std::cerr << "[Debug]:find_predecessor: failed call to getSuccessor: " 
			         << status << std::endl;
		       //debug
#endif
		       
		       errlog::log_error(LOG_LEVEL_DHT, "Failed call to getSuccessor in find_predecessor loop");
		       return status;
		    }
		  else 
		    {
		       Location *uloc = findLocation(dkres);
		       if (uloc)
			 uloc->update_check_time();
		    }
	       }
	     
	     assert(succ_key.count()>0);
	     
	     succloc.setDHTKey(succ_key);
	     succloc.setNetAddress(succ_na);
	  
	     nhops++;
	  }
	
#ifdef DEBUG
	//debug
	assert(dkres.count()>0);
	//debug
#endif
	
	return DHT_ERR_OK;	
     }

   bool DHTVirtualNode::is_dead(const DHTKey &recipientKey, const NetAddress &na,
				   int &status)
     {
	if (_pnode->findVNode(recipientKey))
	  {
	     // this is a local virtual node... Either our successor
	     // is not up-to-date, either we're cut off from the world...
	     // XXX: what to do ?
	     Location *uloc = findLocation(recipientKey);
	     if (uloc)
	       uloc->update_check_time();
	     status = DHT_ERR_OK;
	     return false;
	  }
	else
	  {
	     // let's ping that node.
	     status = DHT_ERR_OK;
	     _pnode->_l1_client->RPC_ping(recipientKey,na,
							status);
	     if (status == DHT_ERR_OK)
	       {
		  Location *uloc = findLocation(recipientKey);
		  if (uloc)
		    uloc->update_check_time();
		  return false;
	       }
	     else return true;
	  }
     }
  
  dht_err DHTVirtualNode::leave()
  {
    dht_err err = DHT_ERR_OK;
    
    // send predecessor to successor.
    DHTKey succ = getSuccessorS();
    DHTKey pred = getPredecessorS();
    if (succ.count() && pred.count())
      {
	int status = DHT_ERR_OK;
	Location *succ_loc = findLocation(succ);
	Location *pred_loc = findLocation(pred);
	_pnode->_l1_client->RPC_notify(succ_loc->getDHTKey(),
				       succ_loc->getNetAddress(),
				       pred_loc->getDHTKey(),
				       pred_loc->getNetAddress(),
				       status);
	if (status != DHT_ERR_OK)
	  errlog::log_error(LOG_LEVEL_ERROR,"Failed to alert successor %s while leaving",
			    succ_loc->getDHTKey().to_rstring().c_str());
	err = status;
      }
    
    /**
     * this node could send the last element of its succlist to 
     * its predecessor.
     * However, this would require a dedicated RPC so we let the
     * regular existing update of succlists do this instead.
     */
    
    errlog::log_error(LOG_LEVEL_DHT,"Virtual node %s successfully left the circle",
		      getIdKey().to_rstring().c_str());

    return err;
  }
   
   /**
    * accessors.
    */
   void DHTVirtualNode::setSuccessor(const DHTKey &dk)
     {
#ifdef DEBUG
	//debug
	assert(dk.count()>0);
	//debug
#endif
	
	mutex_lock(&_succ_mutex);
	if (_successor)
	  delete _successor;
	_successor = new DHTKey(dk);
	mutex_unlock(&_succ_mutex);
     }
   
   void DHTVirtualNode::setSuccessor(const DHTKey& dk, const NetAddress& na)
     {
       DHTKey succ = getSuccessorS();
       if (succ.count() && succ == dk)
	 {
	     /**
	      * lookup for the corresponding location.
	      */
	     Location* loc = addOrFindToLocationTable(dk,na);
	     
	     //TODO: do something with this error ?
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
	     
	     /**
	      * takes the first spot of the finger table.
	      */
	     _fgt->_locs[0] = loc;
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
	     
	     /**
	      * takes the first spot of the finger table.
	      */
	     _fgt->_locs[0] = loc;
	  }
     }

   void DHTVirtualNode::setPredecessor(const DHTKey &dk)
     {
	mutex_lock(&_pred_mutex);
	if (_predecessor)
	  delete _predecessor;
	_predecessor = new DHTKey(dk);
	mutex_unlock(&_pred_mutex);
     }

   void DHTVirtualNode::setPredecessor(const DHTKey& dk, const NetAddress& na)
     {
       DHTKey pred = getPredecessorS();
       if (pred.count() && pred == dk)
	  {
	    /**
	      * lookup for the corresponding location.
              */
	     Location* loc = addOrFindToLocationTable(dk,na);
	     
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

   LocationTable* DHTVirtualNode::getLocationTable() const
     {
	return _lt;
     }
      
   Location* DHTVirtualNode::findLocation(const DHTKey& dk) const
     {
	return _lt->findLocation(dk);
     }
   
   void DHTVirtualNode::addToLocationTable(const DHTKey& dk, const NetAddress& na,
					   Location *&loc) const
     {
	_lt->addToLocationTable(dk, na, loc);
     }

   void DHTVirtualNode::removeLocation(Location *loc)
     {
	_fgt->removeLocation(loc);
	_successors.removeKey(loc->getDHTKey());
	mutex_lock(&_pred_mutex);
	if (_predecessor && *_predecessor == loc->getDHTKey())
	  _predecessor = NULL; // beware.
	mutex_unlock(&_pred_mutex);
	_lt->removeLocation(loc);
     }
   
   NetAddress DHTVirtualNode::getNetAddress() const
     {
	return _pnode->getNetAddress();
     }
   
   Location* DHTVirtualNode::addOrFindToLocationTable(const DHTKey& key, const NetAddress& na)
     {
	return _lt->addOrFindToLocationTable(key, na);
     }

   bool DHTVirtualNode::isPredecessorEqual(const DHTKey &key)
     {
       mutex_lock(&_pred_mutex);
       if (!_predecessor)
	 {
	   mutex_unlock(&_pred_mutex);
	  return false;
	 }
       else 
	 {
	   bool test = (*_predecessor == key);
	   mutex_unlock(&_pred_mutex);
	   return test;
	 }
     }

   bool DHTVirtualNode::not_used_location(Location *loc)
     {
       DHTKey succ = getSuccessorS();
       if (!_fgt->has_key(-1,loc)
	   && !isPredecessorEqual(loc->getDHTKey())
	   && (!succ.count() || succ != loc->getDHTKey()))
	 return true;
       return false;
     }
      
   void DHTVirtualNode::update_successor_list_head()
     {
       mutex_lock(&_succ_mutex);
	_successors.set_direct_successor(_successor);
	mutex_unlock(&_succ_mutex);
     }

  bool DHTVirtualNode::isSuccStable() const
  {
    return _successors.isStable();
  }

  bool DHTVirtualNode::isStable() const
  {
    return _fgt->isStable();
  }

   void DHTVirtualNode::estimate_nodes()
     {
	estimate_nodes(_nnodes,_nnvnodes);
	
#ifdef DEBUG
	//debug
	std::cerr << "[Debug]:estimated number of nodes on the circle: " << _nnodes
	  << " -- and virtual nodes: " << _nnvnodes << std::endl;
	//debug
#endif
     }
      
   void DHTVirtualNode::estimate_nodes(unsigned long &nnodes, unsigned long &nnvnodes)
     {
	DHTKey closest_succ;
	_lt->findClosestSuccessor(_idkey,closest_succ);
	
	/*#ifdef DEBUG
	//debug
	assert(closest_succ.count()>0);
	//debug
	#endif*/
	
	DHTKey diff = closest_succ - _idkey;

	/*#ifdef DEBUG
	//debug
	assert(diff.count()>0);
	//debug
	#endif*/
	
	DHTKey density = diff;
	size_t lts = _lt->size();
	double p = ceil(log(lts)/log(2)); // in powers of two.
	density.operator>>=(p); // approximates diff / ltsize.
	DHTKey mask(1);
	mask <<= KEYNBITS-1;
	p = density.topBitPos();
	
	/*#ifdef DEBUG
	//debug
	assert(p>0);
	//debug
	#endif*/
	
	DHTKey nodes_estimate = mask;
	nodes_estimate.operator>>=(p); // approximates mask/density.
	
	try
	  {
	     nnvnodes = nodes_estimate.to_ulong();
	     nnodes = nnvnodes / dht_configuration::_dht_config->_nvnodes;
	  }
	catch (std::exception ovex) // overflow error if too many bits are set.
	  {
#ifdef DEBUG
	     //debug
	     std::cerr << "exception overflow when computing the number of nodes...\n";
	     //debug
#endif
	     
	     errlog::log_error(LOG_LEVEL_ERROR, "Overflow error computing the number of nodes on the network. This is probably a bug, please report it");
	     nnodes = 0;
	     nnvnodes = 0;
	     return;
	  }
     
	_pnode->estimate_nodes(nnodes,nnvnodes);
     }

  DHTKey DHTVirtualNode::getPredecessorS()
  {
    mutex_lock(&_pred_mutex);
    if (_predecessor)
      {
	DHTKey res = *_predecessor;
	mutex_unlock(&_pred_mutex);
	return res;
      }
    else 
      {
	mutex_unlock(&_pred_mutex);
	return DHTKey();
      }
  }

  DHTKey DHTVirtualNode::getSuccessorS()
  {
    mutex_lock(&_succ_mutex);
    if (_successor)
      { 
	DHTKey res = *_successor;
	mutex_unlock(&_succ_mutex);
	return res;
      } 
    else
      { 
	mutex_unlock(&_succ_mutex);
	return DHTKey();
      }
  }
  
} /* end of namespace. */
