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

#include "DHTKey.h"
#include "serialize.h"
#include <math.h>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

using sp::serialize;

namespace dht
{
   int DHTKey::_n_generated_keys = 0;
   
   DHTKey::DHTKey()
     : std::bitset<KEYNBITS>()
       {
       }
   
   DHTKey::DHTKey(unsigned long val)
     : std::bitset<KEYNBITS>(val)
       {
       }
   
   DHTKey::DHTKey(const DHTKey& dk)
     : std::bitset<KEYNBITS>(dk)
       {
       }

   DHTKey::DHTKey(const std::bitset<KEYNBITS>& bs)
     {	
	for (unsigned int i=0; i<bs.size(); i++)
	  (*this)[i] = bs[i];
     }
   
   DHTKey DHTKey::operator+(const DHTKey& dk)
     {
	/**
	 * first, check if some bits are conflicting.
	 */
	DHTKey res = *this & dk;
	if (!res.count())
	  return *this ^ dk;
	
	/**
	 * else, we need to add bits properly.
	 */
	res = *this;
	int ret = 0;
	for (unsigned int i=0; i<res.size(); i++)
	  {
	     if (res[i] && dk[i])
	       {
		  /**
		   * if a bit is to be moved 'up', put it here.
		   * else clears the current bit.
		   */
		  if (ret)
		    {
		       res.set(i,1);
		       ret--;
		    }
		  else res.reset(i);
		  
		  /**
		   * bits to be moved 'up'.
		   */
		  ret++;
	       }
	     else if (!res[i] && ! dk[i])
	       {
		  if (ret)
		    {
		       ret--;
		       res.set(i,1);
		    }
	       }
	     else 
	       {
		  if (!ret)
		    res.set(i,1);
		  else 
		    res.reset(i);  
	       }	     
	  }
	return res;	
     }

   DHTKey DHTKey::operator-(const DHTKey& dk)
     {
	DHTKey res = *this;
	int ret = 0;
	for (unsigned int i=0; i<res.size(); i++)
	  {
	     if (res[i] && dk[i])
	       {
		  if (ret)
		    res.set(i,1);
		  else
		    res.reset(i);
	       }
	     else if (!res[i] && !dk[i])
	       {
		  if (ret)
		    {
		       res.set(i,1);
		    }
	       }
	     else if (!res[i] && dk[i])
	       {
		  if (!ret)
		    {
		       res.set(i,1);
		       ret++;
		    }
		  else
		    {
		       res.reset(i);
		    }
	       }
	     else 
	       {
		  if (ret)
		    {
		       res.reset(i);
		       ret--;
		    }
	       }
	  }
	return res;
     }

   /* DHTKey DHTKey::operator/(const DHTKey &dk)
     {
	DHTKey remainder = *this;
	DHTKey dividend(0);
	while(remainder >= dk)
	  {
	     remainder = remainder - dk;
	     dividend.operator++();
	  }
	return dividend;
     } */
      
   DHTKey DHTKey::operator++()
     {
	size_t count = 0;
	while(count<size())
	  {
	     if(!operator[](count))
	       {
		  set(count,1);
		  return (*this);
	       }
	     else
	       {
		  reset(count);
	       }
	     count++;
	  }
	return (*this);
     }
   
   DHTKey DHTKey::operator--()
     {
	size_t count = 0;
	while(count<size())
	  {
	     if(operator[](count))
	       {
		  reset(count);
		  return(*this);
	       }
	     else
	       {
		  set(count,1);
	       }
	     count++;
	  }
	return (*this);
     }

   bool DHTKey::operator<(const DHTKey& dk) const
     {
	for (int i=size()-1; i>=0; i--)
	  {
	     if (operator[](i) < dk[i])
	       return true;
	     else if (operator[](i) > dk[i])
	       return false;
	  }
	return false;
     }
   
   bool DHTKey::operator<=(const DHTKey& dk) const
     {
	for (int i=size()-1; i>=0; i--)
	  {
	     if (operator[](i) < dk[i])
	       return true;
	     else if (operator[](i) > dk[i])
	       return false;
	  }
	return true;
     }
   
   bool DHTKey::operator>(const DHTKey& dk) const
     {
	for (int i=size()-1; i>=0; i--)
	  {
	     if (operator[](i) > dk[i])
	       return true;
	     else if (operator[](i) < dk[i])
	       return false;
	  }
	return false;
     }
   
   bool DHTKey::operator>=(const DHTKey& dk) const
     {
	for (int i=size()-1; i>=0; i--)
	  {
	     if (operator[](i) > dk[i])
	       return true;
	     else if (operator[](i) < dk[i])
	       return false;
	  }
	return true;
     }
   
   bool DHTKey::operator==(const DHTKey& dk) const
     {
	DHTKey r = *this ^ dk;
	return !r.count();
     }
   
   
   bool DHTKey::operator!=(const DHTKey& dk) const
     {
	return !(operator==(dk));
     }

