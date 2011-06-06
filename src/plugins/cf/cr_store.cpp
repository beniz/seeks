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

#include "cr_store.h"
#include "cf_configuration.h"
#include "miscutil.h"
#include "errlog.h"

#include <time.h>
#include <sys/time.h>

#include <iostream>

using sp::sweeper;
using sp::miscutil;
using sp::errlog;

namespace seeks_plugins
{

  /*- cached_records. -*/

  cached_record::cached_record(const std::string &key,
                               db_record *rec,
                               cr_cache *cache)
    :_key(key),_rec(rec),_cache(cache)
  {
    update_last_use();
  }

  cached_record::~cached_record()
  {
    // remove from cache object and destroy record.
    _cache->remove(_key);
    if (_rec)
      delete _rec;
    if (_cache->_records.empty())
      delete _cache;
  }

  bool cached_record::sweep_me()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    double dt = difftime(tv_now.tv_sec,_last_use);

    if (dt >= cf_configuration::_config->_record_cache_timeout)
      return true;
    else return false;
  }

  void cached_record::update_last_use()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    _last_use = tv_now.tv_sec;
  }

  /*- cr_cache -*/
  cr_cache::cr_cache(const std::string &peer,
                     cr_store *store)
    :_peer(peer),_store(store)
  {
    mutex_init(&_cache_mutex);
  }

  cr_cache::~cr_cache()
  {
    _store->remove(_peer);
  }

  void cr_cache::add(const std::string &key,
                     db_record *rec)
  {
    mutex_lock(&_cache_mutex);
    hash_map<const char*,cached_record*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=_records.find(key.c_str()))!=_records.end())
      {
        // update existing record with new one.
        // Beware as old record is 'sweepable', a call to unregister_sweepable
        // may be under way.
        cached_record *cro = (*hit).second;
        sweeper::unregister_sweepable(cro);
        mutex_unlock(&_cache_mutex);
        delete cro; // down to top: removes and delete cro;
        mutex_lock(&_cache_mutex);
      }
    cached_record *cr = new cached_record(key,rec,this);
    _records.insert(std::pair<const char*,cached_record*>(cr->_key.c_str(),cr));
    sweeper::register_sweepable(cr);
    mutex_unlock(&_cache_mutex);
  }

  void cr_cache::remove(const std::string &key)
  {
    mutex_lock(&_cache_mutex);
    hash_map<const char*,cached_record*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=_records.find(key.c_str()))!=_records.end())
      {
        _records.erase(hit);
      }
    mutex_unlock(&_cache_mutex);
  }

  cached_record* cr_cache::find(const std::string &key)
  {
    mutex_lock(&_cache_mutex);
    hash_map<const char*,cached_record*,hash<const char*>,eqstr>::const_iterator hit;
    if ((hit=_records.find(key.c_str()))!=_records.end())
      {
        mutex_unlock(&_cache_mutex);
        return (*hit).second;
      }
    mutex_unlock(&_cache_mutex);
    return NULL;
  }

  /*- cr_store -*/

  cr_store::cr_store()
  {
    mutex_init(&_store_mutex);
  }

  cr_store::~cr_store()
  {
  }

  void cr_store::add(const std::string &host,
                     const int &port,
                     const std::string &path,
                     const std::string &key,
                     db_record *rec)
  {
    std::string peer = cr_store::generate_peer(host,port,path);
    add(peer,key,rec);
  }

  void cr_store::add(const std::string &peer,
                     const std::string &key,
                     db_record *rec)
  {
    mutex_lock(&_store_mutex);
    hash_map<const char*,cr_cache*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=_store.find(peer.c_str()))!=_store.end())
      {
        cr_cache *crc = (*hit).second;
        crc->add(key,rec);
      }
    else
      {
        cr_cache *crc = new cr_cache(peer,this);
        crc->add(key,rec);
        _store.insert(std::pair<const char*,cr_cache*>(crc->_peer.c_str(),crc));
      }
    mutex_unlock(&_store_mutex);
  }

  void cr_store::remove(const std::string &host,
                        const int &port,
                        const std::string &path)
  {
    std::string peer = cr_store::generate_peer(host,port,path);
    remove(peer);
  }

  void cr_store::remove(const std::string &peer)
  {
    mutex_lock(&_store_mutex);
    hash_map<const char*,cr_cache*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=_store.find(peer.c_str()))!=_store.end())
      {
        _store.erase(hit);
      }
    else errlog::log_error(LOG_LEVEL_ERROR,"cannot find record cache entry %s",peer.c_str());
    mutex_unlock(&_store_mutex);
  }

  db_record* cr_store::find(const std::string &host,
                            const int &port,
                            const std::string &path,
                            const std::string &key,
                            bool &has_key)
  {
    std::string peer = cr_store::generate_peer(host,port,path);
    return find(peer,key,has_key);
  }

  db_record* cr_store::find(const std::string &peer,
                            const std::string &key,
                            bool &has_key)
  {
    mutex_lock(&_store_mutex);
    hash_map<const char*,cr_cache*,hash<const char*>,eqstr>::const_iterator hit;
    if ((hit=_store.find(peer.c_str()))!=_store.end())
      {
        cr_cache *crc = (*hit).second;
        cached_record *cr = crc->find(key);
        mutex_unlock(&_store_mutex);
        if (!cr)
          {
            has_key = false;
            mutex_unlock(&_store_mutex);
            return NULL;
          }
        has_key = true;
        cr->update_last_use();
        mutex_unlock(&_store_mutex);
        return cr->_rec;
      }
    has_key = false;
    mutex_unlock(&_store_mutex);
    return NULL;
  }

  std::string cr_store::generate_peer(const std::string &host,
                                      const int &port,
                                      const std::string &path)
  {
    if (port != -1)
      return host + ":" + miscutil::to_string(port) + path;
    else return host + path;
  }

} /* end of namespace. */
