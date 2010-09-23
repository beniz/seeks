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

#include "mrf.h"
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#define RMDsize 160

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

  std::vector<f160r> features;
  mrf::mrf_features_query(query,features,
			  min_radius,max_radius);
  
  std::cout << "[Result]: number of features: " << std::dec << features.size() << std::endl;
  std::cout << "[Result]: features as hashes:\n";
  for (size_t i=0;i<features.size();i++)
     {
       //std::cout << features[i] << std::endl;
	byte *hashcode = (byte*)features[i]._feat;
	for (unsigned int j=0; j<RMDsize/8; j++)
	  printf("%02x",hashcode[j]);
	std::cout << std::endl;
	
	/* std::string conv = DHTKey::convert(hashcode).to_rstring();
	 std::cout << "conversion test: " << conv << std::endl; */
     }
}
