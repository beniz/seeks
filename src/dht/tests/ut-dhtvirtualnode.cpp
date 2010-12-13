/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>

#include "DHTVirtualNode.h"
#include "Transport.h"
#include "dht_configuration.h"
#include "errlog.h"
#include "l1_protob_rpc_server.h"

#include <unistd.h>

using namespace dht;
using sp::errlog;

// dkey1 < dkey2 < ... < dkey9
const std::string dkey1 = "10000bf4851cb0b569eee7023b79db6aecfc7bed";
const std::string dkey2 = "20000bf4851cb0b569eee7023b79db6aecfc7bed";
const std::string dkey3 = "30000bf4851cb0b569eee7023b79db6aecfc7bed";
const std::string dkey4 = "40000bf4851cb0b569eee7023b79db6aecfc7bed";
const std::string dkey5 = "50000bf4851cb0b569eee7023b79db6aecfc7bed";
const std::string dkey6 = "60000bf4851cb0b569eee7023b79db6aecfc7bed";
const std::string dkey7 = "70000bf4851cb0b569eee7023b79db6aecfc7bed";
const std::string dkey8 = "80000bf4851cb0b569eee7023b79db6aecfc7bed";
const std::string dkey9 = "90000bf4851cb0b569eee7023b79db6aecfc7bed";

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
          _l1_server->serve_response(msg,"<self>",response);
        }
      else
        {
          throw dht_exception(DHT_ERR_COM_TIMEOUT, "test server timeout");
        }
    }
};

class DHTVirtualNodeTest : public testing::Test
{
  protected:
    DHTVirtualNodeTest()
      : _transport()
    {
    }

    virtual ~DHTVirtualNodeTest()
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

    DHTVirtualNode* new_vnode(DHTKey key = DHTKey::randomKey())
    {
      DHTVirtualNode* vnode = new DHTVirtualNode(&_transport, key);
      _transport._vnodes.insert(std::pair<const DHTKey*,DHTVirtualNode*>(new DHTKey(vnode->getIdKey()),
                                vnode));
      return vnode;
    }

