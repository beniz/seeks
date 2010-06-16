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

#include "rpc_client.h"
#include "DHTNode.h"
#include "spsockets.h"
#include "miscutil.h"
#include "errlog.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

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

   /* void rpc_client::do_rpc_call_static(rpc_call_args *args)
     {
	try
	  {
	     args->_err = args->_client->do_rpc_call(args->_server_na,args->_msg,
						     args->_need_response,args->_response);
	  }
	catch (rpc_client_timeout_error_exception &e)
	  {
	     args->_err = DHT_ERR_COM_TIMEOUT;
	     errlog::log_error(LOG_LEVEL_DHT, e.what().c_str());
	  }
	catch (rpc_client_reception_error_exception &e)
	  {
	     args->_err = DHT_ERR_RESPONSE;
	     errlog::log_error(LOG_LEVEL_DHT, e.what().c_str());
	  }
	catch (rpc_client_socket_error_exception &e)
	  {
	     args->_err = DHT_ERR_SOCKET;
	     errlog::log_error(LOG_LEVEL_DHT, e.what().c_str());
	  }
	catch (rpc_client_host_error_exception &e)
	  {
	     args->_err = DHT_ERR_HOST;
	     errlog::log_error(LOG_LEVEL_DHT, e.what().c_str());
	  }
	catch (rpc_client_sending_error_exception &e)
	  {
	     args->_err = DHT_ERR_CALL;
	     errlog::log_error(LOG_LEVEL_DHT, e.what().c_str());
	  }
     } */
   
   /* dht_err rpc_client::do_rpc_call_threaded(const NetAddress &server_na,
					    const std::string &msg,
					    const bool &need_response,
					    std::string &response)
     {
	rpc_call_args args(this,server_na,msg,need_response,response);
	pthread_t rpc_call_thread;
	int err = pthread_create(&rpc_call_thread,NULL, //joinable
				 (void * (*)(void *))&rpc_client::do_rpc_call_static,&args);
	
	// join.
	pthread_join(rpc_call_thread,NULL);
	
	response = args._response;
	return args._err;
     } */
   
   dht_err rpc_client::do_rpc_call(const NetAddress &server_na,
				   const std::string &msg,
				   const bool &need_response,
				   std::string &response,
				   dht_err &err)
     {
	try
	  {
	     err = do_rpc_call(server_na,msg,
			       need_response,response);
	  }
	catch (rpc_client_timeout_error_exception &e)
	  {
	     err = DHT_ERR_COM_TIMEOUT;
	     errlog::log_error(LOG_LEVEL_DHT, e.what().c_str());
	  }
	catch (rpc_client_reception_error_exception &e)
	  {
	     err = DHT_ERR_RESPONSE;
	     errlog::log_error(LOG_LEVEL_DHT, e.what().c_str());
	  }
	catch (rpc_client_socket_error_exception &e)
	  {
	     err = DHT_ERR_SOCKET;
	     errlog::log_error(LOG_LEVEL_DHT, e.what().c_str());
	  }
	catch (rpc_client_host_error_exception &e)
	  {
	     err = DHT_ERR_HOST;
	     errlog::log_error(LOG_LEVEL_DHT, e.what().c_str());
	  }
	catch (rpc_client_sending_error_exception &e)
	  {
	     err = DHT_ERR_CALL;
	     errlog::log_error(LOG_LEVEL_DHT, e.what().c_str());
	  }
	return err;
     }
   
   dht_err rpc_client::do_rpc_call(const NetAddress &server_na,
				   const std::string &msg,
				   const bool &need_response,
				   std::string &response)
     {
	std::string port_str = miscutil::to_string(server_na.getPort());
	struct addrinfo hints;
	struct addrinfo *result = NULL;
	std::memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM; // udp.
	hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV; /* avoid service look-up */
	
	int err = 0;
	if ((err = getaddrinfo(server_na.getNetAddress().c_str(),port_str.c_str(),&hints,&result)))
	  {
	     errlog::log_error(LOG_LEVEL_DHT, "Cannot resolve %s: %s", server_na.getNetAddress().c_str(),
			       gai_strerror(err));
	     return err;
	  }
		
	// create socket.
	int udp_sock = socket(AF_INET,SOCK_DGRAM,0);
	if (udp_sock < 0)
	  {
	     spsockets::close_socket(udp_sock); // beware.
	     errlog::log_error(LOG_LEVEL_ERROR,"Error creating rpc_client socket");
	     throw rpc_client_socket_error_exception();
	  }
	
	struct sockaddr_in server;
	server.sin_family = AF_INET; // beware, should do AF_INET6 also.
	struct hostent *hp = gethostbyname(server_na.getNetAddress().c_str());
	if (hp == 0)
	  {
	     spsockets::close_socket(udp_sock);
	     errlog::log_error(LOG_LEVEL_ERROR,"Unknown host for rpc_client %s", server_na.getNetAddress().c_str());
	     throw rpc_client_host_error_exception(server_na.getNetAddress());
	  }
		
	/* bcopy((char*)hp->h_addr,(char*)&server.sin_addr,hp->h_length);
	server.sin_port = htons(server_na.getPort());
	int length = sizeof(struct sockaddr_in); */
	char msg_str[msg.length()];
	for (size_t i=0;i<msg.length();i++)
	  msg_str[i] = msg[i];
	//msg_str[msg.length()] = '\0';
	
	//debug
	/* std::cerr << "rpc_client: sending msg of " << sizeof(msg_str) << " bytes...\n";
	 std::cerr << "msg: " << msg_str << std::endl; */
	//debug
	
	// send the message.
	bool sent = false;
	struct addrinfo *rp;
	for (rp=result; rp!=NULL; rp=rp->ai_next)
	  {
	     //int n = sendto(udp_sock,msg_str,sizeof(msg_str),0,(struct sockaddr*)&server,length);
	     int n = sendto(udp_sock,msg_str,sizeof(msg_str),0,rp->ai_addr,rp->ai_addrlen);
	     if (n<0)
	       {
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
	     response.clear();
	     freeaddrinfo(result);
	     spsockets::close_socket(udp_sock);
	     errlog::log_error(LOG_LEVEL_ERROR,"Error sending rpc_client msg");
	     throw rpc_client_sending_error_exception();
	  }
	
	// receive message, if necessary.
	if (!need_response)
	  {
	     spsockets::close_socket(udp_sock);
	     return DHT_ERR_OK;	
	  }
	
	// non blocking on (single) response.
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(udp_sock,&rfds);
	timeval timeout;
	timeout.tv_sec = DHTNode::_dht_config->_l1_client_timeout;
	timeout.tv_usec = 0;
	
	int m = select((int)udp_sock+1,&rfds,NULL,NULL,&timeout);
	
	if (m == 0) // no bits received before the communication timed out.
	  {
	     //debug
	     //std::cerr << "[Debug]: timeout on client response\n";
	     //debug
	     
	     errlog::log_error(LOG_LEVEL_ERROR, "Didn't receive response data in time to layer 1 call");
	     response = "";
	     throw rpc_client_timeout_error_exception();
	  }
	else if (m < 0)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "select() failed!: %E");
	     spsockets::close_socket(udp_sock);
	     return DHT_ERR_UNKNOWN;
	  }
		  
	size_t buflen = 1024;
	char buf[buflen];
	struct sockaddr_in from;
	int length = sizeof(struct sockaddr_in);
	int n = recvfrom(udp_sock,buf,buflen,0,(struct sockaddr*)&from,(socklen_t*)&length);
	if (n<0)
	  {
	     response.clear();
	     spsockets::close_socket(udp_sock);
	     errlog::log_error(LOG_LEVEL_ERROR,"Error in response to rpc_client msg");
	     throw rpc_client_reception_error_exception();
	  }
	
	//debug
	//std::cerr << "[Debug]:received " << n << " bytes\n";
	//std::cerr << "received msg: " << buf << std::endl;
	//debug
	
	response = std::string(buf,n);
	
	//debug
	//std::cerr << "in rpc_client response size: " << response.size() << std::endl;
	//debug
	
	spsockets::close_socket(udp_sock);
	
	return DHT_ERR_OK;
     }
   
} /* end of namespace. */
