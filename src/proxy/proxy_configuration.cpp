/**
 * The seeks proxy is part of the SEEKS project
 * It is based on Privoxy (http://www.privoxy.org), developped
 * by the Privoxy team.
 *
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "proxy_configuration.h"
#include "mem_utils.h"
#include "errlog.h"
#include "miscutil.h"
#include "urlmatch.h"
#include "seeks_proxy.h" // for daemon flag.
#include "gateway.h"
#include "filters.h"
#include "plugin_manager.h"

#include <iostream>

namespace sp
{
   
   /*
    * This takes the "cryptic" hash of each keyword and aliases them to
    * something a little more readable.  This also makes changing the
    * hash values easier if they should change or the hash algorthm changes.
    * Use the included "hash" program to find out what the hash will be
    * for any string supplied on the command line.  (Or just put it in the
    * config file and read the number from the error message in the log).
    *
    * Please keep this list sorted alphabetically (but with the Windows
    * console and GUI specific options last).
    */
   #define hash_activated_plugins             1661287526ul /* "activated-plugin" */
   #define hash_accept_intercepted_requests    761056836ul /* "accept-intercepted-requests" */
   #define hash_admin_address                 3922903443ul /* "admin-address" */
   #define hash_allow_cgi_request_crunching   3572249502ul /* "allow-cgi-request-crunching" */
   #define hash_buffer_limit                  3997655021ul /* "buffer-limit */
   #define hash_confdir                       1496854555ul /* "confdir" */
   #define hash_connection_sharing            1951096841ul /* "connection-sharing" */
   #define hash_debug                         3473953184ul /* "debug" */
   #define hash_deny_access                   2517089737ul /* "deny-access" */
   #define hash_enable_remote_toggle          2307317490ul /* "enable-remote-toggle" */
   #define hash_enable_remote_http_toggle     3749326366ul /* "enable-remote-http-toggle" */
   #define hash_forward                       2453031082ul /* "forward" */
   #define hash_forward_socks4                2224680052ul /* "forward-socks4" */
   #define hash_forward_socks4a               1278450079ul /* "forward-socks4a" */
   #define hash_forward_socks5                1117649931ul /* "forward-socks5" */
   #define hash_forwarded_connect_retries      689162952ul /* "forwarded-connect-retries" */
   #define hash_hostname                      1558631356ul /* "hostname" */
   #define hash_keep_alive_timeout            1578027561ul /* "keep-alive-timeout" */
   #define hash_listen_address                2520413741ul /* "listen-address" */
   #define hash_logdir                        3521470393ul /* "logdir" */
   #define hash_logfile                         38803787ul /* "logfile" */
   #define hash_plugindir                     2324026896ul /* "plugindir" */
   #define hash_datadir                       4055122953ul /* "datadir" */
   #define hash_max_client_connections        1237838806ul /* "max-client-connections" */
   #define hash_permit_access                 1005955844ul /* "permit-access" */
   #define hash_proxy_info_url                 181245282ul /* "proxy-info-url" */
   #define hash_single_threaded               1186286844ul /* "single-threaded" */
   #define hash_socket_timeout                2305082655ul /* "socket-timeout" */
   #define hash_split_large_cgi_forms          443436323ul /* "split-large-cgi-forms" */
   #define hash_suppress_blocklists           1452993892ul /* "suppress-blocklists" */
   #define hash_templdir                      1485902173ul /* "templdir" */
   #define hash_toggle                        1035528941ul /* "toggle" */
   #define hash_trust_info_url                2137035860ul /* "trust-info-url" */
   #define hash_trustfile                      205192134ul /* "trustfile" */
   #define hash_usermanual                    3799188714ul /* "user-manual" */
   #define hash_activity_animation            2082128920ul /* "activity-animation" */
   #define hash_close_button_minimizes        3780370405ul /* "close-button-minimizes" */
   #define hash_hide_console                  2392633815ul /* "hide-console" */
   #define hash_log_buffer_size               4215458813ul /* "log-buffer-size" */
   #define hash_log_font_name                 2835979623ul /* "log-font-name" */
   #define hash_log_font_size                 2867212173ul /* "log-font-size" */
   #define hash_log_highlight_messages        2213926481ul /* "log-highlight-messages" */
   #define hash_log_max_lines                 2725447368ul /* "log-max-lines" */
   #define hash_log_messages                  2667424627ul /* "log-messages" */
   #define hash_show_on_task_bar              4011152997ul /* "show-on-task-bar" */
   
   proxy_configuration::proxy_configuration(const std::string &filename)
     :configuration_spec(filename),_debug(0),_multi_threaded(0),_feature_flags(0),_logfile(NULL),_confdir(NULL),
      _templdir(NULL),_logdir(NULL),_plugindir(NULL),_datadir(NULL),
      _admin_address(NULL),_proxy_info_url(NULL),_usermanual(NULL),
      _hostname(NULL),
      _haddr(NULL),_hport(0),_buffer_limit(0),
      _forward(NULL),_forwarded_connect_retries(0),_max_client_connections(0),_socket_timeout(0)
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
     ,_keep_alive_timeout(0)
