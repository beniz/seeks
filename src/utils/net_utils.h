/**
 * This file is part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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
  
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
        
long mac_addr_sys(u_char *addr)
{   
   struct ifreq ifr;   
   struct ifreq *IFR;
   struct ifconf ifc;
   char buf[1024];
   int s, i;
   int ok = 0;
   
   s = socket(AF_INET, SOCK_DGRAM, 0);
   if (s==-1)
     {
	return -1;
     }
      
   ifc.ifc_len = sizeof(buf);
   ifc.ifc_buf = buf;
   ioctl(s, SIOCGIFCONF, &ifc);
   
   IFR = ifc.ifc_req;
   for (i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; IFR++)
     {
	strcpy(ifr.ifr_name, IFR->ifr_name);
	if (ioctl(s, SIOCGIFFLAGS, &ifr) == 0)
	  {
	     if (! (ifr.ifr_flags & IFF_LOOPBACK))
	       {
		  if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0)
		    {
		       ok = 1;
		       break;
		    }
	       }
	  }
     }
   close(s);
   
   if (ok)
     {
	bcopy( ifr.ifr_hwaddr.sa_data, addr, 6);
     }
   else
     {
	return -1;
     }
   
   return 0;
}
