/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
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
  
#ifndef DHTKEY_H
#define DHTKEY_H

#include <bitset>
#include <vector>
#include "rmd160.h" /* original RIPEMD-160 code. */
#include "stl_hash.h"

#define KEYNBITS 160
//#define KEYNBITS 32
#define KEYNBITSIZE 5  /* 160 / 32 bits (ulong) */
//#define KEYNBITSIZE 1

namespace dht
{
   /**
    * \brief DHT idenfication key.
    * \class DHTKey
    */
   class DHTKey : public std::bitset<KEYNBITS>
     {
	friend std::ostream &operator<<(std::ostream &output, const DHTKey &key);
	
      public:
	/**
	 * Constructors: these come from the std::bitset class.
	 */
	DHTKey();
	
	DHTKey(unsigned long val);
	
	DHTKey(const DHTKey& dk);
	
	template<class _CharT, class _Traits, class _Alloc>
	  DHTKey(const std::basic_string<_CharT, _Traits, _Alloc>& __s,
		 size_t __position=0)
	    : std::bitset<KEYNBITS>(__s, __position)
	      {
	      }
	
	template<class _CharT, class _Traits, class _Alloc>
	  DHTKey(const std::basic_string<_CharT, _Traits, _Alloc>& __s,
		 size_t __position, size_t __n)
	    : std::bitset<KEYNBITS>(__s, __position, __n)
	      {
	      }

	/**
	 * \brief Conversion constructor.
	 */ 
	DHTKey(const std::bitset<KEYNBITS>& bs);
	
	/**
	 * Main operations over keys: add, minus, comparisons, inc, dec, 
	 * successor, predecessor.
	 */ 
	DHTKey operator+(const DHTKey& dk);
	DHTKey operator-(const DHTKey& dk);
	DHTKey operator++();
	DHTKey operator--();
	bool operator<(const DHTKey& dk) const;
	bool operator<=(const DHTKey& dk) const;
	bool operator>(const DHTKey& dk) const;
	bool operator>=(const DHTKey& dk) const;
	bool operator==(const DHTKey& dk) const;
	bool operator!=(const DHTKey& dk) const;

	DHTKey successor(const int& inc);
	
	DHTKey predecessor(const int& dec);
	
	/**
	 * \brief returns the position of the highest positive bit.
	 */
	int topBitPos() const;
	
	/**
	 * \brief checks whether this key is in (a,b) on the circle, excluding bounds.
	 */
	bool between(const DHTKey &a, const DHTKey& b) const;
	
	/**
	 * \brief checks whether this key is in [a,b] on the circle.
	 */
	bool incl(const DHTKey& a, const DHTKey& b) const;
	
	/**
	 * \brief checks whether this key is in [a,b) on the circle.
	 */
	bool leftincl(const DHTKey& a, const DHTKey& b) const;
	
	/**
	 * \brief checks whether this key is in (a,b] on the circle.
	 */
	bool rightincl(const DHTKey& a, const DHTKey& b) const;

	/**
	 * \brief Hashing with RIPEMD-160 (this is free, no patent !) 
	 *   from a string. This comes from the original RIPEMD-160 code:
	 *   RIPEMD-160 software written by Antoon Bosselaers,
	 *   available at http://www.esat.kuleuven.be/~cosicart/ps/AB-9601/.
	 */
	static byte* RMD(byte* message, byte *&hashcode);

	/**
	 * TODO: RIPEMD-160 from u_int32.
	 */
	
	//debug
	static void RMDstring(char* message, char* print);
	static void RMDbits(char* message, char* print);
	static void charToBits(const char& c, std::bitset<8> &bb_char);
	//debug
	
	/**
	 * \brief Convert from the RIPEMD-160 output hash.
	 * @param message ASCII chain to be hashed and turned into a key.
	 * @return DHTKey hashed and converted from the input chain.
	 */
	static DHTKey hashKey(char* message);
	
	/**
	 * \brief converts a 160bit hash into a 160bit DHTKey.
	 * @param hashcode as an array of byte.
	 * @return DHTKey converted from the hashcode.
	 */
	static DHTKey convert(byte *hashcode);
	
	/** 
	 * \brief Random key generation functions: this is for the case
	 *        we do not get any email address or unique ip address to hash from.
	 *        The random bit generator is seeded with gettimeofday, using microseconds.
	 */
	static DHTKey randomKey();
	static int _n_generated_keys; // number of generated keys.
	
	/**
	 * \brief Basic random bit generator, for debugging purposes.
	 */
	static bool irbit2(unsigned long *iseed);
		
	/**
	 * \brief conversion to/from a vector of unsigned char.
	 */
	// DEPRECATED.
	//void tovchar(std::vector<unsigned char>& vuc) const;
	//static DHTKey fromvchar(const std::vector<unsigned char>& vuc);

	/**
	 * \brief conversion to const char* 
	 * @param c_ptr pointer to a char array.
	 */
	void tochar(char* c_ptr) const;
     
	std::string to_rstring() const;
	
	static DHTKey from_rstring(const std::string &str);
	
	std::ostream& print(std::ostream &output) const;
	
	/**
	 * \brief serialization to vector of char.
	 */
	static std::vector<unsigned char> serialize(const DHTKey &dk);
     
	static DHTKey unserialize(const std::vector<unsigned char> &ser);

	static bool lowdhtkey(const DHTKey *dk1, const DHTKey *dk2)
	  {
	     return (*dk1 < *dk2);
	  };
		
     };
   
} /* end of namespace */

using dht::DHTKey;

#ifndef STRUCT_EQDHTKEY
#define STRUCT_EQDHTKEY
   struct eqdhtkey
     {
	bool operator()(const DHTKey* dk1, const DHTKey* dk2) const
	  {
	     return (*dk1 == *dk2);
	  }
     };
#endif

namespace __gnu_cxx // beware
{
   template<>
     struct hash<const DHTKey*>
     {
      /**                                                                                                                                        
       * \brief this is a specialization of the SGI stl hashing function.                                                                        
       * Note: this function _is_ thread reentrant since the local char array                                                                    
       *       is allocated (and destroyed) within the operator. I'm not sure                                                                    
       *       how bad this is...                                                                                                                
       */
      size_t
	operator()(const DHTKey* dk) const
	{
	   char* dk_cc = new char[KEYNBITS+1];
	   dk->tochar(dk_cc);
	   
	   //debug                                                                                                                               
	   //std::cout << "[Debug]:hash dk_cc: " << dk_cc << std::endl;                                                                          
	   //debug                                                                                                                               
	   
	   size_t res =  __stl_hash_string(dk_cc);  //TODO: could use a different hash function.
	   
	   //debug                                                                                                                               
	   //std::cout << "[Debug]:hash: dk: " << *dk << std::endl;                                                                              
	   //std::cout << "[Debug]:hash: res: " << res << std::endl;                                                                             
	   //debug                                                                                                                               
	   
	   delete[] dk_cc;
	   return res;
	}
   };
} // end of namespace.

#endif