#endif
     ,_need_bind(0)
      {
	 load_config();
      }
   
   proxy_configuration::~proxy_configuration()
     {
	// beware: do not delete _forward.
	free_const(_confdir);
	free_const(_logdir);
	free_const(_templdir);
	free_const(_hostname);
	free_const(_haddr);
	free_const(_logfile);
	free_const(_plugindir);
	free_const(_datadir);
	
	freez(_admin_address);
	freez(_proxy_info_url);
	freez(_usermanual);
     }
   
   // virtual functions.

   void proxy_configuration::set_default_config()
     {
	_multi_threaded            = 1;
	_buffer_limit              = 4096 * 1024;
	_usermanual                = strdup(USER_MANUAL_URL);
	_forwarded_connect_retries = 0;
	_max_client_connections    = 0;
	_socket_timeout            = 300; /* XXX: Should be a macro. */
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
	_keep_alive_timeout        = DEFAULT_KEEP_ALIVE_TIMEOUT;
	_feature_flags            &= ~RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE;
	_feature_flags            &= ~RUNTIME_FEATURE_CONNECTION_SHARING;
#endif
	_feature_flags            &= ~RUNTIME_FEATURE_CGI_TOGGLE;
	_feature_flags            &= ~RUNTIME_FEATURE_SPLIT_LARGE_FORMS;
	_feature_flags            &= ~RUNTIME_FEATURE_ACCEPT_INTERCEPTED_REQUESTS;
     
	_activated_plugins.insert(std::pair<const char*,bool>("websearch",true)); // websearch plugin activated by default.
     }
   
   void proxy_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
					       char *buf, const unsigned long &linenum)
     {
	forward_spec *cur_fwd;
	int vec_count;
	char *vec[3];
	char tmp[BUFFER_SIZE];
	char *p;
#ifdef FEATURE_ACL
	access_control_list *cur_acl;
#endif  /* def FEATURE_ACL */
	
	switch(cmd_hash)
	  {
	     /**************************************************************************
	      * activated_plugins
	      * *************************************************************************/
	   case hash_activated_plugins :
	     _activated_plugins.insert(std::pair<const char*,bool>(strdup(arg),true));
	     configuration_spec::html_table_row(_config_args,cmd,arg,"Activated plugin");
	     break;
	     
	     /**************************************************************************
	      * accept-intercepted-requests
	      **************************************************************************/
	   case hash_accept_intercepted_requests:
	     if ((*arg != '\0') && (0 != atoi(arg)))
	       {
		  _feature_flags |= RUNTIME_FEATURE_ACCEPT_INTERCEPTED_REQUESTS;
	       }
	     else
	       {
		  _feature_flags &= ~RUNTIME_FEATURE_ACCEPT_INTERCEPTED_REQUESTS;
	       }
	     break;
	     
	     /**************************************************************************
	      * admin-address email-address
	      **************************************************************************/
	   case hash_admin_address :
	     freez(_admin_address);
	     _admin_address = strdup(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,"Admin email address");
	     break;
	     
	     /**************************************************************************
	      * allow-cgi-request-crunching
	      **************************************************************************/
	   case hash_allow_cgi_request_crunching:
	     if ((*arg != '\0') && (0 != atoi(arg)))
	       {
		  _feature_flags |= RUNTIME_FEATURE_CGI_CRUNCHING;
	       }
	     else
	       {
		  _feature_flags &= ~RUNTIME_FEATURE_CGI_CRUNCHING;
	       }
	     break;
	     
	     /**************************************************************************
	      * buffer-limit n
	      **************************************************************************/
	   case hash_buffer_limit :
	     _buffer_limit = (size_t)(1024 * atoi(arg));
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Maximum size of the buffer for content filtering");
	     break;
	     
	     /**************************************************************************
	      * confdir directory-name
	      **************************************************************************/
	   case hash_confdir :
	     free_const(_confdir);
	     _confdir = seeks_proxy::make_path( NULL, arg);
	     break;
	     
	     /**************************************************************************
	      * connection-sharing (0|1)
	      **************************************************************************/
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
	   case hash_connection_sharing :
	     if ((*arg != '\0') && (0 != atoi(arg)))
	       {
		  _feature_flags |= RUNTIME_FEATURE_CONNECTION_SHARING;
	       }
	     else
	       {
		  _feature_flags &= ~RUNTIME_FEATURE_CONNECTION_SHARING;
	       }
	     break;
#endif
	     
	     /**************************************************************************
	      * debug n
	      * Specifies debug level, multiple values are ORed together.
	      **************************************************************************/
	   case hash_debug :
	     _debug |= atoi(arg);
	     break;
	     
	     /*************************************************************************
	      * deny-access source-ip[/significant-bits] [dest-ip[/significant-bits]]
	      * *************************************************************************/
#ifdef FEATURE_ACL
	   case hash_deny_access:
	     std::cerr << "[Debug]: reading acl.\n";
	     
	     strlcpy(tmp, arg, sizeof(tmp));
	     vec_count = miscutil::ssplit(tmp, " \t", vec, SZ(vec), 1, 1);
	     
	     if ((vec_count != 1) && (vec_count != 2))
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for "
				    "deny-access directive in configuration file.");
		  break;
	       }
	     
	     /* allocate a new node */
	     cur_acl = new access_control_list();
	     
	     if (cur_acl == NULL)
	       {
		  errlog::log_error(LOG_LEVEL_FATAL, "can't allocate memory for configuration");
		  /* Never get here - LOG_LEVEL_FATAL causes program exit */
		  break;
	       }
	     
	     cur_acl->_action = ACL_DENY;
	           
	     if (filters::acl_addr(vec[0], &cur_acl->_src) < 0)
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Invalid source address, port or netmask "
				    "for deny-access directive in configuration file: \"%s\"", vec[0]);
		  delete cur_acl;
		  break;
	       }
	     if (vec_count == 2)
	       {
		  if (filters::acl_addr(vec[1], &cur_acl->_dst) < 0)
		    {
		       errlog::log_error(LOG_LEVEL_ERROR, "Invalid destination address, port or netmask "
					 "for deny-access directive in configuration file: \"%s\"", vec[1]);
		       delete cur_acl;
		       break;
		    }
	       }
	     
