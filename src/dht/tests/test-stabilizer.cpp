/**
 * The Locality Sensitive Distributed Hashtable (LSH-DHT) library is
 * part of the SEEKS project and does provide the components of
 * a peer-to-peer pattern matching overlay network.
 * Copyright (C) 2006 Emmanuel Benazera, juban@free.fr
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

#include "Stabilizer.h"
#include "Random.h"

using namespace dht;
using lsh::Random;

class test_stabilizable : public Stabilizable
{
 public:
   test_stabilizable() 
     {
     }
   
   ~test_stabilizable()
     {
     }
   
   //virtual functions.
  void stabilize_fast()
     {
	int sl_rnd = Random::genUniformUnsInt32(1, 5);
	//sleep(sl_rnd);
     }
   
   void stabilize_slow() {}
   
   bool isstable() { return false; }
   
};


int main()
{
   Stabilizer *stab = new Stabilizer();
   stab->add_fast(new test_stabilizable());

   //sleep(1);
   
   pthread_join(stab->get_tcl_thread(), NULL);
}
