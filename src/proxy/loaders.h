/*********************************************************************
 * Purpose     :  Functions to load and unload the various
 *                configuration files.  Also contains code to manage
 *                the list of active loaders, and to automatically
 *                unload files that are no longer in use.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
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

#ifndef LOADERS_H
#define LOADERS_H

#include "proxy_dts.h"

namespace sp
{

  class loaders
  {
    public:
      static char *read_config_line(char *buf, size_t buflen, FILE *fp, unsigned long *linenum);

      static sp_err edit_read_line(FILE *fp,
                                   char **raw_out,
                                   char **prefix_out,
                                   char **data_out,
                                   int *newline,
                                   unsigned long *line_number);

      static sp_err simple_read_line(FILE *fp, char **dest, int *newline);

      static sp_err load_pattern_file(const char *pattern_filename,
                                      std::vector<url_spec*> &pos_patterns,
                                      std::vector<url_spec*> &neg_patterns);

      /*
       * Various types of newlines that a file may contain.
       */
#define NEWLINE_UNKNOWN 0  /* Newline convention in file is unknown */
#define NEWLINE_UNIX    1  /* Newline convention in file is '\n'   (ASCII 10) */
#define NEWLINE_DOS     2  /* Newline convention in file is '\r\n' (ASCII 13,10) */
#define NEWLINE_MAC     3  /* Newline convention in file is '\r'   (ASCII 13) */

      /*
       * Types of newlines that a file may contain, as strings.  If you have an
       * extremely weird compiler that does not have '\r' == CR == ASCII 13 and
       * '\n' == LF == ASCII 10), then fix CHAR_CR and CHAR_LF in loaders.c as
       * well as these definitions.
       */
#define NEWLINE(style) ((style)==NEWLINE_DOS ? "\r\n" : \
			   ((style)==NEWLINE_MAC ? "\r" : "\n"))


      static short int MustReload;
  };

} /* end of namespace. */

#endif /* ndef LOADERS_H */

