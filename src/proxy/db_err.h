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

#ifndef DB_ERR_H
#define DB_ERR_H

#include "sp_err.h"
typedef int db_err;

#define DB_ERR_CONNECT            501 /**< remote db connection error. */
#define DB_ERR_OPEN               502 /**< db open error. */
#define DB_ERR_CLOSE              503 /**< db close error. */
#define DB_ERR_OPTIMIZE           504 /**< error during db optimization procedure. */
#define DB_ERR_PUT                505 /**< error while putting a element into the db. */
#define DB_ERR_PLUGIN_KEY         506 /**< failed extracting record key and plugin name from internal record key. */
#define DB_ERR_ITER               507 /**< db iterator error. */
#define DB_ERR_MERGE              508 /**< db record merging error. */
#define DB_ERR_MERGE_PLUGIN       509 /**< db record merging from different plugins is an error. */
#define DB_ERR_SERIALIZE          510 /**< db record serialization error. */
#define DB_ERR_REMOVE             511 /**< db record removal error. */
#define DB_ERR_NO_REC             512 /**< db record not found error (on removal for instance). */
#define DB_ERR_CLEAN              513 /**< error while emptying the db. */
#define DB_ERR_SWEEPER_NF         514 /**< sweeper not found. */
#define DB_ERR_UNKNOWN            515 /**< unknown or uncaught error. */
#define DB_ERR_NO_DB              516 /**< no db. */

#endif