# ifdef HAVE_RFC2553
	     else
	       {
		  cur_acl->_wildcard_dst = 1;
	       }
# endif /* def HAVE_RFC2553 */
	     /*
	      * Add it to the list.  Note we reverse the list to get the
	      * behaviour the user expects.  With both the ACL and
	      * actions file, the last match wins.  However, the internal
	      * implementations are different:  The actions file is stored
	      * in the same order as the file, and scanned completely.
	      * With the ACL, we reverse the order as we load it, then
	      * when we scan it we stop as soon as we get a match.
	      */
	     cur_acl->_next  = _acl;
	     _acl = cur_acl;
	     
	     break;
#endif /* def FEATURE_ACL */
		  
	     /*************************************************************************
	      * permit-access source-ip[/significant-bits] [dest-ip[/significant-bits]]
	      * *************************************************************************/
#ifdef FEATURE_ACL
	   case hash_permit_access:
	     strlcpy(tmp, arg, sizeof(tmp));
	     vec_count = miscutil::ssplit(tmp, " \t", vec, SZ(vec), 1, 1);
	     
	     if ((vec_count != 1) && (vec_count != 2))
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for "
				    "permit-access directive in configuration file.");
		  break;
	       }
	     
	     /* allocate a new node */
	     cur_acl = new access_control_list();
	     if (cur_acl == NULL)
	       {
		  errlog::log_error(LOG_LEVEL_FATAL, "can't allocate memory for configuration");
		  /* Never get here - LOG_LEVEL_FATAL causes program exit */
		  break;
	       }
	     
	     cur_acl->_action = ACL_PERMIT;
	     
	     if (filters::acl_addr(vec[0], &cur_acl->_src) < 0)
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Invalid source address, port or netmask "
				    "for permit-access directive in configuration file: \"%s\"", vec[0]);
		  delete cur_acl;
		  break;
	       }
	     if (vec_count == 2)
	       {
		  if (filters::acl_addr(vec[1], &cur_acl->_dst) < 0)
		    {
		       errlog::log_error(LOG_LEVEL_ERROR, "Invalid destination address, port or netmask "
					 "for permit-access directive in configuration file: \"%s\"", vec[1]);
		       delete cur_acl;
		       break;
		    }
	       }
