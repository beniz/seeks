/*********************************************************************
 * Purpose     :  Functions to load and unload the various
 *                configuration files.  Also contains code to manage
 *                the list of active loaders, and to automatically
 *                unload files that are no longer in use.
 *
 * Copyright   :   Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001-2009 the
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


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <assert.h>

#if !defined(_WIN32) && !defined(__OS2__)
#include <unistd.h>
#endif

#include <iostream>

#include "mem_utils.h"
#include "loaders.h"
#include "filters.h"
#include "parsers.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "miscutil.h"
#include "errlog.h"
#include "urlmatch.h"
#include "encode.h"

namespace sp
{

/*********************************************************************
 *
 * Function    :  simple_read_line
 *
 * Description :  Read a single line from a file and return it.
 *                This is basically a version of fgets() that malloc()s
 *                it's own line buffer.  Note that the buffer will
 *                always be a multiple of BUFFER_SIZE bytes long.
 *                Therefore if you are going to keep the string for
 *                an extended period of time, you should probably
 *                strdup() it and freez() the original, to save memory.
 *
 *
 * Parameters  :
 *          1  :  dest = destination for newly malloc'd pointer to
 *                line data.  Will be set to NULL on error.
 *          2  :  fp = File to read from
 *          3  :  newline = Standard for newlines in the file.
 *                Will be unchanged if it's value on input is not
 *                NEWLINE_UNKNOWN.
 *                On output, may be changed from NEWLINE_UNKNOWN to
 *                actual convention in file.
 *
 * Returns     :  SP_ERR_OK     on success
 *                SP_ERR_MEMORY on out-of-memory
 *                SP_ERR_FILE   on EOF.
 *
 *********************************************************************/
sp_err loaders::simple_read_line(FILE *fp, char **dest, int *newline)
{
   size_t len = 0;
   size_t buflen = BUFFER_SIZE;
   char * buf;
   char * p;
   int ch;
   int realnewline = NEWLINE_UNKNOWN;

   if (NULL == (buf = (char*) zalloc(buflen)))
   {
      return SP_ERR_MEMORY;
   }

   p = buf;

/*
 * Character codes.  If you have a wierd compiler and the following are
 * incorrect, you also need to fix NEWLINE() in loaders.h
 */
#define CHAR_CR '\r' /* ASCII 13 */
#define CHAR_LF '\n' /* ASCII 10 */

   for (;;)
   {
      ch = getc(fp);
      if (ch == EOF)
      {
         if (len > 0)
         {
            *p = '\0';
            *dest = buf;
            return SP_ERR_OK;
         }
         else
         {
            freez(buf);
            *dest = NULL;
            return SP_ERR_FILE;
         }
      }
      else if (ch == CHAR_CR)
      {
         ch = getc(fp);
         if (ch == CHAR_LF)
         {
            if (*newline == NEWLINE_UNKNOWN)
            {
               *newline = NEWLINE_DOS;
            }
         }
         else
         {
            if (ch != EOF)
            {
               ungetc(ch, fp);
            }
            if (*newline == NEWLINE_UNKNOWN)
            {
               *newline = NEWLINE_MAC;
            }
         }
         *p = '\0';
         *dest = buf;
         if (*newline == NEWLINE_UNKNOWN)
         {
            *newline = realnewline;
         }
         return SP_ERR_OK;
      }
      else if (ch == CHAR_LF)
      {
         *p = '\0';
         *dest = buf;
         if (*newline == NEWLINE_UNKNOWN)
         {
            *newline = NEWLINE_UNIX;
         }
         return SP_ERR_OK;
      }
      else if (ch == 0)
      {
         *p = '\0';
         *dest = buf;
         return SP_ERR_OK;
      }

      *p++ = (char)ch;

      if (++len >= buflen)
      {
         buflen += BUFFER_SIZE;
         if (NULL == (p = (char*)realloc(buf, buflen)))
         {
            freez(buf);
            return SP_ERR_MEMORY;
         }
         buf = p;
         p = buf + len;
      }
   }
}


