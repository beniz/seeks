/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, <ebenazer@seeks-project.info>
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

#include "se_parser_osearch.h"

#include <string.h>
#include <fstream>
#include <iostream>

using namespace seeks_plugins;

int main(int argc, char **argv)
{
  if (argc < 3)
    {
      std::cout << "Usage: test_osearch_parser <html_page> <rss or atom>\n";
      exit(0);
    }

  const char *htmlpage = argv[1];
  const char *type = argv[2];

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
  if (length==0)
    {
      ifs.close();
      return 0;
    }

  ifs.seekg (offset, std::ios::beg);
  buffer = new char[length];
  ifs.read (buffer, length);
  ifs.close();


  std::vector<search_snippet*> snippets;

  if (strcasecmp(type,"atom") == 0)
    {
      se_parser_osearch_atom spos;
      spos.parse_output(buffer,&snippets,0);
    }
  else if (strcasecmp(type,"rss") == 0)
    {
      se_parser_osearch_rss spos;
      spos.parse_output(buffer,&snippets,0);
    }
  std::cout << "snippets size: " << snippets.size() << std::endl;

  for (size_t i=0; i<snippets.size(); i++)
    snippets.at(i)->print(std::cout);
}
