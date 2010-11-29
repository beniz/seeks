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

#include "DHTNode.h"
#include "errlog.h"

#if GTEST_HAS_PARAM_TEST
using ::testing::TestWithParam;
using ::testing::Values;

using namespace dht;
using sp::errlog;

class BootstrapTest : public TestWithParam<int>
{
  protected:
    BootstrapTest()
      :_nvnodes(GetParam()),_net_addr("localhost"),_net_port(11000),_dnode(NULL),
       _na_dnode(NULL)
    {
    }

    virtual ~BootstrapTest()
    {
    }

    virtual void SetUp()
    {
      // init logging module.
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_INFO | LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_DHT);

      DHTNode::_dht_config = new dht_configuration(DHTNode::_dht_config_filename);
      DHTNode::_dht_config->_nvnodes = _nvnodes;

      _na_dnode = new NetAddress(_net_addr,_net_port);
      _dnode = new DHTNode(_na_dnode->getNetAddress().c_str(),_na_dnode->getPort(),false);
      _dnode->_stabilizer = new Stabilizer(false); // empty stabilizer, not started.
      _dnode->create_vnodes();

      _dnode->init_sorted_vnodes();
      _dnode->init_server();
      _dnode->_l1_server->run_thread();
    }

    virtual void TearDown()
    {
      delete _dnode;
    }

    int _nvnodes;
    std::string _net_addr;
    unsigned short _net_port;
    DHTNode *_dnode;
    NetAddress *_na_dnode;
};

TEST_P(BootstrapTest, self_bootstrap)
{
  _dnode->self_bootstrap();

  // check number of virtual nodes.
  ASSERT_EQ(DHTNode::_dht_config->_nvnodes,_dnode->_vnodes.size());
  ASSERT_EQ(DHTNode::_dht_config->_nvnodes,_dnode->_sorted_vnodes_vec.size());

  // check the correctness of the self-generated circle.
  int nvnodes = DHTNode::_dht_config->_nvnodes;
  for (int i=0; i<nvnodes; i++)
    {
      const DHTKey *dk = _dnode->_sorted_vnodes_vec.at(i);
      DHTVirtualNode *vn = _dnode->findVNode(*dk);

      // assert predecessor.
      if (i == 0)
        ASSERT_EQ(_dnode->_sorted_vnodes_vec.at(nvnodes-1)->to_rstring(),
                  vn->getPredecessor()->to_rstring());
      else ASSERT_EQ(_dnode->_sorted_vnodes_vec.at(i-1)->to_rstring(),
                       vn->getPredecessor()->to_rstring());
      // assert successor.
      if (i!=nvnodes-1)
        ASSERT_EQ(_dnode->_sorted_vnodes_vec.at(i+1)->to_rstring(),
                  vn->getSuccessor()->to_rstring());
      else ASSERT_EQ(_dnode->_sorted_vnodes_vec.at(0)->to_rstring(),
                       vn->getSuccessor()->to_rstring());
    
      // check on location table.
      ASSERT_EQ(nvnodes,vn->getLocationTable()->size());
    }
  
}

INSTANTIATE_TEST_CASE_P(
  OnTheFlyAndPreCalculated,
  BootstrapTest,
  Values(1,2,3,4)
);

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else
// Google Test may not support value-parameterized tests with some
// compilers. If we use conditional compilation to compile out all
// code referring to the gtest_main library, MSVC linker will not link
// that library at all and consequently complain about missing entry
// point defined in that library (fatal error LNK1561: entry point
// must be defined). This dummy test keeps gtest_main linked in.
TEST(DummyTest, ValueParameterizedTestsAreNotSupportedOnThisPlatform) {}
#endif // GTEST_HAS_PARAM_TEST
