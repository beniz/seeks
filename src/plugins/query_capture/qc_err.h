/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, <ebenazer@seeks-project.info>
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

#ifndef QC_ERR_H
#define QC_ERR_H

#include "sp_err.h"
typedef int qc_err;

#define QC_ERR_STORE_QUERY           2001 /**< error while storing query. */
#define QC_ERR_STORE_URL             2002 /**< error while storing URL. */
#define QC_ERR_STORE                 2003 /**< generic storing error (URL + query). */
#define QC_ERR_REMOVE_QUERY          2004 /**< error while removing query. */

#endif
