/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#ifndef UDBS_ERR_H
#define UDBS_ERR_H

#include "sp_err.h"
typedef int udbs_err;

#define UDBS_ERR_SERIALIZE                   4001 /**< msg serialization error. */
#define UDBS_ERR_DESERIALIZE                 4002 /**< msg deserialization error. */
#define UDBS_ERR_CONNECT                     4003 /**< peer connection error. */

#endif
