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

#ifndef DHTVIRTUALNODE_H
#define DHTVIRTUALNODE_H

#include "dht_err.h"
#include "DHTKey.h"
#include "NetAddress.h"
#include "LocationTable.h"
#include "SuccList.h"
#include "mutexes.h"
#include "dht_exception.h"
#include "l1_protob_wrapper.h"

#if (__GNUC__ >= 3)
#include <ext/slist>
#else
#include <slist>
#endif

#if (__GNUC__ >= 3)
using __gnu_cxx::slist;
#else
using std::slist;
#endif

namespace dht
{

  class Location;
  class FingerTable;
  class Transport;

  /**
   * \brief Virtual node of the DHT.
   * \class DHTVirtualNode
   */
  class DHTVirtualNode
  {
    public:
      /**
       * \brief create a new 'virtual' node on the local host.
       * - The node's id is a randomly generated key.
       * - successor and predecessor are NULL.
       */
      DHTVirtualNode(Transport *transport);

      /**
       * \brief constructor based on persistent data, loaded at startup
       *        in Transport.
       */
      DHTVirtualNode(Transport *transport, const DHTKey &idkey, LocationTable *lt);

      /**
       * \brief destructor.
       */
      ~DHTVirtualNode();

      /**
       * Initializes structures and mutexes.
       */
      void init_vnode();

      /**-- functions used in RPC callbacks. --**/
      /**
       * \brief notifies this virtual node that the argument key/address peer
       *        thinks it is its predecessor.
       */
      dht_err notify(const DHTKey& senderKey, const NetAddress& senderAddress);

      /**
       * \brief find the closest predecessor of nodeKey in the finger table.
       * @param nodeKey identification key which closest predecessor is asked for.
       * @param dkres closest predecessor result variable,
       * @param na closest predecessor net address variable,
       * @param dkres_succ successor to dkres.
       * @param status result status.
       */
      void findClosestPredecessor(const DHTKey& nodeKey,
                                  DHTKey& dkres, NetAddress& na,
                                  DHTKey& dkres_succ, NetAddress &dkres_succ_na,
                                  int& status);
      /**
       * \brief this virtual node is being pinged from the outside world.
       */
      void ping();

      /**---------------------------------------**/

      /**-- functions using RPCs. --**/
      void join(const DHTKey& dk_bootstrap,
                const NetAddress &dk_bootstrap_na,
                const DHTKey& senderKey,
                int& status) throw (dht_exception);

      dht_err find_successor(const DHTKey& nodeKey,
                             DHTKey& dkres, NetAddress& na) throw (dht_exception);

      dht_err find_predecessor(const DHTKey& nodeKey,
                               DHTKey& dkres, NetAddress& na) throw (dht_exception);

      bool is_dead(const DHTKey &recipientKey, const NetAddress &na,
                   int &status) throw (dht_exception);

      dht_err leave() throw (dht_exception);

      /**---------------------------**/

      /**
       * accessors.
       */
      const DHTKey& getIdKey() const
      {
        return _idkey;
      }
      const DHTKey* getIdKeyPtr() const
      {
        return &_idkey;
      }
      Transport* getTransport() const
      {
        return _transport;
      }
      DHTKey *getSuccessorPtr() const
      {
        return _successor;
      }
      DHTKey getSuccessorS();
      void setSuccessor(const DHTKey &dk);
      void setSuccessor(const DHTKey& dk, const NetAddress& na);
      DHTKey* getPredecessorPtr() const
      {
        return _predecessor;
      }
      DHTKey getPredecessorS();
      void setPredecessor(const DHTKey &dk);
      void setPredecessor(const DHTKey& dk, const NetAddress& na);
      void clearSuccsList()
      {
        _successors.clear();
      };
      Location* getLocation() const
      {
        return _loc;
      }
      Transport* getTransport()
      {
        return _transport;
      }
      FingerTable* getFingerTable()
      {
        return _fgt;
      }
      std::list<const DHTKey*>::const_iterator getFirstSuccessor() const
      {
        return _successors.begin();
      };
      std::list<const DHTKey*>::const_iterator endSuccessor() const
      {
        return _successors.end();
      };

      /* location finding. */
      LocationTable* getLocationTable() const;
      Location* findLocation(const DHTKey& dk) const;
      void addToLocationTable(const DHTKey& dk, const NetAddress& na,
                              Location *&loc) const;
      void removeLocation(Location *loc);
      NetAddress getNetAddress() const;
      Location* addOrFindToLocationTable(const DHTKey& key, const NetAddress& na);
      bool isPredecessorEqual(const DHTKey &key);
      bool not_used_location(Location *loc);

      /* update of successor list. */
      void update_successor_list_head();

#if 0 // not sure what is the purpose 
      /**
       * \brief estimates the number of nodes and virtual nodes on the circle,
       *        from this virtual node point of view.
       */
      void estimate_nodes();
      void estimate_nodes(unsigned long &nnodes,
                          unsigned long &nnvnodes);

#endif
      /* stable flag. */
      /**
       * \brief whether the list of successors is stabilized.
       */
      bool isSuccStable() const;

      /**
       * \brief whether the list of successors and the finger table are stabilized.
       */
      bool isStable() const;