    DHTVirtualNode* bootstrap(DHTVirtualNode* vnode)
    {
      vnode->setSuccessor(vnode->getIdKey(), vnode->getNetAddress());
      vnode->_connected = true;
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

TEST_F(DHTVirtualNodeTest, init_vnode)
{
  DHTVirtualNode* vnode = new_vnode();
  ASSERT_NE((Location*)NULL, vnode->findLocation(vnode->getIdKey()));
  ASSERT_NE((FingerTable*)NULL, vnode->getFingerTable());
  ASSERT_NE((LocationTable*)NULL, vnode->getLocationTable());
}

TEST_F(DHTVirtualNodeTest, join)
{
  DHTVirtualNode* vnode = bootstrap(new_vnode());
  DHTVirtualNode* joining = new_vnode();

  int status = DHT_ERR_UNKNOWN;
  ASSERT_FALSE(joining->_connected);
  ASSERT_EQ((Location*)NULL, joining->findLocation(vnode->getIdKey()));
  EXPECT_TRUE(joining->_successors.empty());

  // try to join a node that timesout
  joining->join(DHTKey(), NetAddress(), joining->getIdKey(), status);
  ASSERT_FALSE(joining->_connected);
  ASSERT_EQ(DHT_ERR_COM_TIMEOUT, status);

  // successfull join
  joining->join(vnode->getIdKey(), vnode->getNetAddress(), joining->getIdKey(), status);

  ASSERT_EQ(DHT_ERR_OK, status);
  ASSERT_TRUE(joining->_connected);
  Location* location = joining->findLocation(vnode->getIdKey());
  ASSERT_NE((Location*)NULL, location);
  EXPECT_FALSE(joining->_successors.empty());
  EXPECT_EQ(location->getNetAddress(), vnode->getNetAddress());
  EXPECT_EQ(*joining->getSuccessorPtr(), vnode->getIdKey());
  EXPECT_EQ(*vnode->getSuccessorPtr(), vnode->getIdKey());
}

TEST_F(DHTVirtualNodeTest, notify)
{
  DHTVirtualNode* vnode = bootstrap(new_vnode());
  DHTVirtualNode* joining = new_vnode();

  int status = DHT_ERR_UNKNOWN;
  joining->join(vnode->getIdKey(), vnode->getNetAddress(), joining->getIdKey(), status);
  ASSERT_EQ(DHT_ERR_OK, status);
  ASSERT_EQ(0, vnode->getPredecessorS().count());
  ASSERT_EQ(DHT_ERR_OK, vnode->notify(joining->getIdKey(), joining->getNetAddress()));
  ASSERT_EQ(joining->getIdKey(), vnode->getPredecessorS());
}

TEST_F(DHTVirtualNodeTest, find_successor)
{
  DHTVirtualNode* vnode9 = bootstrap(new_vnode(DHTKey::from_rstring(dkey9)));
  {
    DHTKey dkres;
    NetAddress na;
    EXPECT_EQ(DHT_ERR_OK, vnode9->find_successor(vnode9->getIdKey(), dkres, na));
    EXPECT_EQ(vnode9->getIdKey(), dkres);
    EXPECT_EQ(PORT, na.getPort());
  }
  DHTVirtualNode* vnode3 = new_vnode(DHTKey::from_rstring(dkey3));
  connect(vnode9, vnode3, vnode9);
  DHTVirtualNode* vnode2 = new_vnode(DHTKey::from_rstring(dkey2));
  connect(vnode9, vnode2, vnode3);
  // introduce a dead node
  vnode3->setSuccessor(DHTKey::from_rstring(dkey6), NetAddress());
  vnode9->setPredecessor(DHTKey::from_rstring(dkey6));
  {
    DHTKey dkres;
    NetAddress na;
    EXPECT_EQ(DHT_ERR_COM_TIMEOUT, vnode3->find_successor(DHTKey::from_rstring(dkey2), dkres, na));
  }
}

TEST_F(DHTVirtualNodeTest, find_predecessor)
{
  DHTVirtualNode* vnode9 = bootstrap(new_vnode(DHTKey::from_rstring(dkey9)));
  {
    DHTKey dkres;
    NetAddress na;
    EXPECT_EQ(DHT_ERR_OK, vnode9->find_predecessor(vnode9->getIdKey(), dkres, na));
    EXPECT_EQ(vnode9->getIdKey(), dkres);
    EXPECT_EQ(PORT, na.getPort());
  }
  DHTVirtualNode* vnode3 = new_vnode(DHTKey::from_rstring(dkey3));
  connect(vnode9, vnode3, vnode9);
  {
    DHTKey dkres;
    NetAddress na;
    EXPECT_EQ(DHT_ERR_OK, vnode9->find_predecessor(vnode9->getIdKey(), dkres, na));
    EXPECT_EQ(vnode3->getIdKey(), dkres);
    EXPECT_EQ(PORT, na.getPort());
  }
  {
    DHTKey dkres;
    NetAddress na;
    EXPECT_EQ(DHT_ERR_OK, vnode9->find_predecessor(DHTKey::from_rstring(dkey4), dkres, na));
    EXPECT_EQ(vnode3->getIdKey(), dkres);
    EXPECT_EQ(PORT, na.getPort());
  }
  {
    DHTKey dkres;
    NetAddress na;
    EXPECT_EQ(DHT_ERR_OK, vnode3->find_predecessor(vnode9->getIdKey(), dkres, na));
    EXPECT_EQ(vnode3->getIdKey(), dkres);
    EXPECT_EQ(PORT, na.getPort());
  }
  DHTVirtualNode* vnode2 = new_vnode(DHTKey::from_rstring(dkey2));
  connect(vnode9, vnode2, vnode3);
  {
    DHTKey dkres;
    NetAddress na;
    EXPECT_EQ(DHT_ERR_OK, vnode9->find_predecessor(vnode3->getIdKey(), dkres, na));
    EXPECT_EQ(vnode2->getIdKey(), dkres);
    EXPECT_EQ(PORT, na.getPort());
  }
  DHTVirtualNode* vnode4 = new_vnode(DHTKey::from_rstring(dkey4));
  connect(vnode3, vnode4, vnode9);
  // introduce a dead node
  vnode4->setSuccessor(DHTKey::from_rstring(dkey6), NetAddress());
  vnode9->setPredecessor(DHTKey::from_rstring(dkey6));
  {
    DHTKey dkres;
    NetAddress na;
    // immediately returns because node6 timesout (RPC_getSuccessor fails)
    EXPECT_EQ(DHT_ERR_COM_TIMEOUT, vnode4->find_predecessor(vnode2->getIdKey(), dkres, na));
  }
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
