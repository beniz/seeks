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

#include "seeks_proxy.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "mem_utils.h"
#include "mutexes.h"
#include "errlog.h"
#include "miscutil.h"
#include "gateway.h"
#include "filters.h"
#include "parsers.h"
#include "spsockets.h"
#include "urlmatch.h"
#include "cgi.h"
#include "loaders.h"
#include "plugin_manager.h"
#include "interceptor_plugin.h"
#include "action_plugin.h"
#include "filter_plugin.h"
#include "proxy_configuration.h"
#include "sweeper.h"
#include "iso639.h"

namespace sp
{
  /* initialize all static (global) variables... */
  const char* seeks_proxy::_basedir = NULL;
  std::string seeks_proxy::_datadir = "";
  int seeks_proxy::_received_hup_signal = 0;
  int seeks_proxy::_no_daemon = 1;  // To be turned back off after debugging phase...
#ifdef unix
  const char* seeks_proxy::_pidfile = NULL;
#endif
  client_state seeks_proxy::_clients = client_state();
  std::vector<sweepable*> seeks_proxy::_memory_dust = std::vector<sweepable*>();

#if defined(FEATURE_PTHREAD) || defined(_WIN32)
#ifdef MUTEX_LOCKS_AVAILABLE
  //sp_mutex_t seeks_proxy::_log_init_mutex;
  sp_mutex_t seeks_proxy::_connection_reuse_mutex;
#ifndef HAVE_GMTIME_R
  sp_mutex_t seeks_proxy::_gmtime_mutex;
#endif
#endif
#ifndef HAVE_LOCALTIME_R
  sp_mutex_t seeks_proxy::_localtime_mutex;
#endif /* ndef HAVE_GMTIME_R */

#if !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R)
  sp_mutex_t seeks_proxy::_resolver_mutex;
#endif /* !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R) */

#ifndef HAVE_RANDOM
  sp_mutex_t seeks_proxy::_rand_mutex;
#endif /* ndef HAVE_RANDOM */
#endif

#if defined(PROTOBUF) && defined(TC)
  user_db* seeks_proxy::_user_db = NULL;
#endif

#ifdef FEATURE_STATISTICS
  int seeks_proxy::_urls_read = 0;
  int seeks_proxy::_urls_rejected = 0;
#endif

  std::string seeks_proxy::_configfile = "";
  proxy_configuration* seeks_proxy::_config = NULL;

  std::string seeks_proxy::_lshconfigfile = "";
  lsh_configuration* seeks_proxy::_lsh_config = NULL;

  int seeks_proxy::_Argc = 0;
  const char** seeks_proxy::_Argv = NULL;

  bool seeks_proxy::_run_proxy = true;
  pthread_t* seeks_proxy::_httpserv_thread = NULL;

#ifdef FEATURE_TOGGLE
  /* Seeks proxy is enabled by default. */
  int seeks_proxy::_global_toggle_state = 1;
#endif /* def FEATURE_TOGGLE */

  /* HTTP snipplets. */
  static const char CSUCCEED[] =
    "HTTP/1.0 200 Connection established\r\n"
    "Proxy-Agent: Seeks proxy/" VERSION "\r\n\r\n";

  static const char CHEADER[] =
    "HTTP/1.0 400 Invalid header received from client\r\n"
    "Proxy-Agent: Seeks proxy " VERSION "\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n\r\n"
    "Invalid header received from client.\r\n";

  static const char FTP_RESPONSE[] =
    "HTTP/1.0 400 Invalid request received from client\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n\r\n"
    "Invalid request. Seeks proxy doesn't support FTP.\r\n";

  static const char GOPHER_RESPONSE[] =
    "HTTP/1.0 400 Invalid request received from client\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n\r\n"
    "Invalid request. Seeks proxy doesn't support gopher.\r\n";

  /* XXX: should be a template */
  static const char MISSING_DESTINATION_RESPONSE[] =
    "HTTP/1.0 400 Bad request received from client\r\n"
    "Proxy-Agent: Seeks proxy " VERSION "\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n\r\n"
    "Bad request. Seeks proxy was unable to extract the destination.\r\n";

  /* XXX: should be a template */
  static const char INVALID_SERVER_HEADERS_RESPONSE[] =
    "HTTP/1.0 502 Server or forwarder response invalid\r\n"
    "Proxy-Agent: Seeks proxy " VERSION "\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n\r\n"
    "Bad response. The server or forwarder response doesn't look like HTTP.\r\n";

#if 0
  /* XXX: should be a template */
  static const char NULL_BYTE_RESPONSE[] =
    "HTTP/1.0 400 Bad request received from client\r\n"
    "Proxy-Agent: Seeks proxy " VERSION "\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n\r\n"
    "Bad request. Null byte(s) before end of request.\r\n";
#endif

  /* XXX: should be a template */
  static const char MESSED_UP_REQUEST_RESPONSE[] =
    "HTTP/1.0 400 Malformed request after rewriting\r\n"
    "Proxy-Agent: Seeks proxy " VERSION "\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n\r\n"
    "Bad request. Messed up with header filters.\r\n";

  static const char TOO_MANY_CONNECTIONS_RESPONSE[] =
    "HTTP/1.0 503 Too many open connections\r\n"
    "Proxy-Agent: Seeks proxy " VERSION "\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n\r\n"
    "Maximum number of open connections reached.\r\n";

  static const char CLIENT_CONNECTION_TIMEOUT_RESPONSE[] =
    "HTTP/1.0 504 Connection timeout\r\n"
    "Proxy-Agent: Seeks proxy " VERSION "\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n\r\n"
    "The connection timed out because the client request didn't arrive in time.\r\n";

  /* Crunch function flags */
#define CF_NO_FLAGS        0
  /* Cruncher applies to forced requests as well */
#define CF_IGNORE_FORCE    1
  /* Crunched requests are counted for the block statistics */
#define CF_COUNT_AS_REJECT 2

  /* Complete list of cruncher functions */
  const cruncher seeks_proxy::_crunchers_all[] =
  {
    // cruncher( &filters::redirect_url,    CF_NO_FLAGS  ),
    cruncher( &cgi::dispatch_cgi,    CF_IGNORE_FORCE),
    cruncher( NULL,            0 )
  };

  /* Light version, used after tags are applied */
  const cruncher seeks_proxy::_crunchers_light[] =
  {
//	cruncher( &filters::block_url,       CF_COUNT_AS_REJECT ),
    //cruncher( &filters::redirect_url,    CF_NO_FLAGS ),
    cruncher( NULL,            0 )
  };

  /*********************************************************************
   *
   * Function    :  client_protocol_is_unsupported
   *
   * Description :  Checks if the client used a known unsupported
   *                protocol and deals with it by sending an error
   *                response.
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *          2  :  req = the first request line send by the client
   *
   * Returns     :  TRUE if an error response has been generated, or
   *                FALSE if the request doesn't look invalid.
   *
   *********************************************************************/
  int seeks_proxy::client_protocol_is_unsupported(const client_state *csp, char *req)
  {

    /*
     * If it's a FTP or gopher request, we don't support it.
     *
     * These checks are better than nothing, but they might
     * not work in all configurations and some clients might
     * have problems digesting the answer.
     *
     * They should, however, never cause more problems than
     * Seeks proxy's old behaviour (returning the misleading HTML
     * error message:
     *
     * "Could not resolve http://(ftp|gopher)://example.org").
     */
    if (!miscutil::strncmpic(req, "GET ftp://", 10) || !miscutil::strncmpic(req, "GET gopher://", 13))
      {
        const char *response = NULL;
        const char *protocol = NULL;

        if (!miscutil::strncmpic(req, "GET ftp://", 10))
          {
            response = FTP_RESPONSE;
            protocol = "FTP";
          }
        else
          {
            response = GOPHER_RESPONSE;
            protocol = "GOPHER";
          }

        errlog::log_error(LOG_LEVEL_ERROR,
                          "%s tried to use Seeks proxy as %s proxy: %s",
                          csp->_ip_addr_str, protocol, req);
        errlog::log_error(LOG_LEVEL_CLF,
                          "%s - - [%T] \"%s\" 400 0", csp->_ip_addr_str, req);
        freez(req);
        spsockets::write_socket(csp->_cfd, response, strlen(response));

        return TRUE;
      }

    return FALSE;
  }

  /*********************************************************************
   *
   * Function    :  get_request_destination_elsewhere
   *
   * Description :  If the client's request was redirected into
   *                Seeks proxy without the client's knowledge,
   *                the request line lacks the destination host.
   *
   *                This function tries to get it elsewhere,
   *                provided accept-intercepted-requests is enabled.
   *
   *                "Elsewhere" currently only means "Host: header",
   *                but in the future we may ask the redirecting
   *                packet filter to look the destination up.
   *
   *                If the destination stays unknown, an error
   *                response is send to the client and headers
   *                are freed so that chat() can return directly.
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *          2  :  headers = a header list
   *
   * Returns     :  SP_ERR_OK if the destination is now known, or
   *                SP_ERR_PARSE if it isn't.
   *
   *********************************************************************/
  sp_err seeks_proxy::get_request_destination_elsewhere(client_state *csp, std::list<const char*> *headers)
  {
    char *req;

    if (!(csp->_config->_feature_flags & RUNTIME_FEATURE_ACCEPT_INTERCEPTED_REQUESTS))
      {
        errlog::log_error(LOG_LEVEL_ERROR, "%s's request: \'%s\' is invalid."
                          " Seeks proxy isn't configured to accept intercepted requests.",
                          csp->_ip_addr_str, csp->_http._cmd);
        /* XXX: Use correct size */
        errlog::log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 400 0",
                          csp->_ip_addr_str, csp->_http._cmd);

        spsockets::write_socket(csp->_cfd, CHEADER, strlen(CHEADER));
        miscutil::list_remove_all(headers);

        return SP_ERR_PARSE;
      }
    else if (SP_ERR_OK == parsers::get_destination_from_headers(headers, &csp->_http))
      {
#ifndef FEATURE_EXTENDED_HOST_PATTERNS
        /* Split the domain we just got for pattern matching */
        urlmatch::init_domain_components(&csp->_http);
#endif
        return SP_ERR_OK;
      }
    else
      {
        /* We can't work without destination. Go spread the news.*/
        req = miscutil::list_to_text(headers);
        miscutil::chomp(req);
        /* XXX: Use correct size */
        errlog::log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 400 0",
                          csp->_ip_addr_str, csp->_http._cmd);
        errlog::log_error(LOG_LEVEL_ERROR,
                          "Seeks proxy was unable to get the destination for %s's request:\n%s\n%s",
                          csp->_ip_addr_str, csp->_http._cmd, req);
        freez(req);

        spsockets::write_socket(csp->_cfd, MISSING_DESTINATION_RESPONSE, strlen(MISSING_DESTINATION_RESPONSE));
        miscutil::list_remove_all(headers);

