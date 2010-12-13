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

#include "DHTKey.h"
#include "NetAddress.h"
#include "rpc_server.h"
#include "dht_exception.h"

namespace dht
{
  class DHTVirtualNode;
  class rpc_client;

  class Transport : public rpc_server
  {
    public:
      Transport(const NetAddress& na);

      virtual ~Transport();

      virtual void serve_response(const std::string &msg,
                                  const std::string &addr,
                                  std::string &resp_msg);

      void serve_response_uncaught(const std::string &msg,
                                   const std::string &addr,
                                   std::string &resp_msg) throw (dht_exception);

      virtual void do_rpc_call(const NetAddress &server_na,
                               const DHTKey &recipientKey,
                               const std::string &msg,
                               const bool &need_response,
                               std::string &response);

      NetAddress getNetAddress() const
      {
        return _na;
      }

      void rank_vnodes(std::vector<const DHTKey*> &vnode_keys_ord);


      /**
       * \brief fill up and sort sorted set of virtual nodes.
       */
      void init_sorted_vnodes();

      /**
       * \brief finds closest virtual node to the argument key.
       */
      DHTVirtualNode* find_closest_vnode(const DHTKey &key) const;

      DHTVirtualNode* findClosestVNode(const DHTKey& key);

      DHTVirtualNode* findVNode(const DHTKey& recipientKey) const;

      rpc_client *_client;

      /**
       * Sortable list of virtual nodes.
       */
      std::vector<const DHTKey*> _sorted_vnodes_vec;

      /**
       * hash map of DHT virtual nodes.
       */
      hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey> _vnodes;
  };

} /* end of namespace. */

#endif // TRANSPORT_H
