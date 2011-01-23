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
#include "proxy_configuration.h"
#include "plugin_manager.h"
#include "plugin.h"
#include "errlog.h"

#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <assert.h>

#include <vector>
#include <iostream>
#include <sstream>

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
  std::string user_db::_db_version_key = "db-version";
  double user_db::_db_version = 0.5;

  user_db::user_db(const bool &local)
    :_opened(false)
  {
    // init the mutex;
    mutex_init(&_db_mutex);

    // create the db.
#if defined(TT)
    if (local)
#endif
      _hdb = new db_obj_local();
#if defined(TT)
    else _hdb = new db_obj_remote(seeks_proxy::_config->_user_db_haddr,
				  seeks_proxy::_config->_user_db_hport);
#endif   
    _hdb->dbsetmutex();
    static_cast<db_obj_local*>(_hdb)->dbtune(0,-1,-1,HDBTDEFLATE);

    // db location.
    if (local && seeks_proxy::_config->_user_db_file.empty())
      {
	db_obj_local *dol = static_cast<db_obj_local*>(_hdb);
        uid_t user_id = getuid(); // get user for the calling process.
        struct passwd *pw = getpwuid(user_id);
        if (pw)
          {
	    std::string name;
            const char *pw_dir = pw->pw_dir;
            if (pw_dir)
              {
                name = std::string(pw_dir) + "/.seeks/";
                int err = mkdir(name.c_str(),0730); // create .seeks repository in case it does not exist.
                if (err != 0 && errno != EEXIST) // all but file exist errors.
                  {
                    errlog::log_error(LOG_LEVEL_ERROR,"Creating repository %s failed: %s",
                                      name.c_str(),strerror(errno));
                    name = "";
                  }
                else name += db_obj_local::_db_name;
		dol->set_name(name);
	      }
          }
        if (dol->get_name().empty())
          {
            // try datadir, beware, we may not have permission to write.
            if (seeks_proxy::_datadir.empty())
              dol->set_name(db_obj_local::_db_name); // write it down locally.
            else dol->set_name(seeks_proxy::_datadir + db_obj_local::_db_name);
          }
      }
    else if (local) // custom db file.
      {
	db_obj_local *dol = static_cast<db_obj_local*>(_hdb);
        dol->set_name(seeks_proxy::_config->_user_db_file);
      }
    else // remote db.
      {
	
      }
  }

  user_db::user_db(const std::string &dbname)
    :_opened(false)
  {
    // init the mutex;
    mutex_init(&_db_mutex);

    _hdb = new db_obj_local();
    _hdb->dbsetmutex();
    static_cast<db_obj_local*>(_hdb)->dbtune(0,-1,-1,HDBTDEFLATE);
    db_obj_local *dol = static_cast<db_obj_local*>(_hdb);
    dol->set_name(dbname);
  }

  user_db::~user_db()
  {
    // close the db.
    close_db();

    // delete db object.
    delete _hdb;
  }

  int user_db::open_db()
  {
    if (_opened)
      {
        errlog::log_error(LOG_LEVEL_INFO, "user_db already opened");
        return 0;
      }

    // try to get write access, if not, fall back to read-only access, with a warning.
    //if (!tchdbopen(static_cast<db_obj_local*>(_hdb)->_hdb,_hdb->get_name().c_str(), HDBOWRITER | HDBOCREAT | HDBONOLCK))
    if (!_hdb->dbopen(HDBOWRITER | HDBOCREAT | HDBONOLCK))
      {
        int ecode = _hdb->dbecode();
        errlog::log_error(LOG_LEVEL_ERROR,"user db db open error: %s",_hdb->dberrmsg(ecode));
        errlog::log_error(LOG_LEVEL_INFO, "trying to open user_db in read-only mode");
        if (!_hdb->dbopen(HDBOREADER | HDBOCREAT | HDBONOLCK)) // Beware, no effect if db is remote, as ths is set by the server.
          {
            int ecode = _hdb->dbecode();
            errlog::log_error(LOG_LEVEL_ERROR,"user db read-only or creation db open error: %s",_hdb->dberrmsg(ecode));
            _opened = false;
            return ecode;
          }
      }
    uint64_t rn = number_records();
    errlog::log_error(LOG_LEVEL_INFO,"opened user_db %s, (%u records)",
                      _hdb->get_name().c_str(),rn);
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

    if (!_hdb->dbopen(HDBOREADER | HDBOCREAT | HDBONOLCK)) // Beware: no effect is the db is remote as this is set by the server.
      {
        int ecode = _hdb->dbecode();
        errlog::log_error(LOG_LEVEL_ERROR,"user db read-only or creation db open error: %s",_hdb->dberrmsg(ecode));
        _opened = false;
        return ecode;
      }
    uint64_t rn = number_records();
    errlog::log_error(LOG_LEVEL_INFO,"opened user_db %s, (%u records)",
                      _hdb->get_name().c_str(),rn);
    _opened = true;
    return 0;
  }

  int user_db::close_db()
  {
    if (!_opened)
      {
        errlog::log_error(LOG_LEVEL_INFO,"user_db %s already closed", _hdb->get_name().c_str());
        return 0;
      }

    if (!_hdb->dbclose())
      {
        int ecode = _hdb->dbecode();
        errlog::log_error(LOG_LEVEL_ERROR,"user db %s close error: %s", _hdb->get_name().c_str(),
                          _hdb->dberrmsg(ecode));
        return ecode;
      }
    _opened = false;
    return 0;
  }

  int user_db::optimize_db()
  {
    db_obj_local *ldb = dynamic_cast<db_obj_local*>(_hdb);
    if (ldb && !ldb->dboptimize(0,-1,-1,HDBTDEFLATE))
      {
        int ecode = _hdb->dbecode();
        errlog::log_error(LOG_LEVEL_ERROR,"user db optimization error: %s",_hdb->dberrmsg(ecode));
        return ecode;
      }
    else
      {
	//TODO: db_obj_remote.
      }
    errlog::log_error(LOG_LEVEL_INFO,"user db optimized");
    return 0;
  }

  int user_db::set_version(const double &v)
  {
    mutex_lock(&_db_mutex);
    const char *keyc = user_db::_db_version_key.c_str();
    if (!_hdb->dbput(keyc,strlen(keyc),&v,sizeof(double)))
      {
        int ecode = _hdb->dbecode();
        errlog::log_error(LOG_LEVEL_ERROR,"user db adding record error: %s",_hdb->dberrmsg(ecode));
        mutex_unlock(&_db_mutex);
        return -1;
      }
    mutex_unlock(&_db_mutex);
    return 0;
  }

  double user_db::get_version()
  {
    const char *keyc = user_db::_db_version_key.c_str();
    int value_size;
    void *value = _hdb->dbget(keyc,strlen(keyc),&value_size);
    if (!value)
      {
        return 0.0;
      }
    double v = *((double*)value);
    return v;
  }

  bool user_db::is_remote() const
  {
#if defined(TT)
    db_obj_remote *dbojr = dynamic_cast<db_obj_remote*>(_hdb);
    if (dbojr)
      return true;
#endif
    return false;
  }

  bool user_db::is_open() const
  {
    return _opened;
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

  int user_db::find_matching(const std::string &ref_key,
                             const std::string &plugin_name,
                             std::vector<std::string> &matching_keys)
  {
    void *rkey = NULL;
    int rkey_size;
    std::vector<std::string> to_remove;
    _hdb->dbiterinit();
    while ((rkey = _hdb->dbiternext(&rkey_size)) != NULL)
      {
        std::string rkey_str = std::string((const char*)rkey,rkey_size);
        if ((!plugin_name.empty() && rkey_str.find(plugin_name) == std::string::npos)
            || rkey_str.find(ref_key) == std::string::npos)
          {
            free(rkey);
            continue;
          }
        else
          {
            matching_keys.push_back(std::string((char*)rkey));
            free(rkey);
          }
      }
    return 0;
  }

  db_record* user_db::find_dbr(const std::string &key,
                               const std::string &plugin_name)
  {
    // create key.
    std::string rkey = user_db::generate_rkey(key,plugin_name);

    int value_size;
    size_t lrkey = rkey.length();
    char keyc[lrkey];
    for (size_t i=0; i<lrkey; i++)
      keyc[i] = rkey[i];
    void *value = _hdb->dbget(keyc, sizeof(keyc), &value_size);
    if (value)
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
            if (!dbr)
              {
                errlog::log_error(LOG_LEVEL_ERROR,"Plugin %s created a NULL db record",
                                  plugin_name.c_str());
                return NULL;
              }
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
        if (edbr->serialize(str) != 0)
          {
            // serialization error.
            errlog::log_error(LOG_LEVEL_ERROR, "Aborting adding record to user db: record serialization error");
            delete edbr;
            return -1;
          }
        delete edbr;
      }
    else
      {
        if (dbr.serialize(str) != 0)
          {
            // serialization error.
            errlog::log_error(LOG_LEVEL_ERROR, "Aborting adding record to user db: record serialization error");
            return -1;
          }
      }

    // create key.
    std::string rkey = user_db::generate_rkey(key,dbr._plugin_name);

    // add record.
    size_t lrkey = rkey.length();
    char keyc[lrkey];
    for (size_t i=0; i<lrkey; i++)
      keyc[i] = rkey[i];
    size_t lstr = str.length();
    char valc[lstr];
    for (size_t i=0; i<lstr; i++)
      valc[i] = str[i];
    if (!_hdb->dbput(keyc,sizeof(keyc),valc,sizeof(valc))) // erase if record already exists. XXX: study async call.
      {
        int ecode = _hdb->dbecode();
        errlog::log_error(LOG_LEVEL_ERROR,"user db adding record error: %s",_hdb->dberrmsg(ecode));
        mutex_unlock(&_db_mutex);
        return -1;
      }
    mutex_unlock(&_db_mutex);
    return 0;
  }

  int user_db::remove_dbr(const std::string &rkey)
  {
    if (!_hdb->dbout2(rkey.c_str()))
      {
        int ecode = _hdb->dbecode();
        
	// we do not catch the no record error.
	if (ecode != TCENOREC)
	  {
	    errlog::log_error(LOG_LEVEL_ERROR,"user db removing record error: %s",_hdb->dberrmsg(ecode));
	    return -1;
	  }
	return 1;
      }
    errlog::log_error(LOG_LEVEL_INFO,"removed record %s from user db",rkey.c_str());
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
    if (!_hdb->dbvanish())
      {
        int ecode = _hdb->dbecode();
        errlog::log_error(LOG_LEVEL_ERROR,"user db clearing error: %s",_hdb->dberrmsg(ecode));
        return ecode;
      }
    errlog::log_error(LOG_LEVEL_INFO,"cleared all records in db %s",_hdb->get_name().c_str());
    return 0;
  }

  int user_db::prune_db(const time_t &date)
  {
    void *rkey = NULL;
    int rkey_size;
    std::vector<std::string> to_remove;
    _hdb->dbiterinit();
    while ((rkey = _hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        void *value = _hdb->dbget(rkey, rkey_size, &value_size);
        if (value)
          {
            std::string str = std::string((char*)value,value_size);
            free(value);
            std::string key, plugin_name;
            std::string rkey_str = std::string((char*)rkey);
            if (rkey_str != user_db::_db_version_key
                && user_db::extract_plugin_and_key(rkey_str,
                                                   plugin_name,key) != 0)
              {
                errlog::log_error(LOG_LEVEL_ERROR,"Could not extract record plugin and key from internal user db key");
              }
            else if (rkey_str != user_db::_db_version_key)
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                db_record *dbr = NULL;
                if (!pl)
                  {
                    // handle error.
                    errlog::log_error(LOG_LEVEL_ERROR,"Could not find plugin %s for pruning user db record",
                                      plugin_name.c_str());
                    dbr = new db_record();
                  }
                else
                  {
                    dbr = pl->create_db_record();
                  }

                if (dbr->deserialize(str) != 0)
                  {
                    // deserialization error.
                  }
                else if (dbr->_creation_time < date)
                  to_remove.push_back(rkey_str);
                delete dbr;
              }
          }
        free(rkey);
      }
    int err = 0;
    size_t trs = to_remove.size();
    for (size_t i=0; i<trs; i++)
      err += remove_dbr(to_remove.at(i));
    errlog::log_error(LOG_LEVEL_INFO,"Pruned %u records from user db",trs);
    return err;
  }

  int user_db::prune_db(const std::string &plugin_name,
                        const time_t date)
  {
    void *rkey = NULL;
    int rkey_size;
    std::vector<std::string> to_remove;
    _hdb->dbiterinit();
    while ((rkey = _hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        void *value = _hdb->dbget(rkey, rkey_size, &value_size);
        if (value)
          {
            std::string str = std::string((char*)value,value_size);
            free(value);
            std::string key, cplugin_name;
            std::string rkey_str = std::string((char*)rkey);
            if (rkey_str != user_db::_db_version_key
                && user_db::extract_plugin_and_key(rkey_str,
                                                   cplugin_name,key) != 0)
              {
                errlog::log_error(LOG_LEVEL_ERROR,"Could not extract record plugin and key from internal user db key");
              }
            else if (rkey_str != user_db::_db_version_key)
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                db_record *dbr = NULL;
                if (!pl)
                  {
                    // handle error.
                    errlog::log_error(LOG_LEVEL_ERROR,"Could not find plugin %s for pruning user db record",
                                      plugin_name.c_str());
                    dbr = new db_record();
                  }
                else
                  {
                    dbr = pl->create_db_record();
                  }

                if (dbr->deserialize(str) != 0)
                  {
                    // deserialization error.
                    errlog::log_error(LOG_LEVEL_ERROR,"Failed deserializing record %s",rkey_str.c_str());
                  }
                else if (dbr->_plugin_name == plugin_name)
                  if (date == 0 || dbr->_creation_time < date)
                    to_remove.push_back(rkey_str);
                delete dbr;
              }
          }
        free(rkey);
      }
    int err = 0;
    size_t trs = to_remove.size();
    for (size_t i=0; i<trs; i++)
      err += remove_dbr(to_remove.at(i));
    errlog::log_error(LOG_LEVEL_INFO,"Pruned %u records from user db belonging to plugin %s",
                      trs,plugin_name.c_str());
    return err;
  }

  int user_db::do_smthg_db(const std::string &plugin_name,
			   void *data)
  {
    void *rkey = NULL;
    int rkey_size;
    std::vector<std::string> to_remove;
    _hdb->dbiterinit();
    while ((rkey = _hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        void *value = _hdb->dbget(rkey, rkey_size, &value_size);
        if (value)
          {
            std::string str = std::string((char*)value,value_size);
            free(value);
            std::string key, cplugin_name;
            std::string rkey_str = std::string((char*)rkey);
            if (rkey_str != user_db::_db_version_key
                && user_db::extract_plugin_and_key(rkey_str,
                                                   cplugin_name,key) != 0)
              {
                errlog::log_error(LOG_LEVEL_ERROR,"Could not extract record plugin and key from internal user db key");
              }
            else if (rkey_str != user_db::_db_version_key)
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                db_record *dbr = NULL;
                if (!pl)
                  {
                    // handle error.
                    errlog::log_error(LOG_LEVEL_ERROR,"Could not find plugin %s for pruning user db record",
                                      plugin_name.c_str());
                    dbr = new db_record();
                  }
                else
                  {
                    dbr = pl->create_db_record();
                  }

                if (dbr->deserialize(str) != 0)
                  {
                    // deserialization error.
                    errlog::log_error(LOG_LEVEL_ERROR,"Failed deserializing record %s",rkey_str.c_str());
                  }
                else if (dbr->_plugin_name == plugin_name)
                  dbr->do_smthg(data);
		delete dbr;
              }
          }
        free(rkey);
      }
    int err = 0;
    size_t trs = to_remove.size();
    for (size_t i=0; i<trs; i++)
      err += remove_dbr(to_remove.at(i));
    errlog::log_error(LOG_LEVEL_INFO,"Pruned %u records from user db belonging to plugin %s",
                      trs,plugin_name.c_str());
    return err;
  }

  uint64_t user_db::disk_size() const
  {
    return _hdb->dbfsiz();
  }

  uint64_t user_db::number_records() const
  {
    return _hdb->dbrnum();
  }

  uint64_t user_db::number_records(const std::string &plugin_name) const
  {
    uint64_t n = 0;
    void *rkey = NULL;
    int rkey_size;
    _hdb->dbiterinit();
    while ((rkey = _hdb->dbiternext(&rkey_size)) != NULL)
      {
        std::string rec_pn,rec_key;
        std::string rkey_str = std::string((char*)rkey,rkey_size);
        if (rkey_str != user_db::_db_version_key
            && user_db::extract_plugin_and_key(rkey_str,
                                               rec_pn,rec_key) != 0)
          {
            errlog::log_error(LOG_LEVEL_ERROR,"Could not extract record plugin name when counting records: %s",
                              rkey_str.c_str());
          }
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
    output << "db version: " << get_version() << std::endl;
    output << std::endl;
    output << export_db(output,"text") << std::endl;
    return output;
  }

  std::ostream& user_db::export_db(std::ostream &output, std::string format)
  {

    if (format == "text")
      {
      }
    else if (format == "json")
      {
        output << "{" << std::endl << "\"records\": [ " << std::endl;
      }
    else if (format == "xml")
      {
        output << "<queries>" << std::endl;
      }
    else
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Export format %s not supported.", format.c_str());
        return output;
      }

    /* traverse records */
    bool first = true;
    void *rkey = NULL;
    void *value = NULL;
    int rkey_size;
    _hdb->dbiterinit();
    while ((rkey = _hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        value = _hdb->dbget(rkey, rkey_size, &value_size);
        if (value)
          {
            std::string str = std::string((char*)value,value_size);
            free(value);
            std::string key, plugin_name;
            std::string rkey_str = std::string((char*)rkey);
            if (rkey_str != user_db::_db_version_key
                && user_db::extract_plugin_and_key(rkey_str,
                                                   plugin_name,key) != 0)
              {
                errlog::log_error(LOG_LEVEL_ERROR,"Could not extract record plugin and key from internal user db key");
              }
            else if (rkey_str != user_db::_db_version_key)
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                if (!pl)
                  {
                    errlog::log_error(LOG_LEVEL_ERROR,"Could not find plugin %s for printing user db record",
                                      plugin_name.c_str());
                  }
                else
                  {
                    // call to plugin record creation function.
                    db_record *dbr = pl->create_db_record();
                    if (format == "text")
                      {
                        output << "============================================" << std::endl;
                        output << "key: " << key << std::endl;
			dbr->text_export_record(str, output);
                      }
                    else if (format == "json")
                      {
                        if (!first) output << " , " << std::endl;
                        output << " { " << std::endl;
			output << "\"key\": \"" << key << "\",";
                        dbr->json_export_record(str, output);
                        output << " } " << std::endl;
                      }
                    else if (format == "xml")
                      {
                        output << " <query> " << std::endl;
                        output << " <key>" << key << "</key>\n";
			dbr->xml_export_record(str, output);
                        output << " </query> " << std::endl;
                      }
                    delete dbr;
                    first = false;
                  }
              }
          }
        free(rkey);
      }

    if (format == "json")
      {
        output << "] " << std::endl << "}" << std::endl;
      }
    else if ( format == "xml" )
      {
        output << "</querys>" << std::endl;
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
    while (vit!=_db_sweepers.end())
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
    while (vit!=_db_sweepers.end())
      {
        if ((*vit)->sweep_me())
          n += (*vit)->sweep_records();
        ++vit;
      }
    return n; // number of swept records.
  }

} /* end of namespace. */
