/**
 * The seeks proxy is part of the SEEKS project
 * It is based on Privoxy (http://www.privoxy.org), developped
 * by the Privoxy team.
 * 
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef PROXY_DTS_H
#define PROXY_DTS_H

#include "config.h"

#include <list>
#include <map>
#include <set>

#include "stl_hash.h"

#include <stdio.h>
#include <string.h>

#ifdef HAVE_RFC2553
/* Need for class sockaddr_storage */
#include <sys/socket.h>
#endif

#include <pcreposix.h> // for regex_t
#include <pcre.h>
#include "pcrs.h"

#ifdef _WIN32
/*
 * I don't want to have to #include all this just for the declaration
 * of SOCKET.  However, it looks like we have to...
 */
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

typedef SOCKET sp_socket;

#define SP_INVALID_SOCKET INVALID_SOCKET

#else /* ndef _WIN32 */

/**
 * The type used by sockets.  On UNIX it's an int.  Microsoft decided to
 * make it an unsigned.
 */
typedef int sp_socket;

/**
 * The error value used for variables of type jb_socket.  On UNIX this
 * is -1, however Microsoft decided to make socket handles unsigned, so
 * they use a different value.
 */

#define SP_INVALID_SOCKET (-1)

#endif /* undef _WIN32 */

namespace sp
{
   
#include "sp_err.h"

/**
 * Use for statically allocated buffers if you have no other choice.
 * Remember to check the length of what you write into the buffer
 * - we don't want any buffer overflows!
 */
#define BUFFER_SIZE 5000

/**
 * Max length of CGI parameters (arbitrary limit).
 */
#define CGI_PARAM_LEN_MAX 500U

/**
 * Buffer size for capturing struct hostent data in the
 * gethostby(name|addr)_r library calls. Since we don't
 * loop over gethostbyname_r, the buffer must be sufficient
 * to accomodate multiple IN A RRs, as used in DNS round robin
 * load balancing. W3C's wwwlib uses 1K, so that should be
 * good enough for us, too.
 */
/**
 * XXX: Temporary doubled, for some configurations
 * 1K is still too small and we didn't get the
 * real fix ready for inclusion.
 */
#define HOSTENT_BUFFER_SIZE 2048

/**
 * Default TCP/IP address to listen on, as a string.
 * Set to "127.0.0.1:8118".
 */
#define HADDR_DEFAULT   "127.0.0.1:8118"

/* Forward declaration for class client_state. */
class forward_spec;
class file_list;
   
class plugin;
class interceptor_plugin;
class action_plugin;
class filter_plugin;

class proxy_configuration;   

/**
 * A HTTP request.  This includes the method (GET, POST) and
 * the parsed URL.
 *
 * This is also used whenever we want to match a URL against a
 * URL pattern.  This always contains the URL to match, and never
 * a URL pattern.  (See class url_spec).
 */
class http_request
{
 public:
   http_request()
     : _cmd(NULL),_ocmd(NULL),_gpc(NULL),_url(NULL),
       _ver(NULL),_status(0),_host(NULL),_port(0),
       _path(NULL),_hostport(NULL),_ssl(0),_host_ip_addr_str(NULL)
#ifndef FEATURE_EXTENDED_HOST_PATTERNS
     ,_dbuffer(NULL),_dvec(NULL),_dcount(0)
#endif
       {};
   
   ~http_request();
   
   void clear();
   
   char *_cmd;      /**< Whole command line: method, URL, Version */
   char *_ocmd;     /**< Backup of original cmd for CLF logging */
   char *_gpc;      /**< HTTP method: GET, POST, ... */
   char *_url;      /**< The URL */
   char *_ver;      /**< Protocol version */
   int _status;     /**< HTTP Status */
   
   char *_host;     /**< Host part of URL */
   int   _port;     /**< Port of URL or 80 (default) */
   char *_path;     /**< Path of URL */
   char *_hostport; /**< host[:port] */
   int   _ssl;      /**< Flag if protocol is https */
   
