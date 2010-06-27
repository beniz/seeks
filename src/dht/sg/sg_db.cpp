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
#include "sg_configuration.h"
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
	  //_name = "dht/sgs.db";
	_name = "sgs.db";
	else _name = seeks_proxy::_datadir + "/dht/sgs.db";
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
	uint64_t rn = number_records();
	errlog::log_error(LOG_LEVEL_DHT,"opened sg db %s, (%u search groups)",
			  _name.c_str(),rn);
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
   
   uint64_t sg_db::disk_size() const
     {
	return tchdbfsiz(_hdb);
     }

   uint64_t sg_db::number_records() const
     {
	return tchdbrnum(_hdb);
     }
      
   Searchgroup* sg_db::find_sg_db(const DHTKey &sgkey)
     {
	// sgkey to string.
	std::string key_str = sgkey.to_rstring();
	
	int value_size;
	char keyc[key_str.length()];
	for (size_t i=0;i<key_str.length();i++)
	  keyc[i] = key_str[i];
	void *value = tchdbget(_hdb, keyc, sizeof(keyc), &value_size);
	if(value)
	  {
	     // deserialize value into a search group object.
	     std::string str = std::string((char*)value,value_size);
	     Searchgroup *sg = Searchgroup::deserialize_from_string(str);
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
	char keyc[key_str.length()];
	for (size_t i=0;i<key_str.length();i++)
	  keyc[i] = key_str[i];
	char valc[str.length()];
	for (size_t i=0;i<str.length();i++)
	  valc[i] = str[i];
	if (!tchdbput(_hdb,keyc,sizeof(keyc),valc,sizeof(valc)))
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_DHT,"sg db adding record error: %s",tchdberrmsg(ecode));
	     return false;
	  }
	return true;
     }

   void sg_db::prune()
     {
	// try to optimize the db first.
	// XXX: on large db, this could put the node on hold.
	if (!tchdboptimize(_hdb,0,-1,-1,0)) // default degragmentation.
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_DHT,"sg db optimization error: %s",tchdberrmsg(ecode));
	     return;
	  }
	if (sg_configuration::_sg_config->_sg_db_cap > disk_size())
	  return;
	
	// iterate records to remove the oldest ones.
	DHTKey oldest_r;
	struct timeval tv_now;
	time_t oldest_t = tv_now.tv_sec;
	while(sg_configuration::_sg_config->_sg_db_cap < disk_size())
	  {
	     void *key = NULL;
	     int key_size;
	     tchdbiterinit(_hdb);
	     while((key = tchdbiternext(_hdb,&key_size)) != NULL)
	       {
		  int value_size;
		  void *value = tchdbget(_hdb, key, key_size, &value_size);
		  if(value)
		    {
		       std::string str = std::string((char*)value,value_size);
		       free(value);
		       Searchgroup *sg = Searchgroup::deserialize_from_string(str);
		       if (sg->_last_time_of_use < oldest_t)
			 {
			    oldest_t = sg->_last_time_of_use;
			    oldest_r = sg->_idkey;
			 }
		    }
		  free(key);
	       }
	     remove_sg_db(oldest_r);
	     
	     // XXX: secure, may not be needed...
	     if (!tchdboptimize(_hdb,0,-1,-1,0)) // default degragmentation.
	       {
		  int ecode = tchdbecode(_hdb);
		  errlog::log_error(LOG_LEVEL_DHT,"sg db optimization error: %s",tchdberrmsg(ecode));
		  return;
	       }
	  }
     }

   void sg_db::read()
     {
	/* traverse records */
	void *key;
	void *value;
	int key_size;
	tchdbiterinit(_hdb);
	while((key = tchdbiternext(_hdb,&key_size)) != NULL)
	  {
	     int value_size;
	     value = tchdbget(_hdb, key, key_size, &value_size);
	     if(value)
	       {
		  std::string str = std::string((char*)value,value_size);
		  Searchgroup *sg = Searchgroup::deserialize_from_string(str);
		  std::cerr << "sg " << sg->_idkey << " - number of subscribers: " << sg->_vec_subscribers.size() << std::endl;
		  free(value);
	       }
	     free(key);
	  }
     }
      
} /* end of namespace. */
