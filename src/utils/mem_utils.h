/**
 * This file is part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 **/ 

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

#endif
