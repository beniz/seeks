/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010 Loic Dachary <loic@dachary.org>
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

#include "ut-utils.h"

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

typedef DHTTest DHTVirtualNodeTest;

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
  EXPECT_EQ(1, joining->_successors.size());

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
  EXPECT_EQ(joining->getSuccessorS(), vnode->getIdKey());
  EXPECT_EQ(vnode->getSuccessorS(), vnode->getIdKey());
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

TEST_F(DHTVirtualNodeTest, stabilize)
{
  DHTKey key1 = DHTKey::from_rstring(dkey1);
  DHTKey key2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode* vnode2 = new_vnode(key2);
  DHTKey key3 = DHTKey::from_rstring(dkey3);
  DHTVirtualNode* vnode3 = new_vnode(key3);
  DHTKey key4 = DHTKey::from_rstring(dkey4);
  DHTVirtualNode* vnode4 = new_vnode(key4);

  //
  //   key1    key2    key3    key4
  // before
  //             --------------->    A
  //     <-------------- <-------    B
  //                     ------->    C
  //
  // after
  //             -------> ------>    D
  //             <------- <------    E
  //
  vnode2->setSuccessor(key4, NetAddress());   // A
  EXPECT_EQ(key4, vnode2->getSuccessorS());
  vnode4->setPredecessor(key3, NetAddress()); // B
  EXPECT_EQ(key3, vnode4->getPredecessorS());
  vnode3->setPredecessor(key1, NetAddress()); // B
  EXPECT_EQ(key1, vnode3->getPredecessorS());
  vnode3->setSuccessor(key4, NetAddress());   // C
  EXPECT_EQ(key4, vnode3->getSuccessorS());

  vnode2->stabilize();

  EXPECT_EQ(key3, vnode2->getSuccessorS());   // D
  EXPECT_EQ(key4, vnode3->getSuccessorS());   // D
  EXPECT_EQ(key2, vnode3->getPredecessorS()); // E
  EXPECT_EQ(key3, vnode4->getPredecessorS()); // E
}

TEST_F(DHTVirtualNodeTest, stabilize_successor_predecessor_loop_throw)
{
  DHTKey key2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode* vnode2 = bootstrap(new_vnode(key2));

  DHTKey successor_predecessor;
  NetAddress na_successor_predecessor;

  // the successor list is empty,
  bool caught = false;
  try
    {
      vnode2->stabilize_successor_predecessor_loop(successor_predecessor, na_successor_predecessor);
    }
  catch(dht_exception& e)
    {
      caught = true;
      EXPECT_EQ(DHT_ERR_RETRY, e.code());
      EXPECT_NE(std::string::npos, e.what().find("exhausted successor"));
    }
  EXPECT_TRUE(caught);
}

TEST_F(DHTVirtualNodeTest, stabilize_successor_not_set)
{
  DHTKey key2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode* vnode2 = new_vnode(key2);

  // the successor list is empty,
  bool caught = false;
  try
    {
      vnode2->stabilize();
    }
  catch(dht_exception& e)
    {
      caught = true;
      EXPECT_EQ(DHT_ERR_RETRY, e.code());
      EXPECT_NE(std::string::npos, e.what().find("successor is not set"));
    }
  EXPECT_TRUE(caught);
}

TEST_F(DHTVirtualNodeTest, stabilize_successor_predecessor_loop_found)
{
  DHTKey key2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode* vnode2 = new_vnode(key2);

  DHTKey successor_predecessor;
  NetAddress na_successor_predecessor;

  // the successor is found
  DHTKey key4 = DHTKey::from_rstring(dkey4);
  DHTVirtualNode* vnode4 = new_vnode(key4);
  vnode2->setSuccessor(key4, NetAddress());
  vnode4->setPredecessor(key2, NetAddress());
  EXPECT_TRUE(vnode2->stabilize_successor_predecessor_loop(successor_predecessor, na_successor_predecessor));
}