   char *_host_ip_addr_str; /**< String with dotted decimal representation
			     of host's IP. NULL before connect_to() */

#ifndef FEATURE_EXTENDED_HOST_PATTERNS
   char  *_dbuffer; /**< Buffer with '\0'-delimited domain name.           */
   char **_dvec;    /**< List of pointers to the strings in dbuffer.       */
   int    _dcount;  /**< How many parts to this domain? (length of dvec)   */
#endif /* ndef FEATURE_EXTENDED_HOST_PATTERNS */
};

/**
 * Reasons for generating a http_response instead of delivering
 * the requested resource. Mostly ordered the way they are checked
 * for in chat().
 */
#define RSP_REASON_UNSUPPORTED        1
#define RSP_REASON_BLOCKED            2
#define RSP_REASON_UNTRUSTED          3
#define RSP_REASON_REDIRECTED         4
#define RSP_REASON_CGI_CALL           5
#define RSP_REASON_NO_SUCH_DOMAIN     6
#define RSP_REASON_FORWARDING_FAILED  7
#define RSP_REASON_CONNECT_FAILED     8
#define RSP_REASON_OUT_OF_MEMORY      9
#define RSP_REASON_INTERNAL_ERROR     10
#define RSP_REASON_CONNECTION_TIMEOUT 11
#define RSP_REASON_NO_SERVER_DATA     12

/**
 * Response generated by CGI, blocker, or error handler
 */
class http_response
{
 public:
   http_response();
   
   http_response(char *head, char* body);
   
   ~http_response();
   
   void reset();
   
   char  *_status;          /**< HTTP status (string). */
   std::list<const char*> _headers; /**< List of header lines. */
   char  *_head;            /**< Formatted http response head. */
   size_t _head_length;     /**< Length of http response head. */
   char  *_body;            /**< HTTP document body. */
   size_t _content_length;  /**< Length of body, REQUIRED if binary body. */
   int    _is_static;       /**< Nonzero if the content will never change and
			        should be cached by the browser (e.g. images). */
   int _reason;             /**< Why the response was generated in the first place. */
};

/**
 * A URL or a tag pattern.
 */
class url_spec
{
 public:
#ifndef FEATURE_EXTENDED_HOST_PATTERNS
   url_spec()
     : _spec(NULL),_dbuffer(NULL),_dvec(NULL),
       _dcount(0),_unanchored(0),_port_list(NULL),
       _preg(NULL),_tag_regex(NULL)
     {};
#else
   url_spec()
     : _spec(NULL),_host_regex(NULL),_port_list(NULL),
          _preg(NULL),_tag_regex(NULL)
     {};
#endif
   
   url_spec(const char *buf);
   
   ~url_spec();

   static sp_err create_url_spec(url_spec *&url, char *buf);
   
   /** The string which was parsed to produce this url_spec.
        Used for debugging or display only.  */
   char  *_spec;
   
#ifdef FEATURE_EXTENDED_HOST_PATTERNS
   regex_t *_host_regex;/**< Regex for host matching                          */
#else
   char  *_dbuffer;     /**< Buffer with '\0'-delimited domain name, or NULL to match all hosts. */
   char **_dvec;        /**< List of pointers to the strings in dbuffer.       */
   int    _dcount;      /**< How many parts to this domain? (length of dvec)   */
   int    _unanchored;  /**< Bitmap - flags are ANCHOR_LEFT and ANCHOR_RIGHT.  */
#endif /* defined FEATURE_EXTENDED_HOST_PATTERNS */
   
   char  *_port_list;   /**< List of acceptable ports, or NULL to match all ports */
   
   regex_t *_preg;      /**< Regex for matching path part                      */
   regex_t *_tag_regex; /**< Regex for matching tags                           */
};

/**
 * Constant for host part matching in URLs.  If set, indicates that the start of
 * the pattern must match the start of the URL.  E.g. this is not set for the
 * pattern ".example.com", so that it will match both "example.com" and
 * "www.example.com".  It is set for the pattern "example.com", which makes it
 * match "example.com" only, not "www.example.com".
 */
#define ANCHOR_LEFT  1

/**
 * Constant for host part matching in URLs.  If set, indicates that the end of
 * the pattern must match the end of the URL.  E.g. this is not set for the
 * pattern "ad.", so that it will match any host called "ad", irrespective
 * of how many subdomains are in the fully-qualified domain name.
 */
#define ANCHOR_RIGHT 2

/**
 * An I/O buffer.  Holds a string which can be appended to, and can have data
 * removed from the beginning.
 */
class iob
{
 public:
   iob() 
     :_buf(NULL),_cur(NULL),_eod(NULL),_size(0)
     {}; 
   
   ~iob() {};  // beware of memory leaks...
   
   void reset()  // beware of memory leaks...
     {
	delete[] _buf;
	_buf = NULL;
	_cur = NULL;
	_eod = NULL;
	_size = 0;
     };
   
