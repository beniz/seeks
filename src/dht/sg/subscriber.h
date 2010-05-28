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

#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include "DHTKey.h"
#include "NetAddress.h"

#include <time.h>

namespace dht
{
   
   class Subscriber : public NetAddress
     {
      public:
	/**
	 * \brief default object constructor.
	 */
	Subscriber();
	
	/**
	 * \brief constructor.
	 */
	Subscriber(const DHTKey &idkey,
		   const std::string &naddress, const short &port);
	
	/**
	 * \brief deserialize + constructor.
	 */
	Subscriber(const std::string &idkey,
		   const uint32_t &ip, const std::string &port);
	
	/**
	 * \brief copy constructor.
	 */
	Subscriber(const Subscriber &su);
	
	/**
	 * \brief destructor.
	 */
	~Subscriber();
	
	/**
	 * \brief time of day as join date.
	 */
	void set_join_date();
	
      public:
	DHTKey _idkey;  /**< subscriber's virtual node key on the ring. */
	time_t _join_date;  /**< date at which the subscriber did join.
			         Used for responding to peer list requests. */
     };
   
} /* end of namespace. */

#endif
