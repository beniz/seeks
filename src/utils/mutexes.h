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

#ifndef MUTEXES_H
#define MUTEXES_H

#include "config.h"

#ifdef FEATURE_PTHREAD
extern "C"
{
# include <pthread.h>
}
#endif

#ifdef FEATURE_PTHREAD
typedef pthread_mutex_t sp_mutex_t;
#else
typedef CRITICAL_SECTION sp_mutex_t;
#endif

/* mutexes. */
void mutex_lock(sp_mutex_t *mutex);
void mutex_unlock(sp_mutex_t *mutex);
void mutex_init(sp_mutex_t *mutex);

#endif
