/**
 * This file is part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera <ebenazer@seeks-project.info>
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
}
int mutex_trylock(sp_mutex_t *mutex)
{
  return pthread_mutex_trylock(mutex);
}

void mutex_unlock(sp_mutex_t *mutex)
{
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
}

void mutex_init(sp_mutex_t *mutex)
{
  int err = pthread_mutex_init(mutex, 0);
  if (err)
    {
      printf("Fatal error. Mutex initialization failed: %s.\n",
             strerror(err));
      exit(1);
    }
}

void mutex_destroy(sp_mutex_t *mutex)
{
  pthread_mutex_destroy(mutex); // we don't track failures.
}

void cond_init(sp_cond_t *cond)
{
  int err = pthread_cond_init(cond,0);
  if (err)
    {
      errlog::log_error(LOG_LEVEL_FATAL,
                        "Conditional variable initialization failed: %s.\n",strerror(err));
      exit(1); // in case fatal is turned into non exiting call.
    }
}

void cond_wait(sp_cond_t *cond, sp_mutex_t *mutex)
{
  int err = pthread_cond_wait(cond,mutex);
  if (err)
    {
      errlog::log_error(LOG_LEVEL_FATAL,
                        "Mutex conditional wait failed: %s.\n",strerror(err));
      exit(1); // in case fatal is turned into non exiting call.
    }
}

void cond_signal(sp_cond_t *cond)
{
  int err = pthread_cond_signal(cond);
  if (err)
    {
      errlog::log_error(LOG_LEVEL_FATAL,
                        "Conditional signalling failed: %s.\n",strerror(err));
      exit(1); // in case fatal is turned into non exiting call.
    }
}

void cond_broadcast(sp_cond_t *cond)
{
  int err = pthread_cond_broadcast(cond);
  if (err)
    {
      errlog::log_error(LOG_LEVEL_FATAL,
                        "Conditional broadcasting failed: %s.\n",strerror(err));
      exit(1); // in case fatal is turned into non exiting call.
    }
}
