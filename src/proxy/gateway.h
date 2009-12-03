/*********************************************************************
 * Purpose     :  Contains functions to connect to a server, possibly
 *                using a "gateway" (i.e. HTTP proxy and/or SOCKS4
 *                proxy).  Also contains the list of gateway types.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 * 
 *                Written by and Copyright (C) 2001 the SourceForge
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

#ifndef GATEWAY_H
#define GATEWAY_H

#include "proxy_dts.h"

namespace sp
{

/* structure of a socks client operation */
class socks_op 
{
 public:
   socks_op() {}; // TODO.
   ~socks_op() {};
   
   unsigned char _vn;          /* socks version number */
   unsigned char _cd;          /* command code */
   unsigned char _dstport[2];  /* destination port */
   unsigned char _dstip[4];    /* destination address */
   char _userid;               /* first byte of userid */
   char _padding[3];           /* make sure sizeof(struct socks_op) is endian-independent. */
   /* more bytes of the userid follow, terminated by a NULL */
};

/* structure of a socks server reply */
class socks_reply 
{
 public:
   socks_reply() {}; // TODO.
   ~socks_reply() {}; //TODO.
   
   unsigned char _vn;          /* socks version number */
   unsigned char _cd;          /* command code */
   unsigned char _dstport[2];  /* destination port */
   unsigned char _dstip[4];    /* destination address */
};

class gateway
{
 public:

   static sp_socket forwarded_connect(const forward_spec * fwd, 
				      http_request *http, 
				      client_state *csp);
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   
   /*
    * Default number of seconds after which an
    * open connection will no longer be reused.
    */
#define DEFAULT_KEEP_ALIVE_TIMEOUT 180
   
#define MAX_REUSABLE_CONNECTIONS 100
   
   static void set_keep_alive_timeout(unsigned int timeout);
   static void initialize_reusable_connections(void);
   static void forget_connection(sp_socket sfd);
   static void remember_connection(const client_state *csp,
				   const forward_spec *fwd);
   static int close_unusable_connections(void);
   static void mark_connection_closed(reusable_connection *closed_connection);
   static int connection_destination_matches(const reusable_connection *connection,
					     const http_request *http,
					     const forward_spec *fwd);
   static int mark_connection_unused(const reusable_connection *connection);
   static sp_socket get_reusable_connection(const http_request *http,
					    const forward_spec *fwd);
   static unsigned int _keep_alive_timeout;
   static reusable_connection _reusable_connection[MAX_REUSABLE_CONNECTIONS];

#endif /* FEATURE_CONNECTION_KEEP_ALIVE */   
 
 private:
   static sp_socket socks4_connect(const forward_spec *fwd,
				   const char *target_host,
				   int target_port,
				   client_state *csp);
   
   static sp_socket socks5_connect(const forward_spec *fwd,
				   const char *target_host,
				   int target_port,
				   client_state *csp);
   
   static const char* translate_socks5_error(int socks_error);
   
   /*
    * Solaris fix
    */
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

 private:
   static const char _socks_userid[];
};
   
} /* end of namespace. */

#endif /* ndef GATEWAY_H */
