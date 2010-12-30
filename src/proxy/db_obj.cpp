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
#include <iostream>

namespace sp
{

#if defined(TC)

  /*- db_obj_local -*/
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
  
  bool db_obj_local::dbopen(const char *loc, int c)
  {
    return tchdbopen(_hdb,loc,c);
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
  db_obj_remote::db_obj_remote()
    :db_obj()
  {
    
  }

#endif

} /* end of namespace. */
