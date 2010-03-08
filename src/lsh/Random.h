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
 * \brief Routines for random bits and numbers generation.
 * \author E. Benazera, juban@free.fr
 */

#ifndef RANDOM_H
#define RANDOM_H

namespace lsh
{
   class Random
     {
      public:
	static long getRbitsSeed () { return Random::_rbits_seed; }
	
	/**
	 * \brief generate a random 32 bit unsigned integer between minB and maxB.
	 * @param minB lower domain bound,
	 * @param maxB upper domain bound.
	 */
	static unsigned long int genUniformUnsInt32 (const unsigned long int &minB,
						     const unsigned long int &maxB);
	
	static double genUniformDbl32 (const double &minB, const double &maxB);
	
	/**
	 * \brief generate a random real from a normal distribution N(0,1).
	 */
	static double genGaussianDbl32 ();

	/**
	 * \brief random bit generator: should be removed sooner or later, since
	 *        it is nr code and their licence sucks.
	 */
	static bool irbit2(unsigned long *iseed);
	
      private:
	//static const long _rbits_seed = 9457920459875;
	static const long _rbits_seed = 945792045;  //32 bits.
     };
}

#endif
