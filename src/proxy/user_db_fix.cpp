/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "user_db_fix.h"
#include "user_db.h"
#include "plugin_manager.h"
#include "plugin.h"
#include "db_query_record.h"
using seeks_plugins::db_query_record;

#include "errlog.h"
#include "db_obj.h"
#include "seeks_proxy.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <assert.h>

#include <iostream>

namespace sp
{

  int user_db_fix::fix_issue_169()
  {
    /**
     * What we need to do:
     * - create a new db.
     * - iterate records of the existing user db.
     * - transfer non affected records (from uri-capture plugin) to new db with no change.
     * - fix and transfer affected records (from query-capture plugin) to new db.
     * - replace existing db with new db.
     */
    user_db udb; // existing user db.
    int err = udb.open_db_readonly();
    if (err != 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not open the user db for fixing it");
        return -1;
      }
    std::string temp_db_name = "seeks_user.db.tmp";
    user_db cudb(temp_db_name); // new db to be filled up.
    err = cudb.open_db();
    if (err != 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not create the temporary db for fixing the user db");
        udb.close_db();
        return -1;
      }

    /* traverse records */
    void *rkey = NULL;
    void *value = NULL;
    int rkey_size;
    udb._hdb->dbiterinit();
    while ((rkey = udb._hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        value = udb._hdb->dbget(rkey, rkey_size, &value_size);
        if (value)
          {
            std::string str = std::string((char*)value,value_size);
            free(value);
            std::string key, plugin_name;
            if (user_db::extract_plugin_and_key(std::string((char*)rkey),
                                                plugin_name,key) != 0)
              {
                //errlog::log_error(LOG_LEVEL_ERROR,"Could not extract record plugin and key from internal user db key");
              }
            else
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                if (!pl)
                  {
                    errlog::log_error(LOG_LEVEL_ERROR,"Could not find plugin %s for fixing user db record",
                                      plugin_name.c_str());
                  }
                else
                  {
                    // call to plugin record creation function.
                    db_record *dbr = pl->create_db_record();
                    if (dbr->deserialize(str) != 0)  // here we check deserialization even if the record needs not be fixed.
                      {
                      }
                    else
                      {
                        // only records by 'query-capture' plugin are affected.
                        if (dbr->_plugin_name != "query-capture")
                          {
                            cudb.add_dbr(key,*dbr); // add record to new db as is.
                          }
                        else
                          {
                            // fix query-capture record.
                            db_query_record *dqr = static_cast<db_query_record*>(dbr);
                            dqr->fix_issue_169(cudb);
                          }
                        delete dbr;
                      }
                  }
              }
          }
        free(rkey);
      }

    // check that we have at least the same number of records in both db.
    bool replace = false;
    if (udb.number_records() == cudb.number_records())
      {
        replace = true;
        errlog::log_error(LOG_LEVEL_INFO,"user db appears to have been fixed correctly!");
      }
    else errlog::log_error(LOG_LEVEL_ERROR,"Failed fixing the user db");

    // replace current user db with the new (fixed) one.
    if (replace)
      {
        unlink(udb._hdb->get_name().c_str()); // erase current db.
        if (rename(cudb._hdb->get_name().c_str(),udb._hdb->get_name().c_str()) < 0)
          {
            errlog::log_error(LOG_LEVEL_ERROR,"failed renaming fixed user db");
          }
      }
    else unlink(cudb._hdb->get_name().c_str()); // erase temporary db.
    return err;
  }

