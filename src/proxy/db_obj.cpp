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

#include "db_obj.h"
#include "miscutil.h"
#include <iostream>

namespace sp
{

#if defined(TC)

  /*- db_obj_local -*/
  std::string db_obj_local::_db_name = "seeks_user.db"; // default.

  db_obj_local::db_obj_local()
    :db_obj()
  {
    _hdb = tchdbnew();
  }

  db_obj_local::~db_obj_local()
  {
    tchdbdel(_hdb);
  }

  int db_obj_local::dbecode() const
  {
    return tchdbecode(_hdb);
  }

  const char* db_obj_local::dberrmsg(int ecode) const
  {
    return tchdberrmsg(ecode);
  }

  bool db_obj_local::dbsetmutex()
  {
    return tchdbsetmutex(_hdb);
  }

  bool db_obj_local::dbopen(int c)
  {
    return tchdbopen(_hdb,_name.c_str(),c);
  }

  bool db_obj_local::dbclose()
  {
    return tchdbclose(_hdb);
  }

  bool db_obj_local::dbput(const void *kbuf, int ksiz,
                           const void *vbuf, int vsiz)
  {
    return tchdbput(_hdb,kbuf,ksiz,vbuf,vsiz);
  }

  void* db_obj_local::dbget(const void *kbuf, int ksiz, int *sp)
  {
    return tchdbget(_hdb,kbuf,ksiz,sp);
  }

  bool db_obj_local::dbiterinit()
  {
    return tchdbiterinit(_hdb);
  }

  void* db_obj_local::dbiternext(int *sp)
  {
    return tchdbiternext(_hdb,sp);
  }

  bool db_obj_local::dbout2(const char *kstr)
  {
    return tchdbout2(_hdb,kstr);
  }

  bool db_obj_local::dbvanish()
  {
    return tchdbvanish(_hdb);
  }

  uint64_t db_obj_local::dbfsiz() const
  {
    return tchdbfsiz(_hdb);
  }

  uint64_t db_obj_local::dbrnum() const
  {
    return tchdbrnum(_hdb);
  }

  bool db_obj_local::dbtune(int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts)
  {
    return tchdbtune(_hdb,bnum,apow,fpow,opts);
  }

  bool db_obj_local::dboptimize(int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts)
  {
    return tchdboptimize(_hdb,bnum,apow,fpow,opts);
  }

#endif // defined(TC)

#if defined(TT)

  /*- db_obj_remote -*/
  db_obj_remote::db_obj_remote(const std::string &host,
                               const int &port)
    :db_obj(),_host(host),_port(port)
  {
    _hdb = tcrdbnew();
  }

  db_obj_remote::~db_obj_remote()
  {
    tcrdbdel(_hdb);
  }

  int db_obj_remote::dbecode() const
  {
    return tcrdbecode(_hdb);
  }

  const char* db_obj_remote::dberrmsg(int ecode) const
  {
    return tcrdberrmsg(ecode);
  }

  bool db_obj_remote::dbopen(int c)
  {
    // c is ignored
    return tcrdbopen(_hdb,_host.c_str(),_port);
  }

  bool db_obj_remote::dbclose()
  {
    return tcrdbclose(_hdb);
  }

  bool db_obj_remote::dbput(const void *kbuf, int ksiz,
                            const void *vbuf, int vsiz)
  {
    return tcrdbput(_hdb,kbuf,ksiz,vbuf,vsiz);
  }

  void* db_obj_remote::dbget(const void *kbuf, int ksiz, int *sp)
  {
    return tcrdbget(_hdb,kbuf,ksiz,sp);
  }

  bool db_obj_remote::dbiterinit()
  {
    return tcrdbiterinit(_hdb);
  }

  void* db_obj_remote::dbiternext(int *sp)
  {
    return tcrdbiternext(_hdb,sp);
  }

  bool db_obj_remote::dbout2(const char *kstr)
  {
    return tcrdbout2(_hdb,kstr);
  }

  bool db_obj_remote::dbvanish()
  {
    return tcrdbvanish(_hdb);
  }

  uint64_t db_obj_remote::dbfsiz() const
  {
    return tcrdbsize(_hdb);
  }

  uint64_t db_obj_remote::dbrnum() const
  {
    return tcrdbrnum(_hdb);
  }

  //TODO: dbtune & dboptimize.

  std::string db_obj_remote::get_name() const
  {
    return _host + ":" + miscutil::to_string(_port);
  }

#endif

} /* end of namespace. */
