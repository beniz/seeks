/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010, 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#ifndef USER_DB_H
#define USER_DB_H

#include "db_err.h"
#include "mutexes.h"
#include "db_record.h"
#include "sweeper.h"
#include "db_obj.h"

#include <vector>
#include <ostream>

namespace sp
{

  class user_db_sweepable : public sweepable
  {
    public:
      user_db_sweepable();

      virtual ~user_db_sweepable();

      virtual bool sweep_me()
      {
        return false;
      };

      virtual int sweep_records()
      {
        return 0;
      };
  };

  class user_db
  {
    public:
      /**
       * \brief db constructor, called by system.
       * @param local whether the user db is local or remote.
       * @param haddr the host address of the remote db, empty means from config.
       * @param hport the host port of the remote db.
       */
      user_db(const bool &local=true,
              const std::string &dbname="",
              const std::string &haddr="",
              const int &hport=-1,
              const std::string &hpath="",
              const std::string &rsc="");

      /**
       * \brief db constructor, for opening a existing db by its name.
       */
      user_db(const std::string &dbname);

      /**
       * \brief db desctructor, called by system.
       */
      ~user_db();

      /**
       * \brief opens the db if not already opened.
       * @return SP_ERR_OK if no error, DB_ERR_OPEN otherwise.
       */
      db_err open_db();

      /**
       * \brief opens the db in read-only mode if not already opened.
       * @return SP_ERR_OK if no error, DB_ERR_OPEN otherwise.
       */
      db_err open_db_readonly();

      /**
       * \brief closes the db if opened.
       */
      db_err close_db();

      /**
       * \brief optimizes database.
       * @return SP_ERR_OK if no error, DB_ERR_OPTIMIZE otherwise.
       */
      db_err optimize_db();

      /**
       * \brief sets db version
       * @param v version to be set.
       * @return SP_ERR_OK if no error, DB_ERR_PUT otherwise.
       */
      db_err set_version(const double &v);

      /**
       * \brief gets db version.
       * @return db version, 0.0 if an error occurred.
       */
      double get_version();

      /**
       * \brief whether the db is remote or not.
       * @return true if it is a remote db.
       */
      bool is_remote() const;

      /**
       * \brief whether the db is currently open for operations or not.
       * @return true if it is open.
       */
      bool is_open() const;

      /**
       * \brief generates a unique key for the record, from a given key and the plugin name.
       */
      static std::string generate_rkey(const std::string &key,
                                       const std::string &plugin_name);

      /**
       * \brief extracts the record key from the internal record key.
       */
      static std::string extract_key(const std::string &rkey);

      /**
       * \brief extracts both the record key and the plugin name
       *        from the internal record key.
       * @return SP_ERR_OK if no error, DB_ERR_PLUGIN_KEY otherwise.
       */
      static db_err extract_plugin_and_key(const std::string &rkey,
                                           std::string &plugin_name,
                                           std::string &key);

      /**
       * \brief finds a set of matching records based on a key.
       * Beware: this function iterates all records!
       * @param ref_key the reference record key,
       * @param plugin_name the reference plugin name,
       * @param matching_rkeys vector of matching rkeys (of the form plugin_name:key).
       * @return SP_ERR_OK if no error, DB_ERR_ITER otherwise.
       */
      int find_matching(const std::string &ref_key,
                        const std::string &plugin_name,
                        std::vector<std::string> &matching_rkeys);

      /**
       * \brief finds a record based on its key.
       * @param key is the record key.
       * @param plugin_name is the plugin name from which the db record storage name is to be generated.
       * @return db_record if found, NULL otherwise.
       */
      db_record* find_dbr(const std::string &key,
                          const std::string &plugin_name);

      /**
       * \brief serializes and adds record to db.
       * @param key is record key (the internal key is a mixture of plugin name and record key).
       * @param dbr the db record object to be serialized and added.
       * @return SP_ERR_OK if no error, error code otherwise.
       * @see db_err.h
       */
      db_err add_dbr(const std::string &key,
                     const db_record &dbr);

      /**
       * \brief removes record with key 'rkey'.
       * @param key is record key (the internal key is a mixture of plugin name and record key).
       * @return SP_ERR_OK if no error, error code otherwise.
       */
      db_err remove_dbr(const std::string &rkey);

      /**
       * \brief removes record that has 'key' and belongs to plugin 'plugin_name'.
       * @return SP_ERR_OK if no error, error code otherwise.
       */
      db_err remove_dbr(const std::string &key,
                        const std::string &plugin_name);

      /**
       * \brief removes all records in db.
       * @return SP_ERR_OK if no error, DB_ERR_CLEAN otherwise.
       */
      db_err clear_db();

      /**
       * \brief removes all records older than date.
       * @return SP_ERR_OK if no error, error code otherwise.
       */
      db_err prune_db(const time_t &date);

      /**
       * \brief removes all records that belong to plugin 'plugin_name'.
       *        If date != 0 only records that are older than date are removed.
       * @return SP_ERR_OK if no error, error code otherwise.
       */
      db_err prune_db(const std::string &plugin_name,
                      const time_t date = 0);

      /**
       * \brief applies a virtual function do_smthg over data for every
       *        record matching plugin plugin_name.
       * @return SP_ERR_OK if no error, error code otherwise.
       */
      db_err do_smthg_db(const std::string &plugin_name,
                         void *data);

      /**
       * \brief returns db size on disk.
       */
      uint64_t disk_size() const;

      /**
       * \brief returns number of records in db.
       */
      uint64_t number_records() const;

      /**
       * \brief returns the number of records held by a given plugin.
       * This procedure requires a full traverse of the db, and is thus not efficient.
       */
      uint64_t number_records(const std::string &plugin_name) const;

      /**
       * \brief reads and prints out all db records.
       */
      std::ostream& print(std::ostream &output);

      /**
       * \brief export all db records.
       */
      std::ostream& export_db(std::ostream &output, std::string format);

      /**
       * \brief registers user_db_sweepable for plugin-directed db cleanup operations.
       */
      void register_sweeper(user_db_sweepable *uds);

      /**
       * \brief unregisters user_db_sweepable.
       * @return SP_ERR_OK if no error, DB_ERR_SWEEPER_NF otherwise.
       */
      db_err unregister_sweeper(user_db_sweepable *uds);

      /**
       * \brief calls on sweepers.
       */
      int sweep_db();

      /*- 'sn' remote resource calls. -*/
      db_record *find_dbr_rsc_sn(const std::string &key,
                                 const std::string &plugin_name);

    public:
      db_obj *_hdb; /**< local or remote Tokyo Cabinet hashtable db. */
      bool _opened; /**< whether the db is opened. */
      std::vector<user_db_sweepable*> _db_sweepers;

      static std::string _db_version_key; /**< db version record key. */
      static double _db_version; /**< db record structure version. */

    private:
      sp_mutex_t _db_mutex; /**< mutex around db operations. */
      std::string _rsc; /**< remote resource type ("", tt or sn). */
  };

} /* end of namespace. */

#endif
