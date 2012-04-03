/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#ifndef UDB_SERVICE_CONFIGURATION_H
#define UDB_SERVICE_CONFIGURATION_H

#include "configuration_spec.h"

using sp::configuration_spec;

namespace seeks_plugins
{

  class udb_service_configuration : public configuration_spec
  {
    public:
      udb_service_configuration(const std::string &filename);

      ~udb_service_configuration();

      // virtual
      virtual void set_default_config();

      virtual void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
                                     char *buf, const unsigned long &linenum);

      virtual void finalize_configuration();

      // main options
      long _call_timeout; /**< timeout on connection and on data transfer for P2P calls. */
      std::string _p2p_proxy_addr; /**< address of a proxy through which to issue the P2P calls. */
      int _p2p_proxy_port; /**< port of a proxy through which to issue the P2P calls. */

      static udb_service_configuration *_config;
  };

} /* end of namespace. */

#endif
