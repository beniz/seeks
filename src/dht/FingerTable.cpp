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

#include "FingerTable.h"
#include "DHTNode.h"
#include "Transport.h"
#include "Random.h"
#include "errlog.h"

#include <assert.h>

//#define DEBUG

using lsh::Random;
using sp::errlog;

namespace dht
{
  FingerTable::FingerTable(DHTVirtualNode* vnode, LocationTable* lt)
    : Stabilizable(), _vnode(vnode), _lt(lt), _stable_pass1(false), _stable_pass2(false)
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

    mutex_init(&_stable_mutex);
  }

  FingerTable::~FingerTable()
  {
  }

  void FingerTable::findClosestPredecessor(const DHTKey& nodeKey,
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

#ifdef DEBUG
            //debug
            assert(dkres.count()>0);
            //debug
#endif

            na = loc->getNetAddress();
            dkres_succ = DHTKey();
            dkres_succ_na = NetAddress();
            status = DHT_ERR_OK;
            return;
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
#ifdef DEBUG
        //debug
        std::cerr << "[Debug]:found closest predecessor in successor list: "
                  << dkres << std::endl;
        //debug
#endif

        status = DHT_ERR_OK;
        return;
      }

    /**
     * otherwise return current node's id key, and sets the successor
     * (saves an rpc later).
     */
    std::cerr << "[Debug]:closest predecessor is node itself...\n";

    status = DHT_ERR_OK;
    dkres = getVNodeIdKey();
    na = getVNodeNetAddress();
    dkres_succ = getVNodeSuccessorS();
    Location *succ_loc = findLocation(dkres_succ);
    if (!succ_loc)
      {
        status = DHT_ERR_UNKNOWN_PEER_LOCATION;
        return;
      }
    dkres_succ_na = succ_loc->getNetAddress();

#ifdef DEBUG
    //debug
    assert(dkres!=getVNodeIdKey()); // exit.
    //debug
