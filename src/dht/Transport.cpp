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

#include "Transport.h"
#include "rpc_server.h"
#include "rpc_client.h"
#include "errlog.h"
#include "DHTVirtualNode.h"
#include "l1_protob_wrapper.h"

using sp::errlog;

namespace dht
{
  Transport::Transport(const NetAddress& na) :
    rpc_server(na.getNetAddress(),na.getPort())
  {
    _client = new rpc_client();
  }

  Transport::~Transport()
  {
    delete _client;
    _client = NULL;
  }

  void Transport::do_rpc_call(const NetAddress &server_na,
                              const DHTKey &recipientKey,
                              const std::string &msg,
                              const bool &need_response,
                              std::string &response)
  {
    DHTVirtualNode* vnode = findVNode(recipientKey);

    if(vnode)
      {
        serve_response(msg,"<self>",response);
      }
    else
      {
        _client->do_rpc_call(server_na, msg, need_response, response);
      }
  }

  void Transport::rank_vnodes(std::vector<const DHTKey*> &vnode_keys_ord)
  {
    vnode_keys_ord.reserve(_vnodes.size());
    hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey>::const_iterator vit
    = _vnodes.begin();
    while(vit!=_vnodes.end())
      {
        vnode_keys_ord.push_back((*vit).first);
        ++vit;
      }
    std::stable_sort(vnode_keys_ord.begin(),vnode_keys_ord.end(),
                     DHTKey::lowdhtkey);
  }

  DHTVirtualNode* Transport::findClosestVNode(const DHTKey& key)
  {
    DHTKey contactKey = key;

    // rank local nodes.
    std::vector<const DHTKey*> vnode_keys_ord;
    rank_vnodes(vnode_keys_ord);

    // pick up the closest one.
    DHTKey *curr = NULL, *prev = NULL;
    std::vector<const DHTKey*>::const_iterator dit = vnode_keys_ord.begin();
    while(dit!=vnode_keys_ord.end())
      {
        curr = const_cast<DHTKey*>((*dit));
        if (contactKey > *curr)
          break;
        prev = curr;
        ++dit;
      }

    contactKey = prev ? *prev : *curr;

    return _vnodes.find(&contactKey)->second;
  }

  DHTVirtualNode* Transport::findVNode(const DHTKey& recipientKey) const
  {
    hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit;
    if ((hit = _vnodes.find(&recipientKey)) != _vnodes.end())
      return (*hit).second;
    else
      {
        return NULL;
      }
  }

  void Transport::serve_response(const std::string &msg,
                                 const std::string &addr,
                                 std::string &resp_msg)
  {
    try
      {
        serve_response_uncaught(msg, addr, resp_msg);
      }
    catch (dht_exception ex)
      {
        l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(ex.code());
        l1_protob_wrapper::serialize_to_string(l1r,resp_msg);
        delete l1r;
      }
  }

  void Transport::serve_response_uncaught(const std::string &msg,
                                          const std::string &addr,
                                          std::string &resp_msg) throw (dht_exception)
  {
    l1::l1_query l1q;
    try
      {
        l1_protob_wrapper::deserialize(msg,&l1q);
      }
    catch (dht_exception ex)
      {
        errlog::log_error(LOG_LEVEL_DHT, "l1_protob_rpc_server::serve_response exception %s", ex.what().c_str());
        throw dht_exception(DHT_ERR_MSG, "l1_protob_rpc_server::serve_response exception " + ex.what());
      }

    // read query.
    uint32_t layer_id;
    uint32_t fct_id;
    DHTKey recipient_key, sender_key, node_key;
    NetAddress recipient_na, sender_na;
    l1_protob_wrapper::read_l1_query(&l1q,layer_id,fct_id,recipient_key,
                                     recipient_na,sender_key,sender_na,node_key);

    // check on sender address, if specified, that sender is not lying.
    if (sender_na.empty())
      sender_na = NetAddress(addr,0);
    else
      {
        struct addrinfo hints, *result;
        memset((char *)&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_ADDRCONFIG; /* avoid service look-up */
        int ret = getaddrinfo(sender_na.getNetAddress().c_str(), NULL, &hints, &result);
        if (ret != 0)
          {
            errlog::log_error(LOG_LEVEL_ERROR,
                              "Can not resolve %s: %s", sender_na.getNetAddress().c_str(),
                              gai_strerror(ret));
            sender_na = NetAddress();
          }
        else
          {
            bool lying = true;
            struct addrinfo *rp;
            for (rp = result; rp != NULL; rp = rp->ai_next)
              {
                struct sockaddr_in *ai_addr = (struct sockaddr_in*)rp->ai_addr;
                uint32_t v4_addr = ai_addr->sin_addr.s_addr;
                std::string addr_from = NetAddress::unserialize_ip(v4_addr);

                if (addr_from == addr)
                  {
                    lying = false;
                    break;
                  }
              }
            if (lying)
              {
                /**
                			* Seems like the sender is lying on its address.
                			* We still serve it, but we do not use the potentially
                			* fake/lying IP.
                			*/
                sender_na = NetAddress();
              }
          }
      }

    // decides which response to give.
    int status = DHT_ERR_OK;
    DHTVirtualNode* vnode = findVNode(recipient_key);
    if (!vnode)
      throw dht_exception(DHT_ERR_UNKNOWN_PEER, "could not find virtual node for DHTKey " + recipient_key.to_rstring());
#if 0
    // check on the layer id. AAAA
    if (layer_id != 1)
      {
        int status = DHT_ERR_OK;
        lx_server_response(fct_id,recipient_key,recipient_na,sender_key,sender_na,node_key,
                           status,resp_msg,msg);
        return;
      }
#endif
    vnode->execute_callback(fct_id,sender_key,sender_na,node_key,status,resp_msg,msg);
  }

  DHTVirtualNode* Transport::find_closest_vnode(const DHTKey &key) const
  {
    DHTVirtualNode *vnode = NULL;
    std::vector<const DHTKey*>::const_iterator vit = _sorted_vnodes_vec.begin();
    DHTKey vidkey = *(*vit);
    if (key < *(*vit))
      vidkey = *(_sorted_vnodes_vec.back());
    else
      {
        while(vit!=_sorted_vnodes_vec.end()
              && *(*vit) < key)
          {
            vidkey = *(*vit);
            ++vit;
          }
      }
    vnode = findVNode(vidkey);
    return vnode;
  }

  void Transport::init_sorted_vnodes()
  {
    hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit
    = _vnodes.begin();
    while(hit!=_vnodes.end())
      {
        _sorted_vnodes_vec.push_back((*hit).second->getIdKeyPtr());
        ++hit;
      }
    std::sort(_sorted_vnodes_vec.begin(),_sorted_vnodes_vec.end(),DHTKey::lowdhtkey);
  }

} /* end of namespace. */
