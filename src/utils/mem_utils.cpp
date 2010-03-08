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

#include "mem_utils.h"

#include <cstdlib>
#include <cstring>

void free_const(const void *p)
{
   std::free((void *) p);
   p = NULL;
}

void freez(void *p)
{
   std::free(p);
   p = NULL;
};

void* zalloc(size_t size)
{  
   void * ret;
   if ((ret = (void *)std::malloc(size)) != NULL)
     {
	std::memset(ret, 0, size);
     }
   return(ret);
}
