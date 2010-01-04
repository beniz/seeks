/*********************************************************************
 * Purpose     :  Contains functions to connect to a server, possibly
 *                using a "forwarder" (i.e. HTTP proxy and/or a SOCKS4
 *                or SOCKS5 proxy).
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 * 
 *                Written by and Copyright (C) 2001-2009 the SourceForge
 *                Privoxy team. http://www.privoxy.org/
 *
 *                Based on the Internet Junkbuster originally written
 *                by and Copyright (C) 1997 Anonymous Coders and
 *                Junkbusters Corporation.  http://www.junkbusters.com
 *
 *                This program is free software; you can redistribute it
 *                and/or modify it under the terms of the GNU General
 *                Public License as published by the Free Software
 *                Foundation; either version 2 of the License, or (at
 *                your option) any later version.
 *
 *                This program is distributed in the hope that it will
 *                be useful, but WITHOUT ANY WARRANTY; without even the
 *                implied warranty of MERCHANTABILITY or FITNESS FOR A
 *                PARTICULAR PURPOSE.  See the GNU General Public
 *                License for more details.
 *
 *                The GNU General Public License should be included with
 *                this file.  If not, you can view it at
 *                http://www.gnu.org/copyleft/gpl.html
 *                or write to the Free Software Foundation, Inc., 59
 *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <sys/types.h>

#ifndef _WIN32
#include <netinet/in.h>
#endif

#include <errno.h>
#include <string.h>
#include "assert.h"

#ifdef _WIN32
#include <winsock2.h>
#endif /* def _WIN32 */

#include "proxy_dts.h"
#include "seeks_proxy.h"
#include "errlog.h"
#include "spsockets.h"
#include "gateway.h"
#include "miscutil.h"
#include "proxy_configuration.h"
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
#ifdef HAVE_POLL
#ifdef __GLIBC__ 
#include <sys/poll.h>
#else
#include <poll.h>
#endif /* def __GLIBC__ */
#endif /* HAVE_POLL */
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

