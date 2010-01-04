/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and 
 * does provide several locality sensitive hashing schemes for pattern matching over 
 * continuous and discrete spaces.
 * Copyright (C) 2006 Emmanuel Benazera, juban@free.fr
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

#include "LSHFunction.h"
#include <iostream>
#include <stdlib.h>

namespace lsh
{

unsigned int LSHFunction::_asize = 1;  /* default size. Size must be set depending on the dimension
					  of the domain application. */

LSHFunction::LSHFunction ()
{
}
  
LSHFunction::~LSHFunction ()
{
  delete []_a;
}

void LSHFunction::init (const double &b)
{
  _b = b;
  
  if (LSHFunction::_asize != 0)
    _a = new double[LSHFunction::_asize];
  else 
    {
      std::cout << "[Error]:LSHFunction::init: vector size is 0. Exiting.\n";
      exit (EXIT_FAILURE);
    }
}

std::ostream& LSHFunction::print (std::ostream &output) const 
{
  output << "******** lsh function (" << LSHFunction::_asize << ") ********\na: [ ";
  for (unsigned int d=0; d<LSHFunction::_asize; d++)
    output << _a[d] << " ";
  output << "]\n";
  output << "b: " << _b << std::endl;
  return output;
}

} /* end of namespace. */
