/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
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

#include "net_utils.h"

int main( int argc, char **argv)
{
   long stat;
   int i;
   u_char addr[6];
   
   stat = mac_addr_sys(addr);
   if (0 == stat) 
     {
	printf( "MAC address = ");
	for (i=0; i<6; ++i) 
	  {
	     printf("%2.2x", addr[i]);
	  }
	
	printf( "\n");
     }
   else 
     {
	fprintf( stderr, "can't get MAC address: %i\n", stat);
	exit( 1);
     }
   return 0;
}

