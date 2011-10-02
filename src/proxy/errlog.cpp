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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "mem_utils.h"
#include "config.h"
#include "miscutil.h"

/* For gettimeofday() */
#include <sys/time.h>

#if !defined(_WIN32) && !defined(__OS2__)
#include <unistd.h>
#endif /* !defined(_WIN32) && !defined(__OS2__) */

#include <errno.h>
#include <assert.h>

#ifdef _WIN32
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#ifndef _WIN_CONSOLE
#include "w32log.h"
#endif /* ndef _WIN_CONSOLE */
#endif /* def _WIN32 */
#ifdef _MSC_VER
#define inline __inline
#endif /* def _MSC_VER */

#include "errlog.h"
#include "proxy_dts.h"
#include "seeks_proxy.h"

namespace sp
{

  sp_mutex_t errlog::_log_mutex;

  /*
   * LOG_LEVEL_FATAL cannot be turned off.  (There are
   * some exceptional situations where we need to get a
   * message to the user).
   */
#define LOG_LEVEL_MINIMUM  LOG_LEVEL_FATAL

  FILE *errlog::_logfp = NULL;
  int errlog::_debug = (LOG_LEVEL_FATAL | LOG_LEVEL_ERROR);

  /*********************************************************************
   *  *
   * Function    :  fatal_error
   *
   * Description :  Displays a fatal error to standard error (or, on
   *                a WIN32 GUI, to a dialog box), and exits Privoxy
   *                with status code 1.
   *
   * Parameters  :
   *          1  :  error_message = The error message to display.
   *
   * Returns     :  Does not return.
   *
   *********************************************************************/
  void errlog::fatal_error(const char *error_message)
  {
#if defined(_WIN32) && !defined(_WIN_CONSOLE)
    /* Skip timestamp and thread id for the message box. */
    const char *box_message = strstr(error_message, "Fatal error");
    if (NULL == box_message)
      {
        /* Shouldn't happen but ... */
        box_message = error_message;
      }

    MessageBox(g_hwndLogFrame, box_message, "Seeks Error",
               MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST);

    /* Cleanup - remove taskbar icon etc. */
    TermLogWindow();
#endif /* defined(_WIN32) && !defined(_WIN_CONSOLE) */

    if (errlog::_logfp != NULL)
      {
        fputs(error_message, errlog::_logfp);
      }

#if defined(unix)
    if (seeks_proxy::_pidfile)
      {
        unlink(seeks_proxy::_pidfile);
      }
#endif /* unix */

    exit(1);
  }

  /*********************************************************************
   *
   * Function    :  show_version
   *
   * Description :  Logs the Seeks version and the program name.
   *
   * Parameters  :
   *          1  :  prog_name = The program name.
   *
   * Returns     :  Nothing.
   *
   *********************************************************************/
  void errlog::show_version(const char *prog_name)
  {
    errlog::log_error(LOG_LEVEL_INFO, "Seeks version " VERSION);
    if (prog_name != NULL)
      {
        errlog::log_error(LOG_LEVEL_INFO, "Program name: %s", prog_name);
      }
  }

  /*********************************************************************
   *
   * Function    :  init_log_module
   *
   * Description :  Initializes the logging module to log to stderr.
   *                Can only be called while stderr hasn't been closed
   *                yet and is only supposed to be called once.
   *
   * Parameters  :
   *          1  :  prog_name = The program name.
   *
   * Returns     :  Nothing.
   *
   *********************************************************************/
  void errlog::init_log_module(void)
  {
    errlog::lock_logfile();
    errlog::_logfp = stderr;
    errlog::unlock_logfile();
    errlog::set_debug_level(errlog::_debug);
  }

  /*********************************************************************
   *
   * Function    :  set_debug_level
   *
   * Description :  Sets the debug level to the provided value
   *                plus LOG_LEVEL_MINIMUM.
   *
   *                XXX: we should only use the LOG_LEVEL_MINIMUM
   *                until the first time the configuration file has
   *                been parsed.
   *
   * Parameters  :  1: debug_level = The debug level to set.
   *
   * Returns     :  Nothing.
   *
   *********************************************************************/
  void errlog::set_debug_level(int debug_level)
  {
    _debug = debug_level | LOG_LEVEL_MINIMUM;
  }

