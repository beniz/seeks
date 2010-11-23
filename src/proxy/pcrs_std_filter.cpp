/*
 * Copyright   :   Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001, 2004-2009 the
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

#include "pcrs_std_filter.h"

#include <iostream>

namespace seeks_plugins
{
  /*- pcrs_std_filter_elt -*/
  pcrs_std_filter_elt::pcrs_std_filter_elt(const char *pattern_filename,
      const char *code_filename,
      plugin *parent)
      : filter_plugin(pattern_filename,code_filename,true,false,
                      parent)
  {
  }

  pcrs_std_filter_elt::pcrs_std_filter_elt(const char *code_filename,
      plugin *parent)
      : filter_plugin(code_filename, true, false, parent)  // always on.
  {
  }

  char* pcrs_std_filter_elt::run(client_state *csp, char *str)
  {
    return pcrs_plugin_response(csp, str);
  }

} /* end of namespace. */