  int user_db_fix::fix_issue_263()
  {
    user_db udb; // existing user db.
    int err = udb.open_db();
    if (err != 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not open the user db for fixing it");
        return -1;
      }

    // traverse records.
    size_t frec = 0;
    std::map<std::string,db_record*> to_add;
    void *rkey = NULL;
    int rkey_size;
    std::vector<std::string> to_remove;
    udb._hdb->dbiterinit();
    while ((rkey = udb._hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        void *value = udb._hdb->dbget(rkey, rkey_size, &value_size);
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
                errlog::log_error(LOG_LEVEL_ERROR,"Fix 263: could not extract record plugin and key from internal user db key");
              }
            else if (plugin_name != "query-capture")
              {
              }
            else if (rkey_str != user_db::_db_version_key)
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                db_record *dbr = NULL;
                if (!pl)
                  {
                    // handle error.
                    errlog::log_error(LOG_LEVEL_ERROR,"Fix 263: could not find plugin %s for pruning user db record",
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
                else
                  {
                    int f = static_cast<db_query_record*>(dbr)->fix_issue_263();

                    if (f != 0)
                      {
                        frec++;
                        udb.remove_dbr(rkey_str);
                        udb.add_dbr(key,*dbr);
                      }
                  }
                delete dbr;
              }
          }
        free(rkey);
      }

    udb.close_db();
    errlog::log_error(LOG_LEVEL_INFO,"Fix 263: fixed %u records in user db",frec);
    return err;
  }

  int user_db_fix::fix_issue_281()
  {
    user_db udb; // existing user db.
    int err = udb.open_db();
    if (err != 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not open the user db for fixing it");
        return -1;
      }

    // traverse records.
    size_t frec = 0;
    size_t fque = 0;
    uint32_t furls = 0;
    std::map<std::string,db_record*> to_add;
    void *rkey = NULL;
    int rkey_size;
    std::vector<std::string> to_remove;
    udb._hdb->dbiterinit();
    while ((rkey = udb._hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        void *value = udb._hdb->dbget(rkey, rkey_size, &value_size);
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
                errlog::log_error(LOG_LEVEL_ERROR,"Fix 281: could not extract record plugin and key from internal user db key");
              }
            else if (plugin_name != "query-capture")
              {
              }
            else if (rkey_str != user_db::_db_version_key)
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                db_record *dbr = NULL;
                if (!pl)
                  {
                    // handle error.
                    errlog::log_error(LOG_LEVEL_ERROR,"Fix 281: could not find plugin %s for pruning user db record",
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
                else
                  {
                    uint32_t fu = 0;
                    int f = static_cast<db_query_record*>(dbr)->fix_issue_281(fu);

                    if (f != 0)
                      {
                        furls += fu;
                        fque += f;
                        frec++;
                        udb.remove_dbr(rkey_str);
                        udb.add_dbr(key,*dbr);
                      }
                  }
                delete dbr;
              }
          }
        free(rkey);
      }

    udb.close_db();
    errlog::log_error(LOG_LEVEL_INFO,"Fix 281: fixed %u records in user db, %u queries fixed, %u urls fixed",
                      frec,fque,furls);
    return err;
  }

