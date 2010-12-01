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
  
#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include "dht_err.h"
#include "dht_exception.h"
#include "NetAddress.h"

#include <string>

namespace dht
{
   class rpc_client;
   
   /* class rpc_call_args
     {
      public:
	rpc_call_args(rpc_client *client,
		      const NetAddress &server_na, const std::string &msg,
		      const bool &need_response, std::string &response)
	  :_client(client),_server_na(server_na),_msg(msg),
	   _need_response(need_response),_response(response)
	  {
	     _err = DHT_ERR_OK;
	  };
	
	~rpc_call_args() {};
	
	rpc_client *_client;
	NetAddress _server_na;
	std::string _msg;
	bool _need_response;
	std::string _response;
	dht_err _err;
     }; */
      
   class rpc_client
     {
      public:
	rpc_client();
	
	virtual ~rpc_client();
	  
	/* static void do_rpc_call_static(rpc_call_args *args); */
	
	/* dht_err do_rpc_call_threaded(const NetAddress &server_na,
				     const std::string &msg,
				     const bool &need_response,
				     std::string &response); */
	
	/*dht_err do_rpc_call(const NetAddress &server_na,
			    const std::string &msg,
			    const bool &need_response,
			    std::string &response,
			    dht_err &err);*/
	
	void do_rpc_call(const NetAddress &server_na,
			 const std::string &msg,
			 const bool &need_response,
			 std::string &response);
     };
   
} /* end of namespace. */

#endif
