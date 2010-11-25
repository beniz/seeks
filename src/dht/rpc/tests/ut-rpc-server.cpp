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

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>

#include <netinet/in.h>
#include <string>
#include <errno.h>
#include <iostream>
#include <assert.h>
#include <map>

#include "DHTNode.h"
#include "rpc_server.h"
#include "rpc_client.h"

using namespace dht;

typedef std::map<std::string, std::string> string_map;

class test_rpc_server : public rpc_server
{
  public:

    test_rpc_server() : rpc_server("localhost", 0) {}

    virtual void serve_response(const std::string &msg,
                                   const std::string &addr,
                                   std::string &resp_msg)
    {
      if(message2answer.find(msg) != message2answer.end())
        {
          throw dht_exception(DHT_ERR_RESPONSE, "not found");
        }
      else
        {
          resp_msg = message2answer[msg];
        }
    }

    string_map message2answer;
};

class ServerTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
      DHTNode::_dht_config = new dht_configuration("");
      _server = new test_rpc_server();
    }

    virtual void TearDown()
    {
      delete _server;
    }

    test_rpc_server* _server;
};

TEST_F(ServerTest, bind)
{
  _server->bind();
  ASSERT_NE(-1, _server->_udp_sock);
  ASSERT_NE(0, _server->_na.getPort());
}

#if 0
  _server->message2answer["IN"] = "OUT";
  std::string response;
  rpc_client client;
  ASSERT_EQ(DHT_ERR_OK, client.do_rpc_call(_server->_na, "IN", true, response));
  ASSERT_EQ("OUT", response);
#endif

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
