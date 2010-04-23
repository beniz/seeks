/*********************************************************************
 * Purpose     :  Log errors to a designated destination in an elegant,
 *                printf-like fashion.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001-2009 the SourceForge
 *                Privoxy team. http://www.privoxy.org/
 *
 *                Based on the Internet Junkbuster originally written
 *                by and Copyright (C) 1997 Anonymous Coders and
 *                Junkbusters Corporation.  http://www.junkbusters.com
 *
 *                This program is free software; you can redistribute it
 *                and/or modify it under the terms of the GNU General
 *                Public License as published by the Free Software
 *                Foundation; either version 2 of the License, or (at
 *                your option) any later version.
 *
 *                This program is distributed in the hope that it will
 *                be useful, but WITHOUT ANY WARRANTY; without even the
 *                implied warranty of MERCHANTABILITY or FITNESS FOR A
 *                PARTICULAR PURPOSE.  See the GNU General Public
 *                License for more details.
 *
 *                The GNU General Public License should be included with
 *                this file.  If not, you can view it at
 *                http://www.gnu.org/copyleft/gpl.html
 *                or write to the Free Software Foundation, Inc., 59
 *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *********************************************************************/

#ifndef ERRLOG_H
#define ERRLOG_H

#include <stdio.h>

/* Debug level for errors */
 
/* XXX: Should be renamed. */
#define LOG_LEVEL_GPC        0x0001
#define LOG_LEVEL_CONNECT    0x0002
#define LOG_LEVEL_IO         0x0004
#define LOG_LEVEL_HEADER     0x0008
#define LOG_LEVEL_LOG        0x0010
#ifdef FEATURE_FORCE_LOAD
#define LOG_LEVEL_FORCE      0x0020
#endif /* def FEATURE_FORCE_LOAD */
#define LOG_LEVEL_RE_FILTER  0x0040
#define LOG_LEVEL_REDIRECTS  0x0080
#define LOG_LEVEL_DEANIMATE  0x0100
#define LOG_LEVEL_CLF        0x0200 /* Common Log File format */
#define LOG_LEVEL_CRUNCH     0x0400
#define LOG_LEVEL_CGI        0x0800 /* CGI / templates */
#define LOG_LEVEL_DHT        0x8000 /* DHT output */

/* Following are always on: */
#define LOG_LEVEL_INFO    0x1000
#define LOG_LEVEL_ERROR   0x2000
#define LOG_LEVEL_FATAL   0x4000 /* Exits after writing log */

namespace sp
{
   class errlog
     {
      public:
	static void init_error_log(const char *prog_name, const char *logfname);
	static void set_debug_level(int debuglevel);
	static void disable_logging();
	static void init_log_module();
	static void show_version(const char *prog_name);
	static void log_error(int loglevel, const char *fmt, ...);
	static const char* sp_err_to_string(int error);

	static void fatal_error(const char *error_message);

	static long get_thread_id();
	
	static size_t get_log_timestamp(char *buffer, size_t buffer_size);
	static size_t get_clf_timestamp(char *buffer, size_t buffer_size);
	static const char* get_log_level_string(int loglevel);
	
#ifdef _WIN32
	static char *w32_socket_strerr(int errcode, char *tmp_buf);
#endif
	
#ifdef MUTEX_LOCKS_AVAILABLE
	static void lock_logfile();
	static void unlock_logfile();
	static void lock_loginit();
	static void unlock_loginit();
#else /* ! MUTEX_LOCKS_AVAILABLE */
	/*
	 * FIXME we need a cross-platform locking mechanism.
	 * The locking/unlocking functions below should be
	 * fleshed out for non-pthread implementations.
	 */
	static void lock_logfile() {};
	static void unlock_logfile() {};
	static void lock_loginit() {};
	static void unlock_loginit() {};
#endif
	static FILE *_logfp;
	static int _debug;
     };
} /* end of namespace. */

#endif