   char *_buf;    /**< Start of buffer        */
   char *_cur;    /**< Start of relevant data */
   char *_eod;    /**< End of relevant data   */
   size_t _size;  /**< Size as malloc()ed     */
};

/**
 * Return the number of bytes in the I/O buffer associated with the passed
 * client_state pointer.
 * May be zero.
 */
#define IOB_PEEK(CSP) ((CSP->_iob._cur > CSP->_iob._eod) ? (CSP->_iob._eod - CSP->_iob._cur) : 0)

/**
 * Remove any data in the I/O buffer associated with the passed
 * client_state pointer.
 */
#define IOB_RESET(CSP) if(CSP->_iob._buf) CSP->_iob.reset();

/* Bits for csp->content_type bitmask: */
#define CT_TEXT    0x0001U /**< Suitable for pcrs filtering. */
#define CT_GIF     0x0002U /**< Suitable for GIF filtering.  */
#define CT_TABOO   0x0004U /**< DO NOT filter, irrespective of other flags. */
#define CT_CSS     0x0005U /**< Suitable for CSS. */
   
/* 
 * Although these are not, strictly speaking, content types
 * (they are content encodings), it is simple to handle them
 * as such.
 */
#define CT_GZIP    0x0010U /**< gzip-compressed data. */
#define CT_DEFLATE 0x0020U /**< zlib-compressed data. */

/**
 * Flag to signal that the server declared the content type,
 * so we can differentiate between unknown and undeclared
 * content types.
 */
#define CT_DECLARED 0x0040U

/**
 * The mask which includes all actions.
 */
#define ACTION_MASK_ALL        (~0UL)

/**
 * The most compatible set of actions - i.e. none.
 */
#define ACTION_MOST_COMPATIBLE                       0x00000000UL

/** Action bitmap: Block the request. */
#define ACTION_BLOCK                                 0x00000001UL
/** Action bitmap: Deanimate if it's a GIF. */
#define ACTION_DEANIMATE                             0x00000002UL
/** Action bitmap: Downgrade HTTP/1.1 to 1.0. */
#define ACTION_DOWNGRADE                             0x00000004UL
/** Action bitmap: Fast redirects. */
#define ACTION_FAST_REDIRECTS                        0x00000008UL
/** Action bitmap: Remove or add "X-Forwarded-For" header. */
#define ACTION_CHANGE_X_FORWARDED_FOR                0x00000010UL
/** Action bitmap: Hide "From" header. */
#define ACTION_HIDE_FROM                             0x00000020UL
/** Action bitmap: Hide "Referer" header.  (sic - follow HTTP, not English). */
#define ACTION_HIDE_REFERER                          0x00000040UL
/** Action bitmap: Hide "User-Agent" and similar headers. */
#define ACTION_HIDE_USER_AGENT                       0x00000080UL
/** Action bitmap: This is an image. */
#define ACTION_IMAGE                                 0x00000100UL
/** Action bitmap: Sets the image blocker. */
#define ACTION_IMAGE_BLOCKER                         0x00000200UL
/** Action bitmap: Prevent compression. */
#define ACTION_NO_COMPRESSION                        0x00000400UL
/** Action bitmap: Change cookies to session only cookies. */
#define ACTION_NO_COOKIE_KEEP                        0x00000800UL
/** Action bitmap: Block rending cookies. */
#define ACTION_NO_COOKIE_READ                        0x00001000UL
/** Action bitmap: Block setting cookies. */
#define ACTION_NO_COOKIE_SET                         0x00002000UL
/** Action bitmap: Override the forward settings in the config file */
#define ACTION_FORWARD_OVERRIDE                      0x00004000UL
/** Action bitmap: Block as empty document */
#define  ACTION_HANDLE_AS_EMPTY_DOCUMENT             0x00008000UL
/** Action bitmap: Limit CONNECT requests to safe ports. */
#define ACTION_LIMIT_CONNECT                         0x00010000UL
/** Action bitmap: Redirect request. */
#define  ACTION_REDIRECT                             0x00020000UL
/** Action bitmap: Crunch or modify "if-modified-since" header. */
#define ACTION_HIDE_IF_MODIFIED_SINCE                0x00040000UL
/** Action bitmap: Overwrite Content-Type header. */
#define ACTION_CONTENT_TYPE_OVERWRITE                0x00080000UL
/** Action bitmap: Crunch specified server header. */
#define ACTION_CRUNCH_SERVER_HEADER                  0x00100000UL
/** Action bitmap: Crunch specified client header */
#define ACTION_CRUNCH_CLIENT_HEADER                  0x00200000UL
/** Action bitmap: Enable text mode by force */
#define ACTION_FORCE_TEXT_MODE                       0x00400000UL
/** Action bitmap: Enable text mode by force */
#define ACTION_CRUNCH_IF_NONE_MATCH                  0x00800000UL
/** Action bitmap: Enable content-dispostion crunching */
#define ACTION_HIDE_CONTENT_DISPOSITION              0x01000000UL
/** Action bitmap: Replace or block Last-Modified header */
#define ACTION_OVERWRITE_LAST_MODIFIED               0x02000000UL
/** Action bitmap: Replace or block Accept-Language header */
#define ACTION_HIDE_ACCEPT_LANGUAGE                  0x04000000UL

/** Action string index: How to deanimate GIFs */
#define ACTION_STRING_DEANIMATE             0
/** Action string index: Replacement for "From:" header */
#define ACTION_STRING_FROM                  1
/** Action string index: How to block images */
#define ACTION_STRING_IMAGE_BLOCKER         2
/** Action string index: Replacement for "Referer:" header */
#define ACTION_STRING_REFERER               3
/** Action string index: Replacement for "User-Agent:" header */
#define ACTION_STRING_USER_AGENT            4
/** Action string index: Legal CONNECT ports. */
#define ACTION_STRING_LIMIT_CONNECT         5
/** Action string index: Server headers containing this pattern are crunched*/
#define ACTION_STRING_SERVER_HEADER         6
/** Action string index: Client headers containing this pattern are crunched*/
#define ACTION_STRING_CLIENT_HEADER         7
/** Action string index: Replacement for the "Accept-Language:" header*/
#define ACTION_STRING_LANGUAGE              8
/** Action string index: Replacement for the "Content-Type:" header*/
#define ACTION_STRING_CONTENT_TYPE          9
/** Action string index: Replacement for the "content-dispostion:" header*/
#define ACTION_STRING_CONTENT_DISPOSITION  10
/** Action string index: Replacement for the "If-Modified-Since:" header*/
#define ACTION_STRING_IF_MODIFIED_SINCE    11
/** Action string index: Replacement for the "Last-Modified:" header. */
#define ACTION_STRING_LAST_MODIFIED        12
/** Action string index: Redirect URL */
#define ACTION_STRING_REDIRECT             13
/** Action string index: Decode before redirect? */
#define ACTION_STRING_FAST_REDIRECTS       14
/** Action string index: Overriding forward rule. */
#define ACTION_STRING_FORWARD_OVERRIDE     15
/** Action string index: Reason for the block. */
#define ACTION_STRING_BLOCK                16
/** Action string index: what to do with the "X-Forwarded-For" header. */
#define ACTION_STRING_CHANGE_X_FORWARDED_FOR 17
/** Number of string actions. */
#define ACTION_STRING_COUNT                18


/* To make the ugly hack in sed easier to understand */
#define CHECK_EVERY_HEADER_REMAINING 0

/** Index into current_action_spec::multi[] for headers to add. */
#define ACTION_MULTI_ADD_HEADER              0
/** Index into current_action_spec::multi[] for content filters to apply. */
#define ACTION_MULTI_FILTER                  1
/** Index into current_action_spec::multi[] for server-header filters to apply. */
#define ACTION_MULTI_SERVER_HEADER_FILTER    2
/** Index into current_action_spec::multi[] for client-header filters to apply. */
#define ACTION_MULTI_CLIENT_HEADER_FILTER    3
/** Index into current_action_spec::multi[] for client-header tags to apply. */
#define ACTION_MULTI_CLIENT_HEADER_TAGGER    4
/** Index into current_action_spec::multi[] for server-header tags to apply. */
#define ACTION_MULTI_SERVER_HEADER_TAGGER    5
/** Number of multi-string actions. */
#define ACTION_MULTI_COUNT                   6


/**
 * This structure contains a list of actions to apply to a URL.
 * It only contains positive instructions - no "-" options.
 * It is not used to store the actions list itself, only for
 * url_actions() to return the current values.
 */
class current_action_spec
{
 public:
   current_action_spec() 
     :_flags(ACTION_MOST_COMPATIBLE),_string(),_multi()
     {};
   
