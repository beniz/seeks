/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "rdbl_pl.h"

#include <stdlib.h>
#include <iostream>

using namespace sp;
using namespace seeks_plugins;

std::string get_usage()
{
  std::string usage = "Usage: <url> (<encoding>)\n";
  return usage;
}

int main(int argc, char **argv)
{
  if (argc < 2)
    {
      std::cout << get_usage();
      exit(0);
    }

  std::string url = argv[1];
  std::string encoding;
  if (argc > 2)
    encoding = argv[2];
  std::string content;
  sp_err err = rdbl_pl::fetch_url_call_readable(url,content,encoding);
  if (err == SP_ERR_OK)
    std::cout << content;
  else std::cout << "Error: " << err << std::endl;
}