/*********************************************************************
 *
 * Function    :  edit_read_line
 *
 * Description :  Read a single non-empty line from a file and return
 *                it.  Trims comments, leading and trailing whitespace
 *                and respects escaping of newline and comment char.
 *                Provides the line in 2 alternative forms: raw and
 *                preprocessed.
 *                - raw is the raw data read from the file.  If the
 *                  line is not modified, then this should be written
 *                  to the new file.
 *                - prefix is any comments and blank lines that were
 *                  read from the file.  If the line is modified, then
 *                  this should be written out to the file followed
 *                  by the modified data.  (If this string is non-empty
 *                  then it will have a newline at the end).
 *                - data is the actual data that will be parsed
 *                  further by appropriate routines.
 *                On EOF, the 3 strings will all be set to NULL and
 *                0 will be returned.
 *
 * Parameters  :
 *          1  :  fp = File to read from
 *          2  :  raw_out = destination for newly malloc'd pointer to
 *                raw line data.  May be NULL if you don't want it.
 *          3  :  prefix_out = destination for newly malloc'd pointer to
 *                comments.  May be NULL if you don't want it.
 *          4  :  data_out = destination for newly malloc'd pointer to
 *                line data with comments and leading/trailing spaces
 *                removed, and line continuation performed.  May be
 *                NULL if you don't want it.
 *          5  :  newline = Standard for newlines in the file.
 *                On input, set to value to use or NEWLINE_UNKNOWN.
 *                On output, may be changed from NEWLINE_UNKNOWN to
 *                actual convention in file.  May be NULL if you
 *                don't want it.
 *          6  :  line_number = Line number in file.  In "lines" as
 *                reported by a text editor, not lines containing data.
 *
 * Returns     :  SP_ERR_OK     on success
 *                SP_ERR_MEMORY on out-of-memory
 *                SP_ERR_FILE   on EOF.
 *
 *********************************************************************/
