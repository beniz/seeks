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

#include "sg_db.h"
#include "seeks_proxy.h"
#include "errlog.h"

using sp::seeks_proxy;
using sp::errlog;

namespace dht
{
   sg_db::sg_db()
     {
	// create the db.
	_hdb = tchdbnew();
     
	// db location.
	if (seeks_proxy::_datadir.empty())
	  _name = "dht/sgs.db";
	else _name = seeks_proxy::_datadir + "/dht/sgs.db";
	
	// open the db.
     	open_db();
     }
   
   sg_db::~sg_db()
     {
	// close the db.
	close_db();
	
	// delete db.
	tchdbdel(_hdb);
     }
   
   int sg_db::open_db()
     {
	if(!tchdbopen(_hdb, _name.c_str(), HDBOWRITER | HDBOCREAT))
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_DHT,"sg db open error: %s",tchdberrmsg(ecode));
	     _opened = false;
	     return ecode;
	  }
	_opened = true;
	return 0;
     }
   
   int sg_db::close_db()
     {
	if(!tchdbclose(_hdb))
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_DHT,"sg db close error: %s",tchdberrmsg(ecode));
	     return ecode;
	  }
	_opened = false;
	return 0;
     }
      
   Searchgroup* sg_db::find_sg_db(const DHTKey &sgkey)
     {
	// sgkey to string.
	std::string key_str = sgkey.to_rstring();
	
	char *value = tchdbget2(_hdb,key_str.c_str());
	if(value)
	  {
	     // deserialize value into a search group object.
	     Searchgroup *sg = Searchgroup::deserialize_from_string(value);
	     free(value);
	     
	     if (!sg)
	       {
		  errlog::log_error(LOG_LEVEL_DHT,"sg db deserialization error");
		  return NULL;
	       }
	     return sg;
	  }
	else 
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_DHT,"sg db get error: %s",tchdberrmsg(ecode));
	     return NULL;
	  }
     }
   
   bool sg_db::remove_sg_db(const DHTKey &sgkey)
     {
	// sgkey to string.
	std::string key_str = sgkey.to_rstring();
	
	if (!tchdbout2(_hdb,key_str.c_str()))
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_DHT,"sg db removing record error: %s",tchdberrmsg(ecode));
	     return false;
	  }
	return true;
     }
   
   bool sg_db::add_sg_db(Searchgroup *sg)
     {
	// serialize search group.
	std::string str;
	sg->serialize_to_string(str);
		
	// sgkey to string.
	std::string key_str = sg->_idkey.to_rstring();
	
	if (!tchdbput2(_hdb,key_str.c_str(),str.c_str())) // XXX: overwrites existing record if any.
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_DHT,"sg db adding record error: %s",tchdberrmsg(ecode));
	     return false;
	  }
	return true;
     }
      
} /* end of namespace. */
