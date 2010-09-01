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

#ifndef SEEKS_PROXY_H
#define SEEKS_PROXY_H

#include "config.h"
#include "proxy_dts.h"
#include "sweeper.h"
#include "lsh_configuration.h"

#if (defined __NetBSD__) || (defined __OpenBSD__) || (defined ON_OSX)
#define unix 1
#endif

#ifdef FEATURE_PTHREAD
extern "C" 
{
# include <pthread.h>
}
#endif

#ifdef FEATURE_PTHREAD
        typedef pthread_mutex_t sp_mutex_t;
#else
        typedef CRITICAL_SECTION sp_mutex_t;
#endif

using lsh::lsh_configuration;

namespace sp
{

   typedef http_response *(*crunch_func_ptr)(client_state *);
   
   /* A crunch function and its flags */
   class cruncher
     {
      public:
	cruncher()
	  :_cruncher(NULL),_flags(0)
	    {};
	
	cruncher(const crunch_func_ptr cruncher,
		 const int &flags)
	  :_cruncher(cruncher),_flags(flags)
	    {};
	
	~cruncher() {};
	
	const crunch_func_ptr _cruncher;
	const int _flags;
     };
   
   class seeks_proxy
     {
	/*-- global variables --*/
      public:
#ifdef FEATURE_STATISTICS
	static int _urls_read;
	static int _urls_rejected;
#endif /*def FEATURE_STATISTICS*/
	
	static client_state _clients;

	static std::vector<sweepable*> _memory_dust; // sweepable elements (cache, etc...).
	
	static std::string _configfile; // proxy configuration file.

	static proxy_configuration *_config; // proxy configuration object.
	
	static std::string _lshconfigfile; // lsh configuration file.
	
	static lsh_configuration *_lsh_config; // lsh configuration.
	
#ifdef unix
	static const char *_pidfile;
#endif
	static int _no_daemon;
	static const char *_basedir;
	static std::string _datadir;
	static int _received_hup_signal;
	
#ifdef FEATURE_GRACEFUL_TERMINATION
	static int _g_terminate;
#endif
	
#if defined(FEATURE_PTHREAD) || defined(_WIN32)
#define MUTEX_LOCKS_AVAILABLE
	
	static sp_mutex_t _log_mutex;
	static sp_mutex_t _log_init_mutex;
	static sp_mutex_t _connection_reuse_mutex;
	
#ifndef HAVE_GMTIME_R
	static sp_mutex_t _gmtime_mutex;
#endif /* ndef HAVE_GMTIME_R */
	
#ifndef HAVE_LOCALTIME_R
	static sp_mutex_t _localtime_mutex;
#endif /* ndef HAVE_GMTIME_R */
	
#if !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R)
	static sp_mutex_t _resolver_mutex;
#endif /* !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R) */
	
#ifndef HAVE_RANDOM
	static sp_mutex_t _rand_mutex;
#endif /* ndef HAVE_RANDOM */
	
#endif /* FEATURE_PTHREAD */
	
	static int _Argc;
	static const char **_Argv;
	
	#ifdef FEATURE_TOGGLE
	/* Seeks proxy's toggle state */
	static int _global_toggle_state;
	#endif /* def FEATURE_TOGGLE */
	
	/*-- functions. --*/
      public:
	
	/* mutexes. */
	static void mutex_lock(sp_mutex_t *mutex);
	static void mutex_unlock(sp_mutex_t *mutex);
	static void mutex_init(sp_mutex_t *mutex);
	
	static void initialize_mutexes();

	/* main stuff. */
#if !defined(_WIN32)
	static void sig_handler(int the_signal);
#endif
#if !defined(_WIN32) || defined(_WIN_CONSOLE)
	static void usage(const char *myname);
#endif
#ifdef _WIN32
	static void w32_service_listen_loop(void *p);
#endif
	static void listen_loop();
	static sp_socket bind_port_helper(proxy_configuration *config);
	static void chat(client_state *csp);
	static int client_protocol_is_unsupported(const client_state *csp, char *req);
	static sp_err get_request_destination_elsewhere(client_state *csp, std::list<const char*> *headers);
	static sp_err get_server_headers(client_state *csp);
	static const char* crunch_reason(const http_response *rsp);
	static void send_crunch_response(const client_state *csp, http_response *rsp);
	static int crunch_response_triggered(client_state *csp, const cruncher crunchers[]);
	static void build_request_line(client_state *csp, const forward_spec *fwd,
				       char **request_line);
	static sp_err change_request_destination(client_state *csp);

	static int server_response_is_complete(client_state *csp, 
					       unsigned long long content_length);
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
	static void wait_for_alive_connections();
	static void save_connection_destination(sp_socket sfd, const http_request *http,
						const forward_spec *fwd,
						reusable_connection *server_connection);
#endif
	static void mark_server_socket_tainted(client_state *csp);
	static char* get_request_line(client_state *csp);
	static sp_err receive_client_request(client_state *csp);
	static sp_err parse_client_request(client_state *csp);
	static void serve(client_state *csp);

#if defined(unix)
	static void write_pid_file(void);
#endif
	
	static char* make_path(const char *dir, const char *file);
	
      private:
	static const cruncher _crunchers_all[];
	static const cruncher _crunchers_light[];
     
      public:
	static bool _run_proxy;
	static pthread_t *_httpserv_thread; // running HTTP server plugin thread, if any.
     };
     
} /* end of namespace. */

#endif
