/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2006 Emmanuel Benazera, juban@free.fr
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

#include "Random.h"
#include <stdlib.h> /* random () */
#include <math.h>

namespace lsh
{

  unsigned long int Random::genUniformUnsInt32 (const unsigned long int &minB,
      const unsigned long int &maxB)
  {
    unsigned long int r = 0;
    if (RAND_MAX >= maxB - minB)
      r = minB + static_cast<unsigned long int> ((maxB - minB + 1.0) * random () / (RAND_MAX + 1.0));
    else r = minB + static_cast<unsigned long int> ((maxB - minB + 1.0) * (static_cast<unsigned long long int> (random ()) * (static_cast<unsigned long long int> (RAND_MAX) + 1.0) + static_cast<unsigned long long int> (random ()))
               / ((static_cast<unsigned long long int> (RAND_MAX) + 1.0) * (static_cast<unsigned long long int> (RAND_MAX) + 1.0)));
    return r;
  }

  double Random::genUniformDbl32 (const double &minB, const double &maxB)
  {
    double r = 0.0;
    r = minB + (maxB - minB) * static_cast<double> (random ()) / static_cast<double> (RAND_MAX);
    return r;
  }

  double Random::genGaussianDbl32 ()
  {
    double x1, x2;
    do
      {
        x1 = Random::genUniformDbl32 (0.0, 1.0);
      }
    while (x1 == 0);
    x2 = Random::genUniformDbl32 (0.0, 1.0);
    double z = sqrt (-2.0 * log (x1)) * cos (2.0 * M_PI * x2);
    return z;
  }

  /**
   * evil nr code for random bit generation, will remove this later.
   */
#define IB1 1
#define IB2 2
#define IB5 16
#define IB18 131072
#define MASK (IB1+IB2+IB5)

  bool Random::irbit2(unsigned long* iseed)
  {
    if (*iseed & IB18)
      {
        *iseed = ((*iseed ^ MASK) << 1) | IB1;
        return 1;
      }
    else
      {
        *iseed <<= 1;
        return 0;
      }
  }


}
