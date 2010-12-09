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

#include "user_db_fix.h"
#include "user_db.h"
#include "plugin_manager.h"
#include "plugin.h"
#include "db_query_record.h" // XXX: for fixing issue 169. Will disappear afterwards.
using seeks_plugins::db_query_record;

#include "errlog.h"

#include <tcutil.h>
#include <tchdb.h> // tokyo cabinet.
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

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
    user_db cudb("seeks_user.db.tmp"); // new db to be filled up.
    err = cudb.open_db();
    if (err != 0)
      {
        // handle error.
        errlog::log_error(LOG_LEVEL_ERROR,"Could not create the temporary db for fixing the user db");
        udb.close_db();
      }

    /* traverse records */
    void *rkey = NULL;
    void *value = NULL;
    int rkey_size;
    tchdbiterinit(udb._hdb);
    while ((rkey = tchdbiternext(udb._hdb,&rkey_size)) != NULL)
      {
        int value_size;
        value = tchdbget(udb._hdb, rkey, rkey_size, &value_size);
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
        unlink(udb._name.c_str()); // erase current db.
        if (rename(cudb._name.c_str(),udb._name.c_str()) < 0)
          {
            errlog::log_error(LOG_LEVEL_ERROR,"failed renaming fixed user db");
          }
      }
    else unlink(cudb._name.c_str()); // erase temporary db.
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
    tchdbiterinit(udb._hdb);
    while ((rkey = tchdbiternext(udb._hdb,&rkey_size)) != NULL)
      {
        int value_size;
        void *value = tchdbget(udb._hdb, rkey, rkey_size, &value_size);
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

} /* end of namespace. */
