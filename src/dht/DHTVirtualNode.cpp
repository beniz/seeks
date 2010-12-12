/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
 * Copyright (C) 2010  Loic Dachary <loic@dachary.org>
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

#include "dht_configuration.h"
#include "DHTVirtualNode.h"
#include "Transport.h"
#include "FingerTable.h"
#include "RouteIterator.h"
#include "errlog.h"
#include "l1_rpc_interface.h"

#include <math.h>

using sp::errlog;

#define WITH_RANDOM_KEY
//#define DEBUG

namespace dht
{
  DHTVirtualNode::DHTVirtualNode(Transport* transport)
    : _lt(NULL),_transport(transport),_successor(NULL),_predecessor(NULL),
      _successors(this),_fgt(NULL),_nnodes(0),_nnvnodes(0),_connected(false)
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

  DHTVirtualNode::DHTVirtualNode(Transport *transport, const DHTKey &idkey, LocationTable *lt)
    : _lt(lt), _transport(transport),_successor(NULL),_predecessor(NULL),
      _successors(this),_fgt(NULL),_nnodes(0),_nnvnodes(0),_connected(false)
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

    delete _fgt;
    _fgt = NULL;

    delete _lt;
    _lt = NULL;
  }

  NetAddress DHTVirtualNode::getNetAddress() const
  {
    return _transport->getNetAddress();
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
#if 0
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
#endif
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
                            int& status) throw (dht_exception)
  {
    /**
     * reset predecessor.
     */
    mutex_lock(&_pred_mutex);
    _predecessor = NULL;
    mutex_unlock(&_pred_mutex);

    assert(senderKey == _idkey);

    /**
     * query the bootstrap node for our successor.
     */
    DHTKey dkres;
    NetAddress na;
    RPC_joinGetSucc(dk_bootstrap, dk_bootstrap_na,
                    senderKey,
                    dkres, na, status);

    if (status != DHT_ERR_OK)
      return;
    _connected = true;

    setSuccessor(dkres,na);
    update_successor_list_head();
  }

  dht_err DHTVirtualNode::find_successor(const DHTKey& nodeKey,
                                         DHTKey& dkres, NetAddress& na) throw (dht_exception)
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
    RPC_getSuccessor(dk_pred, na_pred,
                     dkres, na, status);
#if 0
    if (status == DHT_ERR_OK)
      {
        Location *uloc = findLocation(dk_pred);
        if (uloc)
          uloc->update_check_time();
      }