   ~current_action_spec();
   
   /** Actions to apply.  A bit set to "1" means perform the action. */
   unsigned long _flags;
   
   /**
    * Paramaters for those actions that require them.
    * Each entry is valid if & only if the corresponding entry in "flags" is
    * set.
    */
   char * _string[ACTION_STRING_COUNT];
   
   /** Lists of strings for multi-string actions. -> ACTION_MULTI_COUNT lists. */
   std::list<const char*>_multi[ACTION_MULTI_COUNT];
}
;

/**
 * This structure contains a list of actions to apply to a URL.
 * It only contains positive instructions - no "-" options.
 * It is not used to store the actions list itself, only for
 * url_actions() to return the current values.
 */
class action_spec
{
 public: 
   action_spec() 
     :_mask(0),_add(0),_string(),_multi_remove(),
      _multi_remove_all(),_multi_add()
     {};
   
   action_spec(const action_spec *src);
   
   ~action_spec();
   
   unsigned long _mask; /**< Actions to keep. A bit set to "0" means remove action. */
   unsigned long _add;  /**< Actions to add.  A bit set to "1" means add action.    */
   
   /**
    * Parameters for those actions that require them.
    * Each entry is valid if & only if the corresponding entry in "flags" is
    * set.
    */
   char * _string[ACTION_STRING_COUNT];
   
