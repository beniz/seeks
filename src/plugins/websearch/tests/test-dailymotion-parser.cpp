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
 *
 * Mediawiki parser by Bram
 **/

#include "se_parser_dailymotion.h"

#include <string.h>
#include <fstream>
#include <iostream>

using namespace seeks_plugins;

int main(int argc, char **argv)
{
   if (argc < 2)
     {
        std::cout << "Usage: test_dailymotion_parser <html_page>\n";
        exit(0);
     }

   const char *htmlpage = argv[1];

   char *buffer;
   std::ifstream ifs;
   long offset = 0;
   long length = -1;
   ifs.open(htmlpage,std::ios::binary);

   if (!ifs.is_open())
     {
        std::cout << "[Error]: can't find file.\n";
        return -1;
     }

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

   //std::cout << "buffer: " << buffer << std::endl;

   se_parser_dailymotion spb;
   std::vector<search_snippet*> snippets;
   spb.parse_output_xml(buffer,&snippets, 0);

   std::cout << "snippets size: " << snippets.size() << std::endl;

   for (size_t i=0;i<snippets.size();i++)
     snippets.at(i)->print(std::cout);
}
