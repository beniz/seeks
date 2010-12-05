/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#ifndef HTTPSERV_CONFIGURATION_H
#define HTTPSERV_CONFIGURATION_H

#include "configuration_spec.h"

using sp::configuration_spec;

namespace seeks_plugins
{

  class httpserv_configuration : public configuration_spec
  {
    public:
      httpserv_configuration(const std::string &filename);

      ~httpserv_configuration();

      // virtual
      virtual void set_default_config();

      virtual void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
                                     char *buf, const unsigned long &linenum);

      virtual void finalize_configuration();

      // main options.
      short _port; /**< server port. */
      std::string _host; /**< server host. */

    public:
      static httpserv_configuration *_hconfig;
  };

} /* end of namespace. */

#endif
