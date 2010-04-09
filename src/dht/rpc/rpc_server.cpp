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
#include "errlog.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>

#include <iostream>

using sp::errlog;
using sp::spsockets;

namespace dht
{
   rpc_server::rpc_server(const std::string &hostname, const short &port)
     :_na(hostname,port)
     {     
     }
   
   rpc_server::~rpc_server()
     {
     }
   
   dht_err rpc_server::run()
     {
	// create socket.
	int udp_sock = socket(AF_INET,SOCK_DGRAM,0);
	
	struct sockaddr_in server;
	size_t length = sizeof(server);
	bzero(&server,length);
	server.sin_family = AF_INET; // beware, should do AF_INET6 also.
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(_na.getPort());
	
	// bind socket.
	int bind_error = bind(udp_sock,(struct sockaddr*)&server,length);
	if (bind_error < 0)
	  {
#ifdef _WIN32
	     errno = WSAGetLastError();
	     if (errno == WSAEADDRINUSE)
#else
	       if (errno == EADDRINUSE)
#endif
		 {
		    spsockets::close_socket(udp_sock);
		    
		    //debug
		    std::cout << "rpc_server, can't bind to " << _na.getNetAddress().c_str()
		      << ":" << _na.getPort() << std::endl;
		    //debug
		    
		    errlog::log_error(LOG_LEVEL_FATAL, "rpc_server: can't bind to %s:%d: "
				      "There may be some other server running on port %d",
				      _na.getNetAddress().c_str(),
				      _na.getPort(), _na.getPort());
		    // TODO: throw exception instead.
		    return(-3);
		 }
	     else
	       {
		  spsockets::close_socket(udp_sock);
		  
		  //debug
		  std::cout << "rpc_server, can't bind to " << _na.getNetAddress().c_str()
		    << ":" << _na.getPort() << std::endl;
		  //debug
		  
		  errlog::log_error(LOG_LEVEL_FATAL, "can't bind to %s:%d: %E",
				    _na.getNetAddress().c_str(), _na.getPort());
		  // TODO: throw exception instead.
		  return(-1);
	       }
	  }
	
	// get messages, one by one.
	size_t buflen = 1024; // TODO: 128 bytes may not be enough -> use dht_configuration value.
	char buf[buflen];
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(struct sockaddr_in);
	while(true)
	  {
	     //debug
	     std::cerr << "[Debug]:rpc_server: listening for dgrams on " 
	       << _na.toString() << "...\n";
	     //debug
	     
	     int n = recvfrom(udp_sock,buf,buflen,0,(struct sockaddr*)&from,&fromlen);
	     if (n < 0)
	       {
		  //debug
		  std::cout << "Error receiving DGRAM message\n";
		  //debug
		  
		  errlog::log_error(LOG_LEVEL_ERROR, "recvfrom: error receiving DGRAM message, %E");
	       }
	     
	     //debug
	     std::cout << "rpc_server: received an " << n << " bytes datagram.\n";
	     //debug
	     
	     // message.
	     errlog::log_error(LOG_LEVEL_LOG, "rpc_server: received a %d bytes datagram", n);
	     //buf[n] = '\0';
	     std::string dtg_str = std::string(buf,n-1);
	     //std::cout << "msg size: " << dtg_str.size() << std::endl;
	     
	     //debug
	     //std::cout << "rpc_server: received msg: " << dtg_str << std::endl;
	     //debug
	     
	     // produce and send a response.
	     std::string resp_msg;
	     try
	       {
		  serve_response(dtg_str,resp_msg);
	       }
	     catch (dht_exception ex)
	       {
		  errlog::log_error(LOG_LEVEL_LOG, "rpc_server exception: %s", ex.what().c_str());
	       
		  // TODO: error decision.
	     	  // TODO: rethrow something.
	       }
	  
	     //debug
	     //std::cerr << "resp_msg: " << resp_msg << std::endl;
	     //debug
	     
	     // send the response back.
	     char msg_str[resp_msg.length()+1];
	     for (size_t i=0;i<resp_msg.length();i++)
	       msg_str[i] = resp_msg[i];
	     msg_str[resp_msg.length()] = '\0';
	     
	     //debug
	     std::cerr << "[Debug]:sending " << sizeof(msg_str) << " bytes\n";
	     //std::cerr << "sent msg: " << msg_str << std::endl;
	     //debug
	     
	     n = sendto(udp_sock,msg_str,sizeof(msg_str),0,(struct sockaddr*)&from,fromlen);
	     if (n<0)
	       {
		  errlog::log_error(LOG_LEVEL_DHT, "Error sending rpc_server answer msg");
		  throw rpc_server_sending_error_exception();
	       }
	  } // end while server listening loop.
	
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
      
   dht_err rpc_server::serve_response(const std::string &msg,
				      std::string &resp_msg)
     {
	return DHT_ERR_OK;
     }
      
} /* end of namespace. */