  int user_db_fix::fix_issue_154()
  {
    user_db udb; // existing user db.
    db_obj_local *dol = static_cast<db_obj_local*>(udb._hdb); // must be a local db, ensured from call in seeks.cpp

    std::string bak_db = dol->get_name() + ".bak154";
    int fdo = open(dol->get_name().c_str(),O_RDONLY);
    if (fdo < 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not open the user db %s for fixing it",
                          dol->get_name().c_str());
        return -1;
      }
    struct stat sb;
    stat(dol->get_name().c_str(),&sb);
    int fdn = open(bak_db.c_str(),O_RDWR | O_TRUNC | O_CREAT,sb.st_mode);
    if (fdn < 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not backup the user db %s into %s for fixing it: %s",
                          dol->get_name().c_str(),bak_db.c_str(),strerror(errno));
        return -1;
      }
    int bufsize = 65535;
    char buf[bufsize];
    int nbytes;
    while((nbytes=read(fdo,&buf,bufsize))>0)
      write(fdn,&buf,nbytes);
    close(fdo);
    close(fdn);
    errlog::log_error(LOG_LEVEL_INFO,"user db %s successful backup in %s",
                      dol->get_name().c_str(),bak_db.c_str());

    int err = udb.open_db();
    if (err != 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not open the user db for fixing it");
        return -1;
      }

    errlog::log_error(LOG_LEVEL_INFO, "Applying fix 154 to user db");

    // traverse records.
    uint32_t furls = 0;
    uint32_t rurls = 0;
    size_t frec = 0;
    size_t fque = 0;
    size_t ffque = 0;
    std::map<std::string,db_record*> to_add;
    void *rkey = NULL;
    int rkey_size;
    std::vector<std::string> to_remove;
    udb._hdb->dbiterinit();
    while ((rkey = udb._hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        void *value = udb._hdb->dbget(rkey, rkey_size, &value_size);
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
                errlog::log_error(LOG_LEVEL_ERROR,"Fix 154: could not extract record plugin and key from internal user db key");
              }
            else if (plugin_name != "query-capture")
              {
              }
            else if (rkey_str != user_db::_db_version_key)
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                db_record *dbr = NULL;
                if (!pl)
                  {
                    // handle error.
                    errlog::log_error(LOG_LEVEL_ERROR,"Fix 154: could not find plugin %s for pruning user db record",
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
                else
                  {
                    uint32_t fu = 0;
                    uint32_t fq = 0;
                    uint32_t ru = 0;
                    int f = static_cast<db_query_record*>(dbr)->fix_issue_154(fu,fq,ru);

                    if (f != 0 || fu != 0)
                      {
                        ffque += fq;
                        furls += fu;
                        rurls += ru;
                        fque += f;
                        frec++;
                        udb.remove_dbr(rkey_str);
                        if (!static_cast<db_query_record*>(dbr)->_related_queries.empty())
                          udb.add_dbr(key,*dbr);
                      }
                  }
                delete dbr;
              }
          }
        free(rkey);
      }

    udb.close_db();
    errlog::log_error(LOG_LEVEL_INFO,"Fix 154: fixed %u records in user db, dumped %u queries, dumped %u urls, fixed %u urls in %u queries",
                      frec,fque,furls,rurls,ffque);
    return err;
  }

  int user_db_fix::fill_up_uri_titles(const long &timeout,
                                      const std::vector<std::list<const char*>*> *headers)
  {
    user_db udb; // existing user db.
    int err = udb.open_db();
    if (err != 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not open the user db for fixing it");
        return -1;
      }

    // traverse records.
    uint32_t furls = 0;
    std::map<std::string,db_record*> to_add;
    void *rkey = NULL;
    int rkey_size;
    std::vector<std::string> to_remove;
    udb._hdb->dbiterinit();
    while ((rkey = udb._hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        void *value = udb._hdb->dbget(rkey, rkey_size, &value_size);
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
                errlog::log_error(LOG_LEVEL_ERROR,"Filling up url titles: could not extract record plugin and key from internal user db key");
              }
            else if (plugin_name != "query-capture")
              {
              }
            else if (rkey_str != user_db::_db_version_key)
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                db_record *dbr = NULL;
                if (!pl)
                  {
                    // handle error.
                    errlog::log_error(LOG_LEVEL_ERROR,"Filling up db url titles: could not find plugin %s",
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
                else
                  {
                    uint32_t fu = 0;
                    static_cast<db_query_record*>(dbr)->fetch_url_titles(fu,timeout,headers);
                    if (fu > 0)
                      {
                        furls += fu;
                        udb.remove_dbr(rkey_str);
                        udb.add_dbr(key,*dbr);
                      }
                  }
                delete dbr;
              }
          }
        free(rkey);
      }

    udb.close_db();
    errlog::log_error(LOG_LEVEL_INFO,"Filling up url titles: %u urls fetched",furls);
    return err;
  }

  int user_db_fix::merge_with(const std::string &db_to_merge)
  {

    user_db udbm(db_to_merge);
    int err = udbm.open_db_readonly();
    if (err != 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not open the db to be merged %s",db_to_merge.c_str());
        return -1;
      }

    user_db *udb = seeks_proxy::_user_db;; // existing user db.
    /*err = udb.open_db();
    if (err != 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not open the user db for merging operation");
        udbm.close_db();
    return -1;
    }*/

    // iterate records in db to be merged,
    // find every record in destination db,
    // merge if found.
    uint32_t rmer = 0;
    void *rkey = NULL;
    int rkey_size;
    udbm._hdb->dbiterinit();
    while ((rkey = udbm._hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        void *value = udbm._hdb->dbget(rkey, rkey_size, &value_size);
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
                errlog::log_error(LOG_LEVEL_ERROR,"db merge_with: could not extract record plugin and key from internal user db key");
              }
            else if (plugin_name != "query-capture")
              {
              }
            else if (rkey_str != user_db::_db_version_key)
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                db_record *dbr = NULL;
                if (!pl)
                  {
                    // handle error.
                    errlog::log_error(LOG_LEVEL_ERROR,"db merge_with: could not find plugin %s",
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
                else
                  {
                    db_record *edbr = udb->find_dbr(key,plugin_name);
                    if (edbr)
                      {
                        rmer++;
                        dbr->merge_with(*edbr);
                        udb->remove_dbr(rkey_str);
                        udb->add_dbr(key,*dbr);
                      }
                    delete edbr;
                  }
                delete dbr;
              }
          }
        free(rkey);
      }
    udb->close_db();
    udbm.close_db();
    errlog::log_error(LOG_LEVEL_INFO,"merged %u records", rmer);
    return err;
  }

  int user_db_fix::fix_issue_575()
  {
    user_db udb; // existing user db.
    db_obj_local *dol = static_cast<db_obj_local*>(udb._hdb); // must be a local db, ensured from call in seeks.cpp

    std::string bak_db = dol->get_name() + ".bak575";
    int fdo = open(dol->get_name().c_str(),O_RDONLY);
    if (fdo < 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not open the user db %s for fixing it",
                          dol->get_name().c_str());
        return -1;
      }
    struct stat sb;
    stat(dol->get_name().c_str(),&sb);
    int fdn = open(bak_db.c_str(),O_RDWR | O_TRUNC | O_CREAT,sb.st_mode);
    if (fdn < 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not backup the user db %s into %s for fixing it: %s",
                          dol->get_name().c_str(),bak_db.c_str(),strerror(errno));
        return -1;
      }
    int bufsize = 65535;
    char buf[bufsize];
    int nbytes;
    while((nbytes=read(fdo,&buf,bufsize))>0)
      write(fdn,&buf,nbytes);
    close(fdo);
    close(fdn);
    errlog::log_error(LOG_LEVEL_INFO,"user db %s successful backup in %s",
                      dol->get_name().c_str(),bak_db.c_str());

    int err = udb.open_db();
    if (err != 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not open the user db for fixing it");
        return -1;
      }

    errlog::log_error(LOG_LEVEL_INFO, "Applying fix 575 to user db");

    // traverse records.
    size_t frec = 0;
    size_t fque = 0;
    size_t ffque = 0;
    std::map<std::string,db_record*> to_add;
    void *rkey = NULL;
    int rkey_size;
    std::vector<std::string> to_remove;
    udb._hdb->dbiterinit();
    while ((rkey = udb._hdb->dbiternext(&rkey_size)) != NULL)
      {
        int value_size;
        void *value = udb._hdb->dbget(rkey, rkey_size, &value_size);
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
                errlog::log_error(LOG_LEVEL_ERROR,"Fix 575: could not extract record plugin and key from internal user db key");
              }
            else if (plugin_name != "query-capture")
              {
              }
            else if (rkey_str != user_db::_db_version_key)
              {
                // get a proper object based on plugin name, and call the virtual function for reading the record.
                plugin *pl = plugin_manager::get_plugin(plugin_name);
                db_record *dbr = NULL;
                if (!pl)
                  {
                    // handle error.
                    errlog::log_error(LOG_LEVEL_ERROR,"Fix 575: could not find plugin %s for pruning user db record",
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
                else
                  {
                    uint32_t fq = 0;
                    std::string nkey = static_cast<db_query_record*>(dbr)->fix_issue_575(fq);
                    if (nkey.empty())
                      nkey = key;

                    if (fq>0)
                      {
                        ffque += fq;
                        fque++;
                        frec++;
                        udb.remove_dbr(rkey_str);
                        udb.add_dbr(nkey,*dbr);
                      }
                  }
                delete dbr;
              }
          }
        free(rkey);
      }

    udb.close_db();
    errlog::log_error(LOG_LEVEL_INFO,"Fix 5775: fixed %u records in user db, dumped %u queries, fixed %u queries",
                      frec,fque,ffque);
    return err;
  }

} /* end of namespace. */