# ifdef HAVE_RFC2553
	     else
	       {
		  cur_acl->_wildcard_dst = 1;
	       }
# endif /* def HAVE_RFC2553 */
	     
	     /*
	      * Add it to the list.  Note we reverse the list to get the
	      * behaviour the user expects.  With both the ACL and
	      * actions file, the last match wins.  However, the internal
	      * implementations are different:  The actions file is stored
	      * in the same order as the file, and scanned completely.
	      * With the ACL, we reverse the order as we load it, then
	      * when we scan it we stop as soon as we get a match.
	      */
	     cur_acl->_next  = _acl;
	     _acl = cur_acl;
	     
	     break;
#endif /* def FEATURE_ACL */
		  
	     /**************************************************************************
	      * enable-remote-toggle 0|1
	      **************************************************************************/
#ifdef FEATURE_TOGGLE
	   case hash_enable_remote_toggle:
	     if ((*arg != '\0') && (0 != atoi(arg)))
	       {
		  _feature_flags |= RUNTIME_FEATURE_CGI_TOGGLE;
	       }
	     else
	       {
		  _feature_flags &= ~RUNTIME_FEATURE_CGI_TOGGLE;
	       }
	     break;
#endif /* def FEATURE_TOGGLE */
	     
	     /**************************************************************************
	      * enable-remote-http-toggle 0|1
	      **************************************************************************/
	   case hash_enable_remote_http_toggle:
	     if ((*arg != '\0') && (0 != atoi(arg)))
	       {
		  _feature_flags |= RUNTIME_FEATURE_HTTP_TOGGLE;
	       }
	     else
	       {
		  _feature_flags &= ~RUNTIME_FEATURE_HTTP_TOGGLE;
	       }
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Whether or not the proxy recognizes special HTTP headers to change its behavior");
	     break;
	     
	     /************************************************************************
	      * forward url-pattern (.|http-proxy-host[:port])
	      ************************************************************************/
	   case hash_forward:
	     strlcpy(tmp, arg, sizeof(tmp));
	     vec_count = miscutil::ssplit(tmp, " \t", vec, SZ(vec), 1, 1);
	     
	     if (vec_count != 2)
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for forward "
				    "directive in configuration file.");
		  miscutil::string_append(&_config_args,
					  "<br>\nWARNING: Wrong number of parameters for "
					  "forward directive in configuration file.");
		  break;
	       }
	     
	     /* allocate a new node */
	     cur_fwd = new forward_spec();
	     if (cur_fwd == NULL)
	       {
		  errlog::log_error(LOG_LEVEL_FATAL, "can't allocate memory for configuration");
		  /* Never get here - LOG_LEVEL_FATAL causes program exit */
		  break;
	       }
	     
	     cur_fwd->_type = SOCKS_NONE;
	     
	     /* Save the URL pattern */
	     if (url_spec::create_url_spec(cur_fwd->_url, vec[0]))
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Bad URL specifier for forward "
				    "directive in configuration file.");
		  miscutil::string_append(&_config_args,
					  "<br>\nWARNING: Bad URL specifier for "
					  "forward directive in configuration file.");
		  break;
	       }
	     
	     /* Parse the parent HTTP proxy host:port */
	     p = vec[1];
	     
	     if (strcmp(p, ".") != 0)
	       {
		  cur_fwd->_forward_port = 8000;
		  urlmatch::parse_forwarder_address(p, &cur_fwd->_forward_host,
						    &cur_fwd->_forward_port);
	       }
	     
	     /* Add to list. */
	     cur_fwd->_next = _forward;
	     _forward = cur_fwd;
	     
	     break;
	     
	     /************************************************************************
	      * forward-socks4 url-pattern socks-proxy[:port] (.|http-proxy[:port])
	      ************************************************************************/ 
	   case hash_forward_socks4:
	     strlcpy(tmp, arg, sizeof(tmp));
	     vec_count = miscutil::ssplit(tmp, " \t", vec, SZ(vec), 1, 1);
	     if (vec_count != 3)
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for "
				    "forward-socks4 directive in configuration file.");
		  miscutil::string_append(&_config_args,
					  "<br>\nWARNING: Wrong number of parameters for "
					  "forward-socks4 directive in configuration file.");
		  break;
	       }
	     
	     /* allocate a new node */
	     cur_fwd = new forward_spec();
	     if (cur_fwd == NULL)
	       {
		  errlog::log_error(LOG_LEVEL_FATAL, "can't allocate memory for configuration");
		  /* Never get here - LOG_LEVEL_FATAL causes program exit */
		  break;
	       }
	     
	     cur_fwd->_type = SOCKS_4;
	     
	     /* Save the URL pattern */
	     if (url_spec::create_url_spec(cur_fwd->_url, vec[0]))
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Bad URL specifier for forward "
				    "directive in configuration file.");
		  miscutil::string_append(&_config_args,
					  "<br>\nWARNING: Bad URL specifier for "
					  "forward directive in configuration file.");
		  break;
	       }
	     
	     /* Parse the SOCKS proxy host[:port] */
	     p = vec[1];
	     
	     /* XXX: This check looks like a bug. */
	     if (strcmp(p, ".") != 0)
	       {
		  cur_fwd->_gateway_port = 1080;
		  urlmatch::parse_forwarder_address(p, &cur_fwd->_gateway_host,
						    &cur_fwd->_gateway_port);
	       }
	     
	     /* Parse the parent HTTP proxy host[:port] */
	     p = vec[2];
	     if (strcmp(p, ".") != 0)
	       {
		  cur_fwd->_forward_port = 8000;
		  urlmatch::parse_forwarder_address(p, &cur_fwd->_forward_host,
						    &cur_fwd->_forward_port);
	       }
	     
	      /* Add to list. */
	     cur_fwd->_next = _forward;
	     _forward = cur_fwd;
	     
	     break;
	     
	     /*************************************************************************
	      * forward-socks4a url-pattern socks-proxy[:port] (.|http-proxy[:port])
	      **************************************************************************/
	   case hash_forward_socks4a:
	   case hash_forward_socks5:
	     strlcpy(tmp, arg, sizeof(tmp));
	     vec_count = miscutil::ssplit(tmp, " \t", vec, SZ(vec), 1, 1);
	     if (vec_count != 3)
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for "
				    "forward-socks4a directive in configuration file.");
		  miscutil::string_append(&_config_args,
					  "<br>\nWARNING: Wrong number of parameters for "
					  "forward-socks4a directive in configuration file.");
		  break;
	       }
	     
	     /* allocate a new node */
	     cur_fwd = new forward_spec();
	     if (cur_fwd == NULL)
	       {
		  errlog::log_error(LOG_LEVEL_FATAL, "can't allocate memory for configuration");
		  /* Never get here - LOG_LEVEL_FATAL causes program exit */
		  break;
	       }
	     
	     if (cmd_hash == hash_forward_socks4a)
	       {
		  cur_fwd->_type = SOCKS_4A;
	       }
	     else
	       {
		  cur_fwd->_type = SOCKS_5;
	       }
	     
	     /* Save the URL pattern */
	     if (url_spec::create_url_spec(cur_fwd->_url, vec[0]))
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Bad URL specifier for forward "
				    "directive in configuration file.");
		  miscutil::string_append(&_config_args,
					  "<br>\nWARNING: Bad URL specifier for "
					  "forward directive in configuration file.");
		  break;
	       }
	     
	     /* Parse the SOCKS proxy host[:port] */
	     p = vec[1];
	     cur_fwd->_gateway_port = 1080;
	     urlmatch::parse_forwarder_address(p, &cur_fwd->_gateway_host,
					       &cur_fwd->_gateway_port);
	     
	     /* Parse the parent HTTP proxy host[:port] */
	     p = vec[2];
	     if (strcmp(p, ".") != 0)
	       {
		  cur_fwd->_forward_port = 8000;
		  urlmatch::parse_forwarder_address(p, &cur_fwd->_forward_host,
						    &cur_fwd->_forward_port);
	       }
	     
	     /* Add to list. */
	     cur_fwd->_next = _forward;
	     _forward = cur_fwd;
	     
	     break;
	     
	     /*************************************************************************
	      * forwarded-connect-retries n
	      *************************************************************************/
	   case hash_forwarded_connect_retries :
	     _forwarded_connect_retries = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"How often the proxy retries if a forwarded connection request fails");
	     break;
	     
	     /*************************************************************************
	      * hostname hostname-to-show-on-cgi-pages
	      ************************************************************************/
	   case hash_hostname :
	     free_const(_hostname);
	     _hostname = strdup(arg);
	     if (NULL == _hostname)
	       {
		  errlog::log_error(LOG_LEVEL_FATAL, "Out of memory saving hostname.");
	       }
	     break;
	  
	     /*************************************************************************
	      * keep-alive-timeout timeout
	      *************************************************************************/
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
	   case hash_keep_alive_timeout :
	     if (*arg != '\0')
	       {
		  int timeout = atoi(arg);
		  if (0 < timeout)
		    {
		       _feature_flags |= RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE;
		       _keep_alive_timeout = (unsigned int)timeout;
		    }
		  else
		    {
		       _feature_flags &= ~RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE;
		    }
	       }
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Number of seconds after which an open connection will no longer be reused");
	     break;