  /*********************************************************************
   *
   * Function    :  disable_logging
   *
   * Description :  Disables logging.
   *
   * Parameters  :  None.
   *
   * Returns     :  Nothing.
   *
   *********************************************************************/
  void errlog::disable_logging(void)
  {
    if (errlog::_logfp != NULL)
      {
        errlog::log_error(LOG_LEVEL_INFO,
                          "No logfile configured. Please enable it before reporting any problems.");
        errlog::lock_logfile();
        fclose(errlog::_logfp);
        errlog::_logfp = NULL;
        errlog::unlock_logfile();
      }
  }

  /*********************************************************************
   *
   * Function    :  init_error_log
   *
   * Description :  Initializes the logging module to log to a file.
   *
   *                XXX: should be renamed.
   *
   * Parameters  :
   *          1  :  prog_name  = The program name.
   *          2  :  logfname   = The logfile to (re)open.
   *
   * Returns     :  N/A
   *
   *********************************************************************/
  void errlog::init_error_log(const char *prog_name, const char *logfname)
  {
    FILE *fp;
    assert(NULL != logfname);
    errlog::lock_loginit();

    if ((errlog::_logfp != NULL) && (errlog::_logfp != stderr))
      {
        errlog::log_error(LOG_LEVEL_INFO, "(Re-)Opening logfile \'%s\'", logfname);
      }

    /* set the designated log file */
    fp = fopen(logfname, "a");
    if ((NULL == fp) && (errlog::_logfp != NULL))
      {
        /*
         * Some platforms (like OS/2) don't allow us to open
         * the same file twice, therefore we give it another
         * shot after closing the old file descriptor first.
         *
         * We don't do it right away because it prevents us
         * from logging the "can't open logfile" message to
         * the old logfile.
         *
         * XXX: this is a lame workaround and once the next
         * release is out we should stop bothering reopening
         * the logfile unless we have to.
         *
         * Currently we reopen it every time the config file
         * has been reloaded, but actually we only have to
         * reopen it if the file name changed or if the
         * configuration reload was caused by a SIGHUP.
         */
        errlog::log_error(LOG_LEVEL_INFO, "Failed to reopen logfile: \'%s\'. "
                          "Retrying after closing the old file descriptor first. If that "
                          "doesn't work, Seeks' proxy will exit without being able to log a message.",
                          logfname);
        errlog::lock_logfile();
        fclose(errlog::_logfp);
        errlog::_logfp = NULL;
        errlog::unlock_logfile();
        fp = fopen(logfname, "a");
      }

    if (NULL == fp)
      {
        errlog::log_error(LOG_LEVEL_FATAL, "init_error_log(): can't open logfile: \'%s\'", logfname);
      }

    /* set logging to be completely unbuffered */
    setbuf(fp, NULL);

    errlog::lock_logfile();
    if (errlog::_logfp != NULL)
      {
        fclose(errlog::_logfp);
      }

    errlog::_logfp = fp;
    errlog::unlock_logfile();

    errlog::show_version(prog_name);

    errlog::unlock_loginit();
  } /* init_error_log */

  /*********************************************************************
   *
   * Function    :  get_thread_id
   *
   * Description :  Returns a number that is different for each thread.
   *
   *                XXX: Should be moved elsewhere (miscutil.c?)
   *
   * Parameters  :  None
   *
   * Returns     :  thread_id
   *
   *********************************************************************/
  long errlog::get_thread_id()
  {
    long this_thread = 1;  /* was: pthread_t this_thread;*/

    /* FIXME get current thread id */
    this_thread = (long)pthread_self();
#ifdef __MACH__
    /*
     * Mac OSX (and perhaps other Mach instances) doesn't have a debuggable
     * value at the first 4 bytes of pthread_self()'s return value, a pthread_t.
     * pthread_t is supposed to be opaque... but it's fairly random, though, so
     * we make it mostly presentable.
     */
    this_thread = abs(this_thread % 1000);
#endif /* def __MACH__ */

    return this_thread;
  }

