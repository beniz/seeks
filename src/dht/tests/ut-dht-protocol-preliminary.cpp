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

using namespace dht;
using sp::errlog;

//const std::string net_addr = "localhost";
//const short net_port = 11000; //TODO: 0 for first available port.

//const std::string pred_key = "33c80b886cdd2eb535b649817a2c24e6fe63820d";
const std::string pred_addr = "localhost";
const short pred_port = 12000;

//const std::string succ_key = "4535fe26b86398a7cddf6047e9c9404074f20a8d";
const std::string succ_addr = "localhost";
const short succ_port = 13000;

const std::string dkey1 = "9ae34be97ae732e5319ce2992cb489eea99f18ed";
const std::string dkey2 = "12b10bf4851cb0b569eee7023b79db6aecfc7bed";
/*d17b8d98350865e759426ade8899e77cc970f11d
c8eb560e300210de9be0ecc84c7789c24043bc9d
275d59a3083ac6fa87b40130de276620a621879d
8b3715ee2b5aa49854dbd4c09a5883b93afa135d
faad1ea5e0e5c5ab54bf5225279ce509f3cb5edd
7da78291e4d8fe6a8fea412fe45701a0b137ac3d
6e67359c3d66fbb024ba788e50ebd5770ad45b3d */

class ProtocolPreliminaryTest : public testing::Test
{
  protected:
    ProtocolPreliminaryTest()
      :_net_addr("localhost"),_net_port(11000),_dnode(NULL),
       _na_dnode(NULL),_v1node(NULL)
    {
    }

    virtual ~ProtocolPreliminaryTest()
    {
    }

    virtual void SetUp()
    {
      // init logging module.
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_ERROR | LOG_LEVEL_DHT);

      dht_configuration::_dht_config = new dht_configuration(DHTNode::_dht_config_filename);
      dht_configuration::_dht_config->_nvnodes = 4;

      _na_dnode = new NetAddress(_net_addr,_net_port);
      _dnode = new DHTNode(_na_dnode->getNetAddress().c_str(),_na_dnode->getPort(),
			   false,"ut-dht-protocol-vnodes-table.dat");
      _dnode->_stabilizer = new Stabilizer(false); // empty stabilizer, not started.
      bool has_persistent_data = _dnode->load_vnodes_table();
      ASSERT_TRUE(has_persistent_data);
      ASSERT_EQ(4,_dnode->_vnodes.size());
      
      _dnode->init_sorted_vnodes();
      _dkeys = &_dnode->_sorted_vnodes_vec[0];
      
      // debug
      std::vector<const DHTKey*>::const_iterator vit
	= _dnode->_sorted_vnodes_vec.begin();
      while(vit!=_dnode->_sorted_vnodes_vec.end())
	{
	  std::cerr << *(*vit) << std::endl;
	  ++vit;
	}
      // debug
      
      _dnode->init_server();
      _dnode->_l1_server->run_thread();
      while(true)
        {
          try
            {
              dht_err status;
              _dnode->_l1_client->RPC_ping(DHTKey(),*_na_dnode,status);
              break;
            }
          catch(dht_exception &e)
            {
              if(e.code() != DHT_ERR_COM_TIMEOUT)
                {
                  throw e;
                }
            }
        }
    }

    virtual void TearDown()
    {
      delete _na_dnode;
      delete _dnode; // stop server, client, stabilizer, hibernates & destroys vnodes.
    }

    std::string _net_addr;
    unsigned short _net_port;
    DHTNode *_dnode;
    NetAddress *_na_dnode;
    DHTVirtualNode *_v1node;
    const DHTKey **_dkeys;
};

TEST_F(ProtocolPreliminaryTest, get_no_successor)
{
  // test no successor found.
  DHTKey dkres;
  NetAddress nares;
  dht_err status = -1;
  _v1node = _dnode->findVNode(*_dkeys[0]);
  _dnode->_l1_client->RPC_getSuccessor(_v1node->getIdKey(),*_na_dnode,
                                       dkres,nares,
                                       status);
  ASSERT_EQ(DHT_ERR_NO_SUCCESSOR_FOUND,status);
}

TEST_F(ProtocolPreliminaryTest, get_no_predecessor)
{
  // test no predecessor found.
  DHTKey dkres;
  NetAddress nares;
  dht_err status = -1;
  _v1node = _dnode->findVNode(*_dkeys[0]);
  _dnode->_l1_client->RPC_getPredecessor(_v1node->getIdKey(),*_na_dnode,
                                         dkres,nares,
                                         status);
  ASSERT_EQ(DHT_ERR_NO_PREDECESSOR_FOUND,status);
}

