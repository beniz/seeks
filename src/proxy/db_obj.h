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

#ifndef DB_OBJ_H
#define DB_OBJ_H

#include "config.h"

#if defined(TC)
  #include <tcutil.h>
  #include <tchdb.h>
#endif

#if defined(TT)
#include <tcrdb.h>
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

namespace sp
{

  class db_obj
  {
  public:
    db_obj() {};
    
    virtual ~db_obj() {};
    
    virtual int dbecode() const = 0;
    
    virtual const char* dberrmsg(int ecode) const = 0;

    virtual bool dbsetmutex() = 0;
    
    virtual bool dbopen(const char *loc, int c) = 0;

    virtual bool dbclose() = 0;

    virtual bool dbput(const void *kbuf, int ksiz,
		       const void *vbuf, int vsiz) = 0;
    
    virtual void* dbget(const void *kbuf, int ksiz, int *sp) = 0;
  
    virtual bool dbiterinit() = 0;
    
    virtual void* dbiternext(int *sp) = 0;
    
    virtual bool dbout2(const char *kstr) = 0;
    
    virtual bool dbvanish() = 0;

    virtual uint64_t dbfsiz() const = 0;
    
    virtual uint64_t dbrnum() const = 0;
  };

#if defined(TC)  
  class db_obj_local : public db_obj
  {
  public:
    db_obj_local();
    
    virtual ~db_obj_local();
    
    virtual int dbecode() const;
    
    virtual const char* dberrmsg(int ecode) const;

    virtual bool dbsetmutex();
    
    virtual bool dbopen(const char *loc, int c);

    virtual bool dbclose();

    virtual bool dbput(const void *kbuf, int ksiz,
		       const void *vbuf, int vsiz);
    
    virtual void* dbget(const void *kbuf, int ksiz, int *sp);
    
    virtual bool dbiterinit();
    
    virtual void* dbiternext(int *sp);
    
    virtual bool dbout2(const char *kstr);
    
    virtual bool dbvanish();

    virtual uint64_t dbfsiz() const;
    
    virtual uint64_t dbrnum() const;

    bool dbtune(int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts);
	
    bool dboptimize(int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts);
    
    TCHDB *_hdb; /**< Tokyo Cabinet hashtable db. */
  };
#endif

#if defined (TT)
  class db_obj_remote : public db_obj
  {
  public:
    db_obj_remote();
    
    virtual ~db_obj_remote();
    
    virtual int dbecode() const = 0;
    
    virtual const char* dberrmsg(int ecode) const = 0;

    virtual bool dbsetmutex() = 0;
    
    virtual bool dbopen(const char *loc, int c) = 0;

    virtual bool dbclose() = 0;

    virtual bool dbput(const void *kbuf, int ksiz,
		       const void *vbuf, int vsiz) = 0;
    
    virtual void* dbget(const void *kbuf, int ksiz, int *sp) = 0;
  
    virtual bool dbiterinit() = 0;
    
    virtual void* dbiternext(int *sp) = 0;
    
    virtual bool dbout2(const char *kstr) = 0;
    
    virtual bool dbvanish() = 0;

    virtual uint64_t dbfsiz() const = 0;
    
    virtual uint64_t dbrnum() const = 0;

    /*bool dbtune(int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts);

      bool dboptimize(int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts); */

    TCRDB *_hdb; /**< Tokyo Tyrant remote db. */
  };
#endif

} /* end of namespace. */

#endif