#endif

	     /*************************************************************************
	      * listen-address [ip][:port]
	      *************************************************************************/
	   case hash_listen_address :
	     free_const(_haddr);
	     _haddr = strdup(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"The IP address and TCP port on which the proxy listens to the client requests");
	     break;
	     
	     /*************************************************************************
	      * plugindir path to a directory
	      *************************************************************************/
	   case hash_plugindir :
	     free_const(_plugindir);
	     _plugindir = strdup(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"The repository to be scanned for precompiled plugins");
	     break;
	       
	     /************************************************************************
	      * datadir path to a directory for data
	      ************************************************************************/
	   case hash_datadir :
	     free_const(_datadir);
	     _datadir = strdup(arg);
	     seeks_proxy::_datadir = std::string(_datadir); // XXX: should be outside this class...
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"The data repository (including plugin data)");
	     break;
	     
	     /*************************************************************************
	      * logdir directory-name
	      *************************************************************************/
	   case hash_logdir :
	     free_const(_logdir);
	     _logdir = seeks_proxy::make_path(NULL, arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"The log file to use");
	     break;
	     
	     /*************************************************************************
	      * logfile log-file-name
	      * In logdir by default
	      *************************************************************************/
	   case hash_logfile :
	     if (!seeks_proxy::_no_daemon)
	       {
		  // TODO: check on seeks_proxy daemon flag.
		  if (!seeks_proxy::_no_daemon)
		    {
		       _logfile = seeks_proxy::make_path(_logdir, arg);
		        if (NULL == _logfile)
			 {
			    errlog::log_error(LOG_LEVEL_FATAL, "Out of memory while creating logfile path");
			 }
		       errlog::init_error_log(seeks_proxy::_Argv[0], _logfile);
		    }
		  else errlog::disable_logging();
	       }
	     break;
	     
	     /*************************************************************************
	      * max-client-connections number
	      *************************************************************************/
	   case hash_max_client_connections :
	     if (*arg != '\0')
	       {
		  int max_client_connections = atoi(arg);
		  if (0 <= max_client_connections)
		    {
		       _max_client_connections = max_client_connections;
		    }
	       }
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Maximum number of client connection that will be served");
	     break;
	     
	     /*************************************************************************
	      * proxy-info-url url
	      *************************************************************************/
	   case hash_proxy_info_url :
	     freez(_proxy_info_url);
	     _proxy_info_url = strdup(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"A URL to documentation about the local proxy setup, configuration or policies");
	     break;
	     
	     /*************************************************************************
	      * single-threaded
	      *************************************************************************/
	   case hash_single_threaded :
	     _multi_threaded = 0;
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Whether to run only on server thread");
	     break;
	     
	     /*************************************************************************
	      * socket-timeout numer_of_seconds
	      *************************************************************************/
	   case hash_socket_timeout :
	     if (*arg != '\0')
	       {
		  int socket_timeout = atoi(arg);
		  if (0 < socket_timeout)
		    {
		       _socket_timeout = socket_timeout;
		    }
		  else
		    {
		       errlog::log_error(LOG_LEVEL_FATAL,
					 "Invalid socket-timeout: '%s'", arg);
		    }
	       }
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Number of seconds after which a socket times out if no data is received");
	     break;
	     
	     /*************************************************************************
	      * split-large-cgi-forms
	      *************************************************************************/
	   case hash_split_large_cgi_forms :
	     if ((*arg != '\0') && (0 != atoi(arg)))
	       {
		  _feature_flags |= RUNTIME_FEATURE_SPLIT_LARGE_FORMS;
	       }
	     else
	       {
		  _feature_flags &= ~RUNTIME_FEATURE_SPLIT_LARGE_FORMS;
	       }
	     break;
	     
	     /*************************************************************************
	      * templdir directory-name
	      *************************************************************************/
	   case hash_templdir :
	     free_const(_templdir);
	     /* if (seeks_proxy::_datadir.empty())
	       _templdir = seeks_proxy::make_path(NULL, arg);
	     else _templdir = seeks_proxy(seeks_proxy::_datadir.c_str(), arg); */
	     _templdir = strdup(arg);
	     break;
	     
	     //TODO: seeks, toggle on/off from a local webpage.
	     /*************************************************************************
	      * toggle (0|1)
	      *************************************************************************/
