/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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

#ifndef SUCCLIST_H
#define SUCCLIST_H

#include "DHTKey.h"
#include "NetAddress.h"
#include "Stabilizer.h"
#include "Location.h"
#include "mutexes.h"
#include "dht_err.h"

#include <list>

namespace dht
{
  class DHTVirtualNode;

  /**
   * \brief list of successors, makes the ring more robust to node failures.
   *        The first element of the list is the successor itself, that is replicated
   *        in the DHTVirtualNode class.
   */
  class SuccList : public Stabilizable, public std::list<Location>
  {
    public:
      /**
       * \brief constructor with backpointer to the virtual node.
       */
      SuccList(DHTVirtualNode *vnode);

      /**
       * \brief destructor.
       */
      ~SuccList();

      Location& getSuccessor()
      {
        return front();
      }

      const Location& getSuccessor() const
      {
        return front();
      }

      void setSuccessor(const Location& successor)
      {
        front() = successor;
      }

      void update_successors();

      void merge_succ_list(std::list<DHTKey> &dkres_list, std::list<NetAddress> &na_list);

      void merge_list(std::list<Location>& other);

      void check();

      bool has_key(const DHTKey &key) const;

      void removeKey(const DHTKey &key);

      void findClosestPredecessor(const DHTKey &nodeKey,
                                  DHTKey &dkres, NetAddress &na,
                                  DHTKey &dkres_succ, NetAddress &dkres_succ_na,
                                  int &status);

      /**
       * virtual functions, from Stabilizable.
       */
      virtual void stabilize_fast() {};

      virtual void stabilize_slow()
      {
        update_successors();
      };

      virtual bool isStable() const
      {
        return _stable;
      }

    public:
      DHTVirtualNode *_vnode;

      static short _max_list_size;

    private:
      bool _stable;
      sp_mutex_t _stable_mutex; // mutex around stable indicator variables.
  };

} /* end of namespace. */

#endif