   DHTKey DHTKey::successor(const int& inc)
     {
	DHTKey inck = 1;
	inck <<= inc;
	return (*this) + inck;
     }
   
   DHTKey DHTKey::predecessor(const int& dec)
     {
	DHTKey deck = 1;
	deck <<= dec;
	return (*this) - deck;
     }

   int DHTKey::topBitPos() const
     {
	for (int p=size()-1; p>=0; p--)
	  {
	     if (operator[](p))
	       return p;
	  }
	return 0;	
     }
      
   bool DHTKey::between(const DHTKey& a, const DHTKey& b) const
     {
	if (a == b)
	  return (operator!=(a));  /* the interval is the whole circle minus key a. */
	else if (a < b)
	  return (operator<(b) && operator>(a));
	else return (operator<(b) || operator>(a));
     }

   bool DHTKey::incl(const DHTKey& a, const DHTKey& b) const
     {
	if ((a == b) && operator==(a))
	  return true;
	else if (a < b)
	  return (operator<=(b) && operator>=(a));
	else return (operator<=(b) || operator>=(a));
     }
   
   bool DHTKey::leftincl(const DHTKey& a, const DHTKey& b) const
     {
	if ((a == b) && operator==(a))
	  return true;
	else if (a < b)
	  return (operator<(b) && operator>=(a));
	else return (operator<(b) || operator>=(a));
     }
   
   bool DHTKey::rightincl(const DHTKey& a, const DHTKey& b) const
     {
	if ((a == b) && operator==(a))
	  return true;
	else if (a < b)
	  return (operator<=(b) && operator>(a));
	else return (operator<=(b) || operator>(a));
     }

   /*-- RIPEMD-160 stuff --*/
#define RMDsize 160
   
   byte* DHTKey::RMD(byte* message, byte *&hashcode)
     /**
      * returns RMD(message)
      * message should be a string terminated by '\0'
      **/
     {
	dword         MDbuf[RMDsize/32];   /* contains (A, B, C, D(, E))   */
	//static byte   hashcode[RMDsize/8]; /* for final hash-value         */
	dword         X[16];               /* current 16-word chunk        */
	unsigned int  i;                   /* counter                      */
	dword         length;              /* length in bytes of message   */
	dword         nbytes;              /* # of bytes not yet processed */
	
	/* initialize */
	hashcode = new byte[RMDsize/8];
	MDinit(MDbuf);
	length = (dword)strlen((char *)message);
	
	/* process message in 16-word chunks */
	for (nbytes=length; nbytes > 63; nbytes-=64) 
	  {
	     for (i=0; i<16; i++) 
	       {
		  X[i] = BYTES_TO_DWORD(message);
		  message += 4;
	       }
	     compress_rmd(MDbuf, X);
	  }
	/* length mod 64 bytes left */
	
	/* finish: */
	MDfinish(MDbuf, message, length, 0);
	
	for (i=0; i<RMDsize/8; i+=4) 
	  {
	     hashcode[i]   =  MDbuf[i>>2];         /* implicit cast to byte  */
	     hashcode[i+1] = (MDbuf[i>>2] >>  8);  /*  extracts the 8 least  */
	     hashcode[i+2] = (MDbuf[i>>2] >> 16);  /*  significant bits.     */
	     hashcode[i+3] = (MDbuf[i>>2] >> 24);
	  }
	return (byte *)hashcode;
     }
   
   void DHTKey::RMDstring(char *message, char *print)
     {
	byte *hashcode = NULL;
	hashcode = RMD((byte *)message,hashcode);
	printf("\n* message: %s\n  hashcode: ", print);
	for (unsigned int i=0; i<RMDsize/8; i++)
	  printf("%02x", hashcode[i]);
	delete[] hashcode;
     }

   void DHTKey::RMDbits(char *message, char *print)
     {
	byte *hashcode = NULL;
	hashcode = RMD((byte*) message,hashcode);
	std::cout << "\n message: " << print << "\n hashcode: \n";
	std::bitset<8> cb;
	for (unsigned int i=0; i<RMDsize/8; i++)
	  {
	     DHTKey::charToBits(static_cast<char>(hashcode[i]), cb);
	     std::cout << cb; // << " ";
	  }
	delete[] hashcode;
     }
   
   void DHTKey::charToBits (const char &c, std::bitset<8> &bb_char)
     {
	char cc = c;
	bb_char.reset ();
	
	unsigned int bpos = 0;
	for (int i=7; i >=0; i--)
	  {
	     if ((cc >> i) & 1) bb_char.set(7-bpos);
	     bpos++;
	  }
     }
   