TEST_F(DHTVirtualNodeTest, stabilize_successor_predecessor_loop_give_up)
{
  DHTKey key2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode* vnode2 = new_vnode(key2);

  DHTKey successor_predecessor;
  NetAddress na_successor_predecessor;

  //
  // the successor list contains
  //   key3
  //   key4
  // when the function reaches key3, it times out and skips to the next entry
  // when it gets to key4, the corresponding virtual node answers that its predecessor is key3
  // since key3 is known to be dead (the function remembers the key of dead nodes),
  // the function gives up and returns false
  // key4 will eventually (at a later point in time) notice that key3 is
  // defunct and another call to the function will provide the caller with an better predecessor.
  //
  DHTKey key4 = DHTKey::from_rstring(dkey4);
  DHTVirtualNode* vnode4 = new_vnode(key4);
  DHTKey key3 = DHTKey::from_rstring(dkey3); // note that there is *no* vnode3, this yields to timeout
  vnode2->setSuccessor(key4, NetAddress());
  vnode2->_successors.push_front(key3);
  vnode2->addOrFindToLocationTable(key3, NetAddress());
  vnode4->setPredecessor(key3, NetAddress());
  EXPECT_FALSE(vnode2->stabilize_successor_predecessor_loop(successor_predecessor, na_successor_predecessor));
}

TEST_F(DHTVirtualNodeTest, stabilize_successor_predecessor_loop_fallback)
{
  DHTKey key2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode* vnode2 = new_vnode(key2);

  DHTKey successor_predecessor;
  NetAddress na_successor_predecessor;

  //
  // the successor list contains
  //   key3
  //   key4
  // when the function reaches key3, it times out and skips to the next entry
  // key4 returns its predecessor : key2
  //
  DHTKey key4 = DHTKey::from_rstring(dkey4);
  DHTVirtualNode* vnode4 = new_vnode(key4);
  DHTKey key3 = DHTKey::from_rstring(dkey3); // note that there is *no* vnode3, this yields to timeout
  vnode2->setSuccessor(key4, NetAddress());
  vnode2->_successors.push_front(key3);
  vnode2->addOrFindToLocationTable(key3, NetAddress());
  vnode4->setPredecessor(key2, NetAddress());
  EXPECT_TRUE(vnode2->stabilize_successor_predecessor_loop(successor_predecessor, na_successor_predecessor));
  EXPECT_EQ(key2, successor_predecessor);
}

TEST_F(DHTVirtualNodeTest, stabilize_successor_predecessor)
{
  DHTKey key2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode* vnode2 = new_vnode(key2);
  DHTKey key4 = DHTKey::from_rstring(dkey4);

  // key4 is not an existing node and the call is expected to timeout
  vnode2->setSuccessor(key4, NetAddress());
  DHTKey predecessor;
  NetAddress na_predecessor;
  EXPECT_EQ(DHT_ERR_COM_TIMEOUT, vnode2->stabilize_successor_predecessor(key4, predecessor, na_predecessor));

  // key4 exists but has no predecessor
  DHTVirtualNode* vnode4 = new_vnode(key4);
  EXPECT_EQ(DHT_ERR_NO_PREDECESSOR_FOUND, vnode2->stabilize_successor_predecessor(key4, predecessor, na_predecessor));

  // key4 exists and has a predecessor
  DHTKey key3 = DHTKey::from_rstring(dkey3);
  vnode4->setPredecessor(key3, NetAddress());
  EXPECT_EQ(DHT_ERR_OK, vnode2->stabilize_successor_predecessor(key4, predecessor, na_predecessor));
  EXPECT_EQ(key3, predecessor);
}

TEST_F(DHTVirtualNodeTest, stabilize_notify)
{
  DHTKey key2 = DHTKey::from_rstring(dkey2);
  DHTVirtualNode* vnode2 = new_vnode(key2);
  DHTKey key4 = DHTKey::from_rstring(dkey4);
  DHTVirtualNode* vnode4 = new_vnode(key4);

  // vnode2 becomes vnode4 predecessor
  vnode2->setSuccessor(key4, vnode4->getNetAddress());
  EXPECT_EQ((DHTKey*)NULL, vnode4->getPredecessorPtr());
  vnode2->stabilize_notify();
  EXPECT_EQ(key2, vnode4->getPredecessorS());

  // vnode3 takes over as vnode4 predecessor
  DHTKey key3 = DHTKey::from_rstring(dkey3);
  DHTVirtualNode* vnode3 = new_vnode(key3);
  vnode3->setSuccessor(key4, vnode4->getNetAddress());
  vnode3->stabilize_notify();
  EXPECT_EQ(key3, vnode4->getPredecessorS());

  // vnode2 tries to takes over as vnode4 predecessor but fails
  vnode2->stabilize_notify();
  EXPECT_EQ(key3, vnode4->getPredecessorS());
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
