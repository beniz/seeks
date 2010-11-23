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

/**
 * \file$Id:
 * \brief LSH uniform hashtable with string and hashing based on the Hamming distance.
 * \author E. Benazera, juban@free.fr
 */

#ifndef LSHUNIFORMHASHTABLEHAMMING_H
#define LSHUNIFORMHASHTABLEHAMMING_H

#include "LSHUniformHashTable.h"
#include "LSHSystemHamming.h"
#include <string>

namespace lsh
{

  /**
   * \class LSHUniformHashTableHamming
   * \brief LSH uniform hashtable with string and hashing based on Hamming distance.
   */
  class LSHUniformHashTableHamming : public LSHUniformHashTable<std::string>
  {
    public:
      LSHUniformHashTableHamming (LSHSystemHamming *lsh_h);

      LSHUniformHashTableHamming (LSHSystemHamming *lsh_h,
                                  const unsigned long int &uhsize);

      ~LSHUniformHashTableHamming ();

      /**-- functions for a local lsh. --*/
      /**
       * Virtual functions.
       */
      void LcomputeMKey (std::string str,
                         unsigned long int *Lmkeys);

      void LcomputeCKey (std::string str,
                         unsigned long int *Lckeys);

      void LcomputeMCKey (std::string str,
                          unsigned long int *Lmkeys,
                          unsigned long int *Lckeys);


      /**
      * LSHSystemHamming object associated to the uniform hashtable.
      */
      LSHSystemHamming *_lsh_h;
  };

} /* end of namespace. */

#endif
