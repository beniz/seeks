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
  
#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include "dht_err.h"
#include "dht_exception.h"
#include "NetAddress.h"

namespace dht
{
   class rpc_client
     {
      public:
	rpc_client();
	
	virtual ~rpc_client();
	  
	dht_err do_rpc_call(const NetAddress &server_na,
			    const std::string &msg,
			    const bool &need_response,
			    std::string &response);
     };
   
   /*- exceptions. -*/
   class rpc_client_socket_error_exception : public dht_exception
     {
      public:
	rpc_client_socket_error_exception()
	  :dht_exception()
	    {
	       _message = "rpc client socket error";
	    };
	virtual ~rpc_client_socket_error_exception() {};
     };
   	
   class rpc_client_host_error_exception : public dht_exception
     {
      public:
	rpc_client_host_error_exception(const std::string &host_str)
	  :dht_exception()
	    {
	       _message = "rpc client unknown host: " + host_str;
	    };
	virtual ~rpc_client_host_error_exception() {};
     };
   
   class rpc_client_sending_error_exception : public dht_exception
     {
      public:
	rpc_client_sending_error_exception()
	  :dht_exception()
	    {
	       _message = "rpc client sending error";
	    }
	virtual ~rpc_client_sending_error_exception() {};
     };
   
   class rpc_client_reception_error_exception : public dht_exception
     {
      public:
	rpc_client_reception_error_exception()
	  :dht_exception()
	    {
	       _message = "rpc client sending error";
	    }
	virtual ~rpc_client_reception_error_exception() {};
     };
   
} /* end of namespace. */

#endif
