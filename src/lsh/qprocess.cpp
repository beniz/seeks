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

#include "qprocess.h"
#include "mrf.h"
#include "miscutil.h"

using sp::miscutil;

namespace lsh
{
   void qprocess::compile_query(const std::string &query,
				std::vector<std::string> &queries)
     {
	// XXX: only || = OR for now. 
	// && = AND is automatic by similarity expansion.
	if (query.find_first_of("||") == std::string::npos)
	  queries.push_back(query);
	else
	  {
	     miscutil::tokenize(query,queries,"&&");
	  }
     }
   
   void qprocess::mrf_query_160(const std::string &query, std::vector<DHTKey> &features,
				const int &min_radius, const int &max_radius)
     {
	static uint32_t window_length = 8; // default.
	
	// get 160bit features for the requested radius.
	std::vector<char*> char_features;
	mrf::mrf_features_query(query,char_features,
				min_radius,max_radius,window_length);
	
	// turn them into DHTKey.
	size_t nfeatures = char_features.size();
	features.reserve(features.size() + nfeatures);
	for (size_t i=0;i<nfeatures;i++)
	  {
	     byte *hashcode = (byte*)char_features.at(i);
	     features.push_back(DHTKey::convert(hashcode));
	     delete[] hashcode;
	  }
     }
      
   void qprocess::generate_query_hashes(const std::string &query,
					const int &min_radius, const int &max_radius,
					std::vector<DHTKey> &features)
     {
	std::vector<std::string> queries;
	qprocess::compile_query(query,queries);
	size_t nqueries = queries.size();
	for (size_t i=0;i<nqueries;i++)
	  {
	     qprocess::mrf_query_160(queries.at(i),features,
				     min_radius,max_radius);
	  }
     }
      
} /* end of namespace. */
