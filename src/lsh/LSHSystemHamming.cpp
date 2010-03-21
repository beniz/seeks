/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2006, 2009 Emmanuel Benazera, juban@free.fr
 *
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

#include "LSHSystemHamming.h"
#include "Random.h"
#include <iostream>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

namespace lsh
{

LSHSystemHamming::LSHSystemHamming(const unsigned int &k, const unsigned int &L)
     :LSHSystem(k,L),_g(NULL),_controlHash(NULL),_mainHash(NULL),_initialized(false)
{
   _smpl_bits = LSHSystemHamming::_char_bit * _k;
   initLSHSystemHamming();
}

LSHSystemHamming::~LSHSystemHamming()
{     
   if (_initialized)
     {
	for (unsigned int l=0;l<_L;l++)
	  {
	     delete[] _controlHash[l];
	     delete[] _mainHash[l];
	  }
	delete[] _controlHash;
	delete[] _mainHash;
	delete[] _g;
     }
}
   
void LSHSystemHamming::initHashingFunctionsFactors ()
{
  unsigned int nk = LSHSystemHamming::_total_bits;
  for (unsigned int l=0; l<_L; l++)
    {
      _controlHash[l] = new unsigned long int[nk];
      _mainHash[l] = new unsigned long int[nk];
      
      /**
       * we use two different seeds.
       */
      unsigned int seed_control = 907452457;
      unsigned int seed_main = 918747475;
      
      srandom (seed_control);
      for (unsigned int k=0; k<nk; k++)
	_controlHash[l][k] = Random::genUniformUnsInt32 (1, LSHSystem::_max_hash_rnd);
      
      srandom (seed_main);
      for (unsigned int k=0; k<nk; k++)
	_mainHash[l][k] = Random::genUniformUnsInt32 (1, LSHSystem::_max_hash_rnd);
    }
}

void LSHSystemHamming::initLSHSystemHamming ()
{
  /**
   * Init static dynamic structures.
   */
  if (_controlHash) delete[] _controlHash;
  if (_mainHash) delete[] _mainHash;
  for (unsigned int l=0;l<_L;l++)
     {
	if (_controlHash)
	  delete[] _controlHash[l];
	if (_mainHash)
	  delete[] _mainHash[l];
     }
   
   if (_g) delete[] _g;

  _controlHash = new unsigned long int*[_L];
  _mainHash = new unsigned long int*[_L];
  _g = new std::bitset<_total_bits> [_L];
  
  /**
   * Init the local dispersion sampling parameter.
   */
  _smpl_bits = LSHSystemHamming::_char_bit * _k;

   /**
    * Init the L g functions using a random bits generator.
    * We need each g as an array of CHAR_BIT * k bits.
    */
   if (static_cast<unsigned int> (CHAR_BIT) != LSHSystemHamming::_char_bit)
     {
	std::cout << "[Error]: system CHAR_BIT is different than 8, see your compiler settings\
		       or wait til we get a proper conversion scheme ready. Exiting.\n";
	exit (-1);
     }
   
   /**
    * Initialize the sampling functions:
    * keep the same seed along the whole generation process, so to get the same numbers
    * on all machines.
    */
   srandom (Random::getRbitsSeed ());  /* reseed. */
   initG ();

   /**
    * Initialize the hashing functions factors.
    */
   initHashingFunctionsFactors ();

   _initialized = true;
}

void LSHSystemHamming::initG ()
{
  for (unsigned int l=0; l<_L; l++)
    {
      _g[l] = std::bitset<_total_bits> ();

      /**
       * we flip exactly k bits.
       */
      unsigned int k=0;
      while (k < _smpl_bits)
	{
	  unsigned long int r_pos = Random::genUniformUnsInt32 (0, _total_bits-2);
	  if (! _g[l].test(r_pos+1))
	    {
	      _g[l].flip (r_pos+1);
	      k++;
	    }
	}

      //debug
      //std::cout << "g[" << l << "]: " << _g[l] << std::endl;
      //debug
    }
}

unsigned int LSHSystemHamming::getG (const int &l, const int &k)
{
   return static_cast<unsigned int> (getG (l)[k]);
}
  
void LSHSystemHamming::charToBits (const char &c, std::bitset<_char_bit> &bb_char)
{     
   char cc = c;
   bb_char.reset ();
   
   unsigned int bpos = 0;
   for (int i=LSHSystemHamming::_char_bit-1; i >=0; i--)
     {
	if ((cc >> i) & 1) bb_char.set (LSHSystemHamming::_char_bit - bpos - 1);
	bpos++;
     }
}
      
void LSHSystemHamming::strToBits (const std::string &str, std::bitset<_total_bits> &bb_str)
{     
   /**
    * take the string's first fixed_str_size characters and turn them into bits.
    */
   std::string fss_str = "";
   if (str.length () > _fixed_str_size)
     fss_str = str.substr (0, _fixed_str_size);
   else fss_str = str + std::string (_fixed_str_size - str.length (), ' '); // fill up with spaces.
   
   //debug
   //std::cout << "fss_str: " << fss_str << std::endl;
   //debug

   std::bitset<LSHSystemHamming::_char_bit> bb_char;
   const char *fss_ptr = fss_str.data ();
   
   for (unsigned int i=0; i<_fixed_str_size; i++)
     {
	const char c = fss_ptr[i];
	LSHSystemHamming::charToBits (c, bb_char);
	unsigned int m = 0;
	for (unsigned int j=i*LSHSystemHamming::_char_bit; j<i*LSHSystemHamming::_char_bit + 8; j++)
	  bb_str[j] = bb_char[m++];
     }

   //debug
   //std::cout << "str to bits: " << bb_str << std::endl;
   //debug
}
   
void LSHSystemHamming::LprojectStr (const std::bitset<_total_bits> &bb_str,
				    const unsigned int L,
				    std::bitset<_total_bits> *bb_hash)
{
  /**
   * This simply is a bit-by-bit logical AND between each g function and 
   * the bit string.
   */
  for (unsigned int l=0; l<L; l++)
    {
      bb_hash[l] = _g[l] & bb_str;
      
      //debug
      //std::cout << "bb_hash[" << l << "]: " << bb_hash[l] << std::endl;
      //debug
    }
}

unsigned long int LSHSystemHamming::bitHash (std::bitset<_total_bits> &bb,
					     unsigned long int **hash_params,
					     const unsigned int &l)
{
  unsigned long int r = 0;
  for (unsigned int i=0; i<bb.size (); i++)
    {
      if (bb[i])
	r += hash_params[l][i] % LSHSystem::_control_hash_prime_bits;
    }
  return r;
}

unsigned long int LSHSystemHamming::controlHash (std::bitset<_total_bits> &bb,
						 const unsigned int &l)
{
  unsigned long int r = bitHash (bb, _controlHash, l);

  //debug
  //std::cout << "controlHash: " << r << std::endl;
  //debug

  return r;
}

void LSHSystemHamming::LcontrolHash (std::bitset<_total_bits> *bb,
				     unsigned long int *Lchashes)
{
  for (unsigned int l=0; l<_L; l++)
    {
      Lchashes[l] = controlHash (bb[l], l);
    }
}

unsigned long int LSHSystemHamming::mainHash (std::bitset<_total_bits> &bb,
					      const unsigned int &l,
					      const unsigned long int &hsize)
{
  unsigned long int r = bitHash (bb, _mainHash, l) % hsize;

  //debug
  //std::cout << "mainHash: " << r << std::endl;
  //debug

  return r;
}

void LSHSystemHamming::LmainHash (std::bitset<_total_bits> *bb,
				  const unsigned long int &hsize,
				  unsigned long int *Lmhashes)
{
  for (unsigned int l=0; l<_L; l++)
    {
      Lmhashes[l] = mainHash (bb[l], l, hsize);
    }
}

void LSHSystemHamming::LmainKeyFromStr(std::string str,
				       unsigned long int *Lmkeys,
				       const unsigned int &uhsize)
{
  std::bitset<_total_bits> bb_str;
  strToBits (str, bb_str);

  //debug
  /* std::cout << "[Debug]:LSHUniformHashTableHamming::LcomputeMKey: bit conversion:\n"
     << bb_str << std::endl; */
  //debug

  std::bitset<_total_bits> *bb_hash
    = new std::bitset<_total_bits> [_L];
  
  LprojectStr (bb_str, _L, bb_hash);

  LmainHash (bb_hash, uhsize, Lmkeys);

  //debug
  /* std::cout << "[Debug]:LSHUniformHashTableHamming::LcomputeMKey: L bit keys:\n";
     for (unsigned int l=0; l<_L; l++)
     std::cout << Lmkeys[l] << std::endl; */
  //debug
  
  delete []bb_hash;
}

void LSHSystemHamming::LcontrolKeyFromStr(std::string str,
					  unsigned long int *Lckeys)
{
  std::bitset<_total_bits> bb_str;
  strToBits (str, bb_str);

  //debug
  /*  std::cout << "[Debug]:LSHUniformHashTableHamming::LcomputeCKey: bit conversion:\n"
      << bb_str << std::endl; */
  //debug

  std::bitset<_total_bits> *bb_hash
    = new std::bitset<_total_bits> [_L];

  LprojectStr (bb_str, _L, bb_hash);

  LcontrolHash (bb_hash, Lckeys);

  //debug
  /* std::cout << "[Debug]:LSHUniformHashTableHamming::LcomputeCKey: L bit keys:\n";
     for (unsigned int l=0; l<_L; l++)
     std::cout << Lmkeys[l] << std::endl; */
  //debug

  delete []bb_hash;
}

void LSHSystemHamming::LKeysFromStr(std::string str,
				    unsigned long int *Lmkeys,
				    unsigned long int *Lckeys,
				    const unsigned int &uhsize)
{
  /**
   * turn the string into bits and projects onto the k planes.
   */
  std::bitset<_total_bits> bb_str;
  strToBits (str, bb_str);
  std::bitset<_total_bits> *bb_hash
     = new std::bitset<_total_bits> [_L];
  LprojectStr (bb_str, _L, bb_hash);

  /**
   * compute main key.
   */
  LmainHash (bb_hash, uhsize, Lmkeys);

  /**
   * compute control key.
   */
  LcontrolHash (bb_hash, Lckeys);

  delete []bb_hash;
}

int LSHSystemHamming::distance(const std::bitset<_total_bits> &str1,
			       const std::bitset<_total_bits> &str2)
{
  int dist = 0;
  for (unsigned int i=0;i<_total_bits;i++)
    {
      if (str1[i] != str2[i])
	dist++;
    }
  return dist;
}

} /* end of namespace */