  /*********************************************************************
   *
   * Function    :  get_log_timestamp
   *
   * Description :  Generates the time stamp for the log message prefix.
   *
   * Parameters  :
   *          1  :  buffer = Storage buffer
   *          2  :  buffer_size = Size of storage buffer
   *
   * Returns     :  Number of written characters or 0 for error.
   *
   *********************************************************************/
  size_t errlog::get_log_timestamp(char *buffer, size_t buffer_size)
  {
    size_t length;
    time_t now;
    struct tm tm_now;
    struct timeval tv_now;
    long msecs;
    int msecs_length = 0;

    gettimeofday(&tv_now, NULL);
    msecs = tv_now.tv_usec / 1000;
    now = tv_now.tv_sec;

#ifdef HAVE_LOCALTIME_R
    tm_now = *localtime_r(&now, &tm_now);
#elif defined(MUTEX_LOCKS_AVAILABLE)
    mutex_lock(&localtime_mutex);
    tm_now = *localtime(&now);
    mutex_unlock(&localtime_mutex);
#else
    tm_now = *localtime(&now);
#endif
    length = strftime(buffer, buffer_size, "%b %d %H:%M:%S", &tm_now);
    if (length > (size_t)0)
      {
        msecs_length = snprintf(buffer+length, buffer_size - length, ".%.3ld", msecs);
      }

    if (msecs_length > 0)
      {
        length += (size_t)msecs_length;
      }
    else
      {
        length = 0;
      }
    return length;
  }

  /*********************************************************************
   *
   * Function    :  get_clf_timestamp
   *
   * Description :  Generates a Common Log Format time string.
   *
   * Parameters  :
   *          1  :  buffer = Storage buffer
   *          2  :  buffer_size = Size of storage buffer
   *
   * Returns     :  Number of written characters or 0 for error.
   *
   *********************************************************************/
  size_t errlog::get_clf_timestamp(char *buffer, size_t buffer_size)
  {
    /*
     * Complex because not all OSs have tm_gmtoff or
     * the %z field in strftime()
     */
    time_t now;
    struct tm *tm_now;
    struct tm gmt;
#ifdef HAVE_LOCALTIME_R
    struct tm dummy;
#endif
    int days, hrs, mins;
    size_t length;
    int tz_length = 0;

    time (&now);
#ifdef HAVE_GMTIME_R
    gmt = *gmtime_r(&now, &gmt);
#elif defined(MUTEX_LOCKS_AVAILABLE)
    mutex_lock(&gmtime_mutex);
    gmt = *gmtime(&now);
    mutex_unlock(&gmtime_mutex);
#else
    gmt = *gmtime(&now);
#endif
#ifdef HAVE_LOCALTIME_R
    tm_now = localtime_r(&now, &dummy);
#elif defined(MUTEX_LOCKS_AVAILABLE)
    mutex_lock(&localtime_mutex);
    tm_now = localtime(&now);
    mutex_unlock(&localtime_mutex);
#else
    tm_now = localtime(&now);
#endif
    days = tm_now->tm_yday - gmt.tm_yday;
    hrs = ((days < -1 ? 24 : 1 < days ? -24 : days * 24) + tm_now->tm_hour - gmt.tm_hour);
    mins = hrs * 60 + tm_now->tm_min - gmt.tm_min;

    length = strftime(buffer, buffer_size, "%d/%b/%Y:%H:%M:%S ", tm_now);

    if (length > (size_t)0)
      {
        tz_length = snprintf(buffer+length, buffer_size-length,
                             "%+03d%02d", mins / 60, abs(mins) % 60);
      }

    if (tz_length > 0)
      {
        length += (size_t)tz_length;
      }
    else
      {
        length = 0;
      }

    return length;
  }

