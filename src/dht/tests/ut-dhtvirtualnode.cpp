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

const std::string dkey1 = "9ae34be97ae732e5319ce2992cb489eea99f18ed";
const std::string dkey2 = "12b10bf4851cb0b569eee7023b79db6aecfc7bed";
/*d17b8d98350865e759426ade8899e77cc970f11d
c8eb560e300210de9be0ecc84c7789c24043bc9d
275d59a3083ac6fa87b40130de276620a621879d
8b3715ee2b5aa49854dbd4c09a5883b93afa135d
faad1ea5e0e5c5ab54bf5225279ce509f3cb5edd
7da78291e4d8fe6a8fea412fe45701a0b137ac3d
6e67359c3d66fbb024ba788e50ebd5770ad45b3d */

class TransportTest : public Transport
{
  public:

    TransportTest() : Transport(NetAddress("self", 1234)) {}

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

    DHTVirtualNode* new_vnode()
    {
      DHTVirtualNode* vnode = new DHTVirtualNode(&_transport);
      _transport._vnodes.insert(std::pair<const DHTKey*,DHTVirtualNode*>(new DHTKey(vnode->getIdKey()),
                                vnode));
      return vnode;
    }

    DHTVirtualNode* new_bootstraped_vnode()
    {
      DHTVirtualNode* vnode = new_vnode();
      vnode->setSuccessor(vnode->getIdKey(), vnode->getNetAddress());
      vnode->_connected = true;
      return vnode;
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
  DHTVirtualNode* vnode = new_bootstraped_vnode();
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
  DHTVirtualNode* vnode = new_bootstraped_vnode();
  DHTVirtualNode* joining = new_vnode();

  int status = DHT_ERR_UNKNOWN;
  joining->join(vnode->getIdKey(), vnode->getNetAddress(), joining->getIdKey(), status);
  ASSERT_EQ(DHT_ERR_OK, status);
  ASSERT_EQ(0, vnode->getPredecessorS().count());
  ASSERT_EQ(DHT_ERR_OK, vnode->notify(joining->getIdKey(), joining->getNetAddress()));
  ASSERT_EQ(joining->getIdKey(), vnode->getPredecessorS());
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