   DHTKey DHTKey::hashKey(char* message)
     {
	/**
	 * generate the RIPEMD-160 hash (40-digit hexadecimal number).
	 */
	byte *hashcode = NULL;
	hashcode = RMD((byte*)message,hashcode);
	
	DHTKey res = DHTKey::convert(hashcode);
	delete[] hashcode;
	return res;
     }
   
   DHTKey DHTKey::convert(byte *hashcode)
     {
	/**
	 * convert to a DHTKey.
	 */
	DHTKey res;
	unsigned int kpos = RMDsize;
	for (unsigned int i=0; i<RMDsize/8; i++)
	  {
	     char cc = static_cast<char>(hashcode[i]);
	     unsigned int bpos = 0;
	     for (int i=7; i>=0; i--)
	       {
		  if ((cc >> i) & 1) res.set(kpos - bpos-1);
		  bpos++;
	       }
	     kpos -= 8;
	  }
	return res;
     }

/*-- random key generation --*/
// TODO: stronger randomness.
   DHTKey DHTKey::randomKey()
     {
	timeval tim;
	gettimeofday(&tim, NULL);
	unsigned long iseed = tim.tv_sec + tim.tv_usec 
	  + DHTKey::_n_generated_keys;
	DHTKey res;
	for (unsigned int b=0; b<KEYNBITS; b++)
	  {
	     res.set(b,DHTKey::irbit2(&iseed));
	  }
	return res;
     }

   #define IB1 1
   #define IB2 2
   #define IB5 16
   #define IB18 131072
   #define MASK (IB1+IB2+IB5)
   
   /**
    * evil nr code for random bit generation, will remove this later.
    */
   bool DHTKey::irbit2(unsigned long* iseed)
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
      
   /*-- conversion stuff --*/
   // oops, NO!
   /* void DHTKey::tovchar(std::vector<unsigned char>& vuc) const
     {
	vuc.clear();
	std::string key_str = to_string();
	for (unsigned int i=0; i<key_str.size(); i++)
	  vuc.push_back(key_str[i]);
     } */
   
   /**
    * ok, there must be a better way of doing this... for now, it's working.
    */
   // oops, NO!
   /* DHTKey DHTKey::fromvchar(const std::vector<unsigned char>& vuc)
     {
	std::string key_str = "";
	for (unsigned int i=0; i<vuc.size(); i++)
	  key_str += std::string(1,vuc[i]);
	return DHTKey(key_str);
     } */

   void DHTKey::tochar(char* c_ptr) const
     {
	std::string temp_str = to_string();
	memcpy(c_ptr, temp_str.c_str(), KEYNBITS*sizeof(char)+1);
     }
   
   std::string DHTKey::to_rstring() const
     {
	std::string key_str = to_string();
	std::ostringstream os;
	for (size_t i=0;i<key_str.size();i+=4)
	  {
	     std::bitset<4> b(key_str.substr(i,i+4));
	     os << std::hex << b.to_ulong();
	  }
	std::string res = os.str();
	return res;
     }

   DHTKey DHTKey::from_rstring(const std::string &str)
     {
	DHTKey res;
	int k = KEYNBITS-1;
	for (size_t i=0;i<str.size();i++)
	  {
	     char chars[2] = { str[i], '\0'};
	     unsigned long u = strtoul(chars, NULL, 16);
	     
	     std::bitset<4> b(u);
	     for (int j=3;j>=0;j--)
	       {
		  res.set(k--,b.test(j));
	       }
	  }
	return res;
     }
        
   std::vector<unsigned char> DHTKey::serialize(const DHTKey &dk)
     {
	std::vector<unsigned char> res;
	size_t pos = 0;
	for (short i=0;i<KEYNBITSIZE;i++)
	  {
	     std::bitset<32> bit32;
	     for (short i=0;i<32;i++)
	       bit32.set(i,dk[pos++]);
	     unsigned long num = bit32.to_ulong();
	     
	     std::vector<unsigned char> res32 = serialize::to_network_order(num);
	     for (short j=0;j<4;j++)
	       res.push_back(res32.at(j));
	  }
	return res;
     }
      
   DHTKey DHTKey::unserialize(const std::vector<unsigned char> &ser)
     {
	DHTKey dk;
	size_t pos = 0;
	for (short i=0;i<KEYNBITSIZE;i++)
	  {
	     std::vector<unsigned char> res32;
	     for (short j=0;j<4;j++)
	       res32.push_back(ser.at(pos++));
	     unsigned long num = serialize::from_network_order(num,res32);
	     std::bitset<32> bit32(num);
	     for (short j=0;j<32;j++)
	       dk.set(i*32+j,bit32[j]);
	  }
	return dk;
     }

   std::ostream& DHTKey::print(std::ostream &output) const
     {
	output << to_rstring();
	return output;
     }
      
   std::ostream& operator<<(std::ostream& output, const DHTKey &key)
     {
	return key.print(output);
     }
      
} /* end of namespace */
