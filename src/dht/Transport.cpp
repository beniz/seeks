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

namespace dht
{
  Transport::Transport(const std::string &hostname, const short &port)
  {
    _l1_server = new l1_protob_rpc_server(hostname,port,this);
    _l1_client = new l1_protob_rpc_client();
  }

  Transport::~Transport()
  {
    delete _l1_server;
    _l1_server = NULL;
    delete _l1_client;
    _l1_client = NULL;
  }

  void Transport::run()
  {
    _l1_server->run();
  }

  void Transport::run_thread()
  {
    _l1_server->run_thread();
  }

  void Transport::stop_thread()
  {
    _l1_server->stop_thread();
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
        _l1_server->serve_response(msg,"<self>",response);
      }
    else
      {
        _l1_client->do_rpc_call(server_na, msg, need_response, response);
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

  DHTVirtualNode* Transport::findVNode(const DHTKey& recipientKey)
  {
    {
      hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit
      = _vnodes.begin();
      if(hit==_vnodes.end())
        return NULL;
    }

    DHTKey contactKey = recipientKey;

    if (contactKey.count() == 0)
      {
        // rank local nodes.
        std::vector<const DHTKey*> vnode_keys_ord;
        rank_vnodes(vnode_keys_ord);

        // pick up the closest one.
        DHTKey *curr = NULL, *prev = NULL;
        std::vector<const DHTKey*>::const_iterator dit = vnode_keys_ord.begin();
        while(dit!=vnode_keys_ord.end())
          {
            curr = const_cast<DHTKey*>((*dit));
            if (senderKey > *curr)
              break;
            prev = curr;
            ++dit;
          }

        contactKey = prev ? *prev : *curr;
      }

    hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit;
    if ((hit = _vnodes.find(&contactKey)) != _vnodes.end())
      return (*hit).second;
    else
      {
        return NULL;
      }
  }

  DHTVirtualNode* Transport::findVNode(const DHTKey& dk) const
  {

  }

  void Transport::getSuccessor_cb(const DHTKey& recipientKey,
                                  DHTKey& dkres, NetAddress& na,
                                  int& status) throw (dht_exception)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node and deal with possible errors.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        dkres = DHTKey();
        na = NetAddress();
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }
    vnode->getSuccessor_cb(dkres, na, status);
  }

  void Transport::getPredecessor_cb(const DHTKey& recipientKey,
                                    DHTKey& dkres, NetAddress& na,
                                    int& status)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node and deal with possible errors.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        dkres = DHTKey();
        na = NetAddress();
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }
    vnode->getPredecessor_cb(dkres, na, status);
  }

  void Transport::notify_cb(const DHTKey& recipientKey,
                            const DHTKey& senderKey,
                            const NetAddress& senderAddress,
                            int& status)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    vnode->notify_cb(senderKey, senderAddress, status);

  }

  void Transport::getSuccList_cb(const DHTKey &recipientKey,
                                 std::list<DHTKey> &dkres_list,
                                 std::list<NetAddress> &na_list,
                                 int &status)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node and deal with possible errors.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        dkres_list = std::list<DHTKey>();
        na_list = std::list<NetAddress>();
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    vnode->getSuccList_cb(dkres_list, na_list, status);
  }

  void Transport::findClosestPredecessor_cb(const DHTKey& recipientKey,
      const DHTKey& nodeKey,
      DHTKey& dkres, NetAddress& na,
      DHTKey& dkres_succ, NetAddress &dkres_succ_na,
      int& status)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        dkres = DHTKey();
        na = NetAddress();
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    vnode->findClosestPredecessor_cb(nodeKey, dkres, na, dkres_succ, dkres_succ_na, status);
  }

  void Transport::joinGetSucc_cb(const DHTKey &recipientKey,
                                 const DHTKey &senderKey,
                                 DHTKey &dkres, NetAddress &na,
                                 int &status)
  {
    status = DHT_ERR_OK;
    vnode = findVNode(recipientKey);
    if (!vnode)
      {
        dkres = DHTKey();
        na = NetAddress();
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    vnode->joinGetSucc_cb(senderKey, dkres, na, status);
  }

  void Transport::ping_cb(const DHTKey& recipientKey,
                          int& status)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    /**
     * ping this virtual node.
     */
    vnode->ping_cb(status);
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
