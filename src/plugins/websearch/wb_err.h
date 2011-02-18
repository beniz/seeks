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

#ifndef WB_ERR_H
#define WB_ERR_H

#include "sp_err.h"

#define WB_ERR_SE_CONNECT        1001 /**< connection error to all search engines. */
#define WB_ERR_QUERY_ENCODING    1002 /**< query encoding error. */
#define WB_ERR_NO_ENGINE         1003 /**< no activated search engine. */
#define WB_ERR_NO_ENGINE_OUTPUT  1004 /**< no output from search engines. */
#define WB_ERR_PARSE             1005 /**< parsing error. */
#define WB_ERR_THREAD            1006 /**< thread creation error. */
#define WB_ERR_NO_REF_SIM        1007 /**< no reference features for similarity analysis. */

#endif
