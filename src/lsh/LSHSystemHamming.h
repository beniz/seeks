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

/**
 * \file$Id:
 * \brief String sampling based on the Hamming distance.
 * \author E. Benazera, juban@free.fr
 */

#ifndef LSHSYSTEMHAMMING_H
#define LSHSYSTEMHAMMING_H

#include "LSHSystem.h"
#include <string>
#include <bitset>

namespace lsh
{
 
   /**
    * \class LSHSystemHamming
    * \brief String sampling based on the Hamming distance.
    */
   class LSHSystemHamming : public LSHSystem
     {
     public:
	LSHSystemHamming(const unsigned int &k, const unsigned int &L);
	
	~LSHSystemHamming();
	
     private:
       /**
	* const parameters, aka #define.
	*/
       static const unsigned int _char_bit = 8; /**< 8 bits, as on most systems,
						   otherwise, we'll have to provide a proper conversion. */

       static const unsigned int _fixed_str_size = 50;  /**< fixed string size. 
							 Static because we're using bitsets...*/
	

     public:
       static const unsigned int _total_bits = LSHSystemHamming::_char_bit * LSHSystemHamming::_fixed_str_size;

       unsigned int _smpl_bits;

      public:
        /**
	 * \brief Initialize the LSHSystem:
	 * - nk * L random functions.
	 */
        void initHashingFunctionsFactors ();
	
        void initLSHSystemHamming ();

	void initG ();
	
	/**
	 * \brief turns a string into an array of bits.
	 * @param str string to be converted.
	 * @param bb_str result vector of bits.
	 */
	void strToBits (const std::string &str, std::bitset<LSHSystemHamming::_total_bits> &bb_str);

	/**
	 * \brief projects bit string bb_str L times with gs.
	 * @param bb_str bit string to be projected.
	 * @param L number of projections.
	 * @param bb_hash (pre-allocated) result array of L projections of total_bits.
	 */
	void LprojectStr (const std::bitset<LSHSystemHamming::_total_bits> &bb_str,
			  const unsigned int L,
			  std::bitset<LSHSystemHamming::_total_bits> *bb_hash);

	/**
	 *
	 *
	 */
	unsigned long int bitHash (std::bitset<LSHSystemHamming::_total_bits> &bb,
				   unsigned long int **hash_params,
				   const unsigned int &l);
	
	unsigned long int controlHash (std::bitset<LSHSystemHamming::_total_bits> &bb,
				       const unsigned int &l);
	
	
	void LcontrolHash (std::bitset<LSHSystemHamming::_total_bits> *bb,
			   unsigned long int *Lchashes);

	/**
	 *
	 *
	 */
	unsigned long int mainHash (std::bitset<LSHSystemHamming::_total_bits> &bb,
				    const unsigned int &l,
				    const unsigned long int &hsize);
	
	void LmainHash (std::bitset<LSHSystemHamming::_total_bits> *bb,
			const unsigned long int &hsize,
			unsigned long int *Lmhashes);
	
	/**
	 * Integrated key management.
	 */
	void LcontrolKeyFromStr(std::string str,
				unsigned long int *Lmkeys);
	
	void LmainKeyFromStr(std::string str,
			     unsigned long int *Lckeys,
			     const unsigned int &uhsize);

	void LKeysFromStr(std::string str,
			  unsigned long int *Lmkeys,
			  unsigned long int *Lckeys,
			  const unsigned int &uhsize);
	

	/**
	 * Hamming distance.
	 */
	int distance(const std::bitset<LSHSystemHamming::_total_bits> &str1,
		     const std::bitset<LSHSystemHamming::_total_bits> &str2);

	/**
	 * accessors.
	 */
	static unsigned int getCharBit () { return LSHSystemHamming::_char_bit; }
	static unsigned int getFixedStrSize () { return LSHSystemHamming::_fixed_str_size; }
	unsigned int getTotalBits () { return LSHSystemHamming::_total_bits; }
	unsigned int getG (const int &l, const int &k);
	std::bitset<LSHSystemHamming::_total_bits>& getG (const int &l) {return _g[l];}
	bool isInitialized () { return _initialized; }
	
      private:
	void charToBits (const char &c, std::bitset<LSHSystemHamming::_char_bit> &bb_char);
	
	/**
	 * Dispersion functions as L arrays of n*k bits.
	 */
	std::bitset<LSHSystemHamming::_total_bits> *_g;

	/**
	 * hashing function random parameters:
	 * 2 Lxnk-dimensionnal arrays of factors, where n is the number of bits / char.
	 */
	unsigned long int **_controlHash;
	unsigned long int **_mainHash;

        bool _initialized;  /**< the static structures have been initialized. */
     };
   
}

#endif
