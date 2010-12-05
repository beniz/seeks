/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include "perl_std_filter.h"

namespace seeks_plugins
{
  perl_std_filter_elt::perl_std_filter_elt(const char *pattern_filename,
      const char *code_filename,
      plugin *parent,
      const std::string &perl_subr)
      : filter_plugin(pattern_filename,code_filename,false,true,parent),
      _perl_subr(perl_subr)
  {
  }

  perl_std_filter_elt::perl_std_filter_elt(const char *code_filename,
      plugin *parent,
      const std::string &perl_subr)
      : filter_plugin(code_filename,false,true,parent),  // filter always on.
      _perl_subr(perl_subr)
  {
  }

  char* perl_std_filter_elt::run(client_state *csp, char *str)
  {
    char **output = perl_execute_subroutine(_perl_subr, &str);
    return output[0]; // beware: returns the first string ? TODO: concat.
  }


} /* end of namespace. */
