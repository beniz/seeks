/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, <ebenazer@seeks-project.info>
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

#include "peer_list.h"
#include "rank_estimators.h"
#include "cf_configuration.h"
#include "db_record.h"
#include "miscutil.h"
#include "errlog.h"

#include <time.h>
#include <sys/time.h>

using sp::sweeper;
using sp::miscutil;
using sp::errlog;

namespace seeks_plugins
{

  /*- peer -*/
  peer::peer()
    :_port(-1),_status(PEER_OK),_retries(0)
  {
    mutex_init(&_st_mutex);
  }

  peer::peer(const std::string &host,
             const int &port,
             const std::string &path,
             const std::string &rsc)
    :_host(host),_port(port),_path(path),_status(PEER_OK),_retries(0),_rsc(rsc)
  {
    mutex_init(&_st_mutex);
    _key = peer::generate_key(host,port,path);
  }

  peer::~peer()
  {
  }

  std::string peer::generate_key(const std::string &host,
                                 const int &port,
                                 const std::string &path)
  {
    if (port == -1)
      return host + path;
    else return host + ":" + miscutil::to_string(port) + path;
  }

  void peer::set_status_ok()
  {
    mutex_lock(&_st_mutex);
    _status = PEER_OK;
    mutex_unlock(&_st_mutex);
  }

  void peer::set_status_no_connect()
  {
    mutex_lock(&_st_mutex);
    _status = PEER_NO_CONNECT;
    mutex_unlock(&_st_mutex);
  }

  void peer::set_status_unknown()
  {
    mutex_lock(&_st_mutex);
    _status = PEER_UNKNOWN;
    mutex_unlock(&_st_mutex);
  }

  enum PEER_STATUS peer::get_status()
  {
    mutex_lock(&_st_mutex);
    enum PEER_STATUS st = _status;
    mutex_unlock(&_st_mutex);
    return st;
  }

  /*- dead_peer -*/
  peer_list* dead_peer::_dpl = NULL;
  peer_list* dead_peer::_pl = NULL;

  dead_peer::dead_peer()
    :peer()
  {
    update_last_check();
  }

  dead_peer::dead_peer(const std::string &host,
                       const int &port,
                       const std::string &path,
                       const std::string &rsc)
    :peer(host,port,path,rsc)
  {
    update_last_check();
    dead_peer::_dpl->add(this);
    sweeper::register_sweepable(this);
    std::string port_str = (_port != -1) ? miscutil::to_string(_port) : "";
    errlog::log_error(LOG_LEVEL_DEBUG,"marking %s%s%s as a dead peer to monitor",
                      _host.c_str(),port_str.c_str(),_path.c_str());
  }

  dead_peer::~dead_peer()
  {
    // remove from dead peers, destroy and add up to living peer set.
    if (dead_peer::_dpl)
      dead_peer::_dpl->remove(_host,_port,_path);
    if (dead_peer::_pl)
      {
        peer *pe = dead_peer::_pl->get(_key);
        if (pe)
          pe->set_status_ok();
      }
    std::string port_str = (_port != -1) ? miscutil::to_string(_port) : "";
    errlog::log_error(LOG_LEVEL_DEBUG,"marking %s%s%s as a living peer",
                      _host.c_str(),port_str.c_str(),_path.c_str());
  }

  void dead_peer::update_last_check()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    _last_check = tv_now.tv_sec;
  }

  bool dead_peer::is_alive() const
  {
    static std::string test_query = "seeksalivetest";
    static std::string test_key = "c7955c712c49bb7602ff85d3b64bc363ac17033b";

    if (_rsc == "bsn")
      {
        int expansion = 1;
        db_record *dbr = NULL;
        try
          {
            // not using store.
            dbr = rank_estimator::find_bqc(_host,_port,_path,test_query,expansion,false);
          }
        catch (sp_exception &e)
          {
            if (dbr)
              delete dbr;
            return false;
          }
        if (dbr)
          delete dbr;
        return true;
      }
    else if (_rsc == "sn")
      {
        bool in_store = false;
        db_record *dbr = NULL;
        try
          {
            user_db udb(false,_host,_port,_path,_rsc);

            // not using the store.
            dbr = rank_estimator::find_dbr(&udb,test_key,"query-capture",in_store,false);
          }
        catch (sp_exception &e)
          {
            if (dbr)
              delete dbr;
            return false;
          }
        if (dbr)
          delete dbr;
        return true;
      }
    else return false;
  }

  bool dead_peer::sweep_me()
  {
    // time check, avoids checking for dead peer
    // too often.
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    double dt = difftime(tv_now.tv_sec,_last_check);
    if (dt < cf_configuration::_config->_dead_peer_check)
      return false;

    // run an 'alive' test, if peer alive, then delete it.
    bool alive = is_alive();
    update_last_check();
    std::string port_str = (_port != -1) ? ":" + miscutil::to_string(_port) : "";
    std::string msg = "checking on " + _host + port_str + _path + " -> alive: " + miscutil::to_string(alive);
    errlog::log_error(LOG_LEVEL_INFO,msg.c_str());
    if (alive)
      {
        return true;
      }
    else return false;
  }

  /*- peer_list -*/
  peer_list::peer_list()
  {
    mutex_init(&_pl_mutex);
  }

  peer_list::~peer_list()
  {
    mutex_lock(&_pl_mutex);
    hash_map<const char*,peer*,hash<const char*>,eqstr>::iterator hit
    = _peers.begin();
    hash_map<const char*,peer*,hash<const char*>,eqstr>::iterator chit;
    while(hit!=_peers.end())
      {
        chit = hit;
        ++hit;
        delete (*chit).second;
      }
    mutex_unlock(&_pl_mutex);
  }

  void peer_list::add(const std::string &host,
                      const int &port,
                      const std::string &path,
                      const std::string &rsc)
  {
    add(new peer(host,port,path,rsc));
  }

  void peer_list::add(peer *p)
  {
    mutex_lock(&_pl_mutex);
    hash_map<const char*,peer*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=_peers.find(p->_key.c_str()))!=_peers.end())
      {
        // do nothing.
        // XXX: update may be needed if datastructure evolves.
      }
    else _peers.insert(std::pair<const char*,peer*>(p->_key.c_str(),p));

    //TODO: hook up callback for incoming peers.

    mutex_unlock(&_pl_mutex);
  }

  void peer_list::remove(const std::string &host,
                         const int &port,
                         const std::string &path)
  {
    std::string key = peer::generate_key(host,port,path);
    remove(key);
  }

  void peer_list::remove(const std::string &key)
  {
    mutex_lock(&_pl_mutex);
    hash_map<const char*,peer*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=_peers.find(key.c_str()))!=_peers.end())
      {
        _peers.erase(hit);
        mutex_unlock(&_pl_mutex);
        return;
      }
    mutex_unlock(&_pl_mutex);
    errlog::log_error(LOG_LEVEL_ERROR,"Cannot find peer %s to remove from peer list",
                      key.c_str());
  }

  peer* peer_list::get(const std::string &key)
  {
    mutex_lock(&_pl_mutex);
    hash_map<const char*,peer*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=_peers.find(key.c_str()))!=_peers.end())
      {
        mutex_unlock(&_pl_mutex);
        return (*hit).second;
      }
    mutex_unlock(&_pl_mutex);
    return NULL;
  }

} /* end of namespace. */
