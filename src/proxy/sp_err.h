/**
 * The seeks proxy is part of the SEEKS project
 * It is based on Privoxy (http://www.privoxy.org), developped
 * by the Privoxy team.
 *
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/**
 * A standard error code.  This should be SP_ERR_OK or one of the SP_ERR_xxx
 * series of errors.
 */
typedef int sp_err;

#define SP_ERR_OK         0 /**< Success, no error                        */
#define SP_ERR_MEMORY     1 /**< Out of memory                            */
#define SP_ERR_CGI_PARAMS 2 /**< Missing or corrupt CGI parameters        */
#define SP_ERR_FILE       3 /**< Error opening, reading or writing a file */
#define SP_ERR_PARSE      4 /**< Error parsing file                       */
#define SP_ERR_MODIFIED   5 /**< File has been modified outside of the
CGI actions editor.                      */
#define SP_ERR_COMPRESS   6 /**< Error on decompression                   */