   /** Lists of strings to remove, for multi-string actions. */
   std::list<const char*> _multi_remove[ACTION_MULTI_COUNT];
   
   /** If nonzero, remove *all* strings from the multi-string action. */
   int _multi_remove_all[ACTION_MULTI_COUNT];
   
   /** Lists of strings to add, for multi-string actions. */
   std::list<const char*> _multi_add[ACTION_MULTI_COUNT];
};

/**
 * This structure is used to store action files.
 *
 * It contains an URL or tag pattern, and the changes to
 * the actions. It's a linked list and should only be
 * free'd through unload_actions_file() unless there's
 * only a single entry.
 */
class url_actions
{
 public:
   url_actions()
     :_action(NULL),_next(NULL)
       {};
     
   ~url_actions() 
     {
	if (_url)
	  delete _url;
     };
   
   url_spec *_url;     /**< The URL or tag pattern. */
   
   action_spec *_action; /**< Action settings that might be shared with
			      the list entry before or after the current
			      one and can't be free'd willy nilly. */
   
   url_actions *_next;   /**< Next action section in file, or NULL. */
};

/*
 * Structure to make sure we only reuse the server socket
 * if the host and forwarding settings are the same.
 */
class reusable_connection
{
 public:
   reusable_connection()
     :_host(NULL),_port(0),_forwarder_type(0),
      _gateway_host(NULL),_gateway_port(0),
      _forward_host(NULL),_forward_port(0)
       {};
   
   ~reusable_connection() {}; // TODO.
   
   sp_socket _sfd;
   int       _in_use;
   time_t    _timestamp; /* XXX: rename? */
   
   time_t    _request_sent;
   time_t    _response_received;
   
   /*
    * Number of seconds after which this
    * connection will no longer be reused.
    */
   unsigned int _keep_alive_timeout;
   
   char *_host;
   int  _port;
   int  _forwarder_type;
   char *_gateway_host;
   int  _gateway_port;
   char *_forward_host;
   int  _forward_port;
};

/*
 * Flags for use in csp->flags
 */

/**
 * Flag for csp->flags: Set if this client is processing data.
 * Cleared when the thread associated with this structure dies.
 */
#define CSP_FLAG_ACTIVE     0x01U

/**
 * Flag for csp->flags: Set if the server's reply is in "chunked"
 * transfer encoding
 */
#define CSP_FLAG_CHUNKED    0x02U

/**
 * Flag for csp->flags: Set if this request was enforced, although it would
 * normally have been blocked.
 */
#define CSP_FLAG_FORCED     0x04U

/**
 * Flag for csp->flags: Set if any modification to the body was done.
 */
#define CSP_FLAG_MODIFIED   0x08U

/**
 * Flag for csp->flags: Set if request was blocked.
 */
#define CSP_FLAG_REJECTED   0x10U

/**
 * Flag for csp->flags: Set if we are toggled on (FEATURE_TOGGLE).
 */
#define CSP_FLAG_TOGGLED_ON 0x20U

/**
 * Flag for csp->flags: Set if an acceptable Connection header
 * has already been set by the client.
 */
#define CSP_FLAG_CLIENT_CONNECTION_HEADER_SET  0x00000040U

/**
 * Flag for csp->flags: Set if an acceptable Connection header
 * has already been set by the server.
 */
#define CSP_FLAG_SERVER_CONNECTION_HEADER_SET  0x00000080U

/**
 * Flag for csp->flags: Signals header parsers whether they
 * are parsing server or client headers.
 */
#define CSP_FLAG_CLIENT_HEADER_PARSING_DONE    0x00000100U

/**
 * Flag for csp->flags: Set if adding the Host: header
 * isn't necessary.
 */
#define CSP_FLAG_HOST_HEADER_IS_SET            0x00000200U

/**
 * Flag for csp->flags: Set if filtering is disabled by X-Filter: No
 * XXX: As we now have tags we might as well ditch this.
 */
#define CSP_FLAG_NO_FILTERING                  0x00000400U

/**
 * Flag for csp->flags: Set the client IP has appended to
 * an already existing X-Forwarded-For header in which case
 * no new header has to be generated.
 */
#define CSP_FLAG_X_FORWARDED_FOR_APPENDED      0x00000800U

/**
 * Flag for csp->flags: Set if the server wants to keep
 * the connection alive.
 */
#define CSP_FLAG_SERVER_CONNECTION_KEEP_ALIVE  0x00001000U

#ifdef FEATURE_CONNECTION_KEEP_ALIVE

/**
 * Flag for csp->flags: Set if the server specified the
 * content length.
 */
#define CSP_FLAG_SERVER_CONTENT_LENGTH_SET     0x00002000U

#endif
   
/**
 * Flag for csp->flags: Set if we know the content lenght,
 * either because the server set it, or we figured it out
 * on our own.
 */
#define CSP_FLAG_CONTENT_LENGTH_SET            0x00004000U

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   
/**
 * Flag for csp->flags: Set if the client wants to keep
 * the connection alive.
 */
#define CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE  0x00008000U

/**
 * Flag for csp->flags: Set if we think we got the whole
 * client request and shouldn't read any additional data
 * coming from the client until the current request has
 * been dealt with.
 */
#define CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ 0x00010000U

/**
 * Flag for csp->flags: Set if the server promised us to
 * keep the connection open for a known number of seconds.
 */
#define CSP_FLAG_SERVER_KEEP_ALIVE_TIMEOUT_SET  0x00020000U

#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

/**
 * Flag for csp->flags: Set if we think we can't reuse
 * the server socket.
 */
#define CSP_FLAG_SERVER_SOCKET_TAINTED          0x00040000U

/**
 * Flag for csp->flags: Set if the Proxy-Connection header
 * is among the server headers.
 */
#define CSP_FLAG_SERVER_PROXY_CONNECTION_HEADER_SET 0x00080000U

/**
 * Flags for use in return codes of child processes
 */

/**
 * Flag for process return code: Set if exiting porcess has been toggled
 * during its lifetime.
 */
#define RC_FLAG_TOGGLED   0x10

/**
 * Flag for process return code: Set if exiting porcess has blocked its
 * request.
 */
#define RC_FLAG_BLOCKED   0x20

/**
 * Maximum number of actions/filter files.  This limit is arbitrary - it's just used
 * to size an array.
 */
//#define MAX_AF_FILES 10

/**
 * The state of a proxy processing thread.
 */
class client_state
{
 public:
   client_state();
   