        return SP_ERR_PARSE;
      }

    /*
    * TODO: If available, use PF's ioctl DIOCNATLOOK as last resort
    * to get the destination IP address, use it as host directly
    * or do a reverse DNS lookup first.
    */
  }


  /*********************************************************************
   *
   * Function    :  get_server_headers
   *
   * Description :  Parses server headers in iob and fills them
   *                into csp->_headers so that they can later be
   *                handled by sed().
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *
   * Returns     :  SP_ERR_OK if everything went fine, or
   *                SP_ERR_PARSE if the headers were incomplete.
   *
   *********************************************************************/
  sp_err seeks_proxy::get_server_headers(client_state *csp)
  {
    int continue_hack_in_da_house = 0; //!!!
    char * header;

    while (((header = parsers::get_header(&csp->_iob)) != NULL) || continue_hack_in_da_house)
      {
        //std::cout << "header: " << header << std::endl;

        if (header == NULL)
          {
            /*
             * continue hack in da house. Ignore the ending of
             * this head and continue enlisting header lines.
             * The reason is described below.
             */
            miscutil::enlist(&csp->_headers, "");
            continue_hack_in_da_house = 0;
            continue;
          }
        else if (0 == miscutil::strncmpic(header, "HTTP/1.1 100", 12))
          {

            /*
             * It's a bodyless continue response, don't
             * stop header parsing after reaching its end.
             *
             * As a result Seeks proxy will concatenate the
             * next response's head and parse and deliver
             * the headers as if they belonged to one request.
             *
             * The client will separate them because of the
             * empty line between them.
             *
             * XXX: What we're doing here is clearly against
             * the intended purpose of the continue header,
             * and under some conditions (HTTP/1.0 client request)
             * it's a standard violation.
             *
             * Anyway, "sort of against the spec" is preferable
             * to "always getting confused by Continue responses"
             * (Seeks proxy's behaviour before this hack was added)
             */
            errlog::log_error(LOG_LEVEL_HEADER, "Continue hack in da house.");
            continue_hack_in_da_house = 1;
          }
        else if (*header == '\0')
          {
            /*
             * If the header is empty, but the Continue hack
             * isn't active, we can assume that we reached the
             * end of the buffer before we hit the end of the
             * head.
             *
             * Inform the caller an let it decide how to handle it.
             */
            return SP_ERR_PARSE;
          }

        if (SP_ERR_MEMORY == miscutil::enlist(&csp->_headers, header))
          {
            /*
             * XXX: Should we quit the request and return a
             * out of memory error page instead?
             */
            errlog::log_error(LOG_LEVEL_ERROR,
                              "Out of memory while miscutil::enlisting server headers. %s lost.",
                              header);
          }
        freez(header);
      }

    return SP_ERR_OK;
  }

  /*********************************************************************
   *
   * Function    :  crunch_reason
   *
   * Description :  Translates the crunch reason code into a string.
   *
   * Parameters  :
   *          1  :  rsp = a http_response
   *
   * Returns     :  A string with the crunch reason or an error description.
   *
   *********************************************************************/
  const char* seeks_proxy::crunch_reason(const http_response *rsp)
  {
    char * reason = NULL;
    assert(rsp != NULL);
    if (rsp == NULL)
      {
        return "Internal error while searching for crunch reason";
      }

    switch (rsp->_reason)
      {
      case RSP_REASON_UNSUPPORTED:
        reason = (char*) "Unsupported HTTP feature";
        break;
      case RSP_REASON_BLOCKED:
        reason = (char*) "Blocked";
        break;
      case RSP_REASON_UNTRUSTED:
        reason = (char*) "Untrusted";
        break;
      case RSP_REASON_REDIRECTED:
        reason = (char*) "Redirected";
        break;
      case RSP_REASON_CGI_CALL:
        reason = (char*) "CGI Call";
        break;
      case RSP_REASON_NO_SUCH_DOMAIN:
        reason = (char*) "DNS failure";
        break;
      case RSP_REASON_FORWARDING_FAILED:
        reason = (char*) "Forwarding failed";
        break;
      case RSP_REASON_CONNECT_FAILED:
        reason = (char*) "Connection failure";
        break;
      case RSP_REASON_OUT_OF_MEMORY:
        reason = (char*) "Out of memory (may mask other reasons)";
        break;
      case RSP_REASON_CONNECTION_TIMEOUT:
        reason = (char*) "Connection timeout";
        break;
      case RSP_REASON_NO_SERVER_DATA:
        reason = (char*) "No server data received";
        break;
      default:
        reason = (char*) "No reason recorded";
        break;
      }
    return reason;
  }

  /*********************************************************************
   *
   * Function    :  send_crunch_response
   *
   * Description :  Delivers already prepared response for
   *                intercepted requests, logs the interception
   *                and frees the response.
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *          1  :  rsp = Fully prepared response. Will be freed on exit.
   *
   * Returns     :  Nothing.
   *
   *********************************************************************/
  void seeks_proxy::send_crunch_response(const client_state *csp, http_response *rsp)
  {
    const http_request *http = &csp->_http;
    char status_code[4];

    assert(rsp != NULL);
    assert(rsp->_head != NULL);

    if (rsp == NULL)
      {
        /*
         * Not supposed to happen. If it does
         * anyway, treat it as an unknown error.
         */
        cgi::cgi_error_unknown(csp, rsp, RSP_REASON_INTERNAL_ERROR);
        /* return code doesn't matter */
      }

    if (rsp == NULL)
      {
        /* If rsp is still NULL, we have serious internal problems. */
        errlog::log_error(LOG_LEVEL_FATAL,
                          "NULL response in send_crunch_response and cgi::cgi_error_unknown failed as well.");
      }

    /*
     * Extract the status code from the actual head
     * that was send to the client. It is the only
     * way to get it right for all requests, including
     * the fixed ones for out-of-memory problems.
     *
     * A head starts like this: 'HTTP/1.1 200...'
     *                           0123456789|11
     *                                     10
     */
    status_code[0] = rsp->_head[9];
    status_code[1] = rsp->_head[10];
    status_code[2] = rsp->_head[11];
    status_code[3] = '\0';

    // debug
    /* std::cout << "Crunched response head:\n";
     std::cout << rsp->_head;
     std::cout << "\nBody:\n";
     std::cout << rsp->_body << std::endl; */
    // debug

    /* Write the answer to the client */
    if (spsockets::write_socket(csp->_cfd, rsp->_head, rsp->_head_length)
        || spsockets::write_socket(csp->_cfd, rsp->_body, rsp->_content_length))
      {
        /* There is nothing we can do about it. */
        errlog::log_error(LOG_LEVEL_ERROR, "write to: %s failed: %E", csp->_http._host);
      }

    /* Log that the request was crunched and why. */
    errlog::log_error(LOG_LEVEL_CRUNCH, "%s: %s", seeks_proxy::crunch_reason(rsp), http->_url);
    errlog::log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" %s %u",
                      csp->_ip_addr_str, http->_ocmd, status_code, rsp->_content_length);

    /* Clean up and return */
    if (cgi::cgi_error_memory() != rsp)
      {
        rsp->reset();
      }

    return;
  }

  /*********************************************************************
   *
   * Function    :  crunch_response_triggered
   *
   * Description :  Checks if the request has to be crunched,
   *                and delivers the crunch response if necessary.
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *          2  :  crunchers = list of cruncher functions to run
   *
   * Returns     :  TRUE if the request was answered with a crunch response
   *                FALSE otherwise.
   *
   *********************************************************************/
  int seeks_proxy::crunch_response_triggered(client_state *csp, const cruncher crunchers[])
  {
    http_response *rsp = NULL;
    const cruncher *c;

    /*
     * If CGI request crunching is disabled,
     * check the CGI dispatcher out of order to
     * prevent unintentional blocks or redirects.
     */
    if (!(csp->_config->_feature_flags & RUNTIME_FEATURE_CGI_CRUNCHING)
        && (NULL != (rsp = cgi::dispatch_cgi(csp))))
      {
        /* Deliver, log and free the interception response. */
        seeks_proxy::send_crunch_response(csp, rsp);
        return TRUE;
      }

    /* std::cout << "looking for a cruncher\n";
     std::cout << "csp flags: " << csp->_flags << std::endl; */

    size_t count = 0;
    for (c = crunchers; c->_cruncher != NULL; c++)
      {
        /*
         * Check the cruncher if either Seeks proxy is toggled
         * on and the request isn't forced, or if the cruncher
         * applies to forced requests as well.
         */
        if (((csp->_flags & CSP_FLAG_TOGGLED_ON) &&
             !(csp->_flags & CSP_FLAG_FORCED)) ||
            (c->_flags & CF_IGNORE_FORCE))
          {
            rsp = c->_cruncher(csp);
            if (NULL != rsp)
              {
                //std::cout << "found cruncher: " << count << std::endl;

                /* Deliver, log and free the interception response. */
                seeks_proxy::send_crunch_response(csp, rsp);
#ifdef FEATURE_STATISTICS
                if (c->_flags & CF_COUNT_AS_REJECT)
                  {
                    csp->_flags |= CSP_FLAG_REJECTED;
                  }
#endif /* def FEATURE_STATISTICS */
                return TRUE;
              }
          }
        count++;
      }
    return FALSE;
  }


  /*********************************************************************
   *
   * Function    :  build_request_line
   *
   * Description :  Builds the HTTP request line.
   *
   *                If a HTTP forwarder is used it expects the whole URL,
   *                web servers only get the path.
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *          2  :  fwd = The forwarding spec used for the request
   *                XXX: Should use http->_fwd instead.
   *          3  :  request_line = The old request line which will be replaced.
   *
   * Returns     :  Nothing. Terminates in case of memory problems.
   *
   *********************************************************************/
  void seeks_proxy::build_request_line(client_state *csp, const forward_spec *fwd, char **request_line)
  {
    http_request *http = &csp->_http;
    assert(http->_ssl == 0);

    /*
     * Downgrade http version from 1.1 to 1.0
     * if +downgrade action applies.
     */
    /* if ( (csp->_action._flags & ACTION_DOWNGRADE)
         && (!miscutil::strcmpic(http->_ver, "HTTP/1.1")))
      {
         freez(http->_ver);
         http->_ver = strdup("HTTP/1.0");
         if (http->_ver == NULL)
           {
    	  errlog::log_error(LOG_LEVEL_FATAL, "Out of memory downgrading HTTP version");
           }
      } */

    /*
     * Rebuild the request line.
     */
    freez(*request_line);
    *request_line = strdup(http->_gpc);
    miscutil::string_append(request_line, " ");

    if (fwd && fwd->_forward_host)
      {
        miscutil::string_append(request_line, http->_url);
      }
    else
      {
        miscutil::string_append(request_line, http->_path);
      }

    miscutil::string_append(request_line, " ");
    miscutil::string_append(request_line, http->_ver);

    if (*request_line == NULL)
      {
        errlog::log_error(LOG_LEVEL_FATAL, "Out of memory writing HTTP command");
      }
    errlog::log_error(LOG_LEVEL_HEADER, "New HTTP Request-Line: %s", *request_line);
  }


  /*********************************************************************
   *
   * Function    :  change_request_destination
   *
   * Description :  Parse a (rewritten) request line and regenerate
   *                the http request data.
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *
   * Returns     :  Forwards the parse_http_request() return code.
   *                Terminates in case of memory problems.
   *
   *********************************************************************/
  sp_err seeks_proxy::change_request_destination(client_state *csp)
  {
    http_request *http = &csp->_http;
    sp_err err;

    errlog::log_error(LOG_LEVEL_INFO, "Rewrite detected: %s", (*csp->_headers.begin()));
    err = urlmatch::parse_http_request((*csp->_headers.begin()), http);
    if (SP_ERR_OK != err)
      {
        errlog::log_error(LOG_LEVEL_ERROR, "Couldn't parse rewritten request: %s.",
                          errlog::sp_err_to_string(err));
      }
    else
      {
        /* XXX: ocmd is a misleading name */
        http->_ocmd = strdup(http->_cmd);
        if (http->_ocmd == NULL)
          {
            errlog::log_error(LOG_LEVEL_FATAL,
                              "Out of memory copying rewritten HTTP request line");
          }
      }

    return err;
  }


