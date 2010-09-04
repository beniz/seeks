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
   std::string user_db::_db_name = "seeks_user.db";
   
   user_db::user_db()
     :_opened(false)
     {
	// create the db.
	_hdb = tchdbnew();
	//TODO: compression.
	
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
   
   user_db::~user_db()
     {
	// close the db.
	close_db();
	
	// delete db object.
	tchdbdel(_hdb);
     }
   
   int user_db::open_db()
     {
	// try to get write access, if not, fall back to read-only access, with a warning.
	if(!tchdbopen(_hdb, _name.c_str(), HDBOWRITER | HDBOCREAT))
	  {
	     int ecode = tchdbecode(_hdb);
	     errlog::log_error(LOG_LEVEL_ERROR,"user db db open error: %s",tchdberrmsg(ecode));
	     errlog::log_error(LOG_LEVEL_INFO, "trying to open user_db in read-only mode");
	     if(!tchdbopen(_hdb, _name.c_str(), HDBOREADER | HDBOCREAT))
	       {
		  int ecode = tchdbecode(_hdb);
		  errlog::log_error(LOG_LEVEL_ERROR,"user db read-only or creation db open error: %s",tchdberrmsg(ecode));
		  _opened = false;
		  return ecode;
	       }
	     _opened = false;
	     return ecode;
	  }
	uint64_t rn = number_records();
	errlog::log_error(LOG_LEVEL_INFO,"opened user_db %s, (%u records)",
			  _name.c_str(),rn);
	_opened = true;
	return 0;
     }
   
   int user_db::close_db()
     {
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
	return rkey.substr(pos);
     }
   
   int user_db::add_dbr(const std::string &key,
			const std::string &plugin_name,
			const db_record &dbr)
     {
	// serialize the record.
	std::string str;
	dbr.serialize(str);
	
	// create key.
	std::string rkey = user_db::generate_rkey(key,plugin_name);
	
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
	     return -1;
	  }
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
	return 0;
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
	return 0;
     }
   
   int user_db::prune_db(const time_t &date)
     {
	void *key = NULL;
	int key_size;
	std::vector<std::string> to_remove;
	tchdbiterinit(_hdb);
	while((key = tchdbiternext(_hdb,&key_size)) != NULL)
	  {
	     int value_size;
	     void *value = tchdbget(_hdb, key, key_size, &value_size);
	     if(value)
	       {
		  std::string str = std::string((char*)value,value_size);
		  free(value);
		  db_record dbr(str);
		  if (dbr._creation_time < date)
		    to_remove.push_back(std::string((char*)key));
	       }
	     free(key);
	  }
	int err = 0;
	size_t trs = to_remove.size();
	for (size_t i=0;i<trs;i++)
	  err += remove_dbr(to_remove.at(i));
	return err;
     }
   
   int user_db::prune_db(const std::string &plugin_name)
     {
	void *key = NULL;
	int key_size;
	std::vector<std::string> to_remove;
	tchdbiterinit(_hdb);
	while((key = tchdbiternext(_hdb,&key_size)) != NULL)
	  {
	     int value_size;
	     void *value = tchdbget(_hdb, key, key_size, &value_size);
	     if(value)
	       {
		  std::string str = std::string((char*)value,value_size);
		  free(value);
		  db_record dbr(str);
		  if (dbr._plugin_name == plugin_name)
		    to_remove.push_back(std::string((char*)key));
	       }
	     free(key);
	  }
	int err = 0;
	size_t trs = to_remove.size();
	for (size_t i=0;i<trs;i++)
	  err += remove_dbr(to_remove.at(i));
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
      
   void user_db::read() const
     {
	/* traverse records */
	void *key = NULL;
	void *value = NULL;
	int key_size;
	tchdbiterinit(_hdb);
	while((key = tchdbiternext(_hdb,&key_size)) != NULL)
	  {
	     int value_size;
	     value = tchdbget(_hdb, key, key_size, &value_size);
	     if(value)
	       {
		  std::string str = std::string((char*)value,value_size);
		  db_record dbr(str);
		  std::cerr << "db_record[" << user_db::extract_key(std::string((char*)key)) 
		    << "]: plugin_name: " << dbr._plugin_name << " - creation time: " << dbr._creation_time << std::endl;
		  free(value);
	       }
	     free(key);
	  }
     }
      
} /* end of namespace. */