   ~client_state(); // beware...

   char* execute_content_filter_plugins();
   
   void add_plugin(plugin *p);
   void add_interceptor_plugin(interceptor_plugin *ip);
   void add_action_plugin(action_plugin *ap);
   void add_filter_plugin(filter_plugin *fp);
   
   /** The proxy's configuration */
   proxy_configuration *_config;
   
   /** The actions to perform on the current request */
   current_action_spec _action;
   
   /** socket to talk to client (web browser) */
   sp_socket _cfd;
   
   /** socket to talk to server (web server or proxy) */
   sp_socket _sfd;
   
   /** current connection to the server (may go through a proxy) */
   reusable_connection _server_connection;
   
   /** Multi-purpose flag container, see CSP_FLAG_* above */
   unsigned int _flags;
   
   /** Client PC's IP address, as reported by the accept() function.
       As a string. */
   char *_ip_addr_str;
   
#ifdef HAVE_RFC2553
   /** Client PC's TCP address, as reported by the accept() function.
       As a sockaddr. */
   sockaddr_storage _tcp_addr;
#else
   /** Client PC's IP address, as reported by the accept() function.
       As a number. */
   unsigned long _ip_addr_long;
#endif /* def HAVE_RFC2553 */
   
   /** The URL that was requested */
   http_request _http;
   
   /*
    * The final forwarding settings.
    * XXX: Currently this is only used for forward-override,
    * so we can free the space in sweep.
    */
   forward_spec *_fwd;
   
   /** An I/O buffer used for buffering data read from the network */
   iob _iob;
   
   /** List of all headers for this request */
   std::list<const char*> _headers;
   
   /** List of all tags that apply to this request */
   std::list<const char*> _tags;
   
   /** MIME-Type key, see CT_* above */
   unsigned int _content_type;
   
   /** Actions files associated with this client */
   //file_list *_actions_list[MAX_AF_FILES];
   
   /** pcrs job files. */
   //file_list *_rlist[MAX_AF_FILES];

   std::set<plugin*> _plugins;
   
   std::list<interceptor_plugin*> _interceptor_plugins;
   
   std::list<action_plugin*> _action_plugins;
   
   std::list<filter_plugin*> _filter_plugins;
   
   /** Length after content modification. */
   unsigned long long _content_length;
   
//#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   /** Expected length of content after which we
    * should stop reading from the server socket.
    */
   /* XXX: is this the right location? */
   unsigned long long _expected_content_length;
//#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */
   
