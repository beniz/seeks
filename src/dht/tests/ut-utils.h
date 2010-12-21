/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010 Loic Dachary <loic@dachary.org>
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

#include "DHTVirtualNode.h"
#include "Transport.h"

#define PORT 1234

class TransportTest : public Transport
{
  public:

    TransportTest() : Transport(NetAddress("self", PORT)) {}

    virtual void do_rpc_call(const NetAddress &server_na,
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
          throw dht_exception(DHT_ERR_COM_TIMEOUT, "test server timeout");
        }
    }

    virtual void validate_sender_na(NetAddress& sender_na, const std::string& addr)
    {
      // relax sender verification for to allow for simulations
    }

};

class DHTTest : public testing::Test
{
  protected:
    DHTTest()
    {
    }

    virtual ~DHTTest()
    {
    }

    virtual void SetUp()
    {
      // init logging module.
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_ERROR | LOG_LEVEL_DHT | LOG_LEVEL_INFO);

      // craft a default configuration.
      dht_configuration::_dht_config = new dht_configuration("");
    }

    virtual void TearDown()
    {
      if (dht_configuration::_dht_config)
        delete dht_configuration::_dht_config;

      hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::iterator hit
      = _transport._vnodes.begin();
      while(hit!=_transport._vnodes.end())
        {
          delete (*hit).second;
          ++hit;
        }
    }

    DHTVirtualNode* bootstrap(DHTVirtualNode* vnode)
    {
      vnode->setSuccessor(vnode->getIdKey(), vnode->getNetAddress());
      vnode->_connected = true;
      return vnode;
    }

    DHTVirtualNode* new_vnode(DHTKey key = DHTKey::randomKey())
    {
      DHTVirtualNode* vnode = new DHTVirtualNode(&_transport, key);
      _transport._vnodes.insert(std::pair<const DHTKey*,DHTVirtualNode*>(new DHTKey(vnode->getIdKey()),
                                vnode));
      return vnode;
    }

    void connect(DHTVirtualNode* first, DHTVirtualNode* second, DHTVirtualNode* third)
    {
      first->setSuccessor(second->getIdKey(), second->getNetAddress());
      second->setPredecessor(first->getIdKey());

      second->setSuccessor(third->getIdKey(), third->getNetAddress());
      third->setPredecessor(second->getIdKey());

      second->_connected = true;
    }

    TransportTest _transport;
};