  /*********************************************************************
   *
   * Function    :  get_log_level_string
   *
   * Description :  Translates a numerical loglevel into a string.
   *
   * Parameters  :
   *          1  :  loglevel = LOG_LEVEL_FOO
   *
   * Returns     :  Log level string.
   *
   *********************************************************************/
  const char* errlog::get_log_level_string(int loglevel)
  {
    const char *log_level_string = NULL;
    //assert(0 < loglevel);

    switch (loglevel)
      {
      case LOG_LEVEL_ERROR:
        log_level_string = "Error";
        break;
      case LOG_LEVEL_FATAL:
        log_level_string = "Fatal error";
        break;
      case LOG_LEVEL_GPC:
        log_level_string = "Request";
        break;
      case LOG_LEVEL_CONNECT:
        log_level_string = "Connect";
        break;
      case LOG_LEVEL_LOG:
        log_level_string = "Writing";
        break;
      case LOG_LEVEL_DEBUG:
        log_level_string = "Debug";
        break;
      case LOG_LEVEL_HEADER:
        log_level_string = "Header";
        break;
      case LOG_LEVEL_INFO:
        log_level_string = "Info";
        break;
      case LOG_LEVEL_RE_FILTER:
        log_level_string = "Re-Filter";
        break;
      case LOG_LEVEL_REDIRECTS:
        log_level_string = "Redirect";
        break;
      case LOG_LEVEL_PARSER:
        log_level_string = "Parser";
        break;
      case LOG_LEVEL_CRUNCH:
        log_level_string = "Crunch";
        break;
      case LOG_LEVEL_CGI:
        log_level_string = "CGI";
        break;
      default:
        log_level_string = "Unknown log level";
        break;
      }

    //assert(NULL != log_level_string);

    return log_level_string;
  }