#endif
  }

  int FingerTable::stabilize() throw (dht_exception)
  {
    static int retries = 3;
    int ret = 0;

#ifdef DEBUG
    //debug
    std::cerr << "[Debug]:FingerTable::stabilize()\n";
    //debug
#endif

    DHTKey recipientKey = _vnode->getIdKey();
    NetAddress na = getVNodeNetAddress();

    /**
     * get successor's predecessor.
     */
    DHTKey succ = _vnode->getSuccessorS();
    if (!succ.count())
      {
        /**
         * TODO: after debugging, write a better handling of this error.
        	 */
        std::cerr << "[Error]:FingerTable::stabilize: this virtual node has no successor: "
                  << recipientKey << ". Exiting\n";
        exit(-1);
      }

    /**
     * lookup for the peer.
     */
    Location* succ_loc = findLocation(succ);
    if (!succ_loc)
      {
        errlog::log_error(LOG_LEVEL_DHT,"Stabilize: cannot find successor %s in location table",
                          succ.to_rstring().c_str());
        return DHT_ERR_UNKNOWN_PEER_LOCATION;
      }

    /**
    	 * RPC call to get the predecessor.
    	 */
    DHTKey succ_pred;
    NetAddress na_succ_pred;

    bool successor_change = false;
    dht_err status = 0;
    dht_err err = DHT_ERR_RETRY;
    std::vector<Location*> dead_locs;
    while (err != DHT_ERR_OK) // in case of failure, will try all successors in the list. If all fail, then rejoin.
      {
        err = DHT_ERR_OK;
        _vnode->RPC_getPredecessor(succ_loc->getDHTKey(), succ_loc->getNetAddress(),
                                   succ_pred, na_succ_pred, status);

#ifdef DEBUG
        //debug
        std::cerr << "[Debug]: predecessor call: -- status= " << status << std::endl;
        //debug
#endif

        /**
         * handle successor failure, retry, then move down the successor list.
         */
        if (status != DHT_ERR_OK
            && (status == DHT_ERR_CALL || status == DHT_ERR_COM_TIMEOUT || status == DHT_ERR_UNKNOWN_PEER
                || ret == retries)) // node is not dead, but predecessor call has failed 'retries' times.
          {
            /**
             * our successor is not responding.
             * let's ping it, if it seems dead, let's move to the next successor on our list.
             */

            bool dead = false;
            _vnode->is_dead(succ,succ_loc->getNetAddress(),
                            status);

            if (dead || ret == retries)
              {
#ifdef DEBUG
                //debug
                std::cerr << "[Debug]:dead successor detected\n";
                //debug
#endif

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

#ifdef DEBUG
                //debug
                std::cerr << "[Debug]:trying successor: " << *(*kit) << std::endl;
                //debug
#endif

                DHTKey *succ_ptr = const_cast<DHTKey*>((*kit));
                assert(succ_ptr); // beware, TODO: this should never happen, take it into account ?
                succ = *succ_ptr;

                // mark dead nodes for tentative removal from location table.
                dead_locs.push_back(succ_loc);

                // sets a new successor.
                succ_loc = findLocation(succ);

                // replaces the successor and the head of the successor list as well.
                _vnode->setSuccessor(succ_loc->getDHTKeyRef(),succ_loc->getNetAddress());
                successor_change = true;

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
#ifdef DEBUG
            //debug
            std::cerr << "[Debug]: trying to call the (new) successor, one more loop.\n";
            //debug
#endif

            continue;
          }

        /**
         * check on RPC status.
         */
        if (status == DHT_ERR_NO_PREDECESSOR_FOUND) // our successor has an unset predecessor.
          {
#ifdef DEBUG
            //debug
            std::cerr << "[Debug]:stabilize: our successor has no predecessor set.\n";
            //debug
#endif

            break;
          }
        else if (status == DHT_ERR_OK)
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
                for (size_t i=0; i<dead_locs.size(); i++)
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
    for (size_t i=0; i<dead_locs.size(); i++)
      {
        Location *dead_loc = dead_locs.at(i);

#ifdef DEBUG
        //debug
        assert(!_vnode->_successors.has_key(dead_loc->getDHTKey()));
        assert(*_vnode->getSuccessorPtr() != dead_loc->getDHTKey());
        //debug
#endif

        _vnode->removeLocation(dead_loc);
      }
    dead_locs.clear();

    /**
     * total failure, we're very much probably cut off from the network.
     * let's try to rejoin.
     * If join fails, let's fall back into a trying mode, in which
     * we try a join every x minutes.
     */
    //TODO: rejoin + wrong test here.
    if (_vnode->_successors.empty())
      {
        /* std::cerr << "[Debug]: no more successors... Should try to rejoin the overlay network\n"; */
        errlog::log_error(LOG_LEVEL_DHT,"Node %s has no more successors",_vnode->getIdKey().to_rstring().c_str());

        // single virtual node rejoin scheme.
        // _vnode->getPNode()->_stabilizer->rejoin(_vnode); AAAA

        //exit(0);
      }
    else if (successor_change)
      {
        // check wether we are on a ring of our virtual nodes only,
        // that is if we are not cut from the network, and need to
        // rejoin.
        // XXX: this should occur after the DHT node has lost connection
        // with the outside world from all of its virtual nodes.
        // _vnode->getPNode()->rejoin(); AAAA
      }

    if (status != DHT_ERR_NO_PREDECESSOR_FOUND && status != DHT_ERR_OK)
      {
#ifdef DEBUG
        //debug
        std::cerr << "[Debug]:FingerTable::stabilize: failed return from getPredecessor\n";
        //debug
#endif

        errlog::log_error(LOG_LEVEL_DHT, "FingerTable::stabilize: failed return from getPredecessor, %u",
                          status);
        //return (dht_err)status;

        std::cerr << "[Debug]: no more successors to try... Should try to rejoin the overlay network\n";
        exit(0);
      }
    else if (status == DHT_ERR_OK)
      {
        /**
         * look up succ_pred, add it to the location table if needed.
         * -> because succ_pred should be in the succlist or a predecessor anyways.
         */
        //Location *succ_pred_loc = _vnode->addOrFindToLocationTable(succ_pred, na_succ_pred);

        /**
         * key check: if a node has taken place in between us and our
         * successor.
         */
        if (succ_pred.between(recipientKey,succ))
          {
            Location *succ_pred_loc = _vnode->addOrFindToLocationTable(succ_pred, na_succ_pred);
            _vnode->setSuccessor(succ_pred_loc->getDHTKeyRef(),succ_pred_loc->getNetAddress());
            _vnode->update_successor_list_head();
          }
        else if (successor_change)
          {
#if 0
            //TODO: replication, move to successor the keys with a replication radius equal to k-1 -> NOP.
            // where k is the replication factor.
            //TODO: needs to move everything from 0 to k-1 !
            //TODO: beware of bootstrap.
            DHTKey vsucc = _vnode->getSuccessorS();
            _vnode->replication_move_keys_forward(vsucc);
#endif
          }
      }

    /**
     * notify RPC.
     *
     * XXX: in non routing mode, there is no notify callback call. This allows the node
     * to remain a connected spectator: successor and predecessors of other nodes remain unchanged.
     */
    if (dht_configuration::_dht_config->_routing)
      {
        _vnode->RPC_notify(succ_loc->getDHTKey(), succ_loc->getNetAddress(),
                           getVNodeIdKey(),getVNodeNetAddress(),
                           status);
        // XXX: handle successor failure, retry, then move down the successor list.
        /**
         * check on RPC status.
         */
        /* if (status != DHT_ERR_OK)
          {
        		 errlog::log_error(LOG_LEVEL_DHT, "FingerTable::stabilize: failed notify call");
        		 return status;
        		 } */
      }
    else // in non routing mode, check whether our predecessor has changed.
      {
        Location *succ_pred_loc = _vnode->addOrFindToLocationTable(succ_pred, na_succ_pred);
        DHTKey vpred = _vnode->getPredecessorS();
        if (vpred.count() && (vpred == _vnode->getIdKey()))
          {
            // XXX: this should not happen. It does when self-bootstrapping in non routing mode,
            // a hopeless situation.
          }
        else if (!vpred.count() || succ_pred_loc->getDHTKey() != vpred)
          _vnode->setPredecessor(succ_pred_loc->getDHTKey(), succ_pred_loc->getNetAddress());
      }

    return status;
  }

  int FingerTable::fix_finger() throw (dht_exception)
  {
    mutex_lock(&_stable_mutex);
    _stable_pass2 = _stable_pass1;
    _stable_pass1 = true; // below we check whether this is true.
    mutex_unlock(&_stable_mutex);

    // TODO: seed.
    unsigned long int rindex = Random::genUniformUnsInt32(1, KEYNBITS-1);

#ifdef DEBUG
    //debug
    std::cerr << "[Debug]:FingerTable::fix_finger: " << rindex << std::endl;
    //debug

    //debug
    assert(rindex > 0);
    assert(rindex < KEYNBITS);
    //debug
#endif

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
#ifdef DEBUG
        //debug
        std::cerr << "[Debug]:fix_finger: error finding successor.\n";
        //debug
#endif

        errlog::log_error(LOG_LEVEL_DHT, "Error finding successor when fixing finger.");
        return status;
      }

#ifdef DEBUG
    //debug
    assert(dkres.count()>0);
    //debug
#endif

    /**
     * lookup result, add it to the location table if needed.
     */
    Location* rloc = _vnode->addOrFindToLocationTable(dkres, na);
    Location *curr_loc = _locs[rindex];
    if (curr_loc && rloc != curr_loc)
      {
        mutex_lock(&_stable_mutex);
        _stable_pass1 = false;
        mutex_unlock(&_stable_mutex);

        // remove location if it not used in other lists.
        if (!_vnode->_successors.has_key(curr_loc->getDHTKey()))
          _vnode->removeLocation(curr_loc);
      }
    else if (!curr_loc)
      {
        mutex_lock(&_stable_mutex);
        _stable_pass1 = false;
        mutex_unlock(&_stable_mutex);
      }

    _locs[rindex] = rloc;

#if 0
    if (!_stable_pass1)
      {
        // reestimate the estimated number of nodes.
        _vnode->estimate_nodes();
        AAAA
      }
#endif

#ifdef DEBUG
    //debug
    print(std::cout);
    //debug
#endif

    return 0;
  }

  bool FingerTable::has_key(const int &index, Location *loc) const
  {
    for (int i=0; i<KEYNBITS; i++)
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
    for (int i=0; i<KEYNBITS; i++)
      {
        if (_locs[i] == loc)
          {
            _locs[i] = NULL;
          }
      }
  }

  bool FingerTable::isStable() const
  {
    return (_stable_pass1 && _stable_pass2);
  }

  void FingerTable::print(std::ostream &out) const
  {
    out << "   ftable: " << _vnode->getIdKey() << std::endl;
    DHTKey vpred = _vnode->getPredecessorS();
    DHTKey vsucc = _vnode->getSuccessorS();
    if (vsucc.count())
      out << "successor: " << vsucc << std::endl;
    if (vpred.count())
      out << "predecessor: " << vpred << std::endl;
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