//#ifdef FEATURE_CONNECTION_KEEP_ALIVE
  /*********************************************************************
   *
   * Function    :  server_response_is_complete
   *
   * Description :  Determines whether we should stop reading
   *                from the server socket.
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *          2  :  content_length = Length of content received so far.
   *
   * Returns     :  TRUE if the response is complete,
   *                FALSE otherwise.
   *
   *********************************************************************/
  int seeks_proxy::server_response_is_complete(client_state *csp,
      unsigned long long content_length)
  {
    int content_length_known = !!(csp->_flags & CSP_FLAG_CONTENT_LENGTH_SET);

    if (!miscutil::strcmpic(csp->_http._gpc, "HEAD"))
      {
        /*
         * "HEAD" implies no body, we are thus expecting
         * no content. XXX: incomplete "list" of methods?
         */
        csp->_expected_content_length = 0;
        content_length_known = TRUE;
      }

    if (csp->_http._status == 304)
      {
        /*
         * Expect no body. XXX: incomplete "list" of status codes?
         */
        csp->_expected_content_length = 0;
        content_length_known = TRUE;
      }

    return (content_length_known && ((0 == csp->_expected_content_length)
                                     || (csp->_expected_content_length <= content_length)));
  }

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
  /*********************************************************************
   *
   * Function    :  wait_for_alive_connections
   *
   * Description :  Waits for alive connections to timeout.
   *
   * Parameters  :  N/A
   *
   * Returns     :  N/A
   *
   *********************************************************************/
  void seeks_proxy::wait_for_alive_connections(void)
  {
    int connections_alive = gateway::close_unusable_connections();
    while (0 < connections_alive)
      {
        errlog::log_error(LOG_LEVEL_CONNECT,
                          "Waiting for %d connections to timeout.",
                          connections_alive);
        sleep(60);
        connections_alive = gateway::close_unusable_connections();
      }
    errlog::log_error(LOG_LEVEL_CONNECT, "No connections to wait for left.");
  }


  /*********************************************************************
   *
   * Function    :  save_connection_destination
   *
   * Description :  Remembers a connection for reuse later on.
   *
   * Parameters  :
   *          1  :  sfd  = Open socket to remember.
   *          2  :  http = The destination for the connection.
   *          3  :  fwd  = The forwarder settings used.
   *          3  :  server_connection  = storage.
   *
   * Returns     : void
   *
   *********************************************************************/
  void seeks_proxy::save_connection_destination(sp_socket sfd,
      const http_request *http,
      const forward_spec *fwd,
      reusable_connection *server_connection)
  {
    assert(sfd != SP_INVALID_SOCKET);
    assert(NULL != http->_host);

    server_connection->_sfd = sfd;
    server_connection->_host = strdup(http->_host);
    if (NULL == server_connection->_host)
      {
        errlog::log_error(LOG_LEVEL_FATAL, "Out of memory saving socket.");
      }
    server_connection->_port = http->_port;

    assert(NULL != fwd);
    assert(server_connection->_gateway_host == NULL);
    assert(server_connection->_gateway_port == 0);
    assert(server_connection->_forwarder_type == 0);
    assert(server_connection->_forward_host == NULL);
    assert(server_connection->_forward_port == 0);

    server_connection->_forwarder_type = fwd->_type;
    if (NULL != fwd->_gateway_host)
      {
        server_connection->_gateway_host = strdup(fwd->_gateway_host);
        if (NULL == server_connection->_gateway_host)
          {
            errlog::log_error(LOG_LEVEL_FATAL, "Out of memory saving gateway_host.");
          }
      }
    else
      {
        server_connection->_gateway_host = NULL;
      }

    server_connection->_gateway_port = fwd->_gateway_port;

    if (NULL != fwd->_forward_host)
      {
        server_connection->_forward_host = strdup(fwd->_forward_host);
        if (NULL == server_connection->_forward_host)
          {
            errlog::log_error(LOG_LEVEL_FATAL, "Out of memory saving forward_host.");
          }
      }
    else
      {
        server_connection->_forward_host = NULL;
      }

    server_connection->_forward_port = fwd->_forward_port;
  }

