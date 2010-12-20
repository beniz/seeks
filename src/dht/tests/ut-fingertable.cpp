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

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>

#include "DHTVirtualNode.h"
#include "Transport.h"
#include "FingerTable.h"
#include "dht_configuration.h"
#include "errlog.h"

#include <unistd.h>

using namespace dht;
using sp::errlog;

// dkey0 < dkey2 < ... < dkey8
const std::string dkey0 = "0000000000000000000000000000000000000000";
const std::string dkey1 = "1000000000000000000000000000000000000000";
const std::string dkey2 = "2000000000000000000000000000000000000000";
const std::string dkey4 = "4000000000000000000000000000000000000000";
const std::string dkey8 = "8000000000000000000000000000000000000000";

#define PORT 1234

class FingerTableTest : public testing::Test
{
  protected:
    FingerTableTest()
      : _transport(NetAddress("self", PORT))
    {
    }

    virtual ~FingerTableTest()
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

    Transport _transport;
};

TEST_F(FingerTableTest, constructor)
{
  DHTVirtualNode* vnode = new_vnode();
  ASSERT_FALSE(vnode->getFingerTable()->isStable());
}

TEST_F(FingerTableTest, fix_finger)
{
  DHTKey key1(1);
  DHTVirtualNode* vnode1 = bootstrap(new_vnode(key1));
  DHTKey key3 = vnode1->getFingerTable()->_starts[KEYNBITS-1].successor(2);
  DHTVirtualNode* vnode3 = new_vnode(key3);
  connect(vnode1, vnode3, vnode1);

  // last slot of the finger table is set
  EXPECT_EQ((Location*)NULL, vnode1->getFingerTable()->_locs[KEYNBITS-1]);
  EXPECT_NE((Location*)NULL, vnode1->getLocationTable()->findLocation(key3));
  EXPECT_EQ((Location*)NULL, vnode1->getFingerTable()->_locs[KEYNBITS-2]);

  vnode1->getFingerTable()->fix_finger(KEYNBITS-1);

  Location* location = vnode1->getFingerTable()->_locs[KEYNBITS-1];
  EXPECT_NE((Location*)NULL, location);
  EXPECT_EQ(location, vnode1->getLocationTable()->findLocation(key3));
  EXPECT_EQ(key3, *location);
  EXPECT_EQ((Location*)NULL, vnode1->getFingerTable()->_locs[KEYNBITS-2]);

  // last slot of the finger table is replaced
  DHTKey key2 = vnode1->getFingerTable()->_starts[KEYNBITS-1].successor(1);
  DHTVirtualNode* vnode2 = new_vnode(key2);
  connect(vnode1, vnode2, vnode3);

  vnode1->getFingerTable()->fix_finger(KEYNBITS-1);

  EXPECT_NE((Location*)NULL, vnode1->getLocationTable()->findLocation(key2));
  location = vnode1->getFingerTable()->_locs[KEYNBITS-1];
  EXPECT_NE((Location*)NULL, location);
  EXPECT_EQ(location, vnode1->getLocationTable()->findLocation(key2));
  EXPECT_EQ(key2, *location);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
