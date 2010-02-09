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

#ifndef STOPWORDLIST_H
#define STOPWORDLIST_H

#include "stl_hash.h"

#include <string>

namespace lsh
{
   class stopwordlist
     {
      public:
	stopwordlist(const std::string &filename);
	
	~stopwordlist();
	
	int load_list(const std::string &filename);

	bool has_word(const std::string &w) const;
	
	std::string _swlistfile;
	hash_map<const char*,bool,hash<const char*>,eqstr> _swlist;
	bool _loaded;
     };
   
} /* end of namespace. */

#endif
