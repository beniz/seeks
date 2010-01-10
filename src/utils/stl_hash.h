/**
 * This file is part of the SEEKS project.
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

#include "config.h"

#ifdef HAVE_TR1_HASH_MAP
//#include <tr1/unordered_map> 
#include <ext/hash_map>
#elif (__GNUC__ >=3)
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#ifdef HAVE_TR1_HASH_MAP
/* using std::tr1::hash;
   using std::tr1::unordered_map; */
// TR1 is not standard yet, and not header compatible,
// so we do backward compatibility for now.
using __gnu_cxx::hash;
using __gnu_cxx::hash_map;
#elif (__GNUC__ >= 3)
using __gnu_cxx::hash;
using __gnu_cxx::hash_map;
#else
using std::hash;
using std::hash_map;
#endif

#include <string.h>

#ifndef STRUCT_EQSTR
#define STRUCT_EQSTR
struct eqstr
{
   bool operator()(const char* s1, const char* s2) const
     {
	return strcmp(s1, s2) == 0;
     }
};
#endif

#include <stdint.h>
#ifndef STRUCT_ID_HASH_UINT
#define STRUCT_ID_HASH_UINT
/**
 * \brief id hash function, returns the element in input.
 */
struct id_hash_uint : public std::unary_function<uint32_t,uint32_t>
{
   uint32_t operator()(const uint32_t &k) const
     {
	return k;
     }
};
#endif