      /* replication. */
      /**
       * replication of content.
       * XXX: replication is not implemented at this level. Inherited classes
       * from DHTVirtualNode can define their response to replication calls.
       */
      virtual dht_err replication_host_keys(const DHTKey &start_key)
      {
        return DHT_ERR_OK;
      };

      virtual void replication_move_keys_backward(const DHTKey &start_key,
          const DHTKey &end_key,
          const NetAddress &senderAddress) { };

      virtual void replication_move_keys_forward(const DHTKey &end_key) { };

      virtual void replication_trickle_forward(const DHTKey &start_key,
          const short &start_replication_radius) { };

      /**----------------------------**/
      /**
       * \brief getSuccessor local callback.
       */
      void getSuccessor_cb(
        DHTKey& dkres, NetAddress& na,
        int& status) throw (dht_exception);

      void getPredecessor_cb(
        DHTKey& dkres, NetAddress& na,
        int& status);

      /**
       * \brief notify callback.
       */
      void notify_cb(
        const DHTKey& senderKey,
        const NetAddress& senderAddress,
        int& status);

      /**
       * \brief getSuccList callback.
       */
      void getSuccList_cb(
        std::list<DHTKey> &dkres_list,
        std::list<NetAddress> &dkres_na,
        int &status);

      /**
       * \brief findClosestPredecessor callback.
       */
      void findClosestPredecessor_cb(
        const DHTKey& nodeKey,
        DHTKey& dkres, NetAddress& na,
        DHTKey& dkres_succ, NetAddress &dkres_succ_na,
        int& status);

      /**
       * \brief joinGetSucc callback.
       */
      void joinGetSucc_cb(
        const DHTKey &senderKey,
        DHTKey& dkres, NetAddress& na,
        int& status);

      /**
       * \brief ping callback.
       */
      void ping_cb(
        int& status);

      // call and response.
      void RPC_call(const uint32_t &fct_id,
                    const DHTKey &recipientKey,
                    const NetAddress& recipient,
                    const DHTKey &nodeKey,
                    l1::l1_response *l1r) throw (dht_exception);

      void RPC_call(const uint32_t &fct_id,
                    const DHTKey &recipientKey,
                    const NetAddress &recipient,
                    l1::l1_response *l1r) throw (dht_exception);

      void RPC_call(const uint32_t &fct_id,
                    const DHTKey &recipientKey,
                    const NetAddress& recipient,
                    const DHTKey &senderKey,
                    const NetAddress& senderAddress,
                    l1::l1_response *l1r) throw (dht_exception);

      void RPC_call(l1::l1_query *&l1q,
                    const DHTKey &recipientKey,
                    const NetAddress &recipient,
                    l1::l1_response *l1r) throw (dht_exception);

      /*- l1 interface. -*/
      virtual void RPC_getSuccessor(const DHTKey& recipientKey,
                                    const NetAddress& recipient,
                                    DHTKey& dkres, NetAddress& na,
                                    int& status) throw (dht_exception);

      virtual void RPC_getPredecessor(const DHTKey& recipientKey,
                                      const NetAddress& recipient,
                                      DHTKey& dkres, NetAddress& na,
                                      int& status) throw (dht_exception);

      virtual void RPC_notify(const DHTKey& recipientKey,
                              const NetAddress& recipient,
                              const DHTKey& senderKey,
                              const NetAddress& senderAddress,
                              int& status) throw (dht_exception);

      virtual void RPC_getSuccList(const DHTKey &recipientKey,
                                   const NetAddress &recipient,
                                   std::list<DHTKey> &dkres_list,
                                   std::list<NetAddress> &na_list,
                                   int &status) throw (dht_exception);

      virtual void RPC_findClosestPredecessor(const DHTKey& recipientKey,
                                              const NetAddress& recipient,
                                              const DHTKey& nodeKey,
                                              DHTKey& dkres, NetAddress& na,
                                              DHTKey& dkres_succ,
                                              NetAddress &dkres_succ_na,
                                              int& status) throw (dht_exception);

      virtual void RPC_joinGetSucc(const DHTKey& recipientKey,
                                   const NetAddress& recipient,
                                   const DHTKey &senderKey,
                                   DHTKey& dkres, NetAddress& na,
                                   int& status) throw (dht_exception);

      virtual void RPC_ping(const DHTKey& recipientKey,
                            const NetAddress& recipient,
                            int& status) throw (dht_exception);
    public:
      /**
       * location table.
       */
      LocationTable *_lt;

    protected:
      /**
       * virtual node's id key.
       */
      DHTKey _idkey;

      Transport* _transport;

      /**
       * closest known successor on the circle.
       */
      DHTKey* _successor;

      /**
       * closest known predecessor on the circle.
       */
      DHTKey* _predecessor;

    public:
      /**
       * Sorted list of successors.
       */
      SuccList _successors;

    protected:
      /**
       * finger table.
       */
      FingerTable* _fgt;

      /**
       * this virtual node local location.
       */
      Location* _loc;

      /**
       * predecessor and successor mutexes.
       */
      sp_mutex_t _pred_mutex;
      sp_mutex_t _succ_mutex;

    public:
      /**
       * estimates of the number of nodes and virtual nodes on the circle.
       */
      unsigned long _nnodes;
      unsigned long _nnvnodes;

      /**
       * whether this node is connected to the ring.
       * By connected it is meant that the virtual node has at least a working
       * successor.
       */
      bool _connected;
  };

} /* end of namespace. */

#endif
