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

using namespace dht;

int main()
{
   DHTKey mask(1);
   mask <<= KEYNBITS-1;
   std::cout << "mask: " << mask << std::endl;

   DHTKey dhtk1 (std::string("10"));
   std::cout << "dhtk1: " << dhtk1 << std::endl;
//   dhtk1.to_ulongs();

   std::cout << "testing add: mask + dhtk1: " << mask + dhtk1 << std::endl;
   std::cout << "testing add: dhtk1 + dhtk1: " << dhtk1 + dhtk1 << std::endl;
   std::cout << "testing add: mask + mask: " << mask + mask << std::endl;

   DHTKey dhtk2 (std::string("110"));
   std::cout << "dhtk2: " << dhtk2 << std::endl;
   std::cout << "testing add: dhtk1 + dhtk2: " << dhtk1 + dhtk2 << std::endl;
   
   std::cout << "testing inc: dhtk1++: " << ++dhtk1 << std::endl;
   DHTKey un(1);
   std::cout << "testing inc: un++: " << ++un << std::endl;
   
   std::cout << "testing dec: un--: " << --un << std::endl;
   std::cout << "testing dec: dhtk2--: " << --dhtk2 << std::endl;
   
   std::cout << "testing minus: dhtk1 - dhtk1: " << dhtk1 - dhtk1 << std::endl;
   std::cout << "testing minus: dhtk2 - dhtk1: " << dhtk2 - dhtk1 << std::endl;

   std::cout << "testing comparisons: dhtk2 < dhtk1: " << (dhtk2 < dhtk1) << std::endl;
   std::cout << "testing comparisons: dhtk2 <= dhtk2: " << (dhtk2 <= dhtk2) << std::endl;
   std::cout << "testing comparisons: dhtk2 > dhtk1: " << (dhtk2 > dhtk1) << std::endl;
   std::cout << "testing comparisons: dhtk2 >= dhtk2: " << (dhtk2 >= dhtk2) << std::endl;
   std::cout << "testing comparisons: dhtk2 == dhtk2: " << (dhtk2 == dhtk2) << std::endl;
   std::cout << "testing comparisons: dhtk1 != dhtk2: " << (dhtk1 != dhtk2) << std::endl;
   std::cout << "testing comparisons: dhtk1 == dhtk2: " << (dhtk1 == dhtk2) << std::endl;
   std::cout << "testing comparisons: dhtk1 between un and dhtk2: " << (dhtk1.between(un,dhtk2)) << std::endl;
   std::cout << "testing comparisons: dhtk2 between un and dhtk1: " << (dhtk2.between(un,dhtk1)) << std::endl;

   std::cout << "testing successor: dhtk1: + 2^1: " << dhtk1.successor(1) << std::endl;
   std::cout << "testing successor: dhtk1: + 2^6: " << dhtk1.successor(6) << std::endl;
   std::cout << "testing predecessor: dhtk1: - 2^1: " << dhtk1.predecessor(1) << std::endl;
   std::cout << "testing predecessor: dhtk1: - 2^5: " << dhtk1.predecessor(5) << std::endl;
   
   std::cout << "testing RIPEMD-160 hashing:"; 
   DHTKey::RMDstring("beniz@localhost","beniz@localhost");
   std::cout << std::endl;
   DHTKey::RMDbits("beniz@localhost", "beniz@localhost");
   std::cout << std::endl;
   std::cout << DHTKey::hashKey("beniz@localhost") << std::endl;

   std::cout << "testing random keys generation:\n";
   for (unsigned int i=0; i<10; i++)
     std::cout << "#" << i << ":\n" << DHTKey::randomKey() << std::endl;

   std::cout << "dhtk2 to string: " << dhtk2.to_string() << std::endl;
   std::vector<unsigned char> vuc;
   std::cout << "testing tovchar: dhtk2: ";
   dhtk2.tovchar(vuc);
   for (unsigned i=0; i<vuc.size(); i++)
     std::cout << vuc[i];
   std::cout << std::endl;
   
   std::cout << "testing fromvchar: dhtk2: " << DHTKey::fromvchar(vuc) << std::endl;

   std::cout << "testing serialization:\n";
   DHTKey dk = DHTKey::randomKey();
   std::cout << "dk: " << dk << std::endl;
   
   std::vector<unsigned char> dkser = DHTKey::serialize(dk);
      
   DHTKey dk2 = DHTKey::unserialize(dkser);
   std::cout << "dk2: " << dk2 << std::endl;
   
   std::cout << "dk == dk2 ?: " << (dk == dk2) << std::endl;
}

