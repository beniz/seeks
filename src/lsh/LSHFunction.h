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

/**
 * \file$Id:
 * \brief Locality sensitive hashing (LSH) functions for use with a Euclidian distance.
 * \author E. Benazera, juban@free.fr
 */

#ifndef LSHFUNCTION_H
#define LSHFUNCTION_H

#include <ostream>

namespace lsh
{
  /**
   * \brief Locality sensitive hashing (LSH) functions for use with a Euclidian distance.
   * \class LSHFunction
   */
  class LSHFunction
  {
    public:
      static unsigned int _asize;

    public:
      LSHFunction ();
      ~LSHFunction ();

      void init (const double &b);

      void setA (const unsigned int &kpos, const double &v)
      {
        _a[kpos] = v;
      }
      double getA (const unsigned int &kpos) const
      {
        return _a[kpos];
      }

      void setB (const double &b)
      {
        _b = b;
      }
      double getB () const
      {
        return _b;
      }

      std::ostream& print (std::ostream &output) const;

    private:
      double *_a;
      double _b;
  };

} /* end of namespace. */

#endif
