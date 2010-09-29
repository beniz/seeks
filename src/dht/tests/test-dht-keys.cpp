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
#include <string>
#include <iostream>
#include <assert.h>

using namespace dht;

int main()
{
   DHTKey mask(1);
   mask <<= KEYNBITS-1;
   std::string rstring = mask.to_rstring();
   std::cout << "mask: " << rstring << std::endl;
   assert(rstring == "8000000000000000000000000000000000000000");
      
   DHTKey dhtk1 (std::string("10"));
   rstring = dhtk1.to_rstring();
   std::cout << "dhtk1: " << rstring << std::endl;
   assert(rstring == "0000000000000000000000000000000000000002");
      
   rstring = (mask + dhtk1).to_rstring();
   std::cout << "testing add: mask + dhtk1: " << rstring << std::endl;
   assert(rstring == "8000000000000000000000000000000000000002");
   
   rstring = (dhtk1 + dhtk1).to_rstring();
   std::cout << "testing add: dhtk1 + dhtk1: " << rstring << std::endl;
   assert(rstring == "0000000000000000000000000000000000000004");
   
   rstring = (mask + mask).to_rstring();
   std::cout << "testing add: mask + mask: " << rstring << std::endl;
   assert(rstring == "0000000000000000000000000000000000000000");
   
   DHTKey dhtk2 (std::string("110"));
   std::cout << "dhtk2: " << dhtk2 << std::endl;
   rstring = (dhtk1 + dhtk2).to_rstring();
   std::cout << "testing add: dhtk1 + dhtk2: " << rstring << std::endl;
   assert(rstring == "0000000000000000000000000000000000000008");
   
   std::cout << "testing inc: dhtk1++: " << ++dhtk1 << std::endl;
   rstring = dhtk1.to_rstring();
   assert(rstring == "0000000000000000000000000000000000000003");
   DHTKey un(1);
   std::cout << "testing inc: un++: " << ++un << std::endl;
   rstring = un.to_rstring();
   assert(rstring == "0000000000000000000000000000000000000002");
      
   std::cout << "testing dec: un--: " << --un << std::endl;
   rstring = un.to_rstring();
   assert(rstring == "0000000000000000000000000000000000000001");
   
   std::cout << "testing dec: dhtk2--: " << --dhtk2 << std::endl;
   rstring = dhtk2.to_rstring();
   assert(rstring == "0000000000000000000000000000000000000005");
   
   rstring = (dhtk1 - dhtk1).to_rstring();
   std::cout << "testing minus: dhtk1 - dhtk1: " << rstring << std::endl;
   assert(rstring == "0000000000000000000000000000000000000000");
   
   rstring = (dhtk2 - dhtk1).to_rstring();
   std::cout << "testing minus: dhtk2 - dhtk1: " << rstring << std::endl;
   assert(rstring == "0000000000000000000000000000000000000002");
   
   std::cout << "dhtk1: " << dhtk1 << std::endl;
   std::cout << "dhtk2: " << dhtk2 << std::endl;
   rstring = (dhtk1 - dhtk2).to_rstring();
   std::cout << "testing minus: dhtk1 - dhtk2: " << rstring << std::endl;
   assert(rstring == "fffffffffffffffffffffffffffffffffffffffe");
   
   bool t;
   std::cout << "testing comparisons: dhtk2 < dhtk1: " << (t = (dhtk2 < dhtk1)) << std::endl;
   assert(t==false);
   std::cout << "testing comparisons: dhtk2 <= dhtk2: " << (t=(dhtk2 <= dhtk2)) << std::endl;
   assert(t==true);
   std::cout << "testing comparisons: dhtk2 > dhtk1: " << (t=(dhtk2 > dhtk1)) << std::endl;
   assert(t==true);
   std::cout << "testing comparisons: dhtk2 >= dhtk2: " << (t=(dhtk2 >= dhtk2)) << std::endl;
   assert(t==true);
   std::cout << "testing comparisons: dhtk2 == dhtk2: " << (t=(dhtk2 == dhtk2)) << std::endl;
   assert(t==true);
   std::cout << "testing comparisons: dhtk1 != dhtk2: " << (t=(dhtk1 != dhtk2)) << std::endl;
   assert(t==true);
   std::cout << "testing comparisons: dhtk1 == dhtk2: " << (t=(dhtk1 == dhtk2)) << std::endl;
   assert(t==false);
   std::cout << "testing comparisons: dhtk1 between un and dhtk2: " << (t=(dhtk1.between(un,dhtk2))) << std::endl;
   assert(t==true);
   std::cout << "testing comparisons: dhtk2 between un and dhtk1: " << (t=(dhtk2.between(un,dhtk1))) << std::endl;
   assert(t==false);

   rstring = dhtk1.successor(1).to_rstring();
   std::cout << "testing successor: dhtk1: + 2^1: " << rstring << std::endl;
   assert(rstring == "0000000000000000000000000000000000000005");
   
   rstring = dhtk1.successor(6).to_rstring();
   std::cout << "testing successor: dhtk1: + 2^6: " << rstring << std::endl;
   assert(rstring == "0000000000000000000000000000000000000043");
   
   rstring = dhtk1.predecessor(1).to_rstring();
   std::cout << "testing predecessor: dhtk1: - 2^1: " << rstring << std::endl;
   assert(rstring == "0000000000000000000000000000000000000001");
   
   rstring = dhtk1.predecessor(5).to_rstring();
   std::cout << "testing predecessor: dhtk1: - 2^5: " << rstring << std::endl;
   assert(rstring == "ffffffffffffffffffffffffffffffffffffffe3");
   
   std::cout << "testing RIPEMD-160 hashing:"; 
   char *me = (char*)"me@localhost";
   DHTKey::RMDstring(me,me);
   std::cout << std::endl;
   DHTKey::RMDbits(me, me);
   std::cout << std::endl;
   DHTKey res = DHTKey::hashKey(me);
   rstring = res.to_rstring();
   std::cout << rstring << std::endl;
   assert(rstring == "ffd9baab4f9869710e290f62c8aa7b79e05bdd90");
   
   std::cout << "testing random keys generation:\n";
   for (unsigned int i=0; i<10; i++)
     std::cout << "#" << i << ":\n" << DHTKey::randomKey() << std::endl;

   std::cout << "dhtk2 to string: " << dhtk2.to_string() << std::endl;
      
   //std::cout << "testing fromvchar: dhtk2: " << DHTKey::fromvchar(vuc) << std::endl;

   std::cout << "testing serialization:\n";
   DHTKey dk = DHTKey::randomKey();
   std::cout << "dk: " << dk << std::endl;
   
   std::vector<unsigned char> dkser = DHTKey::serialize(dk);
      
   DHTKey dk2 = DHTKey::unserialize(dkser);
   std::cout << "dk2: " << dk2 << std::endl;
   
   std::cout << "dk == dk2 ?: " << (dk == dk2) << std::endl;
   assert(dk == dk2);
}