#ifdef FEATURE_TOGGLE
	   case hash_toggle :
	     seeks_proxy::_global_toggle_state = atoi(arg);
	     break;
#endif
	  
	     /*************************************************************************
	      * usermanual url
	      *************************************************************************/
	   case hash_usermanual :
	      
	     // TODO.
	     /*
	      * XXX: If this isn't the first config directive, the
	      * show-status page links to the website documentation
	      * for the directives that were already parsed. Lame.
	      */
	     free_const(_usermanual);
	     _usermanual = strdup(arg);
	     break;
	     
	     /*************************************************************************
	      * Win32 Console options:
	      *************************************************************************/
	     
	     /*************************************************************************
	      * hide-console
	      *************************************************************************/
#ifdef _WIN_CONSOLE
	   case hash_hide_console :
	     hideConsole = 1;
	     break;
#endif /*def _WIN_CONSOLE*/
	     
	     /*************************************************************************
	      * Win32 GUI options:
	      *************************************************************************/	     
#if defined(_WIN32) && ! defined(_WIN_CONSOLE)
	     /*************************************************************************
	      * activity-animation (0|1)
	      *************************************************************************/
	   case hash_activity_animation :
	     g_bShowActivityAnimation = atoi(arg);
	     break;
	     
	     /*************************************************************************
	      *  close-button-minimizes (0|1)
	      *************************************************************************/
	   case hash_close_button_minimizes :
	     g_bCloseHidesWindow = atoi(arg);
	     break;
	     
	     /*************************************************************************
	      * log-buffer-size (0|1)
	      *************************************************************************/
	   case hash_log_buffer_size :
	     g_bLimitBufferSize = atoi(arg);
	     break;
	     
	     /*************************************************************************
	      * log-font-name fontname
	      *************************************************************************/
	   case hash_log_font_name :
	     if (strlcpy(g_szFontFaceName, arg,
			 sizeof(g_szFontFaceName)) >= sizeof(g_szFontFaceName))
	       {
		  errlog::log_error(LOG_LEVEL_FATAL,
				    "log-font-name argument '%s' is longer than %u characters.",
				    arg, sizeof(g_szFontFaceName)-1);
	       }
	     break;
	     
	     /*************************************************************************
	      * log-font-size n
	      *************************************************************************/
	   case hash_log_font_size :
	     g_nFontSize = atoi(arg);
	     break;
	     
	     /*************************************************************************
	      * log-highlight-messages (0|1)
	      *************************************************************************/
	   case hash_log_highlight_messages :
	     g_bHighlightMessages = atoi(arg);
	     break;
	     
	     /*************************************************************************
	      * log-max-lines n
	      *************************************************************************/
	   case hash_log_max_lines :
	     g_nMaxBufferLines = atoi(arg);
	     break;
	     
	     /*************************************************************************
	      * log-messages (0|1)
	      *************************************************************************/
	   case hash_log_messages :
	     g_bLogMessages = atoi(arg);
	     break;
	     
	     /*************************************************************************
	      * show-on-task-bar (0|1)
	      *************************************************************************/
	   case hash_show_on_task_bar :
	     g_bShowOnTaskBar = atoi(arg);
	     break;
