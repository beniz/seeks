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
