/**
 * This file is part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
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

#include <cstdlib>
#include <cstring>

#ifndef MEM_UTILS_H
#define MEM_UTILS_H

#ifndef FREE_CONST
#define FREE_CONST
void free_const(const void *p);
#endif

#ifndef FREEZ
#define FREEZ
void freez(void *p);
#endif

#ifndef ZALLOC
void* zalloc(size_t size);
#endif

struct delete_object
{
  template<typename T>
  void operator()(const T* ptr) const
  {
    delete ptr;
  }
};

#endif