   /**
    * Failure reason to embedded in the CGI error page,
    * or NULL. Currently only used for socks errors.
    */
   char *_error_message;
   
   /** Next thread in linked list. Only read or modify from the main thread! */
   client_state *_next;
};

/**
 * A function to add a header
 */
typedef sp_err (*add_header_func_ptr)(client_state*);

/**
 * A function to process a header
 */
typedef sp_err (*parser_func_ptr)(client_state*, char **);

/**
 * A handler function.
 */
typedef sp_err (*const handler_func_ptr)(client_state *csp, http_response *rsp,
					 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);

/**
 * A loader function.
 */
typedef int (*loader_func_ptr)(client_state *csp);
   
/**
 * List of available CGI functions.
 */
class cgi_dispatcher
{
 public:
   cgi_dispatcher(const char *name, handler_func_ptr handler,
		  const char * description, int harmless)
     :_name(name),_handler(handler),_description(description),_harmless(harmless)
     {};
   
   ~cgi_dispatcher() {}; // TODO.
   
   /** The URL of the CGI, relative to the CGI root. */
   const char * const _name;
   
   /** The handler function for the CGI */
   const handler_func_ptr _handler;
   /* sp_err(* const _handler)(client_state *csp, http_response *rsp, 
			   std::multimap<const char*, const char*, eqstr> *parameters);*/
   
   /** The description of the CGI, to appear on the main menu, or NULL to hide it. */
   const char * const _description;
   
   /** A flag that indicates whether unintentional calls to this CGI can cause damage */
   int _harmless;
};

/**
 * A data file used by the Seeks proxy.  Kept in a linked list.
 */
class file_list
{
 public:
   file_list()
     :_f(NULL),_unloader(NULL),_active(0),
      _lastmodified(0),_filename(NULL),_next(NULL)
     {};
   
   ~file_list() {};
   
   /**
    * This is a pointer to the data structures associated with the file.
    * Read-only once the structure has been created.
    */
   void *_f;
   
   /**
    * The unloader function.
    * Normally NULL.  When we are finished with file (i.e. when we have
    * loaded a new one), set to a pointer to an unloader function.
    * Unloader will be called by sweep() (called from main loop) when
    * all clients using this file are done.  This prevents threading
    * problems.
    */
   void (*_unloader)(void *);
   
   /**
    * Used internally by sweep().  Do not access from elsewhere.
    */
   int _active;
   
   /**
    * File last-modified time, so we can check if file has been changed.
    * Read-only once the structure has been created.
    */
   time_t _lastmodified;
   
   /**
    * The full filename.
    */
   char *_filename;
   
   /**
    * Pointer to next entry in the linked list of all "file_list"s.
    * This linked list is so that sweep() can navigate it.
    * Since sweep() can remove items from the list, we must be careful
    * to only access this value from main thread (when we know sweep
    * won't be running).
    */
   file_list *_next;
};

#define SOCKS_NONE    0    /**< Don't use a SOCKS server               */
#define SOCKS_4      40    /**< original SOCKS 4 protocol              */
#define SOCKS_4A     41    /**< as modified for hosts w/o external DNS */
#define SOCKS_5      50    /**< as modified for hosts w/o external DNS */


/**
 * How to forward a connection to a parent proxy.
 */
class forward_spec
{
 public:
   forward_spec()
     :_url(),_type(0),_gateway_host(NULL),
      _gateway_port(0),_forward_host(NULL),_forward_port(0),
      _next(NULL)
     {};
   
   ~forward_spec();
   
   /** URL pattern that this forward_spec is for. */
   url_spec *_url;
   
   /** Connection type.  Must be SOCKS_NONE, SOCKS_4, SOCKS_4A or SOCKS_5. */
   int _type;
   
   /** SOCKS server hostname.  Only valid if "type" is SOCKS_4 or SOCKS_4A. */
   char *_gateway_host;
   
   /** SOCKS server port. */
   int _gateway_port;
   
   /** Parent HTTP proxy hostname, or NULL for none. */
   char *_forward_host;
   
   /** Parent HTTP proxy port. */
   int _forward_port;
   
   /** Next entry in the linked list. */
   forward_spec *_next;
};

/* Supported filter types */
#define FT_CONTENT_FILTER       0
#define FT_CLIENT_HEADER_FILTER 1
#define FT_SERVER_HEADER_FILTER 2
#define FT_CLIENT_HEADER_TAGGER 3
#define FT_SERVER_HEADER_TAGGER 4

#define MAX_FILTER_TYPES        5

/**
 * This class represents one filter (one block) from
 * the re_filterfile. If there is more than one filter
 * in the file, the file will be represented by a
 * chained list of re_filterfile specs.
 */
class re_filterfile_spec
{
 public:
   re_filterfile_spec()
     :_name(NULL),_description(NULL),_joblist(NULL),
      _type(0),_dynamic(0),_next(NULL)
     {};
   
