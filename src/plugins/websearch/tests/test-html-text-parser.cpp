/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 **/
 
#include "html_txt_parser.h"

#include <string.h>
#include <fstream>
#include <iostream>

using namespace seeks_plugins;

int main(int argc, char **argv)
{
   if (argc < 2)
     {
	std::cout << "Usage: test_html-text_parser <html_page>\n";
	exit(0);
     }
   
   const char *htmlpage = argv[1];
   
   char *buffer;
   std::ifstream ifs;
   long offset = 0;
   long length = -1;
   ifs.open(htmlpage,std::ios::binary);
   if (!ifs.is_open())
     return -1;
   
   ifs.seekg (0, std::ios::end);
   length = ((long)ifs.tellg()) - offset;
   
   if(length==0)
     {
	ifs.close();
	return 0;
     }
   
   ifs.seekg (offset, std::ios::beg);
   buffer = new char[length];
   ifs.read (buffer, length);
   ifs.close();
   
   html_txt_parser htp;
   htp.parse_output(buffer,NULL,0);
   
   std::cout << htp.get_txt_nocopy() << std::endl;
}

   
