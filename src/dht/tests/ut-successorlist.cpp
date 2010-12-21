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

#include "SuccList.h"
#include "dht_configuration.h"
#include "errlog.h"

#include <unistd.h>

using namespace dht;
using sp::errlog;

#include "ut-utils.h"

typedef DHTTest SuccessorListTest;

TEST_F(SuccessorListTest, constructor)
{
  SuccList s((DHTVirtualNode*)NULL);
  EXPECT_EQ((DHTVirtualNode*)NULL, s._vnode);
  EXPECT_FALSE(s.isStable());
  EXPECT_EQ(1, s.size());
}

TEST_F(SuccessorListTest, merge_succ_list)
{
  std::list<DHTKey> kl;
  DHTKey k1(1);
  kl.push_back(k1);
  DHTKey k3(3);
  kl.push_back(k3);
  std::list<NetAddress> nl;
  nl.push_back(NetAddress("self",1));
  nl.push_back(NetAddress("self",3));

  DHTKey k2(2);
  DHTVirtualNode* vnode2 = bootstrap(new_vnode(k2));
  new_vnode(k1);
  vnode2->_successors.merge_succ_list(kl, nl);
  ASSERT_EQ(1, vnode2->_successors.size());
  EXPECT_EQ(k1, *vnode2->_successors.begin());
}

TEST_F(SuccessorListTest, getSuccessor)
{
  SuccList s((DHTVirtualNode*)NULL);
  EXPECT_EQ(0, s.getSuccessor().count());
  Location l1(1);
  s.getSuccessor() = l1;
  EXPECT_EQ(l1, s.getSuccessor());
  Location l2(2);
  s.setSuccessor(l2);
  EXPECT_EQ(l2, s.getSuccessor());
}

TEST_F(SuccessorListTest, merge_check_stable)
{
  Location l1(1);
  DHTVirtualNode* vnode1 = bootstrap(new_vnode(l1));

  EXPECT_EQ(1, vnode1->_successors.size());
  EXPECT_FALSE(vnode1->_successors.isStable());
  vnode1->_successors.check();
  EXPECT_TRUE(vnode1->_successors.isStable());
}

TEST_F(SuccessorListTest, merge_list)
{
  std::list<Location> l;
  Location l1(1);
  Location l2(2);
  Location l3(3);
  Location l4(4);

  DHTVirtualNode* vnode2 = bootstrap(new_vnode(l2));
  SuccList& s = vnode2->_successors;

  EXPECT_EQ(1, s.size());
  EXPECT_EQ(l2, s.getSuccessor());
  s.merge_list(l);
  EXPECT_EQ(l2, s.getSuccessor());

  l.push_back(l2); // removed because it is the key of the node
  l.push_back(l1); // before the node key although numericaly
  // before it : must show at the end
  l.push_back(l3);
  l.push_back(l4); // duplicate of an existing key

  s.push_back(l4);

  s.merge_list(l);
  SuccList::iterator i = s.begin();
  ASSERT_EQ(3, s.size());
  EXPECT_EQ(l3, *i++);
  EXPECT_EQ(l4, *i++);
  EXPECT_EQ(l1, *i++);
}

TEST_F(SuccessorListTest, check)
{
  short max_size = SuccList::_max_list_size;
  SuccList::_max_list_size = 2;

  std::list<Location> l;
  Location l1(1);
  DHTVirtualNode* vnode1 = bootstrap(new_vnode(l1));
  Location l2(2);
  DHTVirtualNode* vnode2 = new_vnode(l2);
  connect(vnode1, vnode2, vnode1);
  Location l3(3);
  Location l4(4);
  DHTVirtualNode* vnode4 = new_vnode(l4);
  connect(vnode2, vnode4, vnode1);
  Location l5(5);
  DHTVirtualNode* vnode5 = new_vnode(l5);
  connect(vnode4, vnode5, vnode1);

  SuccList& s = vnode1->_successors;

  s.check();

  EXPECT_EQ(1, s.size());
  EXPECT_FALSE(s.isStable());

  s.push_back(l3); // will be discarded because it is dead
  s.push_back(l4);
  s.push_back(l5); // will be discarded because it would make a
  // list longer than the maximum
  EXPECT_EQ(4, s.size());

  s.check();

  SuccList::iterator i = s.begin();
  EXPECT_EQ(l2, *i++);
  EXPECT_EQ(l4, *i++);
  EXPECT_EQ(2, s.size());
  EXPECT_TRUE(s.isStable());

  s.pop_back();

  s.check();

  EXPECT_FALSE(s.isStable());

  SuccList::_max_list_size = max_size;
}

TEST_F(SuccessorListTest, check_empty)
{
  Location l1(1);
  Location l2(2);
  DHTVirtualNode* vnode1 = new_vnode(l1);
  vnode1->setSuccessor(l2);

  SuccList& s = vnode1->_successors;
  bool caught = false;
  try
    {
      s.check();
    }
  catch(dht_exception e)
    {
      caught = true;
      EXPECT_EQ(DHT_ERR_RETRY, e.code());
      EXPECT_NE(std::string::npos, e.what().find("exhausted successor list"));
    }
  EXPECT_EQ(1, s.size());
  EXPECT_FALSE(s.isStable());
  EXPECT_TRUE(caught);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