#endif /* FEATURE_CONNECTION_KEEP_ALIVE */


  /*********************************************************************
   *
   * Function    :  mark_server_socket_tainted
   *
   * Description :  Makes sure we don't reuse a server socket
   *                (if we didn't read everything the server sent
   *                us reusing the socket would lead to garbage).
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *
   * Returns     :  void.
   *
   *********************************************************************/
  void seeks_proxy::mark_server_socket_tainted(client_state *csp)
  {
    if ((csp->_flags & CSP_FLAG_SERVER_CONNECTION_KEEP_ALIVE))
      {
        errlog::log_error(LOG_LEVEL_CONNECT,
                          "Marking the server socket %d tainted.", csp->_sfd);
        csp->_flags |= CSP_FLAG_SERVER_SOCKET_TAINTED;
      }
  }

  /*********************************************************************
   *
   * Function    :  get_request_line
   *
   * Description : Read the client request line.
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *
   * Returns     :  Pointer to request line or NULL in case of errors.
   *
   *********************************************************************/
  char* seeks_proxy::get_request_line(client_state *csp)
  {
    char buf[BUFFER_SIZE];
    char *request_line = NULL;
    int len;

    memset(buf, 0, sizeof(buf));
    do
      {
        if (!spsockets::data_is_available(csp->_cfd, csp->_config->_socket_timeout))
          {
            errlog::log_error(LOG_LEVEL_ERROR,
                              "Stopped waiting for the request line.");
            spsockets::write_socket(csp->_cfd, CLIENT_CONNECTION_TIMEOUT_RESPONSE,
                                    strlen(CLIENT_CONNECTION_TIMEOUT_RESPONSE));
            return NULL;
          }
        len = spsockets::read_socket(csp->_cfd, buf, sizeof(buf) - 1);

        if (len <= 0) return NULL;
        /*
         * If there is no memory left for buffering the
         * request, there is nothing we can do but hang up
         */
        if (parsers::add_to_iob(csp, buf, len))
          {
            return NULL;
          }
        request_line = parsers::get_header(&csp->_iob);
      }
    while ((NULL != request_line) && ('\0' == *request_line));

    return request_line;
  }

  /*********************************************************************
   *
   * Function    :  receive_client_request
   *
   * Description : Read the client's request (more precisely the
   *               client headers) and answer it if necessary.
   *
   *               Note that since we're not using select() we could get
   *               blocked here if a client connected, then didn't say
   *               anything!
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *
   * Returns     :  SP_ERR_OK, SP_ERR_PARSE or SP_ERR_MEMORY
   *
   *********************************************************************/
  sp_err seeks_proxy::receive_client_request(client_state *csp)
  {
    char buf[BUFFER_SIZE];
    char *p;
    char *req = NULL;
    http_request *http;
    int len;
    sp_err err;

    /* Temporary copy of the client's headers before they get miscutil::enlisted in csp->_headers */
    std::list<const char*> header_list;
    std::list<const char*> *headers = &header_list;

    http = &csp->_http;
    memset(buf, 0, sizeof(buf));

    req = seeks_proxy::get_request_line(csp);
    if (req == NULL)
      {
        return SP_ERR_PARSE;
      }

    assert(*req != '\0');

    if (seeks_proxy::client_protocol_is_unsupported(csp, req))
      {
        return SP_ERR_PARSE;
      }

#ifdef FEATURE_FORCE_LOAD
    /*
     * If this request contains the FORCE_PREFIX and blocks
     * aren't enforced, get rid of it and set the force flag.
     */
    if (strstr(req, FORCE_PREFIX))
      {
        if (csp->_config->_feature_flags & RUNTIME_FEATURE_ENFORCE_BLOCKS)
          {
            errlog::log_error(LOG_LEVEL_FORCE,
                              "Ignored force prefix in request: \"%s\".", req);
          }
        else
          {
            parsers::strclean(req, FORCE_PREFIX);
            errlog::log_error(LOG_LEVEL_FORCE, "Enforcing request: \"%s\".", req);
            csp->_flags |= CSP_FLAG_FORCED;
          }
      }
#endif /* def FEATURE_FORCE_LOAD */

    err = urlmatch::parse_http_request(req, http);
    freez(req);

    if (SP_ERR_OK != err)
      {
        spsockets::write_socket(csp->_cfd, CHEADER, strlen(CHEADER));
        /* XXX: Use correct size */
        errlog::log_error(LOG_LEVEL_CLF, "%s - - [%T] \"Invalid request\" 400 0", csp->_ip_addr_str);
        errlog::log_error(LOG_LEVEL_ERROR,
                          "Couldn't parse request line received from %s: %s",
                          csp->_ip_addr_str, errlog::sp_err_to_string(err));
        //delete http;
        //http->clear();
        return SP_ERR_PARSE;
      }

    /* grab the rest of the client's headers */
    for (;;)
      {
        p = parsers::get_header(&csp->_iob);
        if (p == NULL)
          {
            /* There are no additional headers to read. */
            break;
          }
        if (*p == '\0')
          {
            /*
             * We didn't receive a complete header
             * line yet, get the rest of it.
             */
            if (!spsockets::data_is_available(csp->_cfd, csp->_config->_socket_timeout))
              {
                errlog::log_error(LOG_LEVEL_ERROR,
                                  "Stopped grabbing the client headers.");
                return SP_ERR_PARSE;
              }
            len = spsockets::read_socket(csp->_cfd, buf, sizeof(buf) - 1);
            if (len <= 0)
              {
                errlog::log_error(LOG_LEVEL_ERROR, "read from client failed: %E");
                return SP_ERR_PARSE;
              }
            if (parsers::add_to_iob(csp, buf, len))
              {
                /*
                * If there is no memory left for buffering the
                * request, there is nothing we can do but hang up
                */
                return SP_ERR_MEMORY;
              }
          }
        else
          {
            /*
             * We were able to read a complete
             * header and can finaly miscutil::enlist it.
             */
            miscutil::enlist(headers, p);
            freez(p);
          }
      }

    if (http->_host == NULL)
      {
        /*
         * If we still don't know the request destination,
         * the request is invalid or the client uses
         * Seeks proxy without its knowledge.
         */
        if (SP_ERR_OK != seeks_proxy::get_request_destination_elsewhere(csp, headers))
          {
            /*
             * Our attempts to get the request destination
             * elsewhere failed or Seeks proxy is configured
             * to only accept proxy requests.
             *
             * An error response has already been send
             * and we're done here.
             */
            return SP_ERR_PARSE;
          }
      }

    /*
     * Determine the actions for this URL
     */
#ifdef FEATURE_TOGGLE
    if (!(csp->_flags & CSP_FLAG_TOGGLED_ON))
      {
        /* Most compatible set of actions (i.e. none) */
        //csp->_action = current_action_spec();
      }
    else
#endif /* ndef FEATURE_TOGGLE */
      {
        plugin_manager::get_url_plugins(csp, http);
      }

    /*
     * Save a copy of the original request for logging
     */
    http->_ocmd = strdup(http->_cmd);
    if (http->_ocmd == NULL)
      {
        errlog::log_error(LOG_LEVEL_FATAL,
                          "Out of memory copying HTTP request line");
      }
    miscutil::enlist(&csp->_headers, http->_cmd);

    /* Append the previously read headers */
    miscutil::list_append_list_unique(&csp->_headers, headers);

    return SP_ERR_OK;
  }

  /*********************************************************************
   *
   * Function    : parse_client_request
   *
   * Description : Parses the client's request and decides what to do
   *               with it.
   *
   *               Note that since we're not using select() we could get
   *               blocked here if a client connected, then didn't say
   *               anything!
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *
   * Returns     :  SP_ERR_OK or SP_ERR_PARSE
   *
   *********************************************************************/
  sp_err seeks_proxy::parse_client_request(client_state *csp)
  {
    http_request *http = &csp->_http;

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
    if ((csp->_config->_feature_flags & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE)
        && (!miscutil::strcmpic(csp->_http._ver, "HTTP/1.1"))
        && (csp->_http._ssl == 0))
      {
        /* Assume persistence until further notice */
        csp->_flags |= CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
      }
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

    //seeks: deprecated, requires a set of filter plugins instead.
    sp_err err = parsers::sed(csp, FILTER_CLIENT_HEADERS); // fixes and set client headers.
    if (SP_ERR_OK != err)
      {
        /* XXX: Should be handled in sed(). */
        assert(err == SP_ERR_PARSE);
        errlog::log_error(LOG_LEVEL_FATAL, "Failed to parse client headers.");
      }
    csp->_flags |= CSP_FLAG_CLIENT_HEADER_PARSING_DONE;

    /* Check request line for rewrites. */
    if ((csp->_headers.empty())
        || (strcmp(http->_cmd, (*csp->_headers.begin())) &&
            (SP_ERR_OK != seeks_proxy::change_request_destination(csp))))
      {
        /*
         * A header filter broke the request line - bail out.
         */
        spsockets::write_socket(csp->_cfd, MESSED_UP_REQUEST_RESPONSE, strlen(MESSED_UP_REQUEST_RESPONSE));
        /* XXX: Use correct size */
        errlog::log_error(LOG_LEVEL_CLF,
                          "%s - - [%T] \"Invalid request generated\" 500 0", csp->_ip_addr_str);
        errlog::log_error(LOG_LEVEL_ERROR,
                          "Invalid request line after applying header filters.");
        return SP_ERR_PARSE;
      }

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
    if ((csp->_flags & CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE))
      {
        if (csp->_iob._cur[0] != '\0')
          {
            csp->_flags |= CSP_FLAG_SERVER_SOCKET_TAINTED;
            if (miscutil::strcmpic(csp->_http._gpc, "GET")
                && miscutil::strcmpic(csp->_http._gpc, "HEAD")
                && miscutil::strcmpic(csp->_http._gpc, "TRACE")
                && miscutil::strcmpic(csp->_http._gpc, "OPTIONS")
                && miscutil::strcmpic(csp->_http._gpc, "DELETE"))
              {
                /* XXX: this is an incomplete hack */
                csp->_flags &= ~CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ;
                errlog::log_error(LOG_LEVEL_CONNECT,
                                  "There might be a request body. The connection will not be kept alive.");
              }
            else
              {
                /* XXX: and so is this */
                csp->_flags |= CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ;
                errlog::log_error(LOG_LEVEL_CONNECT,
                                  "Possible pipeline attempt detected. The connection will not "
                                  "be kept alive and we will only serve the first request.");
                /* Nuke the pipelined requests from orbit, just to be sure. */
                csp->_iob._buf[0] = '\0';
                csp->_iob._eod = csp->_iob._cur = csp->_iob._buf;
              }
          }
        else
          {
            csp->_flags |= CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ;
            errlog::log_error(LOG_LEVEL_CONNECT, "Complete client request received.");
          }

      }

#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

    return SP_ERR_OK;
  }


  /*********************************************************************
   *
   * Function    :  chat
   *
   * Description :  Once a connection to the client has been accepted,
   *                this function is called (via serve()) to handle the
   *                main business of the communication.  When this
   *                function returns, the caller must close the client
   *                socket handle.
   *
   *                FIXME: chat is nearly thousand lines long.
   *                Ridiculous.
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *
   * Returns     :  Nothing.
   *
   *********************************************************************/
  void seeks_proxy::chat(client_state *csp)
  {
    char buf[BUFFER_SIZE];
    char *hdr;
    char *p;
    fd_set rfds;
    int n;
    sp_socket maxfd;
    int server_body;
    int ms_iis5_hack = 0;
    unsigned long long byte_count = 0;
    int forwarded_connect_retries = 0;
    int max_forwarded_connect_retries = csp->_config->_forwarded_connect_retries;
    const forward_spec *fwd = NULL;
    http_request *http;
    long len = 0; /* for buffer sizes (and negative error codes) */

    /* Function that does the content filtering for the current request */
    bool content_filter = false;

    /* Skeleton for HTTP response, if we should intercept the request */
    http_response *rsp;
    timeval timeout;

    memset(buf, 0, sizeof(buf));

    http = &csp->_http;

    if (seeks_proxy::receive_client_request(csp) != SP_ERR_OK)
      {
        return;
      }

    if (seeks_proxy::parse_client_request(csp) != SP_ERR_OK)
      {
        return;
      }

    /* decide how to route the HTTP request (i.e. to another proxy),
       sets default forward settings instead, mandatory. */
    fwd = filters::forward_url(csp, http);
    if (NULL == fwd)
      {
        errlog::log_error(LOG_LEVEL_FATAL, "gateway spec is NULL!?!?  This can't happen!");
        /* Never get here - LOG_LEVEL_FATAL causes program exit */
        return;
      }

    /*
    * build the http request to send to the server
    * we have to do one of the following:
    *
    * create = use the original HTTP request to create a new
    *          HTTP request that has either the path component
    *          without the http://domainspec (w/path) or the
    *          full original URL (w/url)
    *          Note that the path and/or the HTTP version may
    *          have been altered by now.
    *
    * connect = Open a socket to the host:port of the server
    *           and short-circuit server and client socket.
    *
    * pass =  Pass the request unchanged if forwarding a CONNECT
    *         request to a parent proxy. Note that we'll be sending
    *         the CFAIL message ourselves if connecting to the parent
    *         fails, but we won't send a CSUCCEED message if it works,
    *         since that would result in a double message (ours and the
    *         parent's). After sending the request to the parent, we simply
    *         tunnel.
    *
    * here's the matrix:
    *                        SSL
    *                    0        1
    *                +--------+--------+
    *                |        |        |
    *             0  | create | connect|
    *                | w/path |        |
    *  Forwarding    +--------+--------+
    *                |        |        |
    *             1  | create | pass   |
    *                | w/url  |        |
    *                +--------+--------+
    *
    */

    if (http->_ssl == 0)
      {
        free_const((*csp->_headers.begin()));  // beware !!!
        csp->_headers.erase(csp->_headers.begin());
        char *nheader = NULL;
        seeks_proxy::build_request_line(csp, fwd, &nheader);
        csp->_headers.push_front(nheader);
      }

    // calling interceptor plugins. (no header/content modification).
    rsp = NULL;
    std::list<interceptor_plugin*>::const_iterator lit
    = csp->_interceptor_plugins.begin();
    while (lit!=csp->_interceptor_plugins.end())
      {
        /*
         * BEWARE: we keep the first non-NULL interceptor plugin response!
         * This is because there can be many purposes for interception,
         * but room for one response only.
         */
        http_response *crsp = (*lit)->plugin_response(csp);
        if (crsp)
          {
            if (rsp)
              delete rsp;
            rsp = crsp;
            break;
          }
        ++lit;
      }
    if (rsp)
      seeks_proxy::send_crunch_response(csp,rsp);

    // seeks: dispatch_cgi and redirection only.
    /*
     * We have a request. Check if one of the crunchers wants it.
     * That is, block url, redirect url, known cgi call (e.g. local config page),
     * so-called 'direct response' which truely is a check of max forwards.
     */
    if (seeks_proxy::crunch_response_triggered(csp, seeks_proxy::_crunchers_all))
      {
        /*
         * Yes. The client got the crunch response
         * and we are done here after cleaning up.
         */
        miscutil::list_remove_all(&csp->_headers);
        return;
      }

    // Plugins come into action here.
    // Typically, the websearch query interceptor.
    errlog::log_error(LOG_LEVEL_GPC, "%s%s", http->_hostport, http->_path);

    if (fwd && fwd->_forward_host)
      {
        errlog::log_error(LOG_LEVEL_CONNECT, "via [%s]:%d to: %s",
                          fwd->_forward_host, fwd->_forward_port, http->_hostport);
      }
    else
      {
        errlog::log_error(LOG_LEVEL_CONNECT, "to %s", http->_hostport);
      }

    /* here we connect to the server, gateway, or the forwarder */
# ifdef FEATURE_CONNECTION_KEEP_ALIVE
    if ((csp->_sfd != SP_INVALID_SOCKET)
        && spsockets::socket_is_still_usable(csp->_sfd)
        && gateway::connection_destination_matches(&csp->_server_connection, http, fwd))
      {
        errlog::log_error(LOG_LEVEL_CONNECT,
                          "Reusing server socket %u. Opened for %s.",
                          csp->_sfd, csp->_server_connection._host);
      }
    else
      {
        if (csp->_sfd != SP_INVALID_SOCKET)
          {
            errlog::log_error(LOG_LEVEL_CONNECT,
                              "Closing server socket %u. Opened for %s.",
                              csp->_sfd, csp->_server_connection._host);
            spsockets::close_socket(csp->_sfd);
            gateway::mark_connection_closed(&csp->_server_connection);
          }
# endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

        while ((csp->_sfd = gateway::forwarded_connect(fwd, http, csp))  // connecting the webserver.
               && (errno == EINVAL)
               && (forwarded_connect_retries++ < max_forwarded_connect_retries))
          {
            errlog::log_error(LOG_LEVEL_ERROR,
                              "failed request #%u to connect to %s. Trying again.",
                              forwarded_connect_retries, http->_hostport);
          }

        /* deals with an invalid socket. */
        if (csp->_sfd == SP_INVALID_SOCKET)
          {
            if (fwd->_type != SOCKS_NONE)
              {
                /* Socks error. */
                rsp = cgi::error_response(csp, "forwarding-failed");
              }
            else if (errno == EINVAL)
              {
                rsp = cgi::error_response(csp, "no-such-domain");
              }
            else
              {
                rsp = cgi::error_response(csp, "connect-failed");
                errlog::log_error(LOG_LEVEL_CONNECT, "connect to: %s failed: %E",
                                  http->_hostport);
              }
            /* Write the answer to the client */
            if (rsp != NULL)
              {
                seeks_proxy::send_crunch_response(csp, rsp);
              }
            return;
          }

# ifdef FEATURE_CONNECTION_KEEP_ALIVE
        seeks_proxy::save_connection_destination(csp->_sfd, http, fwd, &csp->_server_connection);
        csp->_server_connection._keep_alive_timeout = (unsigned)csp->_config->_keep_alive_timeout;
      }
# endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

    hdr = miscutil::list_to_text(&csp->_headers);
    if (hdr == NULL)
      {
        /* FIXME Should handle error properly */
        errlog::log_error(LOG_LEVEL_FATAL, "Out of memory parsing client header");
      }
    miscutil::list_remove_all(&csp->_headers);

    if (fwd->_forward_host || (http->_ssl == 0))
      {
        /*
         * Write the client's (modified) header to the server
         * (along with anything else that may be in the buffer)
         */
        if (spsockets::write_socket(csp->_sfd, hdr, strlen(hdr))
            || (parsers::flush_socket(csp->_sfd, &csp->_iob) <  0))
          {
            errlog::log_error(LOG_LEVEL_CONNECT,
                              "write header to: %s failed: %E", http->_hostport);

            rsp = cgi::error_response(csp, "connect-failed");
            if (rsp)
              {
                seeks_proxy::send_crunch_response(csp, rsp);
              }
            freez(hdr);
            return;
          }
      }
    else
      {
        /*
         * We're running an SSL tunnel and we're not forwarding,
         * so just send the "connect succeeded" message to the
         * client, flush the rest, and get out of the way.
         */
        if (spsockets::write_socket(csp->_cfd, CSUCCEED, strlen(CSUCCEED)))
          {
            freez(hdr);
            return;
          }
        csp->_iob.reset();
      }
    errlog::log_error(LOG_LEVEL_CONNECT, "to %s successful", http->_hostport);

    csp->_server_connection._request_sent = time(NULL);

    /* we're finished with the client's header */
    freez(hdr);

    maxfd = (csp->_cfd > csp->_sfd) ? csp->_cfd : csp->_sfd;

    /* pass data between the client and server
     * until one or the other shuts down the connection.
     */
    server_body = 0;
    for (;;)
      {
        FD_ZERO(&rfds);

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
        if ((csp->_flags & CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ))
          {
            maxfd = csp->_sfd;
          }
        else
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */
          {
            FD_SET(csp->_cfd, &rfds);
          }
        FD_SET(csp->_sfd, &rfds);   // socket.

//#ifdef FEATURE_CONNECTION_KEEP_ALIVE
        /*	     if ((csp->_flags & CSP_FLAG_CHUNKED)
        		 && !(csp->_flags & CSP_FLAG_CONTENT_LENGTH_SET)
        		 && ((csp->_iob._eod - csp->_iob._cur) >= 5)
        		 && !memcmp(csp->_iob._eod-5, "0\r\n\r\n", 5))
        	       {
        		  errlog::log_error(LOG_LEVEL_CONNECT,
        				    "Looks like we read the last chunk together with "
        				    "the server headers. We better stop reading.");
        		  byte_count = (unsigned long long)(csp->_iob._eod - csp->_iob._cur);
        		  csp->_expected_content_length = byte_count;
        		  csp->_flags |= CSP_FLAG_CONTENT_LENGTH_SET;
        	       } */
        if (server_body && seeks_proxy::server_response_is_complete(csp, byte_count))
          {
            errlog::log_error(LOG_LEVEL_CONNECT,
                              "Done reading from server. Expected content length: %llu. "
                              "Actual content length: %llu. Most recently received: %d.",
                              csp->_expected_content_length, byte_count, len);
            len = 0;
            /*
             * XXX: should not jump around,
             * chat() is complicated enough already.
             */
            goto reading_done;
          }
//#endif  /* FEATURE_CONNECTION_KEEP_ALIVE */

        timeout.tv_sec = csp->_config->_socket_timeout;
        timeout.tv_usec = 0;
        n = select((int)maxfd+1, &rfds, NULL, NULL, &timeout);  // socket monitoring, with timeout.

        if (n == 0) // no bits in time timeout.
          {
            errlog::log_error(LOG_LEVEL_ERROR,
                              "Didn't receive data in time: %s", http->_url);
            if ((byte_count == 0) && (http->_ssl == 0))
              {
                seeks_proxy::send_crunch_response(csp, cgi::error_response(csp, "connection-timeout"));
              }
            seeks_proxy::mark_server_socket_tainted(csp);
            return;
          }
        else if (n < 0)
          {
            errlog::log_error(LOG_LEVEL_ERROR, "select() failed!: %E");
            seeks_proxy::mark_server_socket_tainted(csp);
            return;
          }

        /*
         * This is the body of the browser's request,
         * just read and write it.
         *
         * XXX: Make sure the client doesn't use pipelining
         * behind Seeks proxy's back.
         */
        if (FD_ISSET(csp->_cfd, &rfds))
          {
            len = spsockets::read_socket(csp->_cfd, buf, sizeof(buf) - 1); // reading request from the client.

            if (len <= 0)
              {
                /* XXX: not sure if this is necessary. */
                seeks_proxy::mark_server_socket_tainted(csp);
                break; /* "game over, man" */
              }
            if (spsockets::write_socket(csp->_sfd, buf, (size_t)len)) // writing it to the server.
              {
                errlog::log_error(LOG_LEVEL_ERROR, "write to: %s failed: %E", http->_host);
                seeks_proxy::mark_server_socket_tainted(csp);
                return;
              }
            continue;
          }

        /*
         * The server wants to talk. It could be the header or the body.
         * If hdr' is null, then it's the header otherwise it's the body.
         */
        if (FD_ISSET(csp->_sfd, &rfds))
          {

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
            if (!spsockets::socket_is_still_usable(csp->_cfd))
              {
#ifdef _WIN32
                errlog::log_error(LOG_LEVEL_CONNECT,
                                  "The server still wants to talk, but the client may already have hung up on us.");
#else
                errlog::log_error(LOG_LEVEL_CONNECT,
                                  "The server still wants to talk, but the client hung up on us.");
                seeks_proxy::mark_server_socket_tainted(csp);
                return;
#endif /* def _WIN32 */
              }
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

            fflush(NULL);
            len = spsockets::read_socket(csp->_sfd, buf, sizeof(buf) - 1);  // read from the server.

            if (len < 0)
              {
                errlog::log_error(LOG_LEVEL_ERROR, "read from: %s failed: %E", http->_host);
                if (http->_ssl && (fwd->_forward_host == NULL))
                  {
                    /*
                     * Just hang up. We already confirmed the client's CONNECT
                     * request with status code 200 and unencrypted content is
                     * no longer welcome.
                     */
                    errlog::log_error(LOG_LEVEL_ERROR,
                                      "CONNECT already confirmed. Unable to tell the client about the problem.");
                    return;
                  }
                else if (byte_count)
                  {
                    /*
                     * Just hang up. We already transmitted the original headers
                     * and parts of the original content and therefore missed the
                     * chance to send an error message (without risking data corruption).
                     *
                     * XXX: we could retry with a fancy range request here.
                     */
                    errlog::log_error(LOG_LEVEL_ERROR, "Already forwarded the original headers. "
                                      "Unable to tell the client about the problem.");
                    seeks_proxy::mark_server_socket_tainted(csp);
                    return;
                  }

                /*
                * XXX: Consider handling the cases above the same.
                */
                seeks_proxy::mark_server_socket_tainted(csp);
                len = 0;
              }

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
            if (csp->_flags & CSP_FLAG_CHUNKED)
              {
                if ((len >= 5) && !memcmp(buf+len-5, "0\r\n\r\n", 5))
                  {
                    /* XXX: this is a temporary hack */
                    errlog::log_error(LOG_LEVEL_CONNECT,
                                      "Looks like we reached the end of the last chunk. "
                                      "We better stop reading.");
                    csp->_expected_content_length = byte_count + (unsigned long long)len;
                    csp->_flags |= CSP_FLAG_CONTENT_LENGTH_SET;
                  }
              }
//		  reading_done:
#endif  /* FEATURE_CONNECTION_KEEP_ALIVE */
reading_done:

            /*
             * Add a trailing zero to let be able to use string operations.
             * XXX: do we still need this with filter_popups gone?
             */
            buf[len] = '\0';

            /*
             * Normally, this would indicate that we've read
             * as much as the server has sent us and we can
             * close the client connection.  However, Microsoft
             * in its wisdom has released IIS/5 with a bug that
             * prevents it from sending the trailing \r\n in
             * a 302 redirect header (and possibly other headers).
             * To work around this if we've haven't parsed
             * a full header we'll append a trailing \r\n
             * and see if this now generates a valid one.
             *
             * This hack shouldn't have any impacts.  If we've
             * already transmitted the header or if this is a
             * SSL connection, then we won't bother with this
             * hack.  So we only work on partially received
             * headers.  If we append a \r\n and this still
             * doesn't generate a valid header, then we won't
             * transmit anything to the client.
             */
            if (len == 0)  // indicates end of buffer has been reached while reading.
              {
                if (server_body || http->_ssl)  // server_body seems to only depends on the KEEP_ALIVE feature.
                  {
                    /*
                     * If we have been buffering up the document,
                     * now is the time to apply content modification
                     * and send the result to the client.
                     */
                    if (content_filter)
                      {
                        //p = filters::execute_content_filter(csp, content_filter); // basically applies all applicable +filters.

                        /* std::cout << "[Debug]: executing filters...\n";
                         std::cout << "# csp filters: " << csp->_filter_plugins.size() << std::endl; */

                        // char* csp->execute_content_filter_plugins();
                        p = csp->execute_content_filter_plugins();

                        /*
                         * If the content filter fails, use the original
                         * buffer and length.
                         * (see p != NULL ? p : csp->_iob._cur below)
                         */
                        if (NULL == p)
                          {
                            csp->_content_length = (size_t)(csp->_iob._eod - csp->_iob._cur);
                          }
                        if (SP_ERR_OK != parsers::update_server_headers(csp))
                          {
                            errlog::log_error(LOG_LEVEL_FATAL,
                                              "Failed to update server headers. after filtering.");
                          }
                        hdr = miscutil::list_to_text(&csp->_headers);
                        if (hdr == NULL)
                          {
                            /* FIXME Should handle error properly */
                            errlog::log_error(LOG_LEVEL_FATAL, "Out of memory parsing server header");
                          }

                        // write headers, then content.
                        if (spsockets::write_socket(csp->_cfd, hdr, strlen(hdr))
                            || spsockets::write_socket(csp->_cfd,
                                                       ((p != NULL) ? p : csp->_iob._cur), (size_t)csp->_content_length))
                          {

                            errlog::log_error(LOG_LEVEL_ERROR, "write modified content to client failed: %E");
                            freez(hdr);
                            freez(p);
                            seeks_proxy::mark_server_socket_tainted(csp);
                            return;
                          }
                        freez(hdr);
                        freez(p);
                      }
                    break; /* "game over, man" */
                  } // end body.

                /*
                * This is NOT the body, so
                * Let's pretend the server just sent us a blank line.
                */
                snprintf(buf, sizeof(buf), "\r\n");
                len = (int)strlen(buf);

                /*
                * Now, let the normal header parsing algorithm below do its
                * job.  If it fails, we'll exit instead of continuing.
                */
                ms_iis5_hack = 1;
              }

            /*
             * If this is an SSL connection or we're in the body
             * of the server document, just write it to the client,
             * unless we need to buffer the body for later content-filtering
             */
            if (server_body || http->_ssl)
              {
                if (content_filter)
                  {
                    /*
                     * If there is no memory left for buffering the content, or the buffer limit
                     * has been reached, switch to non-filtering mode, i.e. make & write the
                     * header, flush the iob and buf, and get out of the way.
                     */
                    if (parsers::add_to_iob(csp, buf, len))
                      {
                        size_t hdrlen;
                        long flushed;
                        errlog::log_error(LOG_LEVEL_INFO,
                                          "Flushing header and buffers. Stepping back from filtering.");

                        hdr = miscutil::list_to_text(&csp->_headers);
                        if (hdr == NULL)
                          {
                            /*
                             * Memory is too tight to even generate the header.
                             * Send our static "Out-of-memory" page.
                             */
                            errlog::log_error(LOG_LEVEL_ERROR, "Out of memory while trying to flush.");
                            rsp = cgi::cgi_error_memory();
                            seeks_proxy::send_crunch_response(csp, rsp);
                            seeks_proxy::mark_server_socket_tainted(csp);
                            return;
                          }
                        hdrlen = strlen(hdr);

                        if (spsockets::write_socket(csp->_cfd, hdr, hdrlen)  // write header to the client.
                            || ((flushed = parsers::flush_socket(csp->_cfd, &csp->_iob)) < 0)
                            || (spsockets::write_socket(csp->_cfd, buf, (size_t)len))) // writing buffer directly to the client, not filtering.
                          {
                            errlog::log_error(LOG_LEVEL_CONNECT,
                                              "Flush header and buffers to client failed: %E");
                            freez(hdr);
                            seeks_proxy::mark_server_socket_tainted(csp);
                            return;
                          }

                        /*
                         * Reset the byte_count to the amount of bytes
                         * we just flushed. len will be added a few lines below,
                         * hdrlen doesn't matter for LOG_LEVEL_CLF.
                         */
                        byte_count = (unsigned long long)flushed;
                        freez(hdr);
                        content_filter = false;
                        server_body = 1;
                      }

                    //std::cout << "buffered the body\n";
                  }
                else  // no content filter.
                  {
                    if (spsockets::write_socket(csp->_cfd, buf, (size_t)len))  // write to the client.
                      {
                        errlog::log_error(LOG_LEVEL_ERROR, "write to client failed: %E");
                        seeks_proxy::mark_server_socket_tainted(csp);
                        return;
                      }
                  }
                byte_count += (unsigned long long)len;
                continue;
              }
            else
              {
                const char *header_start;
                /*
                * We're still looking for the end of the server's header.
                * Buffer up the data we just read.  If that fails, there's
                * little we can do but send our static out-of-memory page.
                */
                if (parsers::add_to_iob(csp, buf, len))
                  {
                    errlog::log_error(LOG_LEVEL_ERROR, "Out of memory while looking for end of server headers.");
                    rsp = cgi::cgi_error_memory();
                    seeks_proxy::send_crunch_response(csp, rsp);
                    seeks_proxy::mark_server_socket_tainted(csp);
                    return;
                  }
                header_start = csp->_iob._cur;

                /* Convert iob into something sed() can digest */
                if (SP_ERR_PARSE == seeks_proxy::get_server_headers(csp))
                  {
                    if (ms_iis5_hack)
                      {
                        /*
                         * Well, we tried our MS IIS/5 hack and it didn't work.
                         * The header is incomplete and there isn't anything
                         * we can do about it.
                         */
                        errlog::log_error(LOG_LEVEL_ERROR, "Invalid server headers. "
                                          "Applying the MS IIS5 hack didn't help.");
                        errlog::log_error(LOG_LEVEL_CLF,
                                          "%s - - [%T] \"%s\" 502 0", csp->_ip_addr_str, http->_cmd);
                        spsockets::write_socket(csp->_cfd, INVALID_SERVER_HEADERS_RESPONSE,
                                                strlen(INVALID_SERVER_HEADERS_RESPONSE));
                        seeks_proxy::mark_server_socket_tainted(csp);
                        return;
                      }
                    else
                      {
                        /*
                         * Since we have to wait for more from the server before
                         * we can parse the headers we just continue here.
                         */
                        long header_offset = csp->_iob._cur - header_start;
                        assert(csp->_iob._cur >= header_start);
                        byte_count += (unsigned long long)(len - header_offset);
                        errlog::log_error(LOG_LEVEL_CONNECT, "Continuing buffering headers. "
                                          "byte_count: %llu. header_offset: %d. len: %d.",
                                          byte_count, header_offset, len);
                        continue;
                      }
                  }

                //debug
                /* std::cout << "headers:\n";
                std::list<const char*>::const_iterator lit = csp->_headers.begin();
                while(lit!=csp->_headers.end())
                {
                std::cout << (*lit) << std::endl;
                ++lit;
                } */
                //debug

                /* Did we actually get anything? */
                if (csp->_headers.empty())
                  {
                    errlog::log_error(LOG_LEVEL_ERROR,
                                      "Empty server or forwarder response received on socket %d.", csp->_sfd);
                    errlog::log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 502 0", csp->_ip_addr_str, http->_cmd);
                    seeks_proxy::send_crunch_response(csp, cgi::error_response(csp, "no-server-data"));
                    //delete http;
                    //http->clear();
                    seeks_proxy::mark_server_socket_tainted(csp);
                    return;
                  }

                assert((*csp->_headers.begin()));
                assert(!http->_ssl);
                if (miscutil::strncmpic((*csp->_headers.begin()), "HTTP", 4) &&
                    miscutil::strncmpic((*csp->_headers.begin()), "ICY", 3))
                  {
                    /*
                     * It doesn't look like a HTTP (or Shoutcast) response:
                     * tell the client and log the problem.
                     */
                    if (strlen((*csp->_headers.begin())) > 30)
                      {
                        // beware...
                        char *nstr = strndup(const_cast<char*>((*csp->_headers.begin())),30);
                        nstr[30] = '\0';
                        free_const((*csp->_headers.begin()));  // beware.
                        //(*csp->_headers.begin()) = nstr;
                        csp->_headers.erase(csp->_headers.begin());
                        csp->_headers.push_front(nstr);
                      }

                    errlog::log_error(LOG_LEVEL_ERROR,
                                      "Invalid server or forwarder response. Starts with: %s",
                                      (*csp->_headers.begin()));
                    errlog::log_error(LOG_LEVEL_CLF,
                                      "%s - - [%T] \"%s\" 502 0", csp->_ip_addr_str, http->_cmd);
                    spsockets::write_socket(csp->_cfd, INVALID_SERVER_HEADERS_RESPONSE,
                                            strlen(INVALID_SERVER_HEADERS_RESPONSE));
                    //http->clear();
                    seeks_proxy::mark_server_socket_tainted(csp);
                    return;
                  }

                /*
                * We have now received the entire server header,
                * filter it and send the result to the client
                * Mandatory, clears headers.
                */
                if (SP_ERR_OK != parsers::sed(csp, FILTER_SERVER_HEADERS))
                  {
                    errlog::log_error(LOG_LEVEL_FATAL, "Failed to parse server headers.");
                  }

                hdr = miscutil::list_to_text(&csp->_headers);
                if (hdr == NULL)
                  {
                    /* FIXME Should handle error properly */
                    errlog::log_error(LOG_LEVEL_FATAL, "Out of memory parsing server header");
                  }
                csp->_server_connection._response_received = time(NULL);

                //std::cout << "hdr: " << hdr << std::endl;

                //seeks: deactivated, requires a set of plugins instead.
                // crunchers light is block or redirect url.
                /*		       if (seeks_proxy::crunch_response_triggered(csp, seeks_proxy::_crunchers_light))
                			 { */
                /*
                 * One of the tags created by a server-header
                 * tagger triggered a crunch. We already
                 * delivered the crunch response to the client
                 * and are done here after cleaning up.
                 */
                /* freez(hdr);
                seeks_proxy::mark_server_socket_tainted(csp);
                return;
                } */

                /* Buffer and pcrs filter this if appropriate. */
                if (!http->_ssl) /* We talk plaintext */
                  {
                    //content_filter = filters::get_filter_function(csp); // most likely, filters::pcrs_filter_response.
                    // i.e. applies all applicable +filters.

                    content_filter = !(csp->_content_type & CT_TABOO)
                                     && (csp->_content_type & CT_TEXT) && (!csp->_filter_plugins.empty());
                  }

                /*
                * Only write if we're not buffering for content modification
                */
                if (!content_filter)
                  {
                    /*
                     * Write the server's (modified) header to
                     * the client (along with anything else that
                     * may be in the buffer)
                     */
                    if (spsockets::write_socket(csp->_cfd, hdr, strlen(hdr))  // write server header to the client.
                        || ((len = parsers::flush_socket(csp->_cfd, &csp->_iob)) < 0))
                      {
                        errlog::log_error(LOG_LEVEL_CONNECT, "write header to client failed: %E");

                        /*
                         * The write failed, so don't bother mentioning it
                         * to the client... it probably can't hear us anyway.
                         */
                        freez(hdr);
                        seeks_proxy::mark_server_socket_tainted(csp);
                        return;
                      }
                    byte_count += (unsigned long long)len;
                  }
                else
                  {
                    /*
                     * XXX: the header length should probably
                     * be calculated by get_server_headers().
                     */
                    long header_length = csp->_iob._cur - header_start;
                    assert(csp->_iob._cur > header_start);
                    byte_count += (unsigned long long)(len - header_length);
                  }
                /* we're finished with the server's header */
                freez(hdr);
                server_body = 1;

                /*
                * If this was a MS IIS/5 hack then it means the server
                * has already closed the connection. Nothing more to read.
                * Time to bail.
                */
                if (ms_iis5_hack)
                  {
                    errlog::log_error(LOG_LEVEL_ERROR,
                                      "Closed server connection detected. "
                                      "Applying the MS IIS5 hack didn't help.");
                    errlog::log_error(LOG_LEVEL_CLF,
                                      "%s - - [%T] \"%s\" 502 0", csp->_ip_addr_str, http->_cmd);
                    spsockets::write_socket(csp->_cfd, INVALID_SERVER_HEADERS_RESPONSE,
                                            strlen(INVALID_SERVER_HEADERS_RESPONSE));
                    seeks_proxy::mark_server_socket_tainted(csp);
                    return;
                  }
              }
            continue;
          }
        seeks_proxy::mark_server_socket_tainted(csp);
        return; /* huh? we should never get here */
      }

    if (csp->_content_length == 0)
      {
        /*
         * If Seeks proxy didn't recalculate the Content-Length,
         * byte_count is still correct.
         */
        csp->_content_length = byte_count;
      }

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
    if ((csp->_flags & CSP_FLAG_CONTENT_LENGTH_SET)
        && (csp->_expected_content_length != byte_count))
      {
        errlog::log_error(LOG_LEVEL_CONNECT,
                          "Received %llu bytes while expecting %llu.",
                          byte_count, csp->_expected_content_length);
        seeks_proxy::mark_server_socket_tainted(csp);
      }
#endif

    errlog::log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 200 %llu",
                      csp->_ip_addr_str, http->_ocmd, csp->_content_length);
    csp->_server_connection._timestamp = time(NULL);
  }


  /*********************************************************************
   *
   * Function    :  serve
   *
   * Description :  This is little more than chat.  We only "serve" to
   *                to close (or remember) any socket that chat may have
   *                opened.
   *
   * Parameters  :
   *          1  :  csp = Current client state (buffers, headers, etc...)
   *
   * Returns     :  N/A
   *
   *********************************************************************/
  void seeks_proxy::serve(client_state *csp)
  {
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
    static int monitor_thread_running = 0;
    int continue_chatting = 0;
    unsigned int latency = 0;

    do
      {
        seeks_proxy::chat(csp);

        if ((csp->_flags & CSP_FLAG_SERVER_CONNECTION_KEEP_ALIVE)
            && !(csp->_flags & CSP_FLAG_SERVER_KEEP_ALIVE_TIMEOUT_SET))
          {
            errlog::log_error(LOG_LEVEL_CONNECT, "The server didn't specify how long "
                              "the connection will stay open. Assume it's only a second.");
            csp->_server_connection._keep_alive_timeout = 1;
          }

        continue_chatting = (csp->_config->_feature_flags
                             & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE)
                            && (csp->_flags & CSP_FLAG_SERVER_CONNECTION_KEEP_ALIVE)
                            && !(csp->_flags & CSP_FLAG_SERVER_SOCKET_TAINTED)
                            && (csp->_cfd != SP_INVALID_SOCKET)
                            && (csp->_sfd != SP_INVALID_SOCKET)
                            && spsockets::socket_is_still_usable(csp->_sfd)
                            && (latency < csp->_server_connection._keep_alive_timeout);

        if (continue_chatting)
          {
            unsigned int client_timeout = (unsigned)csp->_server_connection._keep_alive_timeout - latency;
            errlog::log_error(LOG_LEVEL_CONNECT,
                              "Waiting for the next client request. "
                              "Keeping the server socket %d to %s open.",
                              csp->_sfd, csp->_server_connection._host);

            if ((csp->_flags & CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE)
                && spsockets::data_is_available(csp->_cfd, (int)client_timeout)
                && spsockets::socket_is_still_usable(csp->_cfd))
              {
                errlog::log_error(LOG_LEVEL_CONNECT, "Client request arrived in "
                                  "time or the client closed the connection.");
                /*
                * Get the csp in a mostly virgin state again.
                * XXX: Should be done elsewhere.
                */
                csp->_content_type = 0;
                csp->_content_length = 0;
                csp->_expected_content_length = 0;
                freez(csp->_iob._buf);
                memset(&csp->_iob, 0, sizeof(csp->_iob));
                freez(csp->_error_message);
                miscutil::list_remove_all(&csp->_headers);
                miscutil::list_remove_all(&csp->_tags);
                if (NULL != csp->_fwd)
                  {
                    delete csp->_fwd;
                    csp->_fwd = NULL;
                  }

                /* XXX: Store per-connection flags someplace else. */
                csp->_flags = CSP_FLAG_ACTIVE | (csp->_flags & CSP_FLAG_TOGGLED_ON);
              }
            else
              {
                errlog::log_error(LOG_LEVEL_CONNECT,
                                  "No additional client request received in time.");
                if ((csp->_config->_feature_flags & RUNTIME_FEATURE_CONNECTION_SHARING)
                    && (spsockets::socket_is_still_usable(csp->_sfd)))
                  {
                    gateway::remember_connection(csp, filters::forward_url(csp, &csp->_http));
                    csp->_sfd = SP_INVALID_SOCKET;
                    spsockets::close_socket(csp->_cfd);
                    csp->_cfd = SP_INVALID_SOCKET;
                    mutex_lock(&seeks_proxy::_connection_reuse_mutex);
                    if (!monitor_thread_running)
                      {
                        monitor_thread_running = 1;
                        mutex_unlock(&seeks_proxy::_connection_reuse_mutex);
                        seeks_proxy::wait_for_alive_connections();
                        mutex_lock(&seeks_proxy::_connection_reuse_mutex);
                        monitor_thread_running = 0;
                      }
                    mutex_unlock(&seeks_proxy::_connection_reuse_mutex);
                  }

                break;
              }
          }
        else if (csp->_sfd != SP_INVALID_SOCKET)
          {
            errlog::log_error(LOG_LEVEL_CONNECT,
                              "The connection on server socket %d to %s isn't reusable. "
                              "Closing.", csp->_sfd, csp->_server_connection._host);
          }
      }
    while (continue_chatting);

    gateway::mark_connection_closed(&csp->_server_connection);
#else
    seeks_proxy::chat(csp);
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

    if (csp->_sfd != SP_INVALID_SOCKET)
      {
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
        gateway::forget_connection(csp->_sfd);
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */
        spsockets::close_socket(csp->_sfd);
      }

    if (csp->_cfd != SP_INVALID_SOCKET)
      {
        spsockets::close_socket(csp->_cfd);
      }
    csp->_flags &= ~CSP_FLAG_ACTIVE;
  }

  void seeks_proxy::gracious_exit()
  {
    plugin_manager::close_all_plugins();

    iso639::cleanup();
    
#if defined(PROTOBUF) && defined(TC)
    /* closing the user database. */
    if (seeks_proxy::_user_db)
      {
	seeks_proxy::_user_db->optimize_db();
	delete seeks_proxy::_user_db; // also closes the db.
      }
#endif
    if (seeks_proxy::_config)
      delete seeks_proxy::_config;
    if (seeks_proxy::_lsh_config)
      delete seeks_proxy::_lsh_config;
    free_const(seeks_proxy::_basedir);
        
#if defined(unix)
    if (seeks_proxy::_pidfile)
      {
	unlink(seeks_proxy::_pidfile);
      }
#endif /* unix */
  }

