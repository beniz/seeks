/*********************************************************************
 * Purpose     :  Functions to encode and decode URLs, and also to
 *                encode cookies and HTML text.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001 the SourceForge
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

#ifndef ENCODE_H
#define ENCODE_H

#include <cstring>
#include <string>

namespace sp
{

  class encode
  {
    public:
      static char* html_encode(const char *s);
      static char* url_encode(const char *s);
      static char* url_decode(const char *str);
      static char* url_decode_but_not_plus(const char *str);
      static int   xtoi(const char *s);
      static char* html_encode_and_free_original(char *s);
      static int xdtoi(const int d);

      static const char* _url_code_map[256];
      static const char* _html_code_map[256];

      // XXX: C++ HTML decode.
      // In the long term, all functions should be C++.
      static std::string html_decode(const std::string &s);
  };

} /* end of namespace. */

#endif /* ndef ENCODE_H */
