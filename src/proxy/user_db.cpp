/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "user_db.h"
#include "seeks_proxy.h"
#include "plugin_manager.h"
#include "plugin.h"
#include "errlog.h"

#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <vector>
#include <iostream>

namespace sp
{
   /*- user_db_sweepable -*/
   user_db_sweepable::user_db_sweepable()
     :sweepable()
       {
       }
   
   user_db_sweepable::~user_db_sweepable()
     {
     }
      
   /*- user_db -*/
   std::string user_db::_db_name = "seeks_user.db";
   
   user_db::user_db()
     :_opened(false)
     {
	// init the mutex;
	mutex_init(&_db_mutex);
	
	// create the db.
	_hdb = tchdbnew();
	 tchdbsetmutex(_hdb);
	tchdbtune(_hdb,0,-1,-1,HDBTDEFLATE);
	
	// db location.
	uid_t user_id = getuid(); // get user for the calling process.
	struct passwd *pw = getpwuid(user_id);
	if (pw)
	  {
	     const char *pw_dir = pw->pw_dir;
	     if(pw_dir)
	       {
		  _name = std::string(pw_dir) + "/.seeks/";
		  int err = mkdir(_name.c_str(),0730); // create .seeks repository in case it does not exist.
		  if (err != 0 && errno != EEXIST) // all but file exist errors.
		    {
		       errlog::log_error(LOG_LEVEL_ERROR,"Creating repository %s failed: %s",
					 _name.c_str(),strerror(errno));
		       _name = "";
		    }
		  else _name += user_db::_db_name;
	       }
	  }
	if (_name.empty())
	  {
	     // try datadir, beware, we may not have permission to write.
	     if (seeks_proxy::_datadir.empty())
	       _name = user_db::_db_name; // write it down locally.
	     else _name = seeks_proxy::_datadir + user_db::_db_name;
	  }
     }

   user_db::user_db(const std::string &dbname)
     :_name(dbname),_opened(false)
     {
	_hdb = tchdbnew();
     }
      
   user_db::~user_db()
     {
	// close the db.
	close_db();
	
	// delete db object.
	tchdbdel(_hdb);
     }
   