namespace sp
{

#define SOCKS_REQUEST_GRANTED          90
#define SOCKS_REQUEST_REJECT           91
#define SOCKS_REQUEST_IDENT_FAILED     92
#define SOCKS_REQUEST_IDENT_CONFLICT   93

#define SOCKS5_REQUEST_GRANTED             0
#define SOCKS5_REQUEST_FAILED              1
#define SOCKS5_REQUEST_DENIED              2
#define SOCKS5_REQUEST_NETWORK_UNREACHABLE 3
#define SOCKS5_REQUEST_HOST_UNREACHABLE    4
#define SOCKS5_REQUEST_CONNECTION_REFUSED  5
#define SOCKS5_REQUEST_TTL_EXPIRED         6
#define SOCKS5_REQUEST_PROTOCOL_ERROR      7
#define SOCKS5_REQUEST_BAD_ADDRESS_TYPE    8

const char gateway::_socks_userid[] = "anonymous";

#ifdef FEATURE_CONNECTION_KEEP_ALIVE

unsigned int gateway::_keep_alive_timeout = DEFAULT_KEEP_ALIVE_TIMEOUT;

reusable_connection gateway::_reusable_connection[MAX_REUSABLE_CONNECTIONS];

/*********************************************************************
 *
 * Function    :  initialize_reusable_connections
 *
 * Description :  Initializes the reusable_connection structures.
 *                Must be called with connection_reuse_mutex locked.
 *
 * Parameters  : N/A
 *
 * Returns     : void
 *
 *********************************************************************/
void gateway::initialize_reusable_connections(void)
{
   unsigned int slot = 0;

#if !defined(HAVE_POLL) && !defined(_WIN32)
   errlog::log_error(LOG_LEVEL_INFO,
      "Detecting already dead connections might not work "
      "correctly on your platform. In case of problems, "
      "unset the keep-alive-timeout option.");
#endif

   for (slot = 0; slot < SZ(gateway::_reusable_connection); slot++)
   {
      gateway::mark_connection_closed(&gateway::_reusable_connection[slot]);
   }

   errlog::log_error(LOG_LEVEL_CONNECT, "Initialized %d socket slots.", slot);
}


/*********************************************************************
 *
 * Function    :  remember_connection
 *
 * Description :  Remembers a connection for reuse later on.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  fwd = The forwarder settings used.
 *
 * Returns     : void
 *
 *********************************************************************/
void gateway::remember_connection(const client_state *csp, const forward_spec *fwd)
{
   unsigned int slot = 0;
   int free_slot_found = FALSE;
   const reusable_connection *connection = &csp->_server_connection;
   const http_request *http = &csp->_http;

   assert(connection->_sfd != SP_INVALID_SOCKET);

   if (gateway::mark_connection_unused(connection))
   {
      return;
   }

   seeks_proxy::mutex_lock(&seeks_proxy::_connection_reuse_mutex);

   /* Find free socket slot. */
   for (slot = 0; slot < SZ(gateway::_reusable_connection); slot++)
   {
      if (gateway::_reusable_connection[slot]._sfd == SP_INVALID_SOCKET)
      {
         assert(gateway::_reusable_connection[slot]._in_use == 0);
         errlog::log_error(LOG_LEVEL_CONNECT,
            "Remembering socket %d for %s:%d in slot %d.",
            connection->_sfd, http->_host, http->_port, slot);
         free_slot_found = TRUE;
         break;
      }
   }

   if (!free_slot_found)
   {
      errlog::log_error(LOG_LEVEL_CONNECT,
        "No free slots found to remembering socket for %s:%d. Last slot %d.",
        http->_host, http->_port, slot);
      seeks_proxy::mutex_unlock(&seeks_proxy::_connection_reuse_mutex);
      spsockets::close_socket(connection->_sfd);
      return;
   }

   assert(NULL != http->_host);
   gateway::_reusable_connection[slot]._host = strdup(http->_host);
   if (NULL == gateway::_reusable_connection[slot]._host)
   {
      errlog::log_error(LOG_LEVEL_FATAL, "Out of memory saving socket.");
   }
   gateway::_reusable_connection[slot]._sfd = connection->_sfd;
   gateway::_reusable_connection[slot]._port = http->_port;
   gateway::_reusable_connection[slot]._in_use = 0;
   gateway::_reusable_connection[slot]._timestamp = connection->_timestamp;
   gateway::_reusable_connection->_request_sent = connection->_request_sent;
   gateway::_reusable_connection->_response_received = connection->_response_received;
   gateway::_reusable_connection[slot]._keep_alive_timeout = connection->_keep_alive_timeout;

   assert(NULL != fwd);
   assert(gateway::_reusable_connection[slot]._gateway_host == NULL);
   assert(gateway::_reusable_connection[slot]._gateway_port == 0);
   assert(gateway::_reusable_connection[slot]._forwarder_type == SOCKS_NONE);
   assert(gateway::_reusable_connection[slot]._forward_host == NULL);
   assert(gateway::_reusable_connection[slot]._forward_port == 0);

   gateway::_reusable_connection[slot]._forwarder_type = fwd->_type;
   if (NULL != fwd->_gateway_host)
   {
      gateway::_reusable_connection[slot]._gateway_host = strdup(fwd->_gateway_host);
      if (NULL == gateway::_reusable_connection[slot]._gateway_host)
      {
         errlog::log_error(LOG_LEVEL_FATAL, "Out of memory saving gateway_host.");
      }
   }
   else
   {
      gateway::_reusable_connection[slot]._gateway_host = NULL;
   }
   gateway::_reusable_connection[slot]._gateway_port = fwd->_gateway_port;

   if (NULL != fwd->_forward_host)
   {
      gateway::_reusable_connection[slot]._forward_host = strdup(fwd->_forward_host);
      if (NULL == gateway::_reusable_connection[slot]._forward_host)
      {
         errlog::log_error(LOG_LEVEL_FATAL, "Out of memory saving forward_host.");
      }
   }
   else
   {
      gateway::_reusable_connection[slot]._forward_host = NULL;
   }
   gateway::_reusable_connection[slot]._forward_port = fwd->_forward_port;

   seeks_proxy::mutex_unlock(&seeks_proxy::_connection_reuse_mutex);
}


/*********************************************************************
 *
 * Function    :  mark_connection_closed
 *
 * Description : Marks a reused connection closed.
 *
 * Parameters  :
 *          1  :  closed_connection = The connection to mark as closed.
 *
 * Returns     : void
 *
 *********************************************************************/
void gateway::mark_connection_closed(reusable_connection *closed_connection)
{
   closed_connection->_in_use = FALSE;
   closed_connection->_sfd = SP_INVALID_SOCKET;
   freez(closed_connection->_host);
   closed_connection->_host = NULL;
   closed_connection->_port = 0;
   closed_connection->_timestamp = 0;
   closed_connection->_request_sent = 0;
   closed_connection->_response_received = 0;
   closed_connection->_keep_alive_timeout = 0;
   closed_connection->_forwarder_type = SOCKS_NONE;
   freez(closed_connection->_gateway_host);
   closed_connection->_gateway_host = NULL;
   closed_connection->_gateway_port = 0;
   freez(closed_connection->_forward_host);
   closed_connection->_forward_host = NULL;
   closed_connection->_forward_port = 0;
}


/*********************************************************************
 *
 * Function    :  forget_connection
 *
 * Description :  Removes a previously remembered connection from
 *                the list of reusable connections.
 *
 * Parameters  :
 *          1  :  sfd = The socket belonging to the connection in question.
 *
 * Returns     : void
 *
 *********************************************************************/
void gateway::forget_connection(sp_socket sfd)
{
   unsigned int slot = 0;

   assert(sfd != SP_INVALID_SOCKET);

   seeks_proxy::mutex_lock(&seeks_proxy::_connection_reuse_mutex);

   for (slot = 0; slot < SZ(gateway::_reusable_connection); slot++)
   {
      if (gateway::_reusable_connection[slot]._sfd == sfd)
      {
         assert(gateway::_reusable_connection[slot]._in_use);

         errlog::log_error(LOG_LEVEL_CONNECT,
            "Forgetting socket %d for %s:%d in slot %d.",
            sfd, gateway::_reusable_connection[slot]._host,
            gateway::_reusable_connection[slot]._port, slot);
         gateway::mark_connection_closed(&gateway::_reusable_connection[slot]);
         seeks_proxy::mutex_unlock(&seeks_proxy::_connection_reuse_mutex);

         return;
      }
   }

   errlog::log_error(LOG_LEVEL_CONNECT,
      "Socket %d already forgotten or never remembered.", sfd);

   seeks_proxy::mutex_unlock(&seeks_proxy::_connection_reuse_mutex);
}


/*********************************************************************
 *
 * Function    :  connection_destination_matches
 *
 * Description :  Determines whether a remembered connection can
 *                be reused. That is, whether the destination and
 *                the forwarding settings match.
 *
 * Parameters  :
 *          1  :  connection = The connection to check.
 *          2  :  http = The destination for the connection.
 *          3  :  fwd  = The forwarder settings.
 *
 * Returns     :  TRUE for yes, FALSE otherwise.
 *
 *********************************************************************/
int gateway::connection_destination_matches(const reusable_connection *connection,
					    const http_request *http,
					    const forward_spec *fwd)
{
   if ((connection->_forwarder_type != fwd->_type)
    || (connection->_gateway_port   != fwd->_gateway_port)
    || (connection->_forward_port   != fwd->_forward_port)
    || (connection->_port           != http->_port))
   {
      return FALSE;
   }

   if ((    (NULL != connection->_gateway_host)
         && (NULL != fwd->_gateway_host)
	    && miscutil::strcmpic(connection->_gateway_host, fwd->_gateway_host))
       && (connection->_gateway_host != fwd->_gateway_host))
   {
      errlog::log_error(LOG_LEVEL_CONNECT, "Gateway mismatch.");
      return FALSE;
   }

   if ((    (NULL != connection->_forward_host)
         && (NULL != fwd->_forward_host)
	    && miscutil::strcmpic(connection->_forward_host, fwd->_forward_host))
      && (connection->_forward_host != fwd->_forward_host))
   {
      errlog::log_error(LOG_LEVEL_CONNECT, "Forwarding proxy mismatch.");
      return FALSE;
   }

   return (!miscutil::strcmpic(connection->_host, http->_host));
}


/*********************************************************************
 *
 * Function    :  close_unusable_connections
 *
 * Description :  Closes remembered connections that have timed
 *                out or have been closed on the other side.
 *
 * Parameters  :  none
 *
 * Returns     :  Number of connections that are still alive.
 *
 *********************************************************************/
int gateway::close_unusable_connections()
{
   unsigned int slot = 0;
   int connections_alive = 0;

   seeks_proxy::mutex_lock(&seeks_proxy::_connection_reuse_mutex);

   for (slot = 0; slot < SZ(gateway::_reusable_connection); slot++)
   {
      if (!gateway::_reusable_connection[slot]._in_use
         && (SP_INVALID_SOCKET != gateway::_reusable_connection[slot]._sfd))
      {
         time_t time_open = time(NULL) - gateway::_reusable_connection[slot]._timestamp;
         time_t latency = gateway::_reusable_connection[slot]._response_received -
            gateway::_reusable_connection[slot]._request_sent;

         if (gateway::_reusable_connection[slot]._keep_alive_timeout < time_open + latency)
         {
            errlog::log_error(LOG_LEVEL_CONNECT,
               "The connection to %s:%d in slot %d timed out. "
               "Closing socket %d. Timeout is: %d. Assumed latency: %d",
               gateway::_reusable_connection[slot]._host,
               gateway::_reusable_connection[slot]._port, slot,
               gateway::_reusable_connection[slot]._sfd,
               gateway::_reusable_connection[slot]._keep_alive_timeout,
               latency);
            spsockets::close_socket(gateway::_reusable_connection[slot]._sfd);
            gateway::mark_connection_closed(&gateway::_reusable_connection[slot]);
         }
         else if (!spsockets::socket_is_still_usable(gateway::_reusable_connection[slot]._sfd))
         {
            errlog::log_error(LOG_LEVEL_CONNECT,
               "The connection to %s:%d in slot %d is no longer usable. "
               "Closing socket %d.", gateway::_reusable_connection[slot]._host,
               gateway::_reusable_connection[slot]._port, slot,
               gateway::_reusable_connection[slot]._sfd);
            spsockets::close_socket(gateway::_reusable_connection[slot]._sfd);
            gateway::mark_connection_closed(&gateway::_reusable_connection[slot]);
         }
         else
         {
            connections_alive++;
         }
      }
   }

   seeks_proxy::mutex_unlock(&seeks_proxy::_connection_reuse_mutex);

   return connections_alive;
}


/*********************************************************************
 *
 * Function    :  get_reusable_connection
 *
 * Description :  Returns an open socket to a previously remembered
 *                open connection (if there is one).
 *
 * Parameters  :
 *          1  :  http = The destination for the connection.
 *          2  :  fwd  = The forwarder settings.
 *
 * Returns     :  SP_INVALID_SOCKET => No reusable connection found,
 *                otherwise a usable socket.
 *
 *********************************************************************/
sp_socket gateway::get_reusable_connection(const http_request *http,
					   const forward_spec *fwd)
{
   sp_socket sfd = SP_INVALID_SOCKET;
   unsigned int slot = 0;

   gateway::close_unusable_connections();

   seeks_proxy::mutex_lock(&seeks_proxy::_connection_reuse_mutex);

   for (slot = 0; slot < SZ(gateway::_reusable_connection); slot++)
   {
      if (!gateway::_reusable_connection[slot]._in_use
         && (SP_INVALID_SOCKET != gateway::_reusable_connection[slot]._sfd))
      {
         if (gateway::connection_destination_matches(&gateway::_reusable_connection[slot], http, fwd))
         {
            gateway::_reusable_connection[slot]._in_use = TRUE;
            sfd = gateway::_reusable_connection[slot]._sfd;
            errlog::log_error(LOG_LEVEL_CONNECT,
               "Found reusable socket %d for %s:%d in slot %d.",
               sfd, gateway::_reusable_connection[slot]._host, gateway::_reusable_connection[slot]._port, slot);
            break;
         }
      }
   }

   seeks_proxy::mutex_unlock(&seeks_proxy::_connection_reuse_mutex);

   return sfd;
}


/*********************************************************************
 *
 * Function    :  mark_connection_unused
 *
 * Description :  Gives a remembered connection free for reuse.
 *
 * Parameters  :
 *          1  :  connection = The connection in question.
 *
 * Returns     :  TRUE => Socket found and marked as unused.
 *                FALSE => Socket not found.
 *
 *********************************************************************/
int gateway::mark_connection_unused(const reusable_connection *connection)
{
   unsigned int slot = 0;
   int socket_found = FALSE;

   assert(connection->_sfd != SP_INVALID_SOCKET);

   seeks_proxy::mutex_lock(&seeks_proxy::_connection_reuse_mutex);

   for (slot = 0; slot < SZ(gateway::_reusable_connection); slot++)
   {
      if (gateway::_reusable_connection[slot]._sfd == connection->_sfd)
      {
         assert(gateway::_reusable_connection[slot]._in_use);
         socket_found = TRUE;
         errlog::log_error(LOG_LEVEL_CONNECT,
            "Marking open socket %d for %s:%d in slot %d as unused.",
            connection->_sfd, gateway::_reusable_connection[slot]._host,
            gateway::_reusable_connection[slot]._port, slot);
         gateway::_reusable_connection[slot]._in_use = 0;
         gateway::_reusable_connection[slot]._timestamp = connection->_timestamp;
         break;
      }
   }

   seeks_proxy::mutex_unlock(&seeks_proxy::_connection_reuse_mutex);

   return socket_found;
}


/*********************************************************************
 *
 * Function    :  set_keep_alive_timeout
 *
 * Description :  Sets the timeout after which open
 *                connections will no longer be reused.
 *
 * Parameters  :
 *          1  :  timeout = The timeout in seconds.
 *
 * Returns     :  void
 *
 *********************************************************************/
void gateway::set_keep_alive_timeout(unsigned int timeout)
{
   gateway::_keep_alive_timeout = timeout;
}
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */


/*********************************************************************
 *
 * Function    :  forwarded_connect
 *
 * Description :  Connect to a specified web server, possibly via
 *                a HTTP proxy and/or a SOCKS proxy.
 *
 * Parameters  :
 *          1  :  fwd = the proxies to use when connecting.
 *          2  :  http = the http request and apropos headers
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_INVALID_SOCKET => failure, else it is the socket file descriptor.
 *
 *********************************************************************/
sp_socket gateway::forwarded_connect(const forward_spec * fwd,
				     http_request *http,
				     client_state *csp)
{
   const char * dest_host;
   int dest_port;
   sp_socket sfd = SP_INVALID_SOCKET;

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   if ((csp->_config->_feature_flags & RUNTIME_FEATURE_CONNECTION_SHARING)
      && !(csp->_flags & CSP_FLAG_SERVER_SOCKET_TAINTED))
   {
      sfd = gateway::get_reusable_connection(http, fwd);
      if (SP_INVALID_SOCKET != sfd)
      {
         return sfd;
      }
   }
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

   /* Figure out if we need to connect to the web server or a HTTP proxy. */
   if (fwd && fwd->_forward_host)
   {
      /* HTTP proxy */
      dest_host = fwd->_forward_host;
      dest_port = fwd->_forward_port;
   }
   else
   {
      /* Web server */
      dest_host = http->_host;
      dest_port = http->_port;
   }

   /* Connect, maybe using a SOCKS proxy */
   switch (fwd->_type)
   {
      case SOCKS_NONE:
      sfd = spsockets::connect_to(dest_host, dest_port, csp);
         break;
      case SOCKS_4:
      case SOCKS_4A:
      sfd = gateway::socks4_connect(fwd, dest_host, dest_port, csp);
         break;
      case SOCKS_5:
      sfd = gateway::socks5_connect(fwd, dest_host, dest_port, csp);
         break;
      default:
         /* Should never get here */
      errlog::log_error(LOG_LEVEL_FATAL,
            "SOCKS4 impossible internal error - bad SOCKS type.");
   }

   if (SP_INVALID_SOCKET != sfd)
   {
      errlog::log_error(LOG_LEVEL_CONNECT,
         "Created new connection to %s:%d on socket %d.",
         http->_host, http->_port, sfd);
   }

   return sfd;
}


/*********************************************************************
 *
 * Function    :  socks4_connect
 *
 * Description :  Connect to the SOCKS server, and connect through
 *                it to the specified server.   This handles
 *                all the SOCKS negotiation, and returns a file
 *                descriptor for a socket which can be treated as a
 *                normal (non-SOCKS) socket.
 *
 *                Logged error messages are saved to csp->_error_message
 *                and later reused by error_response() for the CGI
 *                message. strdup allocation failures are handled there.
 *
 * Parameters  :
 *          1  :  fwd = Specifies the SOCKS proxy to use.
 *          2  :  target_host = The final server to connect to.
 *          3  :  target_port = The final port to connect to.
 *          4  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_INVALID_SOCKET => failure, else a socket file descriptor.
 *
 *********************************************************************/
sp_socket gateway::socks4_connect(const forward_spec * fwd,
				  const char * target_host,
				  int target_port,
				  client_state *csp)
{
   unsigned int web_server_addr;
   char buf[BUFFER_SIZE];
   socks_op    *c = (socks_op    *)buf;
   socks_reply *s = (socks_reply *)buf;
   size_t n;
   size_t csiz;
   sp_socket sfd;
   int err = 0;
   const char *errstr = NULL;

   if ((fwd->_gateway_host == NULL) || (*fwd->_gateway_host == '\0'))
   {
      /* XXX: Shouldn't the config file parser prevent this? */
      errstr = "NULL gateway host specified.";
      err = 1;
   }

   if (fwd->_gateway_port <= 0)
   {
      errstr = "invalid gateway port specified.";
      err = 1;
   }

   if (err)
   {
      errlog::log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
      csp->_error_message = strdup(errstr); 
      errno = EINVAL;
      return(SP_INVALID_SOCKET);
   }

   /* build a socks request for connection to the web server */

   strlcpy(&(c->_userid), gateway::_socks_userid, sizeof(buf) - sizeof(socks_op));

   csiz = sizeof(*c) + sizeof(gateway::_socks_userid) - sizeof(c->_userid) - sizeof(c->_padding);

   switch (fwd->_type)
   {
      case SOCKS_4:
         web_server_addr = spsockets::resolve_hostname_to_ip(target_host);
         if (web_server_addr == INADDR_NONE)
         {
            errstr = "could not resolve target host";
            errlog::log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s %s", errstr, target_host);
            err = 1;
         }
         else
         {
            web_server_addr = htonl(web_server_addr);
         }
         break;
      case SOCKS_4A:
         web_server_addr = 0x00000001;
         n = csiz + strlen(target_host) + 1;
         if (n > sizeof(buf))
         {
            errno = EINVAL;
            errstr = "buffer cbuf too small.";
            errlog::log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
            err = 1;
         }
         else
         {
            strlcpy(buf + csiz, target_host, sizeof(buf) - sizeof(socks_op) - csiz);
            /*
             * What we forward to the socks4a server should have the
             * size of socks_op, plus the length of the userid plus
             * its \0 byte (which we don't have to add because the
             * first byte of the userid is counted twice as it's also
             * part of sock_op) minus the padding bytes (which are part
             * of the userid as well), plus the length of the target_host
             * (which is stored csiz bytes after the beginning of the buffer),
             * plus another \0 byte.
             */
            assert(n == sizeof(socks_op) + strlen(&(c->_userid)) - sizeof(c->_padding) + strlen(buf + csiz) + 1);
            csiz = n;
         }
         break;
      default:
         /* Should never get here */
         errlog::log_error(LOG_LEVEL_FATAL,
            "socks4_connect: SOCKS4 impossible internal error - bad SOCKS type.");
         /* Not reached */
         return(SP_INVALID_SOCKET);
   }

   if (err)
   {
      csp->_error_message = strdup(errstr);
      return(SP_INVALID_SOCKET);
   }

   c->_vn          = 4;
   c->_cd          = 1;
   c->_dstport[0]  = (unsigned char)((target_port       >> 8  ) & 0xff);
   c->_dstport[1]  = (unsigned char)((target_port             ) & 0xff);
   c->_dstip[0]    = (unsigned char)((web_server_addr   >> 24 ) & 0xff);
   c->_dstip[1]    = (unsigned char)((web_server_addr   >> 16 ) & 0xff);
   c->_dstip[2]    = (unsigned char)((web_server_addr   >>  8 ) & 0xff);
   c->_dstip[3]    = (unsigned char)((web_server_addr         ) & 0xff);

   /* pass the request to the socks server */
   sfd = spsockets::connect_to(fwd->_gateway_host, fwd->_gateway_port, csp);

   if (sfd == SP_INVALID_SOCKET)
   {
      /*
       * XXX: connect_to should fill in the exact reason.
       * Most likely resolving the IP of the forwarder failed.
       */
      errstr = "connect_to failed: see logfile for details";
      err = 1;
   }
   else if (spsockets::write_socket(sfd, (char *)c, csiz))
   {
      errstr = "SOCKS4 negotiation write failed.";
      errlog::log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
      err = 1;
      spsockets::close_socket(sfd);
   }
   else if (spsockets::read_socket(sfd, buf, sizeof(buf)) != sizeof(*s))
   {
      errstr = "SOCKS4 negotiation read failed.";
      errlog::log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
      err = 1;
      spsockets::close_socket(sfd);
   }

   if (err)
   {
      csp->_error_message = strdup(errstr);      
      return(SP_INVALID_SOCKET);
   }

   switch (s->_cd)
   {
      case SOCKS_REQUEST_GRANTED:
         return(sfd);
      case SOCKS_REQUEST_REJECT:
         errstr = "SOCKS request rejected or failed.";
         errno = EINVAL;
         break;
      case SOCKS_REQUEST_IDENT_FAILED:
         errstr = "SOCKS request rejected because "
            "SOCKS server cannot connect to identd on the client.";
         errno = EACCES;
         break;
      case SOCKS_REQUEST_IDENT_CONFLICT:
         errstr = "SOCKS request rejected because "
            "the client program and identd report "
            "different user-ids.";
         errno = EACCES;
         break;
      default:
         errno = ENOENT;
         snprintf(buf, sizeof(buf),
            "SOCKS request rejected for reason code %d.", s->_cd);
         errstr = buf;
   }

   errlog::log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
   csp->_error_message = strdup(errstr);
   spsockets::close_socket(sfd);

   return(SP_INVALID_SOCKET);
}

/*********************************************************************
 *
 * Function    :  translate_socks5_error
 *
 * Description :  Translates a SOCKS errors to a string.
 *
 * Parameters  :
 *          1  :  socks_error = The error code to translate.
 *
 * Returns     :  The string translation.
 *
 *********************************************************************/
const char* gateway::translate_socks5_error(int socks_error)
{
   switch (socks_error)
   {
      /* XXX: these should be more descriptive */
      case SOCKS5_REQUEST_FAILED:
         return "SOCKS5 request failed";
      case SOCKS5_REQUEST_DENIED:
         return "SOCKS5 request denied";
      case SOCKS5_REQUEST_NETWORK_UNREACHABLE:
         return "SOCKS5 network unreachable";
      case SOCKS5_REQUEST_HOST_UNREACHABLE:
         return "SOCKS5 host unreachable";
      case SOCKS5_REQUEST_CONNECTION_REFUSED:
         return "SOCKS5 connection refused";
      case SOCKS5_REQUEST_TTL_EXPIRED:
         return "SOCKS5 TTL expired";
      case SOCKS5_REQUEST_PROTOCOL_ERROR:
         return "SOCKS5 client protocol error";
      case SOCKS5_REQUEST_BAD_ADDRESS_TYPE:
         return "SOCKS5 domain names unsupported";
      case SOCKS5_REQUEST_GRANTED:
         return "everything's peachy";
      default:
         return "SOCKS5 negotiation protocol error";
   }
}

/*********************************************************************
 *
 * Function    :  socks5_connect
 *
 * Description :  Connect to the SOCKS server, and connect through
 *                it to the specified server.   This handles
 *                all the SOCKS negotiation, and returns a file
 *                descriptor for a socket which can be treated as a
 *                normal (non-SOCKS) socket.
 *
 * Parameters  :
 *          1  :  fwd = Specifies the SOCKS proxy to use.
 *          2  :  target_host = The final server to connect to.
 *          3  :  target_port = The final port to connect to.
 *          4  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_INVALID_SOCKET => failure, else a socket file descriptor.
 *
 *********************************************************************/
sp_socket gateway::socks5_connect(const forward_spec *fwd,
				  const char *target_host,
				  int target_port,
				  client_state *csp)
{
   int err = 0;
   char cbuf[300];
   char sbuf[30];
   size_t client_pos = 0;
   int server_size = 0;
   size_t hostlen = 0;
   sp_socket sfd;
   const char *errstr = NULL;

   assert(fwd->_gateway_host);
   if ((fwd->_gateway_host == NULL) || (*fwd->_gateway_host == '\0'))
   {
      errstr = "NULL gateway host specified";
      err = 1;
   }

   if (fwd->_gateway_port <= 0)
   {
      /*
       * XXX: currently this can't happen because in
       * case of invalid gateway ports we use the defaults.
       * Of course we really shouldn't do that.
       */
      errstr = "invalid gateway port specified";
      err = 1;
   }

   hostlen = strlen(target_host);
   if (hostlen > (size_t)255)
   {
      errstr = "target host name is longer than 255 characters";
      err = 1;
   }

   if (fwd->_type != SOCKS_5)
   {
      /* Should never get here */
      errlog::log_error(LOG_LEVEL_FATAL,
         "SOCKS5 impossible internal error - bad SOCKS type");
      err = 1;
   }

   if (err)
   {
      errno = EINVAL;
      assert(errstr != NULL);
      errlog::log_error(LOG_LEVEL_CONNECT, "socks5_connect: %s", errstr);
      csp->_error_message = strdup(errstr);
      return(SP_INVALID_SOCKET);
   }

   /* pass the request to the socks server */
   sfd = spsockets::connect_to(fwd->_gateway_host, fwd->_gateway_port, csp);

   if (sfd == SP_INVALID_SOCKET)
   {
      errstr = "socks5 server unreachable";
      errlog::log_error(LOG_LEVEL_CONNECT, "socks5_connect: %s", errstr);
      csp->_error_message = strdup(errstr);
      return(SP_INVALID_SOCKET);
   }

   client_pos = 0;
   cbuf[client_pos++] = '\x05'; /* Version */
   cbuf[client_pos++] = '\x01'; /* One authentication method supported */
   cbuf[client_pos++] = '\x00'; /* The no authentication authentication method */

   if (spsockets::write_socket(sfd, cbuf, client_pos))
   {
      errstr = "SOCKS5 negotiation write failed";
      csp->_error_message = strdup(errstr);
      errlog::log_error(LOG_LEVEL_CONNECT, "%s", errstr);
      spsockets::close_socket(sfd);
      return(SP_INVALID_SOCKET);
   }

   if (spsockets::read_socket(sfd, sbuf, sizeof(sbuf)) != 2)
   {
      errstr = "SOCKS5 negotiation read failed";
      err = 1;
   }

   if (!err && (sbuf[0] != '\x05'))
   {
      errstr = "SOCKS5 negotiation protocol version error";
      err = 1;
   }

   if (!err && (sbuf[1] == '\xff'))
   {
      errstr = "SOCKS5 authentication required";
      err = 1;
   }

   if (!err && (sbuf[1] != '\x00'))
   {
      errstr = "SOCKS5 negotiation protocol error";
      err = 1;
   }

   if (err)
   {
      assert(errstr != NULL);
      errlog::log_error(LOG_LEVEL_CONNECT, "socks5_connect: %s", errstr);
      csp->_error_message = strdup(errstr);
      spsockets::close_socket(sfd);
      errno = EINVAL;
      return(SP_INVALID_SOCKET);
   }

   client_pos = 0;
   cbuf[client_pos++] = '\x05'; /* Version */
   cbuf[client_pos++] = '\x01'; /* TCP connect */
   cbuf[client_pos++] = '\x00'; /* Reserved, must be 0x00 */
   cbuf[client_pos++] = '\x03'; /* Address is domain name */
   cbuf[client_pos++] = (char)(hostlen & 0xffu);
   assert(sizeof(cbuf) - client_pos > (size_t)255);
   /* Using strncpy because we really want the nul byte padding. */
   strncpy(cbuf + client_pos, target_host, sizeof(cbuf) - client_pos);
   client_pos += (hostlen & 0xffu);
   cbuf[client_pos++] = (char)((target_port >> 8) & 0xff);
   cbuf[client_pos++] = (char)((target_port     ) & 0xff);

   if (spsockets::write_socket(sfd, cbuf, client_pos))
   {
      errstr = "SOCKS5 negotiation read failed";
      csp->_error_message = strdup(errstr);
      errlog::log_error(LOG_LEVEL_CONNECT, "%s", errstr);
      spsockets::close_socket(sfd);
      errno = EINVAL;
      return(SP_INVALID_SOCKET);
   }

   server_size = spsockets::read_socket(sfd, sbuf, sizeof(sbuf));
   if (server_size < 3)
   {
      errstr = "SOCKS5 negotiation read failed";
      err = 1;
   }
   else if (server_size > 20)
   {
      /* This is somewhat unexpected but doesn't realy matter. */
      errlog::log_error(LOG_LEVEL_CONNECT, "socks5_connect: read %d bytes "
         "from socks server. Would have accepted up to %d.",
         server_size, sizeof(sbuf));
   }

   if (!err && (sbuf[0] != '\x05'))
   {
      errstr = "SOCKS5 negotiation protocol version error";
      err = 1;
   }

   if (!err && (sbuf[2] != '\x00'))
   {
      errstr = "SOCKS5 negotiation protocol error";
      err = 1;
   }

   if (!err)
   {
      if (sbuf[1] == SOCKS5_REQUEST_GRANTED)
      {
         return(sfd);
      }
      errstr = translate_socks5_error(sbuf[1]);
   }

   assert(errstr != NULL);
   csp->_error_message = strdup(errstr);
   errlog::log_error(LOG_LEVEL_CONNECT, "socks5_connect: %s", errstr);
   spsockets::close_socket(sfd);
   errno = EINVAL;

   return(SP_INVALID_SOCKET);
}

} /* end of namespace. */
