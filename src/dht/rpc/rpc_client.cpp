/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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

#include "rpc_client.h"
#include "dht_configuration.h"
#include "spsockets.h"
#include "miscutil.h"
#include "errlog.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#include <pthread.h>

#include <iostream>
#include <cstdlib>
#include <cstring>

using sp::spsockets;
using sp::miscutil;
using sp::errlog;

namespace dht
{
  rpc_client::rpc_client()
  {
  }

  rpc_client::~rpc_client()
  {
  }

  void rpc_client::do_rpc_call(const NetAddress &server_na,
                               const std::string &msg,
                               const bool &need_response,
                               std::string &response) const throw (dht_exception)
  {
    response.clear();
    int fd = open();
    try
      {
        send(fd, server_na, msg);
        if (need_response)
          {
            receive(fd, response);
          }
        spsockets::close_socket(fd);
      }
    catch (dht_exception &e)
      {
        spsockets::close_socket(fd);
        throw e;
      }
  }

  int rpc_client::open() const throw (dht_exception)
  {
    // create socket.
    int udp_sock = socket(AF_INET,SOCK_DGRAM,0);
    if (udp_sock < 0)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Error creating rpc_client socket");
        throw dht_exception(DHT_ERR_SOCKET,"Error creating rpc_client socket");
      }

    int opts = fcntl(udp_sock, F_GETFL);
    if (fcntl(udp_sock, F_SETFL, opts | O_NONBLOCK))
      {
        throw dht_exception(DHT_ERR_NETWORK, std::string("rpc_client: cannot set O_NONBLOCK ") + strerror(errno));
      }
    return udp_sock;
  }

  void rpc_client::send(int fd,
                        const NetAddress &server_na,
                        const std::string &msg) const throw (dht_exception)
  {
    struct addrinfo *result = resolve(server_na);
    try
      {
        write(fd, result, msg);
        freeaddrinfo(result);
      }
    catch (dht_exception &e)
      {
        freeaddrinfo(result);
        throw e;
      }
  }


  struct addrinfo *rpc_client::resolve(const NetAddress &server_na) const throw (dht_exception)
  {
    std::string port_str = miscutil::to_string(server_na.getPort());
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    std::memset(&hints,0,sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM; // udp.
    hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV; /* avoid service look-up */

    int gai_err = 0;
    if ((gai_err = getaddrinfo(server_na.getNetAddress().c_str(),port_str.c_str(),&hints,&result)))
      {
        errlog::log_error(LOG_LEVEL_DHT, "Cannot resolve %s: %s", server_na.getNetAddress().c_str(),
                          gai_strerror(gai_err));
        throw dht_exception(DHT_ERR_NETWORK, "Cannot resolve " + server_na.getNetAddress() + ":" + gai_strerror(gai_err));
      }

    return result;
  }

  void rpc_client::write(int fd,
                         struct addrinfo *result,
                         const std::string &msg) const throw (dht_exception)
  {
    bool sent = false;
    struct addrinfo *rp;
    int sendto_errno = 0;
    for (rp=result; rp!=NULL; rp=rp->ai_next)
      {
        int n = sendto(fd,msg.c_str(),msg.length(),0,rp->ai_addr,rp->ai_addrlen);
        if (n<0)
          {
            sendto_errno = errno;
            // try next entry until no more.
          }
        else
          {
            sent = true;
            break;
          }
      }

    if (!sent)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Error sending rpc_client msg");
        throw dht_exception(DHT_ERR_CALL,std::string("rpc_client::write ") + strerror(sendto_errno));
      }
  }

  void rpc_client::receive(int fd, std::string &response) const
  {
    wait(fd);
    read(fd, response);
  }

  void rpc_client::wait(int fd) const throw (dht_exception)
  {
    // non blocking on (single) response.
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd,&rfds);
    timeval timeout;
    timeout.tv_sec = dht_configuration::_dht_config->_l1_client_timeout / 1000;
    timeout.tv_usec = (dht_configuration::_dht_config->_l1_client_timeout % 1000) * 1000;

    int m = select((int)fd+1,&rfds,NULL,NULL,&timeout);

    if (m == 0) // no bits received before the communication timed out.
      {
        errlog::log_error(LOG_LEVEL_ERROR, "Didn't receive response data in time to layer 1 call");
        throw dht_exception(DHT_ERR_COM_TIMEOUT,"rpc_client::receive_wait timeout");
      }
    else if (m < 0)
      {
        errlog::log_error(LOG_LEVEL_ERROR, "rpc_client select() failed!: %E");
        throw dht_exception(DHT_ERR_SOCKET, std::string("rpc_client select ") + strerror(errno));
      }
  }

  void rpc_client::read(int fd, std::string &response) const throw (dht_exception)
  {
    size_t buflen = dht_configuration::_dht_config->_l1_server_max_msg_bytes;
    char buf[buflen];
    struct sockaddr_in from;
    int length = sizeof(struct sockaddr_in);
    int n = recvfrom(fd,buf,buflen,0,(struct sockaddr*)&from,(socklen_t*)&length);
    if (n<0)
      {
        dht_err err = DHT_ERR_RESPONSE;
        if (errno == EWOULDBLOCK || errno == EAGAIN || errno == ECONNRESET)
          err = DHT_ERR_COM_TIMEOUT;
        response.clear();
        errlog::log_error(LOG_LEVEL_ERROR,"Error in response to rpc_client msg");
        throw dht_exception(err, std::string("rpc_client::receive_read ") + strerror(errno));
      }

    response = std::string(buf,n);
  }

} /* end of namespace. */