   int user_db::open_db()
     {
	if (_opened)
	  {
	     errlog::log_error(LOG_LEVEL_INFO, "user_db already opened");
	     return 0;
	  }
		
	// try to get write access, if not, fall back to read-only access, with a warning.
	if(!tchdbopen(_hdb, _name.c_str(), HDBOWRITER | HDBOCREAT | HDBONOLCK))
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_ERROR,"user db db open error: %s",tchdberrmsg(ecode));
	     errlog::log_error(LOG_LEVEL_INFO, "trying to open user_db in read-only mode");
	     if(!tchdbopen(_hdb, _name.c_str(), HDBOREADER | HDBOCREAT | HDBONOLCK))
	       {
		  int ecode = tchdbecode(_hdb);
		  errlog::log_error(LOG_LEVEL_ERROR,"user db read-only or creation db open error: %s",tchdberrmsg(ecode));
		  _opened = false;
		  return ecode;
	       }
	  }
	uint64_t rn = number_records();
	errlog::log_error(LOG_LEVEL_INFO,"opened user_db %s, (%u records)",
			  _name.c_str(),rn);
	_opened = true;
	return 0;
     }
   
   int user_db::open_db_readonly()
     {
	if (_opened)
	  {
	     errlog::log_error(LOG_LEVEL_INFO,"user db already opened");
	     return 0;
	  }
	
	if(!tchdbopen(_hdb, _name.c_str(), HDBOREADER | HDBOCREAT | HDBONOLCK))
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_ERROR,"user db read-only or creation db open error: %s",tchdberrmsg(ecode));
	     _opened = false;
	     return ecode;
	  }
	uint64_t rn = number_records();
	errlog::log_error(LOG_LEVEL_INFO,"opened user_db %s, (%u records)",
			  _name.c_str(),rn);
	_opened = true;
	return 0;
     }
      
   int user_db:: close_db()
     {
	if (!_opened)
	  {
	     errlog::log_error(LOG_LEVEL_INFO,"user_db already closed");
	     return 0;
	  }
	
	if(!tchdbclose(_hdb))
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_ERROR,"user db close error: %s",tchdberrmsg(ecode));
	     return ecode;
	  }
	_opened = false;
	return 0;
     }

   std::string user_db::generate_rkey(const std::string &key,
				      const std::string &plugin_name)
     {
	return plugin_name + ":" + key;
     }
      
   std::string user_db::extract_key(const std::string &rkey)
     {
	size_t pos = rkey.find_first_of(":");
	if (pos == std::string::npos)
	  return "";
	return rkey.substr(pos+1);
     }
   
   int user_db::extract_plugin_and_key(const std::string &rkey,
				       std::string &plugin_name,
				       std::string &key)
     {
	size_t pos = rkey.find_first_of(":");
	if (pos == std::string::npos)
	  return 1;
	plugin_name = rkey.substr(0,pos);
	key = rkey.substr(pos+1);
	return 0;
     }
      
   db_record* user_db::find_dbr(const std::string &key,
				const std::string &plugin_name)
     {
	// create key.
	std::string rkey = user_db::generate_rkey(key,plugin_name);
	
	int value_size;
	char keyc[rkey.length()];
	for (size_t i=0;i<rkey.length();i++)
	  keyc[i] = rkey[i];
	void *value = tchdbget(_hdb, keyc, sizeof(keyc), &value_size);
	if(value)
	  {
	     // deserialize.
	     std::string str = std::string((char*)value,value_size);
	     free(value);
	     
	     db_record *dbr = NULL;
	     
	     // get plugin.
	     plugin *pl = plugin_manager::get_plugin(plugin_name);
	     if (!pl)
	       {
		  // handle error.
		  errlog::log_error(LOG_LEVEL_ERROR,"Could not find plugin %s for creating user db record",
				    plugin_name.c_str());
		  dbr = new db_record(); // using base class for record.
	       }
	     else
	       {
		  // call to plugin record creation function.
		  dbr = pl->create_db_record();
	       }
	     if (dbr->deserialize(str) != 0)
	       {
		  delete dbr;
		  return NULL;
	       }
	     return dbr;
	  }
	else return NULL;
     }
      
   int user_db::add_dbr(const std::string &key,
			const db_record &dbr)
     {
	mutex_lock(&_db_mutex);
	std::string str;
	
	// find record.
	db_record *edbr = find_dbr(key,dbr._plugin_name);
	if (edbr)
	  {
	     // merge records and serialize.
	     int err_m = edbr->merge_with(dbr); // virtual call.
	     edbr->update_creation_time(); // set creation time to the time of this update.
	     if (err_m < 0)
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Aborting adding record to user db: record merging error");
		  delete edbr;
		  mutex_unlock(&_db_mutex);
		  return -1;
	       }
	     else if (err_m == -2)
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Aborting adding record to user db: tried to merge records from different plugins");
		  delete edbr;
		  mutex_unlock(&_db_mutex);
		  return -2;
	       }
	     edbr->serialize(str);
	     delete edbr;
	  }
	else dbr.serialize(str);
		
	// create key.
	std::string rkey = user_db::generate_rkey(key,dbr._plugin_name);
		
	// add record.
	size_t lrkey = rkey.length();
	char keyc[lrkey];
	for (size_t i=0;i<lrkey;i++)
	  keyc[i] = rkey[i];
	size_t lstr = str.length();
	char valc[lstr];
	for (size_t i=0;i<lstr;i++)
	  valc[i] = str[i];
	if (!tchdbput(_hdb,keyc,sizeof(keyc),valc,sizeof(valc))) // erase if record already exists. XXX: study async call.
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_ERROR,"user db adding record error: %s",tchdberrmsg(ecode));
	     mutex_unlock(&_db_mutex);
	     return -1;
	  }
	mutex_unlock(&_db_mutex);
	return 0;
     }
   
   int user_db::remove_dbr(const std::string &rkey)
     {
	if (!tchdbout2(_hdb,rkey.c_str()))
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_ERROR,"user db removing record error: %s",tchdberrmsg(ecode));
	     return -1;
	  }
	return 1;
     }
      
   int user_db::remove_dbr(const std::string &key,
			   const std::string &plugin_name)
     {
	// create key.
	std::string rkey = user_db::generate_rkey(key,plugin_name);
	
	// remove record.
	return remove_dbr(rkey);
     }
         
   int user_db::clear_db()
     {
	if(!tchdbvanish(_hdb))
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_ERROR,"user db clearing error: %s",tchdberrmsg(ecode));
	     return ecode;
	  }
	errlog::log_error(LOG_LEVEL_INFO,"cleared all records in db %s",_name.c_str());
	return 0;
     }
   
   int user_db::prune_db(const time_t &date)
     {
	void *rkey = NULL;
	int rkey_size;
	std::vector<std::string> to_remove;
	tchdbiterinit(_hdb);
	while((rkey = tchdbiternext(_hdb,&rkey_size)) != NULL)
	  {
	     int value_size;
	     void *value = tchdbget(_hdb, rkey, rkey_size, &value_size);
	     if(value)
	       {
		  std::string str = std::string((char*)value,value_size);
		  free(value);
		  std::string key, plugin_name;
		  if (user_db::extract_plugin_and_key(std::string((char*)rkey),
						      plugin_name,key) != 0)
		    {
		       errlog::log_error(LOG_LEVEL_ERROR,"Could not extract record plugin and key from internal user db key");
		    }
		  else
		    {
		       // get a proper object based on plugin name, and call the virtual function for reading the record.
		       plugin *pl = plugin_manager::get_plugin(plugin_name);
		       if (!pl)
			 {
			    // handle error.
			    errlog::log_error(LOG_LEVEL_ERROR,"Could not find plugin %s for pruning user db record",
					      plugin_name.c_str());
			 }
		       else
			 {
			    db_record *dbr = pl->create_db_record();
			    if (dbr->deserialize(str) != 0)
			      {
				 // deserialization error.
			      }
			    else if (dbr->_creation_time < date)
			      to_remove.push_back(std::string((char*)rkey));
			    delete dbr;
			 }
		    }
	       }
	     free(rkey);
	  }
	int err = 0;
	size_t trs = to_remove.size();
	for (size_t i=0;i<trs;i++)
	  err += remove_dbr(to_remove.at(i));
	errlog::log_error(LOG_LEVEL_ERROR,"Pruned %u records from user db",trs);
	return err;
     }
   
   int user_db::prune_db(const std::string &plugin_name,
			 const time_t date)
     {
	void *rkey = NULL;
	int rkey_size;
	std::vector<std::string> to_remove;
	tchdbiterinit(_hdb);
	while((rkey = tchdbiternext(_hdb,&rkey_size)) != NULL)
	  {
	     int value_size;
	     void *value = tchdbget(_hdb, rkey, rkey_size, &value_size);
	     if(value)
	       {
		  std::string str = std::string((char*)value,value_size);
		  free(value);
		  std::string key, plugin_name;
		  if (user_db::extract_plugin_and_key(std::string((char*)rkey),
						      plugin_name,key) != 0)
		    {
		       errlog::log_error(LOG_LEVEL_ERROR,"Could not extract record plugin and key from internal user db key");
		    }
		  else
		    {
		       // get a proper object based on plugin name, and call the virtual function for reading the record.
		       plugin *pl = plugin_manager::get_plugin(plugin_name);
		       if (!pl)
			 {
			    // handle error.
			    errlog::log_error(LOG_LEVEL_ERROR,"Could not find plugin %s for pruning user db record",
					      plugin_name.c_str());
			 }
		       else
			 {
			    db_record *dbr = pl->create_db_record();
			    if (dbr->deserialize(str) != 0)
			      {
				 // deserialization error.
			      }
			    else if (dbr->_plugin_name == plugin_name)
			      if (date == 0 || dbr->_creation_time < date)
				to_remove.push_back(std::string((char*)rkey));
			    delete dbr;
			 }
		    }
	       }
	     free(rkey);
	  }
	int err = 0;
	size_t trs = to_remove.size();
	for (size_t i=0;i<trs;i++)
	  err += remove_dbr(to_remove.at(i));
	errlog::log_error(LOG_LEVEL_ERROR,"Pruned %u records from user db belonging to plugin %s",
			  trs,plugin_name.c_str());
	return err;
     }
      
   uint64_t user_db::disk_size() const
     {
	return tchdbfsiz(_hdb);
     }
   
   uint64_t user_db::number_records() const
     {
	return tchdbrnum(_hdb);
     }
      
   uint64_t user_db::number_records(const std::string &plugin_name) const
     {
	uint64_t n = 0;
	void *rkey = NULL;
	void *value = NULL;
	int rkey_size;
	tchdbiterinit(_hdb);
	while((rkey = tchdbiternext(_hdb,&rkey_size)) != NULL)
	  {
	     std::string rec_pn,rec_key;
	     if (user_db::extract_plugin_and_key(std::string((char*)rkey),
						 rec_pn,rec_key) != 0)
	       errlog::log_error(LOG_LEVEL_ERROR,"Could not extract record plugin name when counting records");
	     else if (rec_pn == plugin_name)
	       n++;
	     free(rkey);
	  }
	return n;
     }
      
   std::ostream& user_db::print(std::ostream &output)
     {
	output << "\nnumber of records: " << number_records() << std::endl;
	output << "size on disk: " << disk_size() << std::endl;
	output << std::endl;
	
	/* traverse records */
	void *rkey = NULL;
	void *value = NULL;
	int rkey_size;
	tchdbiterinit(_hdb);
	while((rkey = tchdbiternext(_hdb,&rkey_size)) != NULL)
	  {
	     int value_size;
	     value = tchdbget(_hdb, rkey, rkey_size, &value_size);
	     if(value)
	       {
		  std::string str = std::string((char*)value,value_size);
		  free(value);
		  std::string key, plugin_name;
		  if (user_db::extract_plugin_and_key(std::string((char*)rkey),
						      plugin_name,key) != 0)
		    {
		       errlog::log_error(LOG_LEVEL_ERROR,"Could not extract record plugin and key from internal user db key");
		    }
		  else
		    {
		       // get a proper object based on plugin name, and call the virtual function for reading the record.
		       plugin *pl = plugin_manager::get_plugin(plugin_name);
		       if (!pl)
			 {
			    // handle error.
			    errlog::log_error(LOG_LEVEL_ERROR,"Could not find plugin %s for printing user db record",
					      plugin_name.c_str());
			    output << "db_record[" << key << "]\n\tplugin_name: " << plugin_name << std::endl;
			 }
		       else
			 {
			    // call to plugin record creation function.
			    db_record *dbr = pl->create_db_record();
			    if (dbr->deserialize(str) != 0)
			      {
				 // deserialization error.
				 // prints out information extracted from the record key.
				 output << "db_record[" << key << "]\n\tplugin_name: " << plugin_name 
				   << " (deserialization error)\n";
			      }
			    else
			      {
				 output << "db_record[" << key << "]\n\tplugin_name: " << dbr->_plugin_name
				   << "\n\tcreation time: " << dbr->_creation_time << std::endl;
				 dbr->print(output);
			      }
			    delete dbr;
			 }
		       output << std::endl;
		    }
	       }
	     free(rkey);
	  }
	return output;
     }
   
   void user_db::register_sweeper(user_db_sweepable *uds)
     {
	_db_sweepers.push_back(uds);
     }
   
   int user_db::unregister_sweeper(user_db_sweepable *uds)
     {
	std::vector<user_db_sweepable*>::iterator vit = _db_sweepers.begin();
	while(vit!=_db_sweepers.end())
	  {
	     if ((*vit) == uds)
	       {
		  _db_sweepers.erase(vit);
		  return 0;
	       }
	     ++vit;
	  }
	return 1; // was not registered in the first place.
     }

   int user_db::sweep_db()
     {
	/**
	 * XXX: for now sweeps on a per-plugin manner. This is sub-optimal
	 * in case several plugins call for a sweep at the same time since
	 * we do a full db traversal per plugin sweep call.
	 * For now, plugins are expected to respond to a sweep call every 
	 * few months or so, and the overhead is no serious problem.
	 */
	int n = 0;
	std::vector<user_db_sweepable*>::const_iterator vit = _db_sweepers.begin();
	while(vit!=_db_sweepers.end())
	  {
	     if ((*vit)->sweep_me())
	       n += (*vit)->sweep_records();
	     ++vit;
	  }
	return n; // number of swept records.
     }
      
} /* end of namespace. */
