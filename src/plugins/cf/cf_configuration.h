/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#ifndef CF_CONFIGURATION_H
#define CF_CONFIGURATION_H

#include "configuration_spec.h"
#include "peer_list.h"

using sp::configuration_spec;

namespace seeks_plugins
{

  class cf_configuration : public configuration_spec
  {
    public:
      cf_configuration(const std::string &filename);

      ~cf_configuration();

      // virtual
      virtual void set_default_config();

      virtual void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
                                     char *buf, const unsigned long &linenum);

      virtual void finalize_configuration();

      // main options.
      float _domain_name_weight; /**< weight given to domain names. */
      int _record_cache_timeout; /**< timeout on cached remote records, in seconds. */
      peer_list *_pl; /**< list of peers for collaborative filtering. */
      peer_list *_dpl; /**< list of dead peers, used in operations, to check/uncheck dead peers from the list. */
      int _dead_peer_check; /**< interval of time between two dead peer checks. */
      short _dead_peer_retries; /**< number of retries before marking a peer as dead. */

      bool _post_url_check; /**< whether to ping on posted URLs. */
      std::string _post_url_ua; /**< default 'User-Agent' header for retrieving posted URLS. */
      int _post_radius; /**< similarity impact radius of posted queries. */

      bool _stop_words_filtering; /** < filter similar queries with stop words. */

      static cf_configuration *_config;
  };

} /* end of namespace. */

#endif

