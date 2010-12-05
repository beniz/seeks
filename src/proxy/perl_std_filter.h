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

#ifndef PERL_STD_FILTER_H
#define PERL_STD_FILTER_H

#include "plugin.h"
#include "filter_plugin.h"

using namespace sp;

namespace seeks_plugins
{
  class perl_std_filter_elt : public filter_plugin
  {
    public:
      perl_std_filter_elt(const char *pattern_filename,
                          const char *code_filename,
                          plugin *parent,
                          const std::string &perl_subr);

      perl_std_filter_elt(const char *code_filename,
                          plugin *parent,
                          const std::string &perl_subr);  // filter always activated, no pattern.

      ~perl_std_filter_elt() {};

      // virtual from plugin_element.
      char* run(client_state *csp, char *str);

    public:
      std::string _perl_subr; // Perl subroutine to be executed by the plugin.
  };

} /* end of namespace. */

#endif
