/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2009, 2010  Emmanuel Benazera, juban@free.fr
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

#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include "dht_err.h"
#include "dht_exception.h"
#include "NetAddress.h"

namespace dht
{
   class rpc_server
     {
      public:
	rpc_server(const std::string &hostname, const short &port);
	
	virtual ~rpc_server();
	
	dht_err run();

	/*- server responses. -*/
	virtual dht_err serve_response(const std::string &msg,
				       std::string &resp_msg);
		
      public:
	NetAddress _na;
     };
   
   /*- exceptions. -*/
   class rpc_server_msg_error_exception : public dht_exception
     {
      public:
	rpc_server_msg_error_exception()
	  :dht_exception()
	    {
	       _message = "received malformed or malserialized message";
	    };
	virtual ~rpc_server_msg_error_exception() {};
     };
   
   class rpc_server_wrong_layer_exception : public dht_exception
     {
      public:
	rpc_server_wrong_layer_exception()
	  :dht_exception()
	    {
	       _message = "received another layer's message";
	    };
	virtual ~rpc_server_wrong_layer_exception() {};
     };
   
} /* end of namespace. */

#endif