#if !defined(_WIN32)
  /*********************************************************************
   *
   * Function    :  sig_handler
   *
   * Description :  Signal handler for different signals.
   *                Exit gracefully on TERM and INT
   *                or set a flag that will cause the errlog
   *                to be reopened by the main thread on HUP.
   *
   * Parameters  :
   *          1  :  the_signal = the signal cause this function to call
   *
   * Returns     :  -
   *
   *********************************************************************/
  void seeks_proxy::sig_handler(int the_signal)
  {
    switch (the_signal)
      {
      case SIGTERM:
      case SIGINT:
        errlog::log_error(LOG_LEVEL_INFO, "exiting by signal %d .. bye", the_signal);
	seeks_proxy::gracious_exit();
        exit(the_signal);
        break;

      case SIGHUP:
# if defined(unix)
        seeks_proxy::_received_hup_signal = 1;
# endif
        break;

      default:
        /*
         * We shouldn't be here, unless we catch signals
         * in main() that we can't handle here!
         */
        errlog::log_error(LOG_LEVEL_FATAL, "sig_handler: exiting on unexpected signal %d", the_signal);
      }
    return;
  }
#endif

#if !defined(_WIN32) || defined(_WIN_CONSOLE)
  /*********************************************************************
   *
   * Function    :  usage
   *
   * Description :  Print usage info & exit.
   *
   * Parameters  :  Pointer to argv[0] for identifying ourselves
   *
   * Returns     :  None.
   *
   *********************************************************************/
  void seeks_proxy::usage(const char *myname)
  {
    printf("Seeks version " VERSION " (" HOME_PAGE_URL ")\n"
           "Usage: %s "
# if defined(unix)
           "[--chroot] "
# endif /* defined(unix) */
           "[--help] "
# if defined(unix)
           "[--daemon] [--pidfile pidfile] [--pre-chroot-nslookup hostname] [--user user[.group]] "
# endif /* defined(unix) */
           "[--version] [--plugin-repository dir] [--data-repository dir] [configfile]\n"
           "Bye\n", myname);
    exit(2);
  }
