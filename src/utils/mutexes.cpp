/**
 * This file is part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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
  
#include "mutexes.h"
#include "errlog.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using sp::errlog;

void mutex_lock(sp_mutex_t *mutex)
{
#ifdef FEATURE_PTHREAD
   int err = pthread_mutex_lock(mutex);
   if (err)
     {
	if (mutex != &errlog::_log_mutex)
	  {
	     errlog::log_error(LOG_LEVEL_FATAL,
			       "Mutex locking failed: %s.\n", strerror(err));
	  }
	exit(1);
     }
#else
   EnterCriticalSection(mutex);
#endif /* def FEATURE_PTHREAD */
}

void mutex_unlock(sp_mutex_t *mutex)
{
#ifdef FEATURE_PTHREAD
   int err = pthread_mutex_unlock(mutex);
   if (err)
     {
	if (mutex != &errlog::_log_mutex)
	  {
	     errlog::log_error(LOG_LEVEL_FATAL,
			       "Mutex unlocking failed: %s.\n", strerror(err));
	  }
	exit(1);
     }
#else
	LeaveCriticalSection(mutex);
#endif /* def FEATURE_PTHREAD */
}

void mutex_init(sp_mutex_t *mutex)
{
#ifdef FEATURE_PTHREAD
   int err = pthread_mutex_init(mutex, 0);
   if (err)
     {
	printf("Fatal error. Mutex initialization failed: %s.\n",
	       strerror(err));
	exit(1);
     }
#else
   InitializeCriticalSection(mutex);
#endif /* def FEATURE_PTHREAD */
}

  
