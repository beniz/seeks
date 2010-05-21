/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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

#include "l1_data_protob_wrapper.h"
#include "errlog.h"
#include <string>
#include <assert.h>

using sp::errlog;

namespace dht
{
   l1::table::vnodes_table* l1_data_protob_wrapper::create_vnodes_table(const std::vector<const DHTKey*> &vnode_ids,
									const std::vector<LocationTable*> &vnode_ltables)
     {
	//debug
	assert(vnode_ids.size()==vnode_ltables.size());
	//debug
	
	l1::table::vnodes_table *l1_vt = new l1::table::vnodes_table();
	size_t nvnodes = vnode_ids.size();
	for (size_t i=0;i<nvnodes;i++)
	  {
	     //serialize vnode key.
	     DHTKey vkey = *vnode_ids.at(i);
	     std::vector<unsigned char> dkser = DHTKey::serialize(vkey);
	     std::string dkser_str(dkser.begin(),dkser.end());
	     dkser.clear();
	     l1::table::vnode_data *vdata = l1_vt->add_vnodes();
	     l1::dht_key *vdkey = vdata->mutable_key();
	     vdkey->set_key(dkser_str);
	     
	     //serialize location table.
	     LocationTable *lt = vnode_ltables.at(i); // beware.
	     l1::table::location_table *loctable = vdata->mutable_locations();
	     hash_map<const DHTKey*,Location*,hash<const DHTKey*>,eqdhtkey>::const_iterator lit
	       = lt->_hlt.begin();
	     while(lit!=lt->_hlt.end())
	       {
		  Location *loc = (*lit).second;
		  l1::vnodeid *vid = loctable->add_locations();
		  DHTKey lvkey = loc->getDHTKey();
		  dkser = DHTKey::serialize(lvkey);
		  std::string vdkser_str(dkser.begin(),dkser.end());
		  dkser.clear();
		  l1::dht_key *ldkey = vid->mutable_key();
		  ldkey->set_key(vdkser_str);
		  l1::net_address *na = vid->mutable_addr();
		  na->set_ip_addr(loc->getNetAddress().getNetAddress());
		  na->set_net_port(loc->getNetAddress().getPort());
		  ++lit;
	       }
	  }
	return l1_vt;
     }

   void l1_data_protob_wrapper::serialize_to_stream(const l1::table::vnodes_table *vt, std::ostream &out)
     {
	if (!vt)
	  {
	     errlog::log_error(LOG_LEVEL_DHT,"Can't serialize a null vnode data table");
	     throw l1_data_fail_serialize_exception();
	  }
	if (!vt->SerializeToOstream(&out))
	  throw l1_data_fail_serialize_exception();
     }
      
   void l1_data_protob_wrapper::read_vnodes_table(const l1::table::vnodes_table *l1_vt,
						  std::vector<const DHTKey*> &vnode_ids,
						  std::vector<LocationTable*> &vnode_ltables)
     {
	int nvnodes = l1_vt->vnodes_size();
	for (int i=0;i<nvnodes;i++)
	  {
	     //unserialize vnode key.
	     l1::table::vnode_data vdata = l1_vt->vnodes(i);
	     l1::dht_key vdkey = vdata.key();
	     std::string vdkey_str = vdkey.key();
	     std::vector<unsigned char> ser;
	     std::copy(vdkey_str.begin(),vdkey_str.end(),std::back_inserter(ser));
	     DHTKey vkey = DHTKey::unserialize(ser);
	     ser.clear();
	     
	     LocationTable *lt = new LocationTable();
	     l1::table::location_table *loctable = vdata.mutable_locations();
	     int nlocs = loctable->locations_size();
	     for (int j=0;j<nlocs;j++)
	       {
		  l1::vnodeid vid = loctable->locations(j);
		  l1::dht_key *ldkey = vid.mutable_key();
		  std::string ldkey_str = ldkey->key();
		  std::copy(ldkey_str.begin(),ldkey_str.end(),std::back_inserter(ser));
		  DHTKey lvkey = DHTKey::unserialize(ser);
		  ser.clear();
		  l1::net_address *vna = vid.mutable_addr();
		  NetAddress na = NetAddress(vna->ip_addr(),vna->net_port());
		  Location *loc = NULL;
		  lt->addToLocationTable(lvkey,na,loc);
		  
		  //debug
		  assert(loc!=NULL);
		  //debug
		  
		  if (vkey == lvkey)
		    vnode_ids.push_back(&loc->getDHTKeyRef());
	       }
	     vnode_ltables.push_back(lt);
	  }
	
	//debug
	assert(vnode_ltables.size()==vnode_ids.size());
	//debug
     }
   
   void l1_data_protob_wrapper::deserialize_from_stream(std::istream &in, l1::table::vnodes_table *vt)
     {
	if (!vt)
	  {
	     errlog::log_error(LOG_LEVEL_DHT, "can't deserialize a null vnode data table");
	     throw l1_data_fail_deserialize_exception();
	  }
	if (!vt->ParseFromIstream(&in))
	  throw l1_data_fail_deserialize_exception();
     }
      
} /* end of namespace. */  
