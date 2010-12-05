/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
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

#ifndef FILTER_PLUGIN_H
#define FILTER_PLUGIN_H

#include "plugin_element.h"

#include <string>
#include <iostream>

namespace sp
{
  class filter_plugin : public plugin_element
  {
    public:
      // useless ?
      filter_plugin(const std::vector<url_spec*> &pos_patterns,
                    const std::vector<url_spec*> &neg_patterns,
                    plugin *parent);

      filter_plugin(const char *filename,
                    plugin *parent);

      filter_plugin(const std::vector<std::string> &pos_patterns,
                    const std::vector<std::string> &neg_patterns,
                    plugin *parent);

      filter_plugin(plugin *parent);  // filter always activated, no code.

      filter_plugin(const std::vector<std::string> &pos_patterns,
                    const std::vector<std::string> &neg_patterns,
                    const char *code_filename,
                    const bool &pcrs, const bool &perl,
                    plugin *parent);

      filter_plugin(const char *pattern_filename,
                    const char *code_filename,
                    const bool &pcrs, const bool &perl,
                    plugin *parent);

      filter_plugin(const char *code_filename,
                    const bool &pcrs, const bool &perl,
                    plugin *parent);  // filter always activated, with code.

      virtual ~filter_plugin();

      virtual char* run(client_state *csp, char *str)
      {
        std::cout << "run mother run...\n";
        return (char*) "";
      };

      virtual std::string print()
      {
        return "";
      }; // virtual

    private:
      std::string _parameters;      // parameters passed to the filters, if any.
      std::string _filter_file;

      bool _enabled; // whether the plugin is enabled.

      static std::vector<std::string> _always; // always activated.
      static std::vector<std::string> init_always();
  };

} /* end of namespace. */

#endif
