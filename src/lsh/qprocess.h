/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
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

#ifndef QPROCESS_H
#define QPROCESS_H

#include "DHTKey.h"
#include "stl_hash.h"
#include <string>
#include <vector>

namespace lsh
{
   
   class qprocess
     {
      public:
	static void compile_query(const std::string &query, std::vector<std::string> &queries);
	
	static void mrf_query_160(const std::string &query, hash_multimap<uint32_t,DHTKey,id_hash_uint> &features,
				  const int &min_radius, const int &max_radius);
     
	static void generate_query_hashes(const std::string &query,
					  const int &min_radius, const int &max_radius,
					  hash_multimap<uint32_t,DHTKey,id_hash_uint> &features);
     };
      
} /* end of namespace. */

#endif
