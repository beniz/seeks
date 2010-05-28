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
 
typedef int dht_err;

#define DHT_ERR_OK                                 0 /**< Success, no error */
#define DHT_ERR_MEMORY                             1 /**< Out of memory */
#define DHT_ERR_NETWORK                            2 /**< Network communication error */
#define DHT_ERR_COMLEVEL                           3 /**< wrong communication level */
#define DHT_ERR_UNKNOWN_PEER                       4 /**< unknown peer in lookup */
#define DHT_ERR_NO_SUCCESSOR_FOUND                 5 /**< can't find a successor to a peer. */
#define DHT_ERR_NO_PREDECESSOR_FOUND               6 /**< can't find a predecessor to a peer. */
#define DHT_ERR_UNKNOWN_PEER_LOCATION              7 /**< can't find a net address for a peer. */
#define DHT_ERR_COM_TIMEOUT                        8 /**< a communication with a peer has timed out. */
#define DHT_ERR_PTHREAD                            9 /**< thread creation error. */
#define DHT_ERR_CALL                              10 /**< call error. */
#define DHT_ERR_RESPONSE                          11 /**< response error. */
#define DHT_ERR_BOOTSTRAP                         12 /**< bootstrap error. */
#define DHT_ERR_SOCKET                            13 /**< socket error. */
#define DHT_ERR_HOST                              14 /**< unknown host or name resolution failure. */
#define DHT_ERR_MSG                               15 /**< msg (serialization/deserialization) error. */
#define DHT_ERR_CALLBACK                          16 /**< unknown callback function. */
#define DHT_ERR_RETRY                             17 /**< error mark while retrying (a call, ...). */
#define DHT_ERR_MAXHOPS                           18 /**< max number of hops reached in key lookup. */
#define DHT_ERR_UNSPECIFIED_SEARCHGROUP           19 /**< unspecified searchgroup key in lookup. */
#define DHT_ERR_UNKNOWN                         1000 /**< unknown error. */
