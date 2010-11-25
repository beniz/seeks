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
#include <errno.h>

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
             throw dht_exception(DHT_ERR_NETWORK, "Cannot resolve " + server_na.getNetAddress() + ":" + gai_strerror(err));
	  }
		
	// create socket.
	int udp_sock = socket(AF_INET,SOCK_DGRAM,0);
	if (udp_sock < 0)
	  {
	     spsockets::close_socket(udp_sock); // beware.
	     errlog::log_error(LOG_LEVEL_ERROR,"Error creating rpc_client socket");
	     throw dht_exception(DHT_ERR_SOCKET,"Error creating rpc_client socket");
	  }
	
	char msg_str[msg.length()];
	for (size_t i=0;i<msg.length();i++)
	  msg_str[i] = msg[i];

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
	     throw dht_exception(DHT_ERR_CALL,"Error sending rpc_client msg");
	  }
	
	// receive message, if necessary.
	if (!need_response)
	  {
	     spsockets::close_socket(udp_sock);
	     return;
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
	     errlog::log_error(LOG_LEVEL_ERROR, "Didn't receive response data in time to layer 1 call");
	     response = "";
	     throw dht_exception(DHT_ERR_COM_TIMEOUT,"Didn't receive response data in time to layer 1 call");
	  }
	else if (m < 0)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "select() failed!: %E");
	     spsockets::close_socket(udp_sock);
             throw new dht_exception(DHT_ERR_SOCKET, std::string("select ") + strerror(errno));
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
	     throw dht_exception(DHT_ERR_RESPONSE,"Error in response to rpc_client msg");
	  }
	
	response = std::string(buf,n);

	spsockets::close_socket(udp_sock);
     }
   
} /* end of namespace. */