#endif /* #if !defined(_WIN32) || defined(_WIN_CONSOLE) */


  /*********************************************************************
   *
   * Function    :  initialize_mutexes
   *
   * Description :  Prepares mutexes if mutex support is available.
   *
   * Parameters  :  None
   *
   * Returns     :  Void, exits in case of errors.
   *
   *********************************************************************/
  void seeks_proxy::initialize_mutexes(void)
  {
#ifdef MUTEX_LOCKS_AVAILABLE
    /*
     * Prepare global mutex semaphores
     */
    mutex_init(&errlog::_log_mutex);
    //mutex_init(&seeks_proxy::_log_init_mutex);
    mutex_init(&seeks_proxy::_connection_reuse_mutex);

    /*
     * XXX: The assumptions below are a bit naive
     * and can cause locks that aren't necessary.
     *
     * For example older FreeBSD versions (< 6.x?)
     * have no gethostbyname_r, but gethostbyname is
     * thread safe.
     */
#if !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R)
    mutex_init(&seeks_proxy::_resolver_mutex);
#endif /* !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R) */
    /*
     * XXX: should we use a single mutex for
     * localtime() and gmtime() as well?
     */
#ifndef HAVE_GMTIME_R
    mutex_init(&seeks_proxy::_gmtime_mutex);
#endif /* ndef HAVE_GMTIME_R */

#ifndef HAVE_LOCALTIME_R
    mutex_init(&seeks_proxy::_localtime_mutex);
#endif /* ndef HAVE_GMTIME_R */

#ifndef HAVE_RANDOM
    mutex_init(&seeks_proxy::_rand_mutex);
#endif /* ndef HAVE_RANDOM */
#endif /* def MUTEX_LOCKS_AVAILABLE */
  
    // initialize sweeper's mutex.
    mutex_init(&sweeper::_mem_dust_mutex);
  }