#endif

    return status;
  }

  dht_err DHTVirtualNode::find_predecessor(const DHTKey& nodeKey,
      DHTKey& dkres, NetAddress& na) throw (dht_exception)
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
        std::cerr << "[Error]:Transport::find_predecessor: this virtual node has no successor:"
                  << nodeKey << ".Exiting\n";
        exit(-1);
      }

    Location succloc(succ,NetAddress()); // warning: at this point the address is not needed.
    RouteIterator rit;
    rit._hops.push_back(new Location(rloc.getDHTKey(),rloc.getNetAddress()));

    short nhops = 0;
    // while(!nodekey in ]rloc,succloc])
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

        RPC_findClosestPredecessor(recipientKey, recipient,
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

                    RPC_findClosestPredecessor(past_loc->getDHTKey(),
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
#if 0
                    Location *uloc = findLocation((*rtit)->getDHTKey());
                    if (uloc)
                      uloc->update_check_time();
#endif
                    rtit++;
                    rit.erase_from(rtit);
                  }
                ret++; // we have a limited number of such routing trials.
              }
          } /* end if !DHT_ERR_OK */
        else
          {
#if 0
            Location *uloc = findLocation(recipientKey);
            if (uloc)
              uloc->update_check_time();
#endif
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
            RPC_getSuccessor(dkres, na,
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
#if 0
                Location *uloc = findLocation(dkres);
                if (uloc)
                  uloc->update_check_time();
#endif
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
                               int &status) throw (dht_exception)
  {
    // let's ping that node.
    status = DHT_ERR_OK;
    RPC_ping(recipientKey,na,
             status);
    if (status == DHT_ERR_OK)
      {
#if 0
        Location *uloc = findLocation(recipientKey);
        if (uloc)
          uloc->update_check_time();
#endif
        return false;
      }
    else return true;
  }

  dht_err DHTVirtualNode::leave() throw (dht_exception)
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
        RPC_notify(succ_loc->getDHTKey(),
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
#if 0
  NetAddress DHTVirtualNode::getNetAddress() const
  {
    return _transport->getNetAddress();
  }
#endif
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

#if 0
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

    _transport->estimate_nodes(nnodes,nnvnodes);
  }
#endif

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

  /**----------------------------**/
  /**
   * RPC virtual functions (callbacks).
   */
  void DHTVirtualNode::getSuccessor_cb(
    DHTKey& dkres, NetAddress& na,
    int& status) throw (dht_exception)
  {
    status = DHT_ERR_OK;

    DHTVirtualNode* vnode = this;

    /**
     * return successor info.
     */
    dkres = vnode->getSuccessorS();
    if (!dkres.count())
      {
        //TODO: this node has no successor yet: this means it must have joined
        //      the network a short time ago or that it is alone on the network.
        na = NetAddress();
        status = DHT_ERR_NO_SUCCESSOR_FOUND;
        return;
      }

    /**
     * fetch location information.
     */
    Location* resloc = vnode->findLocation(dkres);
    if (!resloc)
      {
        // XXX: we should never reach here.
        std::cout << "[Error]:RPC_getSuccessor_cb: our own successor is an unknown location !\n";
        throw dht_exception(DHT_ERR_UNKNOWN_PEER_LOCATION,"RPC_getSuccessor_cb: our own successor is an unknown location");
      }

    /**
     * Setting RPC results.
     */
    na = resloc->getNetAddress();

    //debug
    //assert(dkres.count()>0);
    //debug
  }

  void DHTVirtualNode::getPredecessor_cb(
    DHTKey& dkres, NetAddress& na,
    int& status)
  {
    status = DHT_ERR_OK;

    DHTVirtualNode* vnode = this;

    /**
     * return predecessor info.
     */
    dkres = vnode->getPredecessorS();
    if (!dkres.count())
      {
        na = NetAddress();
        status = DHT_ERR_NO_PREDECESSOR_FOUND;
        return;
      }

    /**
     * fetch location information.
     */
    Location* resloc = vnode->findLocation(dkres);
    if (!resloc)
      {
        std::cout << "[Error]:RPC_getPredecessor_cb: our own predecessor is an unknown location !\n";
        na = NetAddress();
        status = DHT_ERR_UNKNOWN_PEER_LOCATION;
        return;
      }

    /**
     * Setting RPC results.
     */
    na = resloc->getNetAddress();
  }

  void DHTVirtualNode::notify_cb(
    const DHTKey& senderKey,
    const NetAddress& senderAddress,
    int& status)
  {
    status = DHT_ERR_OK;

    DHTVirtualNode* vnode = this;

    /**
     * notifies this node that the argument node (key) thinks it is
     * its predecessor.
     */
    status = vnode->notify(senderKey, senderAddress);

  }

  void DHTVirtualNode::getSuccList_cb(
    std::list<DHTKey> &dkres_list,
    std::list<NetAddress> &na_list,
    int &status)
  {
    status = DHT_ERR_OK;

    DHTVirtualNode* vnode = this;

    if (vnode->_successors.empty())
      {
        dkres_list = std::list<DHTKey>();
        na_list = std::list<NetAddress>();
        status = DHT_ERR_OK;
        return;
      }

    /**
     * return successor list.
     */
    std::list<const DHTKey*>::const_iterator sit = vnode->_successors._succs.begin();
    while(sit!=vnode->_successors._succs.end())
      {
        dkres_list.push_back(*(*sit));
        ++sit;
      }

    if (dkres_list.empty())
      {
        na_list = std::list<NetAddress>();
        status = DHT_ERR_NO_SUCCESSOR_FOUND;
        return;
      }

    /**
     * fetch location information.
     */
    std::list<DHTKey>::const_iterator tit = dkres_list.begin();
    while(tit!=dkres_list.end())
      {
        Location *resloc = vnode->findLocation((*tit));
        if (!resloc)
          {
            dkres_list.clear();
            na_list = std::list<NetAddress>();
            status = DHT_ERR_UNKNOWN_PEER_LOCATION;
            return;
          }
        na_list.push_back(resloc->getNetAddress());
        ++tit;
      }

    //debug
    assert(!na_list.empty());
    //debug
  }

  void DHTVirtualNode::findClosestPredecessor_cb(
    const DHTKey& nodeKey,
    DHTKey& dkres, NetAddress& na,
    DHTKey& dkres_succ, NetAddress &dkres_succ_na,
    int& status)
  {
    status = DHT_ERR_OK;

    DHTVirtualNode* vnode = this;

    /**
     * return closest predecessor.
     */
    vnode->findClosestPredecessor(nodeKey, dkres, na, dkres_succ, dkres_succ_na, status);

    if(status != DHT_ERR_OK && status != DHT_ERR_UNKNOWN_PEER_LOCATION)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed findClosestPredecessor_cb");
        return;
      }

    //debug
    assert(dkres.count()>0);
    //debug
  }

  void DHTVirtualNode::joinGetSucc_cb(
    const DHTKey &senderKey,
    DHTKey &dkres, NetAddress &na,
    int &status)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]:executing joinGetSucc_cb.\n";
    //debug