   ~re_filterfile_spec();
   
   char *_name;                      /**< Name from FILTER: statement in re_filterfile. */
   char *_description;               /**< Description from FILTER: statement in re_filterfile. */
   std::list<const char*> _patterns;         /**< The patterns from the re_filterfile. */
   pcrs_job *_joblist;               /**< The resulting compiled pcrs_jobs. */
   int _type;                        /**< Filter type (content, client-header, server-header). */
   int _dynamic;                     /**< Set to one if the pattern might contain variables
				          and has to be recompiled for every request. */
   re_filterfile_spec *_next; /**< The pointer for chaining. */
};


#ifdef FEATURE_ACL

#define ACL_PERMIT   1  /**< Accept connection request */
#define ACL_DENY     2  /**< Reject connection request */

/**
 * An IP address pattern.  Used to specify networks in the ACL.
 */
class access_control_addr
{
 public:
   access_control_addr() {};
   
   ~access_control_addr() {};
   
#ifdef HAVE_RFC2553
   sockaddr_storage _addr; /* <The TCP address in network order. */
   sockaddr_storage _mask; /* <The TCP mask in network order. */
#else
   unsigned long _addr;  /**< The IP address as an integer. */
   unsigned long _mask;  /**< The network mask as an integer. */
   unsigned long _port;  /**< The port number. */
#endif /* HAVE_RFC2553 */
};

/**
 * An access control list (ACL) entry.
 *
 * This is a linked list.
 */
class access_control_list
{
 public:
   // TODO: constructor.
   
   ~access_control_list() {};
   
   access_control_addr _src;  /**< Client IP address */
   access_control_addr _dst;  /**< Website or parent proxy IP address */
#ifdef HAVE_RFC2553
   int _wildcard_dst;            /** < dst address is wildcard */
#endif
   
   short _action;                /**< ACL_PERMIT or ACL_DENY */
   access_control_list *_next;   /**< The next entry in the ACL. */
};

#endif /* def FEATURE_ACL */

// deprecated...
/** Maximum number of loaders (actions, re_filter, ...) */
#define NLOADERS 8

/** Calculates the number of elements in an array, using sizeof. */
#define SZ(X)  (sizeof(X) / sizeof(*X))

#ifdef FEATURE_FORCE_LOAD
/** The force load URL prefix. */
#define FORCE_PREFIX "/SEEKS-PROXY-FORCE"
#endif /* def FEATURE_FORCE_LOAD */

#ifdef FEATURE_NO_GIFS
/** The MIME type for images ("image/png" or "image/gif"). */
#define BUILTIN_IMAGE_MIMETYPE "image/png"
#else
#define BUILTIN_IMAGE_MIMETYPE "image/gif"
#endif /* def FEATURE_NO_GIFS */

/*
 * Hardwired URLs
 */

/** URL for the Seeks home page. */
#define HOME_PAGE_URL     "http://www.seeks-project.info/"

/** URL for the Seeks user manual. */
#define USER_MANUAL_URL   HOME_PAGE_URL VERSION "/user-manual/"

/** Prefix for actions help links  (append to USER_MANUAL_URL). */
#define ACTIONS_HELP_PREFIX "actions-file.html#"

/** Prefix for config option help links (append to USER_MANUAL_URL). */
#define CONFIG_HELP_PREFIX  "config.html#"

/*
 * The "hosts" to intercept and display CGI pages.
 * First one is a hostname only, second one can specify host and path.
 *
 * Notes:
 * 1) Do not specify the http: prefix
 * 2) CGI_SITE_2_PATH must not end with /, one will be added automatically.
 * 3) CGI_SITE_2_PATH must start with /, unless it is the empty string.
 */
#define CGI_SITE_1_HOST "s.s"
#define CGI_SITE_2_HOST "config.seeks.info"
#define CGI_SITE_2_PATH ""  // repository to serve files from.

#define CGI_SITE_FILE_SERVER "public"
#define CGI_SITE_PLUGIN_FILE_SERVER "plugins"
   
/**
 * The prefix for CGI pages.  Written out in generated HTML.
 * INCLUDES the trailing slash.
 */
//#define CGI_PREFIX  "http://" CGI_SITE_2_HOST CGI_SITE_2_PATH "/"
#define CGI_PREFIX "http://s.s/"
   
} /* end of namespace */

#endif 

