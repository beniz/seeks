/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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

#ifndef QUERY_CAPTURE_H
#define QUERY_CAPTURE_H

#define NO_PERL

#include "plugin.h"
#include "interceptor_plugin.h"
#include "sp_exception.h"
#include "query_capture_configuration.h"
#include "DHTKey.h"

using namespace sp;
using dht::DHTKey;

namespace seeks_plugins
{

  class query_db_sweepable : public user_db_sweepable
  {
    public:
      query_db_sweepable();

      virtual ~query_db_sweepable();

      virtual bool sweep_me();

      virtual int sweep_records();

      time_t _last_sweep;
  };

  class query_capture : public plugin
  {
    public:
      query_capture();

      virtual ~query_capture();

      virtual void start();

      virtual void stop();

      static sp_err cgi_qc_redir(client_state *csp,
                                 http_response *rsp,
                                 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

      static sp_err qc_redir(client_state *csp,
                             http_response *rsp,
                             const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                             char *&urlp);

      virtual sp::db_record* create_db_record();

      int remove_all_query_records();

      static void store_queries(const std::string &query,
                                const std::string &url, const std::string &host) throw (sp_exception);

      // store_query called from websearch plugin.
      void store_queries(const std::string &query) const throw (sp_exception);

      static void process_url(std::string &url, std::string &host);

      static void process_url(std::string &url, std::string &host, std::string &path);

      static void process_get(std::string &get);
  };

  class query_capture_element : public interceptor_plugin
  {
    public:
      query_capture_element(plugin *parent);

      virtual ~query_capture_element();

      virtual http_response* plugin_response(client_state *csp);

      static void store_queries(const std::string &query,
                                const std::string &url, const std::string &host,
                                const std::string &plugin_name) throw (sp_exception);

      static void store_queries(const std::string &query,
                                const std::string &plugin_name) throw (sp_exception);

      static void store_query(const DHTKey &key,
                              const std::string &query,
                              const uint32_t &radius,
                              const std::string &plugin_name) throw (sp_exception);

      static void store_url(const DHTKey &key, const std::string &query,
                            const std::string &url, const std::string &host,
                            const uint32_t &radius,
                            const std::string &plugin_name) throw (sp_exception);

      static void remove_url(const DHTKey &key, const std::string &query,
                             const std::string &url, const std::string &host,
                             const short &url_hits, const uint32_t &radius,
                             const std::string &plugin_name) throw (sp_exception);

      static void get_useful_headers(const std::list<const char*> &headers,
                                     std::string &host, std::string &referer,
                                     std::string &get, std::string &base_url);

      static std::string no_command_query(const std::string &query);

    public:
      query_db_sweepable _qds;

      static std::string _capt_filename; // pattern capture file.
      static std::string _cgi_site_host; // default local cgi address.
  };

} /* end of namespace. */

#endif