sp_err loaders::edit_read_line(FILE *fp,
			       char **raw_out,
			       char **prefix_out,
			       char **data_out,
			       int *newline,
			       unsigned long *line_number)
{
   char *p;          /* Temporary pointer   */
   char *linebuf;    /* Line read from file */
   char *linestart;  /* Start of linebuf, usually first non-whitespace char */
   int contflag = 0; /* Nonzero for line continuation - i.e. line ends '\' */
   int is_empty = 1; /* Flag if not got any data yet */
   char *raw    = NULL; /* String to be stored in raw_out    */
   char *prefix = NULL; /* String to be stored in prefix_out */
   char *data   = NULL; /* String to be stored in data_out   */
   int scrapnewline;    /* Used for (*newline) if newline==NULL */
   sp_err rval = SP_ERR_OK;

   assert(fp);
   assert(raw_out || data_out);
   assert(newline == NULL
       || *newline == NEWLINE_UNKNOWN
       || *newline == NEWLINE_UNIX
       || *newline == NEWLINE_DOS
       || *newline == NEWLINE_MAC);

   if (newline == NULL)
   {
      scrapnewline = NEWLINE_UNKNOWN;
      newline = &scrapnewline;
   }

   /* Set output parameters to NULL */
   if (raw_out)
   {
      *raw_out    = NULL;
   }
   if (prefix_out)
   {
      *prefix_out = NULL;
   }
   if (data_out)
   {
      *data_out   = NULL;
   }

   /* Set string variables to new, empty strings. */

   if (raw_out)
   {
      raw = strdup("");
      if (NULL == raw)
      {
         return SP_ERR_MEMORY;
      }
   }
   if (prefix_out)
   {
      prefix = strdup("");
      if (NULL == prefix)
      {
         freez(raw);
	 raw = NULL;
         return SP_ERR_MEMORY;
      }
   }
   if (data_out)
   {
      data = strdup("");
      if (NULL == data)
      {
         freez(raw);
	 raw = NULL;
         freez(prefix);
	 prefix = NULL;
         return SP_ERR_MEMORY;
      }
   }

   /* Main loop.  Loop while we need more data & it's not EOF. */
   while ( (contflag || is_empty)
	   && (SP_ERR_OK == (rval = loaders::simple_read_line(fp, &linebuf, newline))))
   {
      if (line_number)
      {
         (*line_number)++;
      }
      if (raw)
      {
         miscutil::string_append(&raw,linebuf);
         if (miscutil::string_append(&raw,NEWLINE(*newline)))
         {
            freez(prefix);
	    prefix = NULL;
            freez(data);
	    data = NULL;
            freez(linebuf);
            return SP_ERR_MEMORY;
         }
      }

      /* Line continuation? Trim escape and set flag. */
      p = linebuf + strlen(linebuf) - 1;
      contflag = ((*linebuf != '\0') && (*p == '\\'));
      if (contflag)
      {
         *p = '\0';
      }

      /* Trim leading spaces if we're at the start of the line */
      linestart = linebuf;
      assert(NULL != data);
      if (*data == '\0')
      {
         /* Trim leading spaces */
         while (*linestart && isspace((int)(unsigned char)*linestart))
         {
            linestart++;
         }
      }

      /* Handle comment characters. */
      p = linestart;
      while ((p = strchr(p, '#')) != NULL)
      {
         /* Found a comment char.. */
         if ((p != linebuf) && (*(p-1) == '\\'))
         {
            /* ..and it's escaped, left-shift the line over the escape. */
            char *q = p - 1;
            while ((*q = *(q + 1)) != '\0')
            {
               q++;
            }
            /* Now scan from just after the "#". */
         }
         else
         {
            /* Real comment.  Save it... */
            if (p == linestart)
            {
               /* Special case:  Line only contains a comment, so all the
                * previous whitespace is considered part of the comment.
                * Undo the whitespace skipping, if any.
                */
               linestart = linebuf;
               p = linestart;
            }
            if (prefix)
            {
               miscutil::string_append(&prefix,p);
               if (miscutil::string_append(&prefix, NEWLINE(*newline)))
               {
                  freez(raw);
		  raw = NULL;
                  freez(data);
		  data = NULL;
                  freez(linebuf);
                  return SP_ERR_MEMORY;
               }
            }

            /* ... and chop off the rest of the line */
            *p = '\0';
         }
      } /* END while (there's a # character) */

      /* Write to the buffer */
      if (*linestart)
      {
         is_empty = 0;
         if (data)
         {
            if (miscutil::string_append(&data, linestart))
            {
               freez(raw);
	       raw = NULL;
               freez(prefix);
	       prefix = NULL;
               freez(linebuf);
               return SP_ERR_MEMORY;
            }
         }
      }

      freez(linebuf);
   } /* END while(we need more data) */

   /* Handle simple_read_line() errors - ignore EOF */
   if ((rval != SP_ERR_OK) && (rval != SP_ERR_FILE))
   {
      freez(raw);
      raw = NULL;
      freez(prefix);
      prefix = NULL;
      freez(data);
      data = NULL;
      return rval;
   }

   if (raw ? (*raw == '\0') : is_empty)
   {
      /* EOF and no data there.  (Definition of "data" depends on whether
       * the caller cares about "raw" or just "data").
       */
      freez(raw);
      raw = NULL;
      freez(prefix);
      prefix = NULL;
      freez(data);
      data = NULL;

      return SP_ERR_FILE;
   }
   else
   {
      /* Got at least some data */

      /* Remove trailing whitespace */
      miscutil::chomp(data);

      if (raw_out)
      {
         *raw_out    = raw;
      }
      else
      {
         freez(raw);
	 raw = NULL;
      }
      if (prefix_out)
      {
         *prefix_out = prefix;
      }
      else
      {
         freez(prefix);
	 prefix = NULL;
      }
      if (data_out)
      {
         *data_out   = data;
      }
      else
      {
         freez(data);
	 data = NULL;
      }
      return SP_ERR_OK;
   }
}


/*********************************************************************
 *
 * Function    :  read_config_line
 *
 * Description :  Read a single non-empty line from a file and return
 *                it.  Trims comments, leading and trailing whitespace
 *                and respects escaping of newline and comment char.
 *
 * Parameters  :
 *          1  :  buf = Buffer to use.
 *          2  :  buflen = Size of buffer in bytes.
 *          3  :  fp = File to read from
 *          4  :  linenum = linenumber in file
 *
 * Returns     :  NULL on EOF or error
 *                Otherwise, returns buf.
 *
 *********************************************************************/
char* loaders::read_config_line(char *buf, size_t buflen, FILE *fp, unsigned long *linenum)
{
   sp_err err;
   char *buf2 = NULL;
   err = loaders::edit_read_line(fp, NULL, NULL, &buf2, NULL, linenum);
   if (err)
   {
      if (err == SP_ERR_MEMORY)
      {
         errlog::log_error(LOG_LEVEL_FATAL, "Out of memory loading a config file");
      }
      return NULL;
   }
   else
   {
      assert(buf2);
      assert(strlen(buf2) + 1U < buflen);
      strncpy(buf, buf2, buflen - 1);
      freez(buf2);
      buf[buflen - 1] = '\0';
      return buf;
   }
}

} /* end of namespace. */
