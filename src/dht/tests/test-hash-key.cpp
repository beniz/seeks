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

#include "DHTKey.h"
#include <iostream>

using namespace dht;

int main()
{
   hash_map<const DHTKey*, int, hash<const DHTKey*>, eqdhtkey> hmkey;
   
   DHTKey* key1 = new DHTKey(DHTKey::randomKey());
   std::cout << "testing insertion of key1: " << *key1 << std::endl;
   hmkey[key1] = 1;
   
   hash_map<const DHTKey*, int, hash<const DHTKey*>, eqdhtkey>::const_iterator hmit = hmkey.find(key1);
   bool found = (hmit != hmkey.end());
   std::cout << "testing find: " << found << std::endl;
   
   int key1v = hmkey[key1];
   std::cout << "fetching key1: " << key1v << std::endl;
   
   DHTKey* key2 = new DHTKey(DHTKey::randomKey());
   hmkey[key2] = 2;
   std::cout << "fetching key2: " << hmkey[key2] << std::endl;
}
