/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
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

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "NetAddress.h"
#include "dht_exception.h"
#include "l1_protob_rpc_server.h"
#include "l1_protob_rpc_client.h"

namespace dht
{
  class Transport
  {
    public:
      Transport(const std::string &hostname, const short &port);

      virtual ~Transport();

      void run();

      void run_thread() throw (dht_exception);

      void stop_thread();

      void do_rpc_call(const NetAddress &server_na,
                       const DHTKey &recipientKey,
                       const std::string &msg,
                       const bool &need_response,
                       std::string &response) const throw (dht_exception);

    private:
      void rank_vnodes(std::vector<const DHTKey*> &vnode_keys_ord);

    public:

      /**
       * \brief fill up and sort sorted set of virtual nodes.
       */
      void init_sorted_vnodes();

      /**
       * \brief finds closest virtual node to the argument key.
       */
      DHTVirtualNode* find_closest_vnode(const DHTKey &key) const;

      DHTVirtualNode* findVNode(const DHTKey& dk) const;

      void getSuccessor_cb(const DHTKey& recipientKey,
                           DHTKey& dkres, NetAddress& na,
                           int& status) throw (dht_exception);

      void getPredecessor_cb(const DHTKey& recipientKey,
                             DHTKey& dkres, NetAddress& na,
                             int& status);

      void notify_cb(const DHTKey& recipientKey,
                     const DHTKey& senderKey,
                     const NetAddress& senderAddress,
                     int& status);

      void getSuccList_cb(const DHTKey &recipientKey,
                          std::list<DHTKey> &dkres_list,
                          std::list<NetAddress> &dkres_na,
                          int &status);

      void findClosestPredecessor_cb(const DHTKey& recipientKey,
                                     const DHTKey& nodeKey,
                                     DHTKey& dkres, NetAddress& na,
                                     DHTKey& dkres_succ, NetAddress &dkres_succ_na,
                                     int& status);

      void joinGetSucc_cb(const DHTKey &recipientKey,
                          const DHTKey &senderKey,
                          DHTKey& dkres, NetAddress& na,
                          int& status);

      void ping_cb(const DHTKey& recipientKey,
                   int& status);

      l1_protob_rpc_server *_l1_server;

      l1_protob_rpc_client *_l1_client;

      /**
       * hash map of DHT virtual nodes.
       */
      hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey> _vnodes;
  };

} /* end of namespace. */

#endif // TRANSPORT_H
