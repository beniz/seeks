/*********************************************************************
 * Purpose     :  Contains wrappers for system-specific sockets code,
 *                so that the rest of Junkbuster can be more
 *                OS-independent.  Contains #ifdefs to make this work
 *                on many platforms.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *                
 *                Written by and Copyright (C) 2001-2009 the
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

#ifndef SPSOCKETS_H
#define SPSOCKETS_H

#include "proxy_dts.h"

namespace sp
{
   class client_state;

   class spsockets
     {
      public:
	static sp_socket connect_to(const char *host, int portnum, client_state *csp);
	static int write_socket(sp_socket fd, const char *buf, size_t n);
	static int read_socket(sp_socket fd, char *buf, int n);
	static int data_is_available(sp_socket fd, int seconds_to_wait);
	static void close_socket(sp_socket fd);
	
	static int bind_port(const char *hostnam, int portnum, sp_socket *pfd);
	static int accept_connection(struct client_state * csp, sp_socket fd);
	static void get_host_information(sp_socket afd, char **ip_address, char **hostname);
	
	static unsigned long resolve_hostname_to_ip(const char *host);
	
	static int socket_is_still_usable(sp_socket sfd);
     };
   	
} /* end of namespace. */
   
#endif /* ndef SPSOCKETS_H */