#ifdef _WIN32
  /* Without this simple workaround we get this compiler warning from _beginthread
   *     warning C4028: formal parameter 1 different from declaration
   */
  void seeks_proxy::w32_service_listen_loop(void *p)
  {
    seeks_proxy::listen_loop();
  }
#endif /* def _WIN32 */

  /*********************************************************************
   *
   * Function    :  listen_loop
   *
   * Description :  bind the listen port and enter a "FOREVER" listening loop.
   *
   * Parameters  :  N/A
   *
   * Returns     :  Never.
   *
   *********************************************************************/
  void seeks_proxy::listen_loop()
  {
    client_state *csp = NULL;
    sp_socket bfd;

    unsigned int active_threads = 0;

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
    /*
     * XXX: Should be relocated once it no
     * longer needs to emit log messages.
     */
    gateway::initialize_reusable_connections();
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

    bfd = seeks_proxy::bind_port_helper(seeks_proxy::_config);

#ifdef FEATURE_GRACEFUL_TERMINATION
    while (!g_terminate)
#else
    for (;;)
#endif
      {
#if !defined(FEATURE_PTHREAD) && !defined(_WIN32)
        while (waitpid(-1, NULL, WNOHANG) > 0)
          {
            /* zombie children */
          }
#endif /* !defined(FEATURE_PTHREAD) && !defined(_WIN32) */

        /*
        * Free data that was used by died threads
        */
        active_threads = sweeper::sweep();

#if defined(unix)
        /*
        * Re-open the errlog after HUP signal
        */
        if (seeks_proxy::_received_hup_signal)
          {
            if (NULL != seeks_proxy::_config->_logfile)
              {
                errlog::init_error_log(seeks_proxy::_Argv[0], seeks_proxy::_config->_logfile);
              }
            seeks_proxy::_received_hup_signal = 0;
          }
#endif
        if ( NULL == (csp = new client_state()))  // new client_state.
          {
            errlog::log_error(LOG_LEVEL_FATAL, "malloc(%d) for csp failed: %E", sizeof(*csp));
            continue;
          }
        csp->_flags |= CSP_FLAG_ACTIVE;
        csp->_sfd    = SP_INVALID_SOCKET;

        seeks_proxy::_config->load_config(); // reload when running, if file has changed.
        csp->_config = seeks_proxy::_config;

        if (seeks_proxy::_config->_need_bind )
          {
            /*
             * Since we were listening to the "old port", we will not see
             * a "listen" param change until the next request.  So, at
             * least 1 more request must be made for us to find the new
             * setting.  I am simply closing the old socket and binding the
             * new one.
             *
             * Which-ever is correct, we will serve 1 more page via the
             * old settings.  This should probably be a "show-proxy-args"
             * request.  This should not be a so common of an operation
             * that this will hurt people's feelings.
             */

            spsockets::close_socket(bfd);

            bfd = seeks_proxy::bind_port_helper(seeks_proxy::_config);
          }

        errlog::log_error(LOG_LEVEL_CONNECT, "Listening for new connections ... ");

        if (!spsockets::accept_connection(csp, bfd))  // accepting connection.
          {
            errlog::log_error(LOG_LEVEL_CONNECT, "accept failed: %E");
            delete csp;
            continue;
          }
        else
          {
            errlog::log_error(LOG_LEVEL_CONNECT, "accepted connection from %s", csp->_ip_addr_str);
          }

#ifdef FEATURE_TOGGLE
        if (seeks_proxy::_global_toggle_state)
#endif /* def FEATURE_TOGGLE */
          {
            csp->_flags |= CSP_FLAG_TOGGLED_ON;
          }

#ifdef FEATURE_ACL
        if (filters::block_acl(NULL,csp))
          {
            errlog::log_error(LOG_LEVEL_CONNECT, "Connection from %s dropped due to ACL", csp->_ip_addr_str);
            spsockets::close_socket(csp->_cfd);
            freez(csp->_ip_addr_str);
            delete csp;
            continue;
          }
#endif /* def FEATURE_ACL */

        if ((0 != seeks_proxy::_config->_max_client_connections)
            && (active_threads >= (size_t)seeks_proxy::_config->_max_client_connections))
          {
            errlog::log_error(LOG_LEVEL_CONNECT,
                              "Rejecting connection from %s. Maximum number of connections reached.",
                              csp->_ip_addr_str);
            spsockets::write_socket(csp->_cfd, TOO_MANY_CONNECTIONS_RESPONSE,
                                    strlen(TOO_MANY_CONNECTIONS_RESPONSE));
            spsockets::close_socket(csp->_cfd);
            freez(csp->_ip_addr_str);
            delete csp;
            continue;
          }

        /* add it to the list of clients */
        csp->_next = seeks_proxy::_clients._next;
        seeks_proxy::_clients._next = csp;

        if (seeks_proxy::_config->_multi_threaded)
          {
            int child_id;

#undef SELECTED_ONE_OPTION

            /* Use Pthreads in preference to native code */
#if defined(FEATURE_PTHREAD) && !defined(SELECTED_ONE_OPTION)
# define SELECTED_ONE_OPTION
            {
              pthread_t the_thread;
              pthread_attr_t attrs;

              pthread_attr_init(&attrs);
              pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
              errno = pthread_create(&the_thread, &attrs,
                                     (void * (*)(void *))serve, csp);
              child_id = errno ? -1 : 0;
              pthread_attr_destroy(&attrs);
            }
#endif

#if defined(_WIN32) && !defined(_CYGWIN) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
            child_id = _beginthread(
                         (void (*)(void *))serve,
                         64 * 1024,
                         csp);
#endif

#if !defined(SELECTED_ONE_OPTION)
            child_id = fork();

            /* This block is only needed when using fork().
             * When using threads, the server thread was
             * created and run by the call to _beginthread().
             */
            if (child_id == 0)   /* child */
              {
                int rc = 0;
#ifdef FEATURE_TOGGLE
                int inherited_toggle_state = seeks_proxy::_global_toggle_state;
# endif /* def FEATURE_TOGGLE */
                seeks_proxy::serve(csp);

                /*
                 * If we've been toggled or we've blocked the request, tell Mom
                 */
# ifdef FEATURE_TOGGLE
                if (inherited_toggle_state != seeks_proxy::_global_toggle_state)
                  {
                    rc |= RC_FLAG_TOGGLED;
                  }
# endif /* def FEATURE_TOGGLE */

# ifdef FEATURE_STATISTICS
                if (csp->_flags & CSP_FLAG_REJECTED)
                  {

                    rc |= RC_FLAG_BLOCKED;
                  }
# endif /* ndef FEATURE_STATISTICS */
                _exit(rc);
              }

            else if (child_id > 0) /* parent */
              {
                /* in a fork()'d environment, the parent's
                 * copy of the client socket and the CSP
                 * are not used.
                 */
                int child_status;
#if !defined(_WIN32) && !defined(__CYGWIN__)
                wait( &child_status );

                /*
                 * Evaluate child's return code: If the child has
                 *  - been toggled, toggle ourselves
                 *  - blocked its request, bump up the stats counter
                 */
#ifdef FEATURE_TOGGLE
                if (WIFEXITED(child_status) && (WEXITSTATUS(child_status) & RC_FLAG_TOGGLED))
                  {
                    seeks_proxy::_global_toggle_state = !seeks_proxy::_global_toggle_state;
                  }
#endif /* def FEATURE_TOGGLE */

#ifdef FEATURE_STATISTICS
                urls_read++;
                if (WIFEXITED(child_status) && (WEXITSTATUS(child_status) & RC_FLAG_BLOCKED))
                  {

                    urls_rejected++;
                  }
#endif /* def FEATURE_STATISTICS */

#endif /* !defined(_WIN32) && defined(__CYGWIN__) */
                spsockets::close_socket(csp->_cfd);
                csp->_flags &= ~CSP_FLAG_ACTIVE;
              }
#endif

#undef SELECTED_ONE_OPTION
            if (child_id < 0)
              {
                /*
                 * Spawning the child failed, assume it's because
                 * there are too many children running already.
                 * XXX: If you assume ...
                 */
                errlog::log_error(LOG_LEVEL_ERROR,
                                  "Unable to take any additional connections: %E");
                spsockets::write_socket(csp->_cfd, TOO_MANY_CONNECTIONS_RESPONSE,
                                        strlen(TOO_MANY_CONNECTIONS_RESPONSE));
                spsockets::close_socket(csp->_cfd);
                csp->_flags &= ~CSP_FLAG_ACTIVE;
              }
          }
        else
          {
            seeks_proxy::serve(csp);
          }
      }

    /* NOTREACHED unless FEATURE_GRACEFUL_TERMINATION is defined */

    /* Clean up.  Aim: free all memory (no leaks) */
#ifdef FEATURE_GRACEFUL_TERMINATION
    errlog::log_error(LOG_LEVEL_ERROR, "Graceful termination requested");

    if (seeks_proxy::_config->_multi_threaded)
      {
        int i = 60;
        do
          {
            sleep(1);
            sweeper::sweep();
          }
        while ((clients->_next != NULL) && (--i > 0));
        if (i <= 0)
          {
            errlog::log_error(LOG_LEVEL_ERROR, "Graceful termination failed - still some live clients after 1 minute wait.");
          }
      }

    sweeper::sweep();

#if defined(unix)
    freez(seeks_proxy::_basedir);
#endif
#if defined(_WIN32) && !defined(_WIN_CONSOLE)
    /* Cleanup - remove taskbar icon etc. */
    TermLogWindow();
#endif
    exit(0);
#endif /* FEATURE_GRACEFUL_TERMINATION */
  }

  /********************************************************************
   *
   * Function    :  bind_port_helper
   *
   * Description :  Bind the listen port.  Handles logging, and aborts
   *                on failure.
   *
   * Parameters  :
   *          1  :  config = Seeks proxy configuration.  Specifies port
   *                         to bind to.
   *
   * Returns     :  Port that was opened.
   *
   *********************************************************************/
  sp_socket seeks_proxy::bind_port_helper(proxy_configuration *config)
  {
    int result;
    sp_socket bfd;

    if (config->_haddr == NULL)
      {
        errlog::log_error(LOG_LEVEL_INFO, "Listening on port %d on all IP addresses",
                          config->_hport);
      }
    else
      {
        errlog::log_error(LOG_LEVEL_INFO, "Listening on port %d on IP address %s",
                          config->_hport, config->_haddr);
      }
    result = spsockets::bind_port(config->_haddr, config->_hport, &bfd);
    if (result < 0)
      {
        switch (result)
          {
          case -3 :
            errlog::log_error(LOG_LEVEL_FATAL, "can't bind to %s:%d: "
                              "There may be another Seeks proxy or some other "
                              "proxy running on port %d",
                              (NULL != config->_haddr) ? config->_haddr : "INADDR_ANY",
                              config->_hport, config->_hport);
          case -2 :
            errlog::log_error(LOG_LEVEL_FATAL, "can't bind to %s:%d: "
                              "The hostname is not resolvable",
                              (NULL != config->_haddr) ? config->_haddr : "INADDR_ANY", config->_hport);
          default :
            errlog::log_error(LOG_LEVEL_FATAL, "can't bind to %s:%d: %E",
                              (NULL != config->_haddr) ? config->_haddr : "INADDR_ANY", config->_hport);
          }

        /* shouldn't get here */
        return SP_INVALID_SOCKET;
      }
    config->_need_bind = 0;
    return bfd;
  }

