/**
 * This file is part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
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

#include <vector>
#include <bitset>
#include <limits.h>

namespace sp
{
   class serialize
     {
      public:
	template<typename integer_type>
	  static std::vector<unsigned char> to_network_order(const integer_type &number,
							     unsigned int byte_size = 8)
	    {
	       unsigned int const num_bytes_in_int = (sizeof(integer_type) * CHAR_BIT) / byte_size;
	       
	       unsigned int mask = 0;
	       for (unsigned int i = 0; i < byte_size; ++i)
		 mask = (mask << 1) | 0x1;
	       
	       std::vector<unsigned char> result(num_bytes_in_int);
	       for (unsigned int i = 0; i < num_bytes_in_int; ++i)
		 result[i] = (number >> (byte_size * i)) & mask;
	       
	       return result;
	    };
     
	template<typename integer_type>
	  static integer_type from_network_order(integer_type &res, 
						 std::vector<unsigned char> bytestring,
						 unsigned int byte_size = 8)
	    {
	       res = 0;
	       unsigned int bs = bytestring.size();
	       for (int i=bs-1;i>=0;i--)
		 {
		    res = (res << 8) + bytestring[i];
		 }
	       return res;
	    };
     };
   
} /* end of namespace. */
