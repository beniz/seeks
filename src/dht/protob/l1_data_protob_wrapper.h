/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
 * Copyright (C) 2010  Loic Dachary <loic@dachary.org>
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

#ifndef L1_DATA_PROTOB_WRAPPER_H
#define L1_DATA_PROTOB_WRAPPER_H

#include "dht_layer1_data.pb.h"
#include "DHTKey.h"
#include "LocationTable.h"
#include "dht_exception.h"

#include <vector>
#include <ostream>
#include <istream>

namespace dht
{
   class l1_data_protob_wrapper
     {
      public:
	/**
	 * From data to protobuffers.
	 */
	static l1::table::vnodes_table* create_vnodes_table(const std::vector<const DHTKey*> &vnode_ids,
							    const std::vector<LocationTable*> &vnode_ltables);
		
	/**
	 * serialize and write the output to a stream.
	 */
	static void serialize_to_stream(const l1::table::vnodes_table *vt, std::ostream &out);
	
	/**
	 * From protobuffers to data.
	 */
	static void read_vnodes_table(const l1::table::vnodes_table *l1_vt,
				      std::vector<const DHTKey*> &vnode_ids,
				      std::vector<LocationTable*> &vnode_ltables);
     
	/**
	 * deserialize and read from an intput stream.
	 */
	static void deserialize_from_stream(std::istream &in, l1::table::vnodes_table *vt);
     };
   
} /* end of namespace. */

#endif
