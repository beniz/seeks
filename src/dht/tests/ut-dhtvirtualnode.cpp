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

/*- protocol tests. -*/
class DHTVirtualNodeTest : public testing::Test
{
  protected:
    DHTVirtualNodeTest()
      : _transport(NetAddress())
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
      vnode->setPredecessor(vnode->getIdKey(), vnode->getNetAddress());
      vnode->_connected = true;
      return vnode;
    }

#if 0
    void start_dnode(DHTNode *&dnode, NetAddress *&na_dnode,
                     const std::string &net_addr,
                     const unsigned &net_port,
                     const std::string &dkey)
    {
      // create dummy node.
      na_dnode = new NetAddress(net_addr,net_port);
      dnode = new DHTNode(na_dnode->getNetAddress().c_str(),na_dnode->getPort(),
                          false,"");

      // create running stabilizer.
      dnode->_stabilizer = new Stabilizer(true);

      // craft virtual node keys.
      DHTKey dhtkey = DHTKey::from_rstring(dkey);
      _vnode_ids.push_back(&dhtkey);
      _vnode_lttables.push_back(new LocationTable());
      dnode->load_vnodes_and_tables(_vnode_ids,_vnode_lttables);
      _vnode_ids.clear();

      // init server.
      dnode->init_sorted_vnodes();
      dnode->init_server();
      dnode->_l1_server->run_thread();

      while(true)
        {
          dht_err status;
          dnode->_l1_client->RPC_ping(DHTKey(),*na_dnode,status);
          if (status != DHT_ERR_COM_TIMEOUT)
            break;
        }
    }
#endif

    Transport _transport;
};