#endif
	     
	     /*************************************************************************
	      * Warnings about unsupported features
	      *************************************************************************/
#ifndef FEATURE_TOGGLE
	   case hash_enable_remote_toggle:
#endif /* ndef FEATURE_TOGGLE */

#ifndef FEATURE_TOGGLE
	   case hash_toggle :
#endif /* ndef FEATURE_TOGGLE */
	     
#ifndef _WIN_CONSOLE
	   case hash_hide_console :
#endif /* ndef _WIN_CONSOLE */
	     
#if defined(_WIN_CONSOLE) || ! defined(_WIN32)
	   case hash_activity_animation :
	   case hash_close_button_minimizes :
	   case hash_log_buffer_size :
	   case hash_log_font_name :
	   case hash_log_font_size :
	   case hash_log_highlight_messages :
	   case hash_log_max_lines :
	   case hash_log_messages :
	   case hash_show_on_task_bar :
#endif /* defined(_WIN_CONSOLE) || ! defined(_WIN32) */
	     /* These warnings are annoying - so hide them. -- Jon */
	     /* errlog::log_error(LOG_LEVEL_INFO, "Unsupported directive \"%s\" ignored.", cmd); */
	     break;
	     
	     /*************************************************************************/
	   default :
	     /*************************************************************************/
	     /*
	      * I decided that I liked this better as a warning than an
	      * error.  To change back to an error, just change log level
	      * to LOG_LEVEL_FATAL.
	      */
	     errlog::log_error(LOG_LEVEL_ERROR, "Ignoring unrecognized configuration command '%s' (%luul) in line %lu "
			       "in configuration file (%s).", buf, cmd_hash, linenum, _filename.c_str());
	     miscutil::string_append(&_config_args,
				     " <strong class='warning'>Warning: Ignoring unrecognized directive:</strong>");
	     break;
	     
	  } // end switch.
	
     }
   
   void proxy_configuration::finalize_configuration()
     {
	// TODO.
	errlog::set_debug_level(_debug);
	
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
	if (_feature_flags & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE)
	  {
	     if (_multi_threaded)
	       {
		  gateway::set_keep_alive_timeout(_keep_alive_timeout);
	       }
	     else
	       {
		  /**
		   * While we could use keep-alive without multiple threads
		   * if we didn't bother with enforcing the connection timeout,
		   * that might make Tor users sad, even though they shouldn't
		   * enable the single-threaded option anyway.
		   *
		   * XXX: We could still use Proxy-Connection: keep-alive.
		   */
		  _feature_flags &= ~RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE;
		  errlog::log_error(LOG_LEVEL_ERROR,
				    "Config option single-threaded disables connection keep-alive.");
	       }
	  }
	else if ((_feature_flags & RUNTIME_FEATURE_CONNECTION_SHARING))
	  {
	     errlog::log_error(LOG_LEVEL_ERROR, "Config option connection-sharing "
			       "has no effect if keep-alive-timeout isn't set.");
	     _feature_flags &= ~RUNTIME_FEATURE_CONNECTION_SHARING;
	  }
#endif
	    
	 if (NULL == _config_args)
	  {
	     errlog::log_error(LOG_LEVEL_FATAL, "Out of memory loading config - insufficient memory for _config_args in configuration");
	  }
	
	if (NULL == _haddr )
	  {
	     _haddr = strdup( HADDR_DEFAULT );
	  }

	char *p = NULL;
	if ( NULL != _haddr )
	  {
	     if ((*_haddr == '[')
		 && (NULL != (p = strchr(_haddr, ']')))
		 && (p[1] == ':')
		 && (0 < (_hport = atoi(p + 2))))
	       {
		  *p = '\0';
		  memmove((void *)_haddr, _haddr + 1,
			  (size_t)(p - _haddr));
	       }
	     else if (NULL != (p = strchr(_haddr, ':'))
		      && (0 < (_hport = atoi(p + 1))))
	       {
		  *p = '\0';
	       }
	     else
	       {
		  errlog::log_error(LOG_LEVEL_FATAL, "invalid bind port spec %s", _haddr);
		  /* Never get here - LOG_LEVEL_FATAL causes program exit */
	       }
	      if (_haddr == '\0')
	       {
		  /*
		   * Only the port specified. We stored it in _hport
		   * and don't need its text representation anymore.
		   */
		  free_const(_haddr);
	       }
	  }
	
	_need_bind = 1;
	// TODO: deal with port change here, and the need for a bind.

#ifdef SEEKS_PLUGINS_LIBDIR
	// nothing configured,  use default.
	if (!_plugindir)
	{
	   free_const(_plugindir);
	   _plugindir = strdup(SEEKS_PLUGINS_LIBDIR);
	}
#endif
 
#ifdef SEEKS_DATADIR
	// nothing configured, use default.
	if (seeks_proxy::_datadir.empty())
	{
	   seeks_proxy::_datadir = std::string(SEEKS_DATADIR);
	}
#endif
	
	// update template directory as necessary.
	const char *templdir_old = _templdir;
	if (!seeks_proxy::_datadir.empty())
	  {
	     std::string templdir_str = seeks_proxy::_datadir + "/" + std::string(_templdir);
	     _templdir = strdup(templdir_str.c_str());
	  }
	else _templdir = seeks_proxy::make_path(NULL,_templdir);

	free_const(templdir_old);
     }

   bool proxy_configuration::is_plugin_activated(const char *pname)
     {
	hash_map<const char*,bool,hash<const char*>,eqstr>::const_iterator hit;
	if ((hit = _activated_plugins.find(pname))!=_activated_plugins.end())
	  return true;
	else return false;
     }
   
} /* end of namespace. */
