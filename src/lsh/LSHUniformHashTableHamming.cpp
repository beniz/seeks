/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2006, 2009 Emmanuel Benazera, juban@free.fr
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

#include "LSHUniformHashTableHamming.h"

namespace lsh
{

  LSHUniformHashTableHamming::LSHUniformHashTableHamming(LSHSystemHamming *lsh_h)
      : LSHUniformHashTable<std::string>(),_lsh_h(lsh_h)
  {
  }

  LSHUniformHashTableHamming::LSHUniformHashTableHamming(LSHSystemHamming *lsh_h,
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
    _lsh_h->LmainKeyFromStr(str,Lmkeys,_uhsize);
  }

  void LSHUniformHashTableHamming::LcomputeCKey (std::string str,
      unsigned long int *Lckeys)
  {
    _lsh_h->LcontrolKeyFromStr(str,Lckeys);
  }

  void LSHUniformHashTableHamming::LcomputeMCKey (std::string str,
      unsigned long int *Lmkeys,
      unsigned long int *Lckeys)
  {
    _lsh_h->LKeysFromStr(str,Lmkeys,Lckeys,_uhsize);
  }


} /* end of namespace. */