TEST_F(ProtocolPreliminaryTest, get_predecessor)
{
  // get first virtual node.
  _v1node = _dnode->findVNode(*_dkeys[0]);

  // set predecessor
  DHTKey pred_dhtkey = *_dkeys[3];
  NetAddress *na_pred = new NetAddress(pred_addr,pred_port);
  _v1node->setPredecessor(pred_dhtkey,*na_pred);

  // test predecessor.
  DHTKey dkres;
  NetAddress nares;
  dht_err status = -1;
  _dnode->_l1_client->RPC_getPredecessor(_v1node->getIdKey(),*_na_dnode,
                                         dkres,nares,
                                         status);
  ASSERT_EQ(DHT_ERR_OK,status);
  ASSERT_EQ(_dkeys[3]->to_rstring(),dkres.to_rstring());
  ASSERT_EQ(nares.getNetAddress(),na_pred->getNetAddress());
  ASSERT_EQ(nares.getPort(),na_pred->getPort());

  delete na_pred;
}

TEST_F(ProtocolPreliminaryTest, get_successor)
{
  // get first virtual node.
  _v1node = _dnode->findVNode(*_dkeys[0]);
  
  // set successor.
  DHTKey succ_dhtkey = *_dkeys[1];
  NetAddress *na_succ = new NetAddress(succ_addr,succ_port);
  _v1node->setSuccessor(succ_dhtkey,*na_succ);

  // test predecessor.
  DHTKey dkres;
  NetAddress nares;
  dht_err status = -1;
  _dnode->_l1_client->RPC_getSuccessor(_v1node->getIdKey(),*_na_dnode,
                                       dkres,nares,
                                       status);
  ASSERT_EQ(DHT_ERR_OK,status);
  ASSERT_EQ(_dkeys[1]->to_rstring(),dkres.to_rstring());
  ASSERT_EQ(nares.getNetAddress(),na_succ->getNetAddress());
  ASSERT_EQ(nares.getPort(),na_succ->getPort());

  delete na_succ;
}

TEST_F(ProtocolPreliminaryTest, find_closest_predecessor)
{
  // get first virtual node.
  _v1node = _dnode->findVNode(*_dkeys[0]);

  // set successor.
  DHTKey succ_dhtkey = *_dkeys[1];
  NetAddress *na_succ = new NetAddress(succ_addr,succ_port);
  _v1node->setSuccessor(succ_dhtkey,*na_succ);
  
  // set predecessor
  DHTKey pred_dhtkey = *_dkeys[3];
  NetAddress *na_pred = new NetAddress(pred_addr,pred_port);
  _v1node->setPredecessor(pred_dhtkey,*na_pred);
  
  // test find_closest_predecessor.
  DHTKey dkres, dkres_succ;
  NetAddress nares, nares_succ;
  DHTKey nodekey = DHTKey::from_rstring(dkey1);
  dht_err status = -1;
  _dnode->_l1_client->RPC_findClosestPredecessor(_v1node->getIdKey(),
						 _v1node->getNetAddress(),
						 nodekey,
						 dkres,nares,
						 dkres_succ,nares_succ,
						 status);
  ASSERT_EQ(DHT_ERR_OK,status);
  ASSERT_EQ(succ_dhtkey.to_rstring(),dkres.to_rstring());
  ASSERT_TRUE(dkres < nodekey);
    
  // dkres_succ is empty, findClosestPredecessor is not using succlist,
  // and cannot return successors.
  ASSERT_EQ(DHTKey().to_rstring(),dkres_succ.to_rstring());  

  delete na_pred;
  delete na_succ;
}

TEST_F(ProtocolPreliminaryTest, notify)
{
  // craft predecessor.
  DHTKey pred_dhtkey = *_dkeys[3];
  NetAddress *na_pred = new NetAddress(pred_addr,pred_port);

  // test notify.
  dht_err status = -1;
  _v1node = _dnode->findVNode(*_dkeys[0]);
  _dnode->_l1_client->RPC_notify(_v1node->getIdKey(),*_na_dnode,
				 pred_dhtkey,*na_pred,status);
  ASSERT_EQ(DHT_ERR_OK,status);
  
  delete na_pred;
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
