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

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>

#include "DHTKey.h"
#include <string>
#include <iostream>
#include <assert.h>

using namespace dht;

TEST(DHTKeyTest, shift_and_rstring)
{
  DHTKey mask(1);
  mask <<= KEYNBITS-1;
  std::string rstring = mask.to_rstring();
  std::cout << "mask: " << rstring << std::endl;
  ASSERT_EQ("8000000000000000000000000000000000000000",rstring);
}

TEST(DHTKeyTest, add_and_rstring)
{
  DHTKey mask(1);
  mask <<= KEYNBITS-1;

  DHTKey dhtk1(std::string("10"));
  std::string rstring = dhtk1.to_rstring();
  std::cout << "dhtk1: " << rstring << std::endl;
  ASSERT_EQ("0000000000000000000000000000000000000002",rstring);

  rstring = (mask + dhtk1).to_rstring();
  std::cout << "testing add: mask + dhtk1: " << rstring << std::endl;
  ASSERT_EQ("8000000000000000000000000000000000000002",rstring);

  rstring = (dhtk1 + dhtk1).to_rstring();
  std::cout << "testing add: dhtk1 + dhtk1: " << rstring << std::endl;
  ASSERT_EQ("0000000000000000000000000000000000000004",rstring);

  rstring = (mask + mask).to_rstring();
  std::cout << "testing add: mask + mask: " << rstring << std::endl;
  ASSERT_EQ("0000000000000000000000000000000000000000",rstring);

  DHTKey dhtk2 (std::string("110"));
  std::cout << "dhtk2: " << dhtk2 << std::endl;
  rstring = (dhtk1 + dhtk2).to_rstring();
  std::cout << "testing add: dhtk1 + dhtk2: " << rstring << std::endl;
  ASSERT_EQ("0000000000000000000000000000000000000008",rstring);
}

TEST(DHTKeyTest, increment_and_rstring)
{
  DHTKey dhtk1(std::string("10"));
  std::cout << "testing inc: dhtk1++: " << ++dhtk1 << std::endl;
  std::string rstring = dhtk1.to_rstring();
  ASSERT_EQ("0000000000000000000000000000000000000003",rstring);
  DHTKey un(1);
  std::cout << "testing inc: un++: " << ++un << std::endl;
  rstring = un.to_rstring();
  ASSERT_EQ("0000000000000000000000000000000000000002",rstring);
}

TEST(DHTKeyTest, decrement_and_rstring)
{
  DHTKey un(1);
  ++un;
  std::cout << "testing dec: un--: " << --un << std::endl;
  std::string rstring = un.to_rstring();
  ASSERT_EQ("0000000000000000000000000000000000000001",rstring);

  DHTKey dhtk2 (std::string("110"));
  std::cout << "testing dec: dhtk2--: " << --dhtk2 << std::endl;
  rstring = dhtk2.to_rstring();
  ASSERT_EQ("0000000000000000000000000000000000000005",rstring);
}

TEST(DHTKeyTest, substract_and_rstring)
{
  DHTKey dhtk1(std::string("11"));
  std::string rstring = (dhtk1 - dhtk1).to_rstring();
  std::cout << "testing minus: dhtk1 - dhtk1: " << rstring << std::endl;
  ASSERT_EQ("0000000000000000000000000000000000000000",rstring);

  DHTKey dhtk2 (std::string("101"));
  rstring = (dhtk2 - dhtk1).to_rstring();
  std::cout << "testing minus: dhtk2 - dhtk1: " << rstring << std::endl;
  ASSERT_EQ("0000000000000000000000000000000000000002",rstring);

  std::cout << "dhtk1: " << dhtk1 << std::endl;
  std::cout << "dhtk2: " << dhtk2 << std::endl;
  rstring = (dhtk1 - dhtk2).to_rstring();
  std::cout << "testing minus: dhtk1 - dhtk2: " << rstring << std::endl;
  ASSERT_EQ("fffffffffffffffffffffffffffffffffffffffe",rstring);
}

TEST(DHTKeyTest, comparisons)
{
  DHTKey dhtk1(std::string("11"));
  DHTKey dhtk2 (std::string("101"));
  DHTKey un(1);
  bool t;
  std::cout << "testing comparisons: dhtk2 < dhtk1: " << (t = (dhtk2 < dhtk1)) << std::endl;
  ASSERT_EQ(false,t);
  std::cout << "testing comparisons: dhtk2 <= dhtk2: " << (t=(dhtk2 <= dhtk2)) << std::endl;
  ASSERT_EQ(true,t);
  std::cout << "testing comparisons: dhtk2 > dhtk1: " << (t=(dhtk2 > dhtk1)) << std::endl;
  ASSERT_EQ(true,t);
  std::cout << "testing comparisons: dhtk2 >= dhtk2: " << (t=(dhtk2 >= dhtk2)) << std::endl;
  ASSERT_EQ(true,t);
  std::cout << "testing comparisons: dhtk2 == dhtk2: " << (t=(dhtk2 == dhtk2)) << std::endl;
  ASSERT_EQ(true,t);
  std::cout << "testing comparisons: dhtk1 != dhtk2: " << (t=(dhtk1 != dhtk2)) << std::endl;
  ASSERT_EQ(true,t);
  std::cout << "testing comparisons: dhtk1 == dhtk2: " << (t=(dhtk1 == dhtk2)) << std::endl;
  ASSERT_EQ(false,t);
  std::cout << "testing comparisons: dhtk1 between un and dhtk2: " << (t=(dhtk1.between(un,dhtk2))) << std::endl;
  ASSERT_EQ(true,t);
  std::cout << "testing comparisons: dhtk2 between un and dhtk1: " << (t=(dhtk2.between(un,dhtk1))) << std::endl;
  ASSERT_EQ(false,t);
}

TEST(DHTKeyTest, successor_and_rstring)
{
  DHTKey dhtk1(std::string("11"));
  std::string rstring = dhtk1.successor(1).to_rstring();
  std::cout << "testing successor: dhtk1: + 2^1: " << rstring << std::endl;
  ASSERT_EQ("0000000000000000000000000000000000000005",rstring);

  rstring = dhtk1.successor(6).to_rstring();
  std::cout << "testing successor: dhtk1: + 2^6: " << rstring << std::endl;
  ASSERT_EQ("0000000000000000000000000000000000000043",rstring);
}

TEST(DHTKeyTest, predecessor_and_rstring)
{
  DHTKey dhtk1(std::string("11"));
  std::string rstring = dhtk1.predecessor(1).to_rstring();
  std::cout << "testing predecessor: dhtk1: - 2^1: " << rstring << std::endl;
  ASSERT_EQ("0000000000000000000000000000000000000001",rstring);

  rstring = dhtk1.predecessor(5).to_rstring();
  std::cout << "testing predecessor: dhtk1: - 2^5: " << rstring << std::endl;
  ASSERT_EQ("ffffffffffffffffffffffffffffffffffffffe3",rstring);
}

/* std::cout << "testing RIPEMD-160 hashing:"; */

TEST(DHTKeyTest, ripemd160)
{
  char *me = (char*)"me@localhost";
  DHTKey::RMDstring(me,me);
  //std::cout << std::endl;
  DHTKey::RMDbits(me, me);
  //std::cout << std::endl;
  DHTKey res = DHTKey::hashKey(me);
  std::string rstring = res.to_rstring();
  std::cout << rstring << std::endl;
  ASSERT_EQ("b343435189b7c846938c3069c31a5165ed15a848",rstring);
}

/*   std::cout << "testing random keys generation:\n";
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
   assert(dk == dk2); */
//}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  RUN_ALL_TESTS();
}
