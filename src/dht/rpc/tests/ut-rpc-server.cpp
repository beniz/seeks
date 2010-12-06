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
#include <fcntl.h>
#include <iostream>
#include <assert.h>
#include <map>

#include "DHTNode.h"
#include "rpc_server.h"
#include "rpc_client.h"
#include "spsockets.h"

using sp::spsockets;
using namespace dht;

class ServerTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
      dht_configuration::_dht_config = new dht_configuration("");
    }
};

TEST_F(ServerTest, bind_normal)
{
  rpc_server server("localhost",0);
  server.bind();
  EXPECT_NE(-1, server._udp_sock);
  EXPECT_NE(0, server._na.getPort());
  int opts = fcntl(server._udp_sock, F_GETFL);
  EXPECT_TRUE(opts & O_NONBLOCK);
}

TEST_F(ServerTest, bind_addressfail)
{
  rpc_server server("unlikely.unknown",0);
  try
    {
      server.bind();
    }
  catch(dht_exception &e)
    {
      EXPECT_EQ(DHT_ERR_NETWORK,e.code());
      EXPECT_NE(std::string::npos, e.what().find("unlikely.unknown"));
    }
}

TEST_F(ServerTest, bind_socketfail)
{
  std::vector<int> v;
  while(true)
    {
      int fd;
      if((fd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
        break;
      v.push_back(fd);
    }
  rpc_server server("127.0.0.1",0);
  try
    {
      server.bind();
    }
  catch(dht_exception &e)
    {
      EXPECT_EQ(DHT_ERR_NETWORK,e.code());
      EXPECT_NE(std::string::npos, e.what().find("socket "));
    }
  for(std::vector<int>::iterator i = v.begin(); i != v.end(); i++)
    close(*i);
}

class test_rpc_server : public rpc_server
{
  public:

    typedef std::map<std::string, std::string> string_map;

    test_rpc_server() :
      rpc_server("localhost", 0)
    {
      message2answer["PING"] = "PONG";
      message2answer["MSG1"] = "RSP1";
    }

    virtual void serve_response(const std::string &msg,
                                const std::string &addr,
                                std::string &resp_msg)
    {
      if(message2answer.find(msg) == message2answer.end())
        {
          std::cerr << "message2answer[" + msg + "] not found and ignored" << std::endl;
          throw dht_exception(DHT_ERR_RESPONSE, "not found");
        }
      else
        {
          resp_msg = message2answer[msg];
        }
    }

    string_map message2answer;
};

class ServerTestRunning : public testing::Test
{
  protected:

    virtual void SetUp()
    {
      dht_configuration::_dht_config = new dht_configuration("");
      _server._timeout.tv_sec = 0; // ms
      _server._timeout.tv_usec = 1; // ms
      _server.run_thread();
      mutex_lock(&_server._select_mutex);
      mutex_unlock(&_server._select_mutex);
      rpc_client client;
      std::string response;
      while(true)
        {
          try
            {
              client.do_rpc_call(_server._na, "PING", true, response);
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
      EXPECT_EQ("PONG", response);
    }

    virtual void TearDown()
    {
      EXPECT_FALSE(_server._abort);
      _server.stop_thread();
      EXPECT_TRUE(_server._abort);
    }

    test_rpc_server _server;
};

TEST_F(ServerTestRunning, client_message)
{
  // a message is sent to the server and the expected response is received
  std::string response;
  rpc_client client;
  client.do_rpc_call(_server._na, "MSG1", true, response);
  EXPECT_EQ("RSP1", response);
}

TEST_F(ServerTestRunning, client_message_timeout_select)
{
  // the server is blocked (using the _select_mutex) before it can
  // answer and the client timeout waiting for a response
  mutex_lock(&_server._select_mutex);
  std::string response;
  rpc_client client;
  dht_configuration::_dht_config->_l1_client_timeout = 1; // ms
  try
    {
      client.do_rpc_call(_server._na, "MSG1", true, response);
      FAIL();
    }
  catch (dht_exception &e)
    {
      EXPECT_EQ(DHT_ERR_COM_TIMEOUT, e.code());
      EXPECT_NE(std::string::npos, e.what().find("receive_wait"));
    }
  mutex_unlock(&_server._select_mutex);
}

TEST_F(ServerTestRunning, client_message_timeout_recvfrom)
{
  // the server is blocked (using the _select_mutex) before it can
  // answer and the client timeout waiting for a response
  mutex_lock(&_server._select_mutex);
  std::string response;
  rpc_client client;
  dht_configuration::_dht_config->_l1_client_timeout = 1; // ms
  try
    {
      int fd = client.open();
      client.send(fd, _server._na, "MSG1");
      client.read(fd, response);
      FAIL();
    }
  catch (dht_exception &e)
    {
      EXPECT_EQ(DHT_ERR_COM_TIMEOUT, e.code());
      EXPECT_NE(std::string::npos, e.what().find("receive_read"));
    }
  mutex_unlock(&_server._select_mutex);
}

TEST_F(ServerTestRunning, client_message_error_sendto)
{
  // the sendto system call returns on error
  rpc_client client;
  struct addrinfo *result = NULL;
  try
    {
      result = client.resolve(_server._na);
      client.write(-1, result, "");
    }
  catch (dht_exception &e)
    {
      EXPECT_EQ(DHT_ERR_CALL, e.code());
      EXPECT_NE(std::string::npos, e.what().find("write"));
      if(result)
        freeaddrinfo(result);
    }
}

TEST_F(ServerTestRunning, server_loop_timeout)
{
  rpc_client client;
  int fd = client.open();
  mutex_lock(&_server._select_mutex);
  client.send(fd, _server._na, "MSG1");
  char buf[1024];
  int n = recvfrom(_server._udp_sock,buf,1024,0,0,0);
  ASSERT_EQ(4, n);
  try
    {
      _server.run_loop_once();
    }
  catch (dht_exception &e)
    {
      EXPECT_EQ(DHT_ERR_COM_TIMEOUT, e.code());
      EXPECT_NE(std::string::npos, e.what().find("recvfrom"));
    }
  mutex_unlock(&_server._select_mutex);
  spsockets::close_socket(fd);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
