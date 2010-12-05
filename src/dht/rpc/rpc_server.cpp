/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2009  Emmanuel Benazera, juban@free.fr
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

#include "rpc_server.h"
#include "dht_err.h"
#include "spsockets.h"
#include "dht_exception.h"
#include "dht_configuration.h"
#include "miscutil.h"
#include "errlog.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

using sp::errlog;
using sp::miscutil;
using sp::spsockets;

namespace dht
{
  rpc_server::rpc_server(const std::string &hostname, const short &port)
    :_na(hostname,port),_udp_sock(-1),_abort(false)
  {
    mutex_init(&_run_mutex);
  }

  rpc_server::~rpc_server()
  {
    close_socket();
  }

  void rpc_server::close_socket()
  {
    if(_udp_sock >= 0)
      {
        spsockets::close_socket(_udp_sock);
        _udp_sock = -1;
      }
  }

  void rpc_server::bind()
  {
    // resolve hostname.
    std::string port_str = miscutil::to_string(_na.getPort());
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    std::memset(&hints,0,sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;

    // create socket.
    if((_udp_sock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
      {
        throw dht_exception(DHT_ERR_NETWORK, std::string("socket ") + strerror(errno));
      }

    int err = 0;
    if ((err = getaddrinfo(_na.getNetAddress().c_str(),port_str.c_str(),&hints,&result)))
      {
        errlog::log_error(LOG_LEVEL_ERROR, "Cannot resolve %s: %s", _na.getNetAddress().c_str(),
                          gai_strerror(err));
        throw dht_exception(DHT_ERR_NETWORK, "Cannot resolve " + _na.getNetAddress() + ":" + gai_strerror(err));
      }

    // bind socket.
    bool bindb = false;
    struct addrinfo *rp;
    for (rp=result; rp!=NULL; rp=rp->ai_next)
      {
        int bind_error = ::bind(_udp_sock,rp->ai_addr,rp->ai_addrlen);

        if (bind_error < 0)
          {
#ifdef _WIN32
            errno = WSAGetLastError();
            if (errno == WSAEADDRINUSE)
#else
            if (errno == EADDRINUSE)
#endif
              {
                freeaddrinfo(result);
                close_socket();
                errlog::log_error(LOG_LEVEL_ERROR, "rpc_server: can't bind to %s:%d: "
                                  "There may be some other server running on port %d",
                                  _na.getNetAddress().c_str(),
                                  _na.getPort(), _na.getPort());
                throw dht_exception(DHT_ERR_NETWORK, "rpc_server: can't bind to " + _na.getNetAddress() + ":" + sp::miscutil::to_string(_na.getPort()) + ":" + "There may be some other server running on the same port");
              }
          }
        else
          {
            if(_na.getPort() == 0)
              {
                struct sockaddr_in a;
                socklen_t a_len = sizeof(a);
                if(getsockname(_udp_sock, (sockaddr*)&a, &a_len) < 0)
                  {
                    errlog::log_error(LOG_LEVEL_ERROR, "getsockname(%d): %s", _udp_sock, strerror(errno));
                    throw dht_exception(DHT_ERR_NETWORK, "getsockname(" + sp::miscutil::to_string(_udp_sock) + "): " + strerror(errno));
                  }
                _na.setPort(ntohs(a.sin_port));
              }
            bindb = true;
            break;
          }
      }

    freeaddrinfo(result);

    if (!bindb)
      {
        close_socket();
        errlog::log_error(LOG_LEVEL_ERROR, "can't bind to %s:%d: %E",
                          _na.getNetAddress().c_str(), _na.getPort());
        throw dht_exception(DHT_ERR_NETWORK, "rpc_server: can't bind any IP " + _na.getNetAddress() + ":" + sp::miscutil::to_string(_na.getPort()));
      }
  }

  void rpc_server::run()
  {
    bind();

    std::cerr << "[Debug]:rpc_server: listening for dgrams on "
              << _na.toString() << "...\n";

    int running = true;
    while(running)
      {
        try
          {
            FD_ZERO(&_rfds);
            FD_SET(_udp_sock,&_rfds);
            _select_timeout.tv_sec = 1; // 1 second timeout is default.
            _select_timeout.tv_usec = 0;

            // non blocking on (single) service.
            int m = select((int)_udp_sock+1,&_rfds,NULL,NULL,&_select_timeout);
            if (m > 0)
              run_loop_once();
            else if (m < 0)
              {
                errlog::log_error(LOG_LEVEL_ERROR, "rpc_server select() failed!: %E");
                spsockets::close_socket(_udp_sock);
                throw new dht_exception(DHT_ERR_SOCKET, std::string("rpc_server select ") + strerror(errno));
              }

            mutex_lock(&_run_mutex);
            running = !_abort;
            mutex_unlock(&_run_mutex);
          }
        catch (dht_exception &e)
          {
            errlog::log_error(LOG_LEVEL_ERROR, "server loop caught %s", e.to_string().c_str());
          }
      }
    pthread_exit(NULL);
  }

  void rpc_server::run_loop_once()
  {

    // get messages, one by one.
    size_t buflen = dht_configuration::_dht_config->_l1_server_max_msg_bytes;  //TODO: of transient l2 messages!
    char buf[buflen];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(struct sockaddr_in);

    int n = recvfrom(_udp_sock,buf,buflen,0,(struct sockaddr*)&from,&fromlen);
    if (n < 0)
      {
        close_socket();
        errlog::log_error(LOG_LEVEL_ERROR, "recvfrom: error receiving DGRAM message, %E");
        throw new dht_exception(DHT_ERR_SOCKET, std::string("recvfrom: error receiving DGRAM message ") + strerror(errno));
      }

    // get our caller's address.
    char addr_buf[NI_MAXHOST];
    if(getnameinfo((struct sockaddr *) &from, fromlen,
                   addr_buf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST))
      throw dht_exception(DHT_ERR_SOCKET, std::string("getnameinfo ") + strerror(errno));

    // message.
    errlog::log_error(LOG_LEVEL_DHT, "rpc_server: received a %d bytes datagram from %s", n, addr_buf);
    std::string dtg_str = std::string(buf,n);

    // TODO: ACL on incoming calls.
    std::string addr_str = std::string(addr_buf);

    // produce and send a response.
    std::string resp_msg;
    serve_response(dtg_str,addr_str,resp_msg);

    char msg_str[resp_msg.length()];
    for (size_t i=0; i<resp_msg.length(); i++)
      msg_str[i] = resp_msg[i];
    if(sendto(_udp_sock,msg_str,sizeof(msg_str),0,(struct sockaddr*)&from,fromlen) < 0)
      {
        close_socket();
        errlog::log_error(LOG_LEVEL_DHT, "Error sending rpc_server answer msg");
        throw dht_exception(DHT_ERR_MSG, "Error sending rpc_server answer msg");
      }
  }

  void rpc_server::run_thread()
  {
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    //pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);
    int err = pthread_create(&_rpc_server_thread,&attrs,
                             (void * (*)(void *))&rpc_server::run_static,this);
    pthread_attr_destroy(&attrs);

    if (err)
      throw dht_exception(DHT_ERR_PTHREAD, std::string("pthread_create ") + strerror(errno));
  }

  void rpc_server::run_static(rpc_server *server)
  {
    server->run();
  }

  void rpc_server::stop_thread()
  {
    mutex_lock(&_run_mutex);
    _abort = true;
    mutex_unlock(&_run_mutex);
    pthread_join(_rpc_server_thread,NULL);
  }

  void rpc_server::serve_response(const std::string &msg,
                                  const std::string &addr,
                                  std::string &resp_msg)
  {
  }
} /* end of namespace. */
