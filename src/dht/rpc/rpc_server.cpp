/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2009  Emmanuel Benazera, juban@free.fr
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
#include "spsockets.h"
#include "DHTNode.h" // for accessing dht_config.
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
    :_na(hostname,port),
     _udp_sock(-1)
  {
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

  dht_err rpc_server::bind()
  {
    // resolve hostname.
    std::string port_str = miscutil::to_string(_na.getPort());
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    std::memset(&hints,0,sizeof(struct addrinfo));
    if (!miscutil::strcmpic(_na.getNetAddress().c_str(),"localhost"))
      hints.ai_family = AF_INET;
    else hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM; // udp.
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    int err = 0;
    if ((err = getaddrinfo(_na.getNetAddress().c_str(),port_str.c_str(),&hints,&result)))
      {
        errlog::log_error(LOG_LEVEL_FATAL, "Cannot resolve %s: %s", _na.getNetAddress().c_str(),
                          gai_strerror(err));
        return err;
      }

    // create socket.
    _udp_sock = socket(AF_INET,SOCK_DGRAM,0);

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
                errlog::log_error(LOG_LEVEL_FATAL, "rpc_server: can't bind to %s:%d: "
                                  "There may be some other server running on port %d",
                                  _na.getNetAddress().c_str(),
                                  _na.getPort(), _na.getPort());
                // TODO: throw exception.
                return(-3);
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
                    errlog::log_error(LOG_LEVEL_FATAL, "getsockname(%d): %s", _udp_sock, strerror(errno));
                    throw;
                  }
                _na.setPort(ntohs(a.sin_port));
              }
            bindb = true;
            break;
          }
      }

    if (!bindb)
      {
        freeaddrinfo(result);
        close_socket();
        errlog::log_error(LOG_LEVEL_FATAL, "can't bind to %s:%d: %E",
                          _na.getNetAddress().c_str(), _na.getPort());
        //TODO: throw exception.
      }
    return DHT_ERR_OK;
  }

  dht_err rpc_server::run()
  {
    dht_err err;

    if((err = bind()) != DHT_ERR_OK)
      return err;

    //debug
    std::cerr << "[Debug]:rpc_server: listening for dgrams on "
              << _na.toString() << "...\n";
    //debu

    while((err = run_loop_once()) == DHT_ERR_OK) ;
    return err;
  }

  dht_err rpc_server::run_loop_once()
  {

    // get messages, one by one.
    size_t buflen = DHTNode::_dht_config->_l1_server_max_msg_bytes;  //TODO: of transient l2 messages!
    char buf[buflen];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(struct sockaddr_in);

    int n = recvfrom(_udp_sock,buf,buflen,0,(struct sockaddr*)&from,&fromlen);
    if (n < 0)
      {
        close_socket();
        errlog::log_error(LOG_LEVEL_ERROR, "recvfrom: error receiving DGRAM message, %E");
        return DHT_ERR_SOCKET; // TODO: exception.
      }

    // get our caller's address.
    char addr_buf[NI_MAXHOST];
    int retval = getnameinfo((struct sockaddr *) &from, fromlen,
                             addr_buf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

    // message.
    errlog::log_error(LOG_LEVEL_DHT, "rpc_server: received a %d bytes datagram from %s", n, addr_buf);
    std::string dtg_str = std::string(buf,n);

    // TODO: ACL on incoming calls.
    std::string addr_str = std::string(addr_buf);

    // produce and send a response.
    std::string resp_msg;
    try
      {
        serve_response(dtg_str,addr_str,resp_msg);
      }
    catch (dht_exception ex)
      {
        close_socket();
        errlog::log_error(LOG_LEVEL_DHT, "rpc_server exception: %s", ex.what().c_str());
        return DHT_ERR_CALLBACK;
      }

    //debug
    //std::cerr << "resp_msg: " << resp_msg << std::endl;
    //debug

    // send the response back.
    char msg_str[resp_msg.length()];
    for (size_t i=0; i<resp_msg.length(); i++)
      msg_str[i] = resp_msg[i];
    //msg_str[resp_msg.length()] = '\0';

    //debug
    //std::cerr << "[Debug]:sending " << sizeof(msg_str) << " bytes\n";
    //std::cerr << "sent msg: " << msg_str << std::endl;
    //debug

    n = sendto(_udp_sock,msg_str,sizeof(msg_str),0,(struct sockaddr*)&from,fromlen);
    if (n<0)
      {
        close_socket();
        errlog::log_error(LOG_LEVEL_DHT, "Error sending rpc_server answer msg");
        throw rpc_server_sending_error_exception();
      }
    return DHT_ERR_OK;
  }

  dht_err rpc_server::run_thread()
  {
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    //pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);
    int err = pthread_create(&_rpc_server_thread,&attrs,
                             (void * (*)(void *))&rpc_server::run_static,this);
    pthread_attr_destroy(&attrs);

    if (err == 0)
      return DHT_ERR_OK;
    else return DHT_ERR_PTHREAD;
  }

  int rpc_server::detach_thread()
  {
    return pthread_detach(_rpc_server_thread);
  }

  void rpc_server::run_static(rpc_server *server)
  {
    //TODO: error catching...
    server->run();
  }
  
  int rpc_server::stop_thread()
  {
    int err = pthread_kill(_rpc_server_thread,0);
    if (err = 0)
      return DHT_ERR_OK;
    else return DHT_ERR_PTHREAD;
  }
  
  dht_err rpc_server::serve_response(const std::string &msg,
                                     const std::string &addr,
                                     std::string &resp_msg)
  {
    return DHT_ERR_OK;
  }
} /* end of namespace. */
