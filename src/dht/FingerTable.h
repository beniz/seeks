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

#ifndef FINGERTABLE_H
#define FINGERTABLE_H

#include "Location.h"
#include "LocationTable.h"
#include "DHTVirtualNode.h"
#include "DHTKey.h"
#include "Stabilizer.h"
#include "mutexes.h"

namespace dht
{
  class LocationTable;

  class FingerTable : public Stabilizable
  {
    public:
      FingerTable(DHTVirtualNode* vnode, LocationTable* lt);

      ~FingerTable();

      /**
       * accessors.
       */
      Location* getFinger(const int& i)
      {
        return _locs[i];
      }
      void setFinger(const int& i, Location* loc);

      /**
       * accessors to vnode stuff.
       */
      const DHTKey& getVNodeIdKey() const
      {
        return _vnode->getIdKey();
      }
      NetAddress getVNodeNetAddress() const
      {
        return _vnode->getNetAddress();
      }
      DHTKey* getVNodeSuccessorPtr() const
      {
        return _vnode->getSuccessorPtr();
      }
      DHTKey getVNodeSuccessorS() const
      {
        return _vnode->getSuccessorS();
      }
      Location* findLocation(const DHTKey& dk) const
      {
        return _vnode->findLocation(dk);
      }

      /**
       * \brief test whether a location is used in the finger table.
       */
      bool has_key(const int &index, Location *loc) const;

      /**
       * \brief remove location from all indexes in the finger table.
       */
      void removeLocation(Location *loc);

      /**
       * \brief find closest predecessor to a given key.
       * Successor information is unset by default, set only if the closest node
       * is this virtual node.
       */
      void findClosestPredecessor(const DHTKey& nodeKey,
                                  DHTKey& dkres, NetAddress& na,
                                  DHTKey& dkres_succ, NetAddress &dkres_succ_na,
                                  int& status);

      /**
       * \brief refresh a random table entry.
       */
      void fix_finger() throw (dht_exception);
      bool fix_finger(unsigned long int rindex);

      /**
       * virtual functions, from Stabilizable.
       */
      virtual void stabilize_fast() throw (dht_exception)
      {
        _vnode->stabilize();
      };
      virtual void stabilize_slow() throw (dht_exception)
      {
        fix_finger();
      };
      virtual bool isStable() const;

      void print(std::ostream &out) const;

      /**
       * virtual node to which this table refers.
       */
      DHTVirtualNode* _vnode;

      /**
       * shared table of known peers on the network.
       */
      LocationTable* _lt;

      /**
       * \brief finger table starting point:
       * i.e. [k] = (n+2^{k-1}) mod 2^m, 1<k<m, with m=KEYNBITS,
       * and n is the virtual node's key.
       */
      DHTKey _starts[KEYNBITS];

      /**
       * \brief finger table: each location _locs[i] is the peer with key
       * that is the closest known successor to _starts[i].
       */
      Location* _locs[KEYNBITS];

      /**
       * Finger table staibility is maintained over two passes.
       */
      bool _stable_pass1;
      bool _stable_pass2;
      sp_mutex_t _stable_mutex; // mutex around stable indicator.
  };

} /* end of namespace */

#endif
