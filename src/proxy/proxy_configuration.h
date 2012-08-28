/**
 * The seeks proxy is part of the SEEKS project
 * It is based on Privoxy (http://www.privoxy.org), developped
 * by the Privoxy team.
 *
 * Copyright (C) 2009-2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef PROXY_CONFIGURATION_H
#define PROXY_CONFIGURATION_H

#include "configuration_spec.h"

namespace sp
{
  /** configuration_spec::feature_flags: Web-based toggle. */
#define RUNTIME_FEATURE_CGI_TOGGLE                   2U

  /** configuration_spec::feature_flags: HTTP-header-based toggle. */
#define RUNTIME_FEATURE_HTTP_TOGGLE                  4U

  /** configuration_spec::feature_flags: Split large forms to limit the number of GET arguments. */
#define RUNTIME_FEATURE_SPLIT_LARGE_FORMS            8U

  /** configuration_spec::feature_flags: Check the host header for requests with host-less request lines. */
#define RUNTIME_FEATURE_ACCEPT_INTERCEPTED_REQUESTS 16U

  /** configuration_spec::feature_flags: Don't allow to circumvent blocks with the force prefix. */
#define RUNTIME_FEATURE_ENFORCE_BLOCKS              32U

  /** configuration_spec::feature_flags: Allow to block or redirect CGI requests. */
#define RUNTIME_FEATURE_CGI_CRUNCHING               64U

  /** configuration_spec::feature_flags: Try to keep the connection to the server alive. */
#define RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE      128U

  /** configuration_spec::feature_flags: Share outgoing connections between different client connections. */
#define RUNTIME_FEATURE_CONNECTION_SHARING         256U

  class proxy_configuration : public configuration_spec
  {
    public:
      proxy_configuration(const std::string &filename);

      ~proxy_configuration();

      // virtual.
      virtual void set_default_config();

      virtual void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
                                     char *buf, const unsigned long &linenum);

      virtual void finalize_configuration();

      // others.
      bool is_plugin_activated(const char *pname);
      int get_plugin_priority(const char *pname);
      
      // variables.

      /** What to log */
      int _debug;

      /** Nonzero to enable multithreading. */
      int _multi_threaded;

      /**
       * Bitmask of features that can be enabled/disabled through the config
       * file.  Currently defined bits:
       *
       * - RUNTIME_FEATURE_CGI_TOGGLE
       * - RUNTIME_FEATURE_HTTP_TOGGLE
       * - RUNTIME_FEATURE_SPLIT_LARGE_FORMS
       */
      unsigned _feature_flags;

      /** The log file name. */
      const char *_logfile;

      /** The config file directory. */
      const char *_confdir;

      /** The directory for customized CGI templates. */
      const char *_templdir;

      /** The log file directory. */
      const char *_logdir;

      /** The plugin repository. */
      const char *_plugindir;

      /** The data repository for all data (including plugin data). */
      const char *_datadir;

      /** The list of activated plugins. */
      hash_map<const char*,int,hash<const char*>,eqstr> _activated_plugins;

      /** The administrator's email address */
      char *_admin_address;

      /** A URL with info on this proxy */
      char *_proxy_info_url;

      /** URL to the user manual (on our website or local copy) */
      char *_usermanual;

      /** The hostname to show on CGI pages, or NULL to use the real one. */
      const char *_hostname;

      /** IP address to bind to.  Defaults to HADDR_DEFAULT == 127.0.0.1. */
      char *_haddr;

      /** Port to bind to.  Defaults to HADDR_PORT == 8118. */
      int _hport;

      /** Size limit for IOB */
      size_t _buffer_limit;

      /** Information about parent proxies (forwarding). */
      forward_spec *_forward;

      /** Number of retries in case a forwarded connection attempt fails */
      int _forwarded_connect_retries;

      /** Maximum number of client connections. */
      int _max_client_connections;

      /* Timeout when waiting on sockets for data to become available. */
      int _socket_timeout;

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
      /* Maximum number of seconds after which an open connection will no longer be reused. */
      unsigned int _keep_alive_timeout;
#endif

      /* Nonzero if we need to bind() to the new port. */
      int _need_bind;

#ifdef FEATURE_ACL
      /* Access control list. */
      access_control_list *_acl;
#endif

      /* automatically disable proxy. */
      bool _automatic_proxy_disable;

      /* cors enabled. */
      bool _cors_enabled;

      /* cors allowed domains. */
      std::string _cors_allowed_domains;

      /*  user db file. */
      std::string _user_db_file;

      /* user remote db address. */
      char *_user_db_haddr;

      /* user db remote port. */
      int _user_db_hport;

      /* user db startup check. */
      bool _user_db_startup_check;

      /* user db optimization. */
      bool _user_db_optimize;

      /* user db flag for db > 2Gb. */
      bool _user_db_large;

      /* user db initial number of buckets. */
      int64_t _user_db_bnum;

      /* pointer to source code. */
      std::string _url_source_code;

      /* transfer timeout when fetching content for analysis & caching. */
      long _ct_transfer_timeout;

      /* connection timeout when fetching content for analysis & caching. */
      long _ct_connect_timeout;
  };

} /* end of namespace. */

#endif
