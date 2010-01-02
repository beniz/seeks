/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2006, 2009 Emmanuel Benazera, juban@free.fr
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "LSHUniformHashTableHamming.h"

namespace lsh
{

LSHUniformHashTableHamming::LSHUniformHashTableHamming(const LSHSystemHamming &lsh_h)
  : LSHUniformHashTable<std::string>(),_lsh_h(lsh_h)
{
}

LSHUniformHashTableHamming::LSHUniformHashTableHamming(const LSHSystemHamming &lsh_h,
						       const unsigned long int &uhsize)
  : LSHUniformHashTable<std::string>(uhsize),_lsh_h(lsh_h)
{
}

LSHUniformHashTableHamming::~LSHUniformHashTableHamming()
{
}

void LSHUniformHashTableHamming::LcomputeMKey (std::string str,
					       unsigned long int *Lmkeys)
{
   _lsh_h.LmainKeyFromStr(str,Lmkeys,_uhsize);
}

void LSHUniformHashTableHamming::LcomputeCKey (std::string str,
                                               unsigned long int *Lckeys)
{
   _lsh_h.LcontrolKeyFromStr(str,Lckeys);
}

void LSHUniformHashTableHamming::LcomputeMCKey (std::string str,
						unsigned long int *Lmkeys,
						unsigned long int *Lckeys)
{
   _lsh_h.LKeysFromStr(str,Lmkeys,Lckeys,_uhsize);
}


} /* end of namespace. */