  /*********************************************************************
   *
   * Function    :  log_error
   *
   * Description :  This is the error-reporting and logging function.
   *
   * Parameters  :
   *          1  :  loglevel  = the type of message to be logged
   *          2  :  fmt       = the main string we want logged, printf-like
   *          3  :  ...       = arguments to be inserted in fmt (printf-like).
   *
   * Returns     :  N/A
   *
   *********************************************************************/
  void errlog::log_error(int loglevel, const char *fmt, ...)
  {
    va_list ap;
    char *outbuf = NULL;
    static char *outbuf_save = NULL;
    char tempbuf[BUFFER_SIZE];
    size_t length = 0;
    const char * src = fmt;
    long thread_id;
    char timestamp[30];

    /*
     * XXX: Make this a config option,
     * why else do we allocate instead of using
     * an array?
     */
    size_t log_buffer_size = BUFFER_SIZE;

#if defined(_WIN32) && !defined(_WIN_CONSOLE)
    /*
     * Irrespective of debug setting, a GET/POST/CONNECT makes
     * the taskbar icon animate.  (There is an option to disable
     * this but checking that is handled inside LogShowActivity()).
     */
    if ((loglevel == LOG_LEVEL_GPC) || (loglevel == LOG_LEVEL_CRUNCH))
      {
        LogShowActivity();
      }
#endif /* defined(_WIN32) && !defined(_WIN_CONSOLE) */

    /*
     * verify that the loglevel applies to current
     * settings and that logging is enabled.
     * Bail out otherwise.
     */
    if ((0 == (loglevel & errlog::_debug))
#ifndef _WIN32
        || (errlog::_logfp == NULL)
#endif
       )
      {
        if (loglevel == LOG_LEVEL_FATAL)
          {
            errlog::fatal_error("Fatal error. You're not supposed to"
                                "see this message. Please file a bug report.");
          }
        return;
      }

    thread_id = errlog::get_thread_id();
    errlog::get_log_timestamp(timestamp, sizeof(timestamp));

    /* protect the whole function because of the static buffer (outbuf) */
    errlog::lock_logfile();

    if (NULL == outbuf_save)
      {
        outbuf_save = (char*)zalloc(log_buffer_size + 1); /* +1 for paranoia */
        if (NULL == outbuf_save)
          {
            snprintf(tempbuf, sizeof(tempbuf),
                     "%s %08lx Fatal error: Out of memory in log_error().",
                     timestamp, thread_id);
            errlog::fatal_error(tempbuf); /* Exit */
            return;
          }
      }

    outbuf = outbuf_save;

    /*
     * Memsetting the whole buffer to zero (in theory)
     * makes things easier later on.
     */
    std::memset(outbuf, 0, log_buffer_size);

    /* Add prefix for everything but Common Log Format messages */
    if (loglevel != LOG_LEVEL_CLF)
      {
        length = (size_t)snprintf(outbuf, log_buffer_size, "%s %08lx %s: ",
                                  timestamp, thread_id, get_log_level_string(loglevel));
      }

    /* get ready to scan var. args. */
    va_start(ap, fmt);

    /* build formatted message from fmt and var-args */
    while ((*src) && (length < log_buffer_size-2))
      {
        const char *sval = NULL; /* %N string  */
        int ival;                /* %N string length or an error code */
        unsigned uval;           /* %u value */
        long lval;               /* %l value */
        unsigned long ulval;     /* %ul value */
        double dval;
        char ch;
        const char *format_string = tempbuf;

        ch = *src++;
        if (ch != '%')
          {
            outbuf[length++] = ch;
            /*
             * XXX: Only necessary on platforms where multiple threads
             * can write to the buffer at the same time because we
             * don't support mutexes (OS/2 for example).
             */
            outbuf[length] = '\0';
            continue;
          }

        outbuf[length] = '\0';
        ch = *src++;
        switch (ch)
          {
          case '%':
            tempbuf[0] = '%';
            tempbuf[1] = '\0';
            break;
          case 'd':
            ival = va_arg( ap, int );
            snprintf(tempbuf, sizeof(tempbuf), "%d", ival);
            break;
          case 'u':
            uval = va_arg( ap, unsigned );
            snprintf(tempbuf, sizeof(tempbuf), "%u", uval);
            break;
          case 'g':
            dval = va_arg( ap, double );
            snprintf(tempbuf, sizeof(tempbuf), "%g", dval);
            break;
          case 'l':
            /* this is a modifier that must be followed by u, lu, or d */
            ch = *src++;
            if (ch == 'd')
              {
                lval = va_arg( ap, long );
                snprintf(tempbuf, sizeof(tempbuf), "%ld", lval);
              }
            else if (ch == 'u')
              {
                ulval = va_arg( ap, unsigned long );
                snprintf(tempbuf, sizeof(tempbuf), "%lu", ulval);
              }
            else if ((ch == 'l') && (*src == 'u'))
              {
                unsigned long long lluval = va_arg(ap, unsigned long long);
                snprintf(tempbuf, sizeof(tempbuf), "%llu", lluval);
                src++;
              }
            else
              {
                snprintf(tempbuf, sizeof(tempbuf), "Bad format string: \"%s\"", fmt);
                //loglevel = LOG_LEVEL_FATAL;
              }
            break;
          case 'c':
            /*
             * Note that char paramaters are converted to int, so we need to
             * pass "int" to va_arg.  (See K&R, 2nd ed, section A7.3.2, page 202)
             */
            tempbuf[0] = (char) va_arg(ap, int);
            tempbuf[1] = '\0';
            break;
          case 's':
            format_string = va_arg(ap, char *);
            if (format_string == NULL)
              {
                format_string = "[null]";
              }
            break;
          case 'N':
            /*
             * Non-standard: Print a counted unterminated string.
             * Takes 2 parameters: int length, const char * string.
             */
            ival = va_arg(ap, int);
            sval = va_arg(ap, char *);
            if (sval == NULL)
              {
                format_string = "[null]";
              }
            else if (ival <= 0)
              {
                if (0 == ival)
                  {
                    /* That's ok (but stupid) */
                    tempbuf[0] = '\0';
                  }
                else
                  {
                    /*
                    * That's not ok (and even more stupid)
                    */
                    assert(ival >= 0);
                    format_string = "[counted string lenght < 0]";
                  }
              }
            else if ((size_t)ival >= sizeof(tempbuf))
              {
                /*
                 * String is too long, copy as much as possible.
                 * It will be further truncated later.
                 */
                memcpy(tempbuf, sval, sizeof(tempbuf)-1);
                tempbuf[sizeof(tempbuf)-1] = '\0';
              }
            else
              {
                memcpy(tempbuf, sval, (size_t) ival);
                tempbuf[ival] = '\0';
              }
            break;
          case 'E':
            /* Non-standard: Print error code from errno */
#ifdef _WIN32
            ival = WSAGetLastError();
            format_string = w32_socket_strerr(ival, tempbuf);
#else /* ifndef _WIN32 */
            ival = errno;
# ifdef HAVE_STRERROR
            format_string = strerror(ival);
# else /* ifndef HAVE_STRERROR */
            format_string = NULL;
# endif /* ndef HAVE_STRERROR */
            if (sval == NULL)
              {
                snprintf(tempbuf, sizeof(tempbuf), "(errno = %d)", ival);
              }
#endif /* ndef _WIN32 */
            break;
          case 'T':
            /* Non-standard: Print a Common Log File timestamp */
            errlog::get_clf_timestamp(tempbuf, sizeof(tempbuf));
            break;
          default:
            snprintf(tempbuf, sizeof(tempbuf), "Bad format string: \"%s\"", fmt);
            //loglevel = LOG_LEVEL_FATAL;
            break;
          }
        /* switch( p ) */
        assert(length < log_buffer_size);
        length += strlcpy(outbuf + length, format_string, log_buffer_size - length);

        if (length >= log_buffer_size-2)
          {
            static char warning[] = "... [too long, truncated]";

            length = log_buffer_size - sizeof(warning) - 1;
            length += strlcpy(outbuf + length, warning, log_buffer_size - length);
            assert(length < log_buffer_size);
            break;
          }
      }
    /* for( p ... ) */
    /* done with var. args */
    va_end(ap);

    assert(length < log_buffer_size);
    length += strlcpy(outbuf + length, "\n", log_buffer_size - length);

    /* Some sanity checks */
    if ((length >= log_buffer_size)
        || (outbuf[log_buffer_size-1] != '\0')
        || (outbuf[log_buffer_size] != '\0')
       )
      {
        /* Repeat as assertions */
        assert(length < log_buffer_size);
        assert(outbuf[log_buffer_size-1] == '\0');
        /*
         * outbuf's real size is log_buffer_size+1,
         * so while this looks like an off-by-one,
         * we're only checking our paranoia byte.
         */
        assert(outbuf[log_buffer_size] == '\0');
        snprintf(outbuf, log_buffer_size,
                 "%s %08lx Fatal error: log_error()'s sanity checks failed."
                 "length: %d. Exiting.",
                 timestamp, thread_id, (int)length);
        loglevel = LOG_LEVEL_FATAL;
      }
#ifndef _WIN32
    /*
     * On Windows this is acceptable in case
     * we are logging to the GUI window only.
     */
    assert(NULL != errlog::_logfp);
#endif

    if (loglevel == LOG_LEVEL_FATAL)
      {
        errlog::fatal_error(outbuf_save);
        /* Never get here */
      }

    if (errlog::_logfp != NULL)
      {
        fputs(outbuf_save, errlog::_logfp);
      }

#if defined(_WIN32) && !defined(_WIN_CONSOLE)
    /* Write to display */
    LogPutString(outbuf_save);
#endif /* defined(_WIN32) && !defined(_WIN_CONSOLE) */

    errlog::unlock_logfile();
  }

