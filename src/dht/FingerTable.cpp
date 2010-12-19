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

  void FingerTable::fix_finger() throw (dht_exception)
  {
    // TODO: seed.
    unsigned long int rindex = Random::genUniformUnsInt32(1, KEYNBITS-1);

    /**
     * find_successor call.
     */
    DHTKey dkres;
    NetAddress na;
    if(_vnode->find_successor(_starts[rindex], dkres, na) != DHT_ERR_OK)
      return;

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

#if 0
    if (!_stable_pass1)
      {
        // reestimate the estimated number of nodes.
        _vnode->estimate_nodes();
      }
#endif
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
