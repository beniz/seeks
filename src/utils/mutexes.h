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

#ifndef MUTEXES_H
#define MUTEXES_H

#include "config.h"
#include <pthread.h>

typedef pthread_mutex_t sp_mutex_t;
typedef pthread_cond_t  sp_cond_t;

/* mutexes. */
void mutex_lock(sp_mutex_t *mutex);
int mutex_trylock(sp_mutex_t *mutex);
void mutex_unlock(sp_mutex_t *mutex);
void mutex_init(sp_mutex_t *mutex);
void mutex_destroy(sp_mutex_t *mutex);

/* condition variables. */
void cond_init(sp_cond_t *cond);
void cond_wait(sp_cond_t *cond, sp_mutex_t *mutex);
void cond_signal(sp_cond_t *cond);
void cond_broadcast(sp_cond_t *cond);

#endif
