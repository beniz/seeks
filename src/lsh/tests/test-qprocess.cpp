/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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
  
#include "qprocess.h"

#include <iostream>

using namespace lsh;

int main(int argc, char *argv[])
{
   
   if (argc<4)
     {
	std::cout << "Usage: test_mrf_query_160 <query> <min_radius> <max_radius>\n";
	exit(0);
     }
   
   std::string query = std::string(argv[1]);
   int min_radius = atoi(argv[2]);
   int max_radius = atoi(argv[3]);

   hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
   qprocess::generate_query_hashes(query,min_radius,max_radius,features);
   
   hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit
     = features.begin();
   while(hit!=features.end())
     {
	std::cout << "radius=" << (*hit).first << " / " << (*hit).second.to_rstring() << std::endl;
	++hit;
     }
}