#if defined(unix)
  /*********************************************************************
   *
   * Function    :  write_pid_file
   *
   * Description :  Writes a pid file with the pid of the main process
   *
   * Parameters  :  None
   *
   * Returns     :  N/A
   *
   *********************************************************************/
  void seeks_proxy::write_pid_file(void)
  {
    FILE   *fp;

    /*
     * If no --pidfile option was given,
     * we can live without one.
     */
    if (seeks_proxy::_pidfile == NULL) return;

    if ((fp = fopen(seeks_proxy::_pidfile, "w")) == NULL)
      {
        errlog::log_error(LOG_LEVEL_INFO, "can't open pidfile '%s': %E",
                          seeks_proxy::_pidfile);
      }
    else
      {
        fprintf(fp, "%u\n", (unsigned int) getpid());
        fclose (fp);
      }

    return;
  }
#endif /* def unix */

  /*********************************************************************
   *
   * Function    :  make_path
   *
   * Description :  Takes a directory name and a file name, returns
   *                the complete path.  Handles windows/unix differences.
   *                If the file name is already an absolute path, or if
   *                the directory name is NULL or empty, it returns
   *                the filename.
   *
   * Parameters  :
   *          1  :  dir: Name of directory or NULL for none.
   *          2  :  file: Name of file.  Should not be NULL or empty.
   *
   * Returns     :  "dir/file" (Or on windows, "dir\file").
   *                It allocates the string on the heap.  Caller frees.
   *                Returns NULL in error (i.e. NULL file or out of
   *                memory)
   *
   *********************************************************************/
  char* seeks_proxy::make_path(const char *dir, const char *file)
  {
    if ((file == NULL) || (*file == '\0'))
      {
        return NULL; /* Error */
      }

    if ((dir == NULL) || (*dir == '\0') /* No directory specified */
#if defined(_WIN32) || defined(__OS2__)
        || (*file == '\\') || (file[1] == ':') /* Absolute path (DOS) */
#else /* ifndef _WIN32 || __OS2__ */
        || (*file == '/') /* Absolute path (U*ix) */
#endif /* ifndef _WIN32 || __OS2__  */
       )
      {
        return strdup(file);
      }
    else
      {
        char * path;
        size_t path_size = strlen(dir) + strlen(file) + 2; /* +2 for trailing (back)slash and \0 */

#if defined(unix)
        if ( *dir != '/' && seeks_proxy::_basedir && *seeks_proxy::_basedir )
          {
            /*
             * Relative path, so start with the base directory.
             */
            path_size += strlen(seeks_proxy::_basedir) + 1; /* +1 for the slash */
            path = (char*) zalloc(path_size);
            if (!path ) errlog::log_error(LOG_LEVEL_FATAL, "malloc failed!");
            strlcpy(path, seeks_proxy::_basedir, path_size);
            strlcat(path, "/", path_size);
            strlcat(path, dir, path_size);
          }
        else
#endif /* defined unix */
          {
            path = (char*) zalloc(path_size);
            if (!path ) errlog::log_error(LOG_LEVEL_FATAL, "malloc failed!");
            strlcpy(path, dir, path_size);
          }
        assert(NULL != path);
#if defined(_WIN32) || defined(__OS2__)
        if (path[strlen(path)-1] != '\\')
          {
            strlcat(path, "\\", path_size);
          }
#else /* ifndef _WIN32 */
        if (path[strlen(path)-1] != '/')
          {
            strlcat(path, "/", path_size);
          }
#endif /* ifndef _WIN32 */
        strlcat(path, file, path_size);

        return path;
      }
  }

} /* end of namespace. */
