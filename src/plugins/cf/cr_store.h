/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

/**
 * \brief cr_store is a record store in which items are put from a
 *        cr_store object. Item removal however is automatic. Every time
 *        a cached_record is deleted, its removal trickles up the chain
 *        and frees cr_cache entries as needed.
 *
 *        Adding an item to the cr_store registers a cached_record object
 *        with the Seeks' proxy sweeper. When a cached_record object times
 *        out, the sweeper deletes it, triggering the deletion chain.
 */

#ifndef CR_STORE_H
#define CR_STORE_H

#include "stl_hash.h"
#include "sweeper.h"
#include "db_record.h"
#include "mutexes.h"

using sp::sweepable;
using sp::db_record;

namespace seeks_plugins
{

  class cr_cache;
  class cr_store;

  class cached_record : public sweepable
  {
    public:
      cached_record(const std::string &key,
                    db_record *rec,
                    cr_cache *cache);

      virtual ~cached_record();

      virtual bool sweep_me();

      void update_last_use();

      std::string _key;
      db_record *_rec;
      time_t _last_use;

    private:
      cr_cache *_cache;
  };

  class cr_cache
  {
    public:
      cr_cache(const std::string &peer,
               cr_store *store);

      ~cr_cache();

      void add(const std::string &key,
               db_record *rec);

      void remove(const std::string &key);

      cached_record *find(const std::string &key);

      std::string _peer;
      hash_map<const char*,cached_record*,hash<const char*>,eqstr> _records;

    private:
      cr_store *_store;
      sp_mutex_t _cache_mutex;
  };

  class cr_store
  {
    public:
      cr_store();

      ~cr_store();

      void add(const std::string &host,
               const int &port,
               const std::string &path,
               const std::string &key,
               db_record *rec);

      void add(const std::string &peer,
               const std::string &key,
               db_record *rec);

      void remove(const std::string &host,
                  const int &port,
                  const std::string &path);

      void remove(const std::string &peer);

      db_record* find(const std::string &host,
                      const int &port,
                      const std::string &path,
                      const std::string &key,
                      bool &has_key);

      db_record* find(const std::string &peer,
                      const std::string &key,
                      bool &has_key);

      static std::string generate_peer(const std::string &host,
                                       const int &port,
                                       const std::string &path="");

      hash_map<const char*,cr_cache*,hash<const char*>,eqstr> _store;
    private:
      sp_mutex_t _store_mutex;
  };

} /* end of namespace. */

#endif