#endif

    DHTVirtualNode *vnode = this;

    status = vnode->find_successor(senderKey, dkres, na);
    if (status < 0) // TODO: more precise check.
      {
        dkres = DHTKey();
        na = NetAddress();
        status = DHT_ERR_UNKNOWN; // TODO: some other error ?
      }
    else if (dkres.count() == 0) // TODO: if not found.
      {
        na = NetAddress();
        status = DHT_ERR_NO_SUCCESSOR_FOUND;
        return;
      }

    status = DHT_ERR_OK;
  }

  void DHTVirtualNode::ping_cb(
    int& status)
  {
    status = DHT_ERR_OK;

    DHTVirtualNode* vnode = this;

    /**
     * ping this virtual node.
     */
    vnode->ping();
  }

  void DHTVirtualNode::RPC_call(const uint32_t &fct_id,
                                const DHTKey &recipientKey,
                                const NetAddress& recipient,
                                const DHTKey &nodeKey,
                                l1::l1_response *l1r) throw (dht_exception)
  {
    // serialize.
    l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,
                        recipientKey,recipient,
                        nodeKey);
    RPC_call(l1q,recipientKey,recipient,l1r);
  }

  void DHTVirtualNode::RPC_call(const uint32_t &fct_id,
                                const DHTKey &recipientKey,
                                const NetAddress &recipient,
                                l1::l1_response *l1r) throw (dht_exception)
  {
    // serialize.
    l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,
                        recipientKey,recipient);

    RPC_call(l1q,recipientKey,recipient,l1r);
  }

  void DHTVirtualNode::RPC_call(const uint32_t &fct_id,
                                const DHTKey &recipientKey,
                                const NetAddress& recipient,
                                const DHTKey &senderKey,
                                const NetAddress& senderAddress,
                                l1::l1_response *l1r) throw (dht_exception)
  {
    // serialize.
    DHTKey nodeKey;
    l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,
                        recipientKey,recipient,
                        senderKey,senderAddress,
                        nodeKey); // empty nodeKey.
    RPC_call(l1q,recipientKey,recipient,l1r);
  }

  void DHTVirtualNode::RPC_call(l1::l1_query *&l1q,
                                const DHTKey &recipientKey,
                                const NetAddress& recipient,
                                l1::l1_response *l1r) throw (dht_exception)
  {
    std::string msg_str;
    l1_protob_wrapper::serialize_to_string(l1q,msg_str);
    std::string resp_str;

    try
      {
        _transport->do_rpc_call(recipient,recipientKey,msg_str,true,resp_str);
      }
    catch (dht_exception &e)
      {
        delete l1q;
        if (e.code() == DHT_ERR_COM_TIMEOUT)
          {
            l1r->set_error_status(e.code());
            return;
          }
        else
          {
            throw e;
          }
      }
    delete l1q;

    // deserialize response.
    try
      {
        l1_protob_wrapper::deserialize(resp_str,l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"rpc l1 error: %s",e.what().c_str());
        throw dht_exception(DHT_ERR_NETWORK, e.what());
      }
  }

  void DHTVirtualNode::RPC_getSuccessor(const DHTKey &recipientKey,
                                        const NetAddress& recipient,
                                        DHTKey& dkres, NetAddress& na,
                                        int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_getSuccessor call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        DHTVirtualNode::RPC_call(hash_get_successor,
                                 recipientKey,recipient,
                                 &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed getSuccessor call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed getSuccessor call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status,
                                        dkres,na);
    status = error_status; // remote error status.
  }

  void DHTVirtualNode::RPC_getPredecessor(const DHTKey& recipientKey,
                                          const NetAddress& recipient,
                                          DHTKey& dkres, NetAddress& na,
                                          int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_getPredecessor call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        DHTVirtualNode::RPC_call(hash_get_predecessor,
                                 recipientKey,recipient,
                                 &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed getPredecessor call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed getPredecessor call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status,
                                        dkres,na);
    status = error_status;
  }

  void DHTVirtualNode::RPC_notify(const DHTKey& recipientKey,
                                  const NetAddress& recipient,
                                  const DHTKey& senderKey,
                                  const NetAddress& senderAddress,
                                  int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_notify call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        DHTVirtualNode::RPC_call(hash_notify,
                                 recipientKey,recipient,
                                 senderKey,senderAddress,
                                 &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed notify call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed notify call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status);
    status = error_status;
  }

  void DHTVirtualNode::RPC_getSuccList(const DHTKey &recipientKey,
                                       const NetAddress &recipient,
                                       std::list<DHTKey> &dkres_list,
                                       std::list<NetAddress> &na_list,
                                       int &status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_getSuccList call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        DHTVirtualNode::RPC_call(hash_get_succlist,
                                 recipientKey,recipient,
                                 &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed getSuccList call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed getSuccList call to " + recipient.toString() + ":" + e.what());
      }

    /// handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status,
                                        dkres_list,na_list);
    status = error_status; // remote error status.
  }

  void DHTVirtualNode::RPC_findClosestPredecessor(const DHTKey& recipientKey,
      const NetAddress& recipient,
      const DHTKey& nodeKey,
      DHTKey& dkres, NetAddress& na,
      DHTKey& dkres_succ,
      NetAddress &dkres_succ_na,
      int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_findClosestPredecessor call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        DHTVirtualNode::RPC_call(hash_find_closest_predecessor,
                                 recipientKey,recipient,
                                 nodeKey,&l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed findClosestPredecessor call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed findClosestPredecessor call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status,
                                        dkres,na,
                                        dkres_succ,dkres_succ_na);
    status = error_status;
  }

  void DHTVirtualNode::RPC_joinGetSucc(const DHTKey& recipientKey,
                                       const NetAddress& recipient,
                                       const DHTKey &senderKey,
                                       DHTKey& dkres, NetAddress& na,
                                       int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_joinGetSucc call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    // empty address.
    const NetAddress senderAddress = NetAddress();

    try
      {
        DHTVirtualNode::RPC_call(hash_join_get_succ,
                                 recipientKey,recipient,
                                 senderKey, senderAddress,
                                 &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed joinGetSucc call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed joinGetSucc call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status,
                                        dkres,na);
    status = error_status;
  }

  void DHTVirtualNode::RPC_ping(const DHTKey& recipientKey,
                                const NetAddress& recipient,
                                int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_ping call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        DHTVirtualNode::RPC_call(hash_ping,
                                 recipientKey,recipient,
                                 &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed ping call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed ping call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status);
    status = error_status;
  }

} /* end of namespace. */