  /*********************************************************************
   *
   * Function    :  sp_err_to_string
   *
   * Description :  Translates SP_ERR_FOO codes into strings.
   *
   *
   * Parameters  :
   *          1  :  error = a valid jb_err code
   *
   * Returns     :  A string with the jb_err translation
   *
   *********************************************************************/
  const char* errlog::sp_err_to_string(int error)
  {
    switch (error)
      {
      case SP_ERR_OK:
        return "Success, no error";
      case SP_ERR_MEMORY:
        return "Out of memory";
      case SP_ERR_CGI_PARAMS:
        return "Missing or corrupt CGI parameters";
      case SP_ERR_FILE:
        return "Error opening, reading or writing a file";
      case SP_ERR_PARSE:
        return "Parse error";
      case SP_ERR_MODIFIED:
        return "File has been modified outside of the CGI actions editor.";
      case SP_ERR_COMPRESS:
        return "(De)compression failure";
      default:
        assert(0);
        return "Unknown error";
      }
    assert(0);
    return "Internal error";
  }

#ifdef _WIN32
  /*********************************************************************
   *
   * Function    :  w32_socket_strerr
   *
   * Description :  Translate the return value from WSAGetLastError()
   *                into a string.
   *
   * Parameters  :
   *          1  :  errcode = The return value from WSAGetLastError().
   *          2  :  tmp_buf = A temporary buffer that might be used to
   *                          store the string.
   *
   * Returns     :  String representing the error code.  This may be
   *                a global string constant or a string stored in
   *                tmp_buf.
   *
   *********************************************************************/
  char* errlog::w32_socket_strerr(int errcode, char *tmp_buf)
  {

# define TEXT_FOR_ERROR(code,text) \
   if (errcode == code)           \
       {                         \
	  return #code " - " text;    \
       }


    TEXT_FOR_ERROR(WSAEACCES, "Permission denied")
    TEXT_FOR_ERROR(WSAEADDRINUSE, "Address already in use.")
    TEXT_FOR_ERROR(WSAEADDRNOTAVAIL, "Cannot assign requested address.");
    TEXT_FOR_ERROR(WSAEAFNOSUPPORT, "Address family not supported by protocol family.");
    TEXT_FOR_ERROR(WSAEALREADY, "Operation already in progress.");
    TEXT_FOR_ERROR(WSAECONNABORTED, "Software caused connection abort.");
    TEXT_FOR_ERROR(WSAECONNREFUSED, "Connection refused.");
    TEXT_FOR_ERROR(WSAECONNRESET, "Connection reset by peer.");
    TEXT_FOR_ERROR(WSAEDESTADDRREQ, "Destination address required.");
    TEXT_FOR_ERROR(WSAEFAULT, "Bad address.");
    TEXT_FOR_ERROR(WSAEHOSTDOWN, "Host is down.");
    TEXT_FOR_ERROR(WSAEHOSTUNREACH, "No route to host.");
    TEXT_FOR_ERROR(WSAEINPROGRESS, "Operation now in progress.");
    TEXT_FOR_ERROR(WSAEINTR, "Interrupted function call.");
    TEXT_FOR_ERROR(WSAEINVAL, "Invalid argument.");
    TEXT_FOR_ERROR(WSAEISCONN, "Socket is already connected.");
    TEXT_FOR_ERROR(WSAEMFILE, "Too many open sockets.");
    TEXT_FOR_ERROR(WSAEMSGSIZE, "Message too long.");
    TEXT_FOR_ERROR(WSAENETDOWN, "Network is down.");
    TEXT_FOR_ERROR(WSAENETRESET, "Network dropped connection on reset.");
    TEXT_FOR_ERROR(WSAENETUNREACH, "Network is unreachable.");
    TEXT_FOR_ERROR(WSAENOBUFS, "No buffer space available.");
    TEXT_FOR_ERROR(WSAENOPROTOOPT, "Bad protocol option.");
    TEXT_FOR_ERROR(WSAENOTCONN, "Socket is not connected.");
    TEXT_FOR_ERROR(WSAENOTSOCK, "Socket operation on non-socket.");
    TEXT_FOR_ERROR(WSAEOPNOTSUPP, "Operation not supported.");
    TEXT_FOR_ERROR(WSAEPFNOSUPPORT, "Protocol family not supported.");
    TEXT_FOR_ERROR(WSAEPROCLIM, "Too many processes.");
    TEXT_FOR_ERROR(WSAEPROTONOSUPPORT, "Protocol not supported.");
    TEXT_FOR_ERROR(WSAEPROTOTYPE, "Protocol wrong type for socket.");
    TEXT_FOR_ERROR(WSAESHUTDOWN, "Cannot send after socket shutdown.");
    TEXT_FOR_ERROR(WSAESOCKTNOSUPPORT, "Socket type not supported.");
    TEXT_FOR_ERROR(WSAETIMEDOUT, "Connection timed out.");
    TEXT_FOR_ERROR(WSAEWOULDBLOCK, "Resource temporarily unavailable.");
    TEXT_FOR_ERROR(WSAHOST_NOT_FOUND, "Host not found.");
    TEXT_FOR_ERROR(WSANOTINITIALISED, "Successful WSAStartup not yet performed.");
    TEXT_FOR_ERROR(WSANO_DATA, "Valid name, no data record of requested type.");
    TEXT_FOR_ERROR(WSANO_RECOVERY, "This is a non-recoverable error.");
    TEXT_FOR_ERROR(WSASYSNOTREADY, "Network subsystem is unavailable.");
    TEXT_FOR_ERROR(WSATRY_AGAIN, "Non-authoritative host not found.");
    TEXT_FOR_ERROR(WSAVERNOTSUPPORTED, "WINSOCK.DLL version out of range.");
    TEXT_FOR_ERROR(WSAEDISCON, "Graceful shutdown in progress.");

    sprintf(tmp_buf, "(error number %d)", errcode);
    return tmp_buf;
  }

#endif /* def _WIN32 */

} /* end of namespace. */