TEST_F(DHTVirtualNodeTest, join)
{
  DHTVirtualNode* vnode = new_bootstraped_vnode();
  DHTVirtualNode* joining = new_vnode();

  int status = DHT_ERR_UNKNOWN;
  ASSERT_FALSE(joining->_connected);
  joining->join(vnode->getIdKey(), vnode->getNetAddress(), joining->getIdKey(), status);
  ASSERT_EQ(DHT_ERR_OK, status);
  ASSERT_TRUE(joining->_connected);
  EXPECT_EQ(*joining->getSuccessorPtr(), vnode->getIdKey());
  EXPECT_EQ(*vnode->getSuccessorPtr(), vnode->getIdKey());

#if 0
  DHTVirtualNode
  // create dnode2.
  DHTVirtualNodeTest::start_dnode(_dnode2,_na_dnode2,
                                  _net_addr2,_net_port2,
                                  dkey2);
  DHTKey dhtkey1 = DHTKey::from_rstring(dkey1);
  DHTKey dhtkey2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode *vnode2 = _dnode2->findVNode(dhtkey2);
  dht_err status = _dnode2->join(*_na_dnode1,dhtkey1);
  ASSERT_EQ(DHT_ERR_OK,status);
  ASSERT_TRUE(vnode2->getSuccessorS().count() > 0);
  ASSERT_EQ(dkey1,vnode2->getSuccessorS().to_rstring());

  // voluntary leave.
  _dnode2->leave();
#endif
}
#if 0
//TODO: test predecessor & successor.
TEST_F(DHTVirtualNodeTest, predecessor_successor_stable)
{
  DHTVirtualNodeTest::start_dnode(_dnode2,_na_dnode2,
                                  _net_addr2,_net_port2,
                                  dkey2);
  DHTKey dhtkey1 = DHTKey::from_rstring(dkey1);
  DHTKey dhtkey2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode *vnode1 = _dnode1->findVNode(dhtkey1);
  DHTVirtualNode *vnode2 = _dnode2->findVNode(dhtkey2);
  dht_err status = _dnode2->join(*_na_dnode1,dhtkey1);
  ASSERT_EQ(DHT_ERR_OK,status);
  ASSERT_TRUE(vnode2->getSuccessorS().count() > 0);
  ASSERT_EQ(dkey1,vnode2->getSuccessorS().to_rstring());

  /* std::cerr << "dnode1 slow clicks: " << _dnode1->_stabilizer->_slow_clicks << std::endl;
     std::cerr << "dnode2 slow clicks: " << _dnode2->_stabilizer->_slow_clicks << std::endl; */

  // testing stability of succlist and successors.
  uint64_t max_slow_clicks = 3; // at least two passes for stability.
  short event_timecheck = dht_configuration::_dht_config->_event_timecheck;
  while(_dnode1->_stabilizer->_slow_clicks < max_slow_clicks)
    {
      if (!_dnode1->isSuccStable())
        sleep(event_timecheck);
      else break;
    }
  ASSERT_TRUE(_dnode1->isSuccStable());
  ASSERT_EQ(vnode2->getIdKey().to_rstring(),
            vnode1->getSuccessorS().to_rstring());

  while(_dnode2->_stabilizer->_slow_clicks < max_slow_clicks)
    {
      if (!_dnode2->isSuccStable())
        sleep(event_timecheck);
      else break;
    }
  ASSERT_TRUE(_dnode2->isSuccStable());
  ASSERT_EQ(vnode1->getIdKey().to_rstring(),
            vnode2->getSuccessorS().to_rstring());

  // testing predecessors (notified by other node).
  uint64_t max_fast_clicks = 10;
  while(_dnode2->_stabilizer->_fast_clicks < max_fast_clicks)
    {
      sleep(event_timecheck);
      if (vnode1->getPredecessorS().count() > 0
          && vnode1->getPredecessorS() == vnode2->getIdKey())
        break;
    }
  ASSERT_TRUE(vnode1->getPredecessorS().count() > 0);
  ASSERT_EQ(vnode2->getIdKey().to_rstring(),
            vnode1->getPredecessorS().to_rstring());
  while(_dnode1->_stabilizer->_fast_clicks < max_fast_clicks)
    {
      sleep(event_timecheck);
      if (vnode2->getPredecessorS().count() > 0
          && vnode2->getPredecessorS() == vnode1->getIdKey())
        break;
    }
  ASSERT_TRUE(vnode2->getPredecessorS().count() > 0);
  ASSERT_EQ(vnode1->getIdKey().to_rstring(),
            vnode2->getPredecessorS().to_rstring());

  // voluntary leave.
  _dnode2->leave();
}
#endif

//TODO: test finger table.

//TODO: find closest predecessor.

//TODO: estimate of number of nodes.

//TODO: dnode1 gets down, dnode2 should correct its successor & predecessor.
#if 0
TEST_F(DHTVirtualNodeTest, node_fail_before_stable)
{
  // create dnode2.
  DHTVirtualNodeTest::start_dnode(_dnode2,_na_dnode2,
                                  _net_addr2,_net_port2,
                                  dkey2);
  DHTKey dhtkey1 = DHTKey::from_rstring(dkey1);
  DHTKey dhtkey2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode *vnode2 = _dnode2->findVNode(dhtkey2);
  dht_err status = _dnode2->join(*_na_dnode1,dhtkey1);
  ASSERT_EQ(DHT_ERR_OK,status);
  ASSERT_TRUE(vnode2->getSuccessorS().count() > 0);
  ASSERT_EQ(dkey1,vnode2->getSuccessorS().to_rstring());

  // dnode1 fails.
  delete _dnode1;
  _dnode1 = NULL;

  uint64_t max_slow_clicks = 5; // at least two passes for stability.
  short event_timecheck = dht_configuration::_dht_config->_event_timecheck;
  while(_dnode2->_stabilizer->_slow_clicks < max_slow_clicks)
    {
      if (!_dnode2->isSuccStable())
        sleep(event_timecheck);
      else break;
    }
  ASSERT_TRUE(_dnode2->isSuccStable());
}
#endif

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
