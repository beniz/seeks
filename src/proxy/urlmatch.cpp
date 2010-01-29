/*********************************************************************
 * Purpose     :  Declares functions to match URLs against URL
 *                patterns.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001-2009
 *                the Privoxy team. http://www.privoxy.org/
 *
 *                Based on the Internet Junkbuster originally written
 *                by and Copyright (C) 1997 Anonymous Coders and
 *                Junkbusters Corporation.  http://www.junkbusters.com
 *
 *                This program is free software; you can redistribute it
 *                and/or modify it under the terms of the GNU General
 *                Public License as published by the Free Software
 *                Foundation; either version 2 of the License, or (at
 *                your option) any later version.
 *
 *                This program is distributed in the hope that it will
 *                be useful, but WITHOUT ANY WARRANTY; without even the
 *                implied warranty of MERCHANTABILITY or FITNESS FOR A
 *                PARTICULAR PURPOSE.  See the GNU General Public
 *                License for more details.
 *
 *                The GNU General Public License should be included with
 *                this file.  If not, you can view it at
 *                http://www.gnu.org/copyleft/gpl.html
 *                or write to the Free Software Foundation, Inc., 59
 *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *********************************************************************/

#include "config.h"

#ifndef _WIN32
#include <stdio.h>
#include <sys/types.h>
#endif

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#if !defined(_WIN32) && !defined(__OS2__)
#include <unistd.h>
#endif

#include "mem_utils.h"
#include "urlmatch.h"
#include "miscutil.h"
#include "errlog.h"

#include <iostream>

namespace sp
{

#ifndef FEATURE_EXTENDED_HOST_PATTERNS
/*********************************************************************
 *
 * Function    :  init_domain_components
 *
 * Description :  Splits the domain name so we can compare it
 *                against wildcards. It used to be part of
 *                parse_http_url, but was separated because the
 *                same code is required in chat in case of
 *                intercepted requests.
 *
 * Parameters  :
 *          1  :  http = pointer to the http class to hold elements.
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out of memory
 *                SP_ERR_PARSE on malformed command/URL
 *                             or >100 domains deep.
 *
 *********************************************************************/
sp_err urlmatch::init_domain_components(http_request *http)
{
   char *vec[BUFFER_SIZE];
   size_t size;
   char *p;

   http->_dbuffer = strdup(http->_host);
   if (NULL == http->_dbuffer)
   {
      return SP_ERR_MEMORY;
   }

   /* map to lower case */
   for (p = http->_dbuffer; *p ; p++)
   {
      *p = (char)tolower((int)(unsigned char)*p);
   }

   /* split the domain name into components */
   http->_dcount = miscutil::ssplit(http->_dbuffer, ".", vec, SZ(vec), 1, 1);

   if (http->_dcount <= 0)
   {
      /*
       * Error: More than SZ(vec) components in domain
       *    or: no components in domain
       */
      errlog::log_error(LOG_LEVEL_ERROR, "More than SZ(vec) components in domain or none at all.");
      return SP_ERR_PARSE;
   }

   /* save a copy of the pointers in dvec */
   size = (size_t)http->_dcount * sizeof(*http->_dvec);

   http->_dvec = (char**) zalloc(size);
   if (NULL == http->_dvec)
   {
      return SP_ERR_MEMORY;
   }

   memcpy(http->_dvec, vec, size);

   return SP_ERR_OK;
}
#endif /* ndef FEATURE_EXTENDED_HOST_PATTERNS */


/*********************************************************************
 *
 * Function    :  parse_http_url
 *
 * Description :  Parse out the host and port from the URL.  Find the
 *                hostname & path, port (if ':'), and/or password (if '@')
 *
 * Parameters  :
 *          1  :  url = URL (or is it URI?) to break down
 *          2  :  http = pointer to the http class to hold elements.
 *                       Must be initialized with valid values (like NULLs).
 *          3  :  require_protocol = Whether or not URLs without
 *                                   protocol are acceptable.
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out of memory
 *                SP_ERR_PARSE on malformed command/URL
 *                             or >100 domains deep.
 *
 *********************************************************************/
sp_err urlmatch::parse_http_url(const char *url, http_request *http, int require_protocol)
{
   int host_available = 1; /* A proxy can dream. */

   /*
    * Save our initial URL
    */
   http->_url = strdup(url);
   if (http->_url == NULL)
   {
      return SP_ERR_MEMORY;
   }

   /*
    * Check for * URI. If found, we're done.
    */  
   if (*http->_url == '*')
   {
      if  ( NULL == (http->_path = strdup("*"))
         || NULL == (http->_hostport = strdup("")) ) 
      {
         return SP_ERR_MEMORY;
      }
      if (http->_url[1] != '\0')
      {
         return SP_ERR_PARSE;
      }
      return SP_ERR_OK;
   }

   /*
    * Split URL into protocol,hostport,path.
    */
   {
      char *buf;
      char *url_noproto;
      char *url_path;

      buf = strdup(url);
      if (buf == NULL)
      {
         return SP_ERR_MEMORY;
      }
      
      /* Find the start of the URL in our scratch space */
      url_noproto = buf;
      if (miscutil::strncmpic(url_noproto, "http://",  7) == 0)
      {
         url_noproto += 7;
      }
      else if (miscutil::strncmpic(url_noproto, "https://", 8) == 0)
      {
         /*
          * Should only happen when called from cgi_show_url_info().
          */
         url_noproto += 8;
         http->_ssl = 1;
      }
      else if (*url_noproto == '/')
      {
        /*
         * Short request line without protocol and host.
         * Most likely because the client's request
         * was intercepted and redirected into the proxy.
         */
         http->_host = NULL;
         host_available = 0;
      }
      else if (require_protocol)
      {
         freez(buf);
	 buf = NULL;
         return SP_ERR_PARSE;
      }

      url_path = strchr(url_noproto, '/');
      if (url_path != NULL)
      {
         /*
          * Got a path.
          *
          * NOTE: The following line ignores the path for HTTPS URLS.
          * This means that you get consistent behaviour if you type a
          * https URL in and it's parsed by the function.  (When the
          * URL is actually retrieved, SSL hides the path part).
          */
         http->_path = strdup(http->_ssl ? "/" : url_path);
         *url_path = '\0';
         http->_hostport = strdup(url_noproto);
      }
      else
      {
         /*
          * Repair broken HTTP requests that don't contain a path,
          * or CONNECT requests
          */
         http->_path = strdup("/");
         http->_hostport = strdup(url_noproto);
      }

      freez(buf);

      if ( (http->_path == NULL)
        || (http->_hostport == NULL))
      {
         return SP_ERR_MEMORY;
      }
   }

   if (!host_available)
   {
      /* Without host, there is nothing left to do here */
      return SP_ERR_OK;
   }

   /*
    * Split hostport into user/password (ignored), host, port.
    */
   {
      char *buf;
      char *host;
      char *port;

      buf = strdup(http->_hostport);
      if (buf == NULL)
      {
         return SP_ERR_MEMORY;
      }

      /* check if url contains username and/or password */
      host = strchr(buf, '@');
      if (host != NULL)
      {
         /* Contains username/password, skip it and the @ sign. */
         host++;
      }
      else
      {
         /* No username or password. */
         host = buf;
      }

      /* Move after hostname before port number */
      if (*host == '[')
      {
         /* Numeric IPv6 address delimited by brackets */
         host++;
         port = strchr(host, ']');

         if (port == NULL)
         {
            /* Missing closing bracket */
            freez(buf);
            return SP_ERR_PARSE;
         }

         *port++ = '\0';

         if (*port == '\0')
         {
            port = NULL;
         }
         else if (*port != ':')
         {
            /* Garbage after closing bracket */
            freez(buf);
	    buf = NULL;
            return SP_ERR_PARSE;
         }
      }
      else
      {
         /* Plain non-escaped hostname */
         port = strchr(host, ':');
      }

      /* check if url contains port */
      if (port != NULL)
      {
         /* Contains port */
         /* Terminate hostname and point to start of port string */
         *port++ = '\0';
         http->_port = atoi(port);
      }
      else
      {
         /* No port specified. */
         http->_port = (http->_ssl ? 443 : 80);
      }

      http->_host = strdup(host);

      freez(buf);

      if (http->_host == NULL)
      {
         return SP_ERR_MEMORY;
      }
   }

#ifdef FEATURE_EXTENDED_HOST_PATTERNS
   return SP_ERR_OK;
#else
   /* Split domain name so we can compare it against wildcards */
   return urlmatch::init_domain_components(http);
#endif /* def FEATURE_EXTENDED_HOST_PATTERNS */
}

void urlmatch::parse_url_host_and_path(const std::string &url,
				       std::string &host, std::string &path)
{
   size_t p1 = 0;
   if ((p1=url.find("http://"))!=std::string::npos)
     p1 += 7;
   else if ((p1=url.find("https://"))!=std::string::npos)
     p1 += 8;
   size_t p2 = 0;
   if ((p2 = url.find("/",p1))!=std::string::npos)
     {
	host  = url.substr(p1,p2-p1);
	path = url.substr(p2);
     }
   else 
     {
	host = url.substr(p1);
	path = "";
     }
}
   
/*********************************************************************
 *
 * Function    :  unknown_method
 *
 * Description :  Checks whether a method is unknown.
 *
 * Parameters  :
 *          1  :  method = points to a http method
 *
 * Returns     :  TRUE if it's unknown, FALSE otherwise.
 *
 *********************************************************************/
int urlmatch::unknown_method(const char *method)
{
   static const char *known_http_methods[] = {
      /* Basic HTTP request type */
      "GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS", "TRACE", "CONNECT",
      /* webDAV extensions (RFC2518) */
      "PROPFIND", "PROPPATCH", "MOVE", "COPY", "MKCOL", "LOCK", "UNLOCK",
      /*
       * Microsoft webDAV extension for Exchange 2000.  See:
       * http://lists.w3.org/Archives/Public/w3c-dist-auth/2002JanMar/0001.html
       * http://msdn.microsoft.com/library/en-us/wss/wss/_webdav_methods.asp
       */ 
      "BCOPY", "BMOVE", "BDELETE", "BPROPFIND", "BPROPPATCH",
      /*
       * Another Microsoft webDAV extension for Exchange 2000.  See:
       * http://systems.cs.colorado.edu/grunwald/MobileComputing/Papers/draft-cohen-gena-p-base-00.txt
       * http://lists.w3.org/Archives/Public/w3c-dist-auth/2002JanMar/0001.html
       * http://msdn.microsoft.com/library/en-us/wss/wss/_webdav_methods.asp
       */ 
      "SUBSCRIBE", "UNSUBSCRIBE", "NOTIFY", "POLL",
      /*
       * Yet another WebDAV extension, this time for
       * Web Distributed Authoring and Versioning (RFC3253)
       */
      "VERSION-CONTROL", "REPORT", "CHECKOUT", "CHECKIN", "UNCHECKOUT",
      "MKWORKSPACE", "UPDATE", "LABEL", "MERGE", "BASELINE-CONTROL", "MKACTIVITY",
   };

   for (size_t i = 0; i < SZ(known_http_methods); i++)
   {
      if (0 == miscutil::strcmpic(method, known_http_methods[i]))
      {
         return FALSE;
      }
   }

   return TRUE;
}


/*********************************************************************
 *
 * Function    :  parse_http_request
 *
 * Description :  Parse out the host and port from the URL.  Find the
 *                hostname & path, port (if ':'), and/or password (if '@')
 *
 * Parameters  :
 *          1  :  req = HTTP request line to break down
 *          2  :  http = pointer to the http class to hold elements
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out of memory
 *                SP_ERR_CGI_PARAMS on malformed command/URL
 *                                  or >100 domains deep.
 *
 *********************************************************************/
sp_err urlmatch::parse_http_request(const char *req, http_request *&http)
{
   char *buf;
   char *v[10]; /* XXX: Why 10? We should only need three. */
   int n;
   sp_err err;

   buf = strdup(req);
   if (buf == NULL)
   {
      return SP_ERR_MEMORY;
   }

   n = miscutil::ssplit(buf, " \r\n", v, SZ(v), 1, 1);
   if (n != 3)
   {
      freez(buf);
      buf = NULL;
      return SP_ERR_PARSE;
   }

   /*
    * Fail in case of unknown methods
    * which we might not handle correctly.
    *
    * XXX: There should be a config option
    * to forward requests with unknown methods
    * anyway. Most of them don't need special
    * steps.
    */
   if (urlmatch::unknown_method(v[0]))
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Unknown HTTP method detected: %s", v[0]);
      freez(buf);
      buf = NULL;
      return SP_ERR_PARSE;
   }

   if (miscutil::strcmpic(v[2], "HTTP/1.1") && miscutil::strcmpic(v[2], "HTTP/1.0"))
   {
      errlog::log_error(LOG_LEVEL_ERROR, "The only supported HTTP "
         "versions are 1.0 and 1.1. This rules out: %s", v[2]);
      freez(buf);
      buf = NULL;
      return SP_ERR_PARSE;
   }

   http->_ssl = !miscutil::strcmpic(v[0], "CONNECT");

   err = urlmatch::parse_http_url(v[1], http, !http->_ssl);
   if (err)
   {
      freez(buf);
      buf = NULL;
      return err;
   }

   /*
    * Copy the details into the class
    */
   http->_cmd = strdup(req);
   http->_gpc = strdup(v[0]);
   http->_ver = strdup(v[2]);

   freez(buf);
   buf = NULL;

   if ( (http->_cmd == NULL)
     || (http->_gpc == NULL)
     || (http->_ver == NULL) )
   {
      return SP_ERR_MEMORY;
   }

   //std::cerr << "parse_http_request: cmd: " << http->_cmd << std::endl;
   
   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  compile_pattern
 *
 * Description :  Compiles a host, domain or TAG pattern.
 *
 * Parameters  :
 *          1  :  pattern = The pattern to compile.
 *          2  :  anchoring = How the regex should be modified
 *                            before compilation. Can be either
 *                            one of NO_ANCHORING, LEFT_ANCHORED,
 *                            RIGHT_ANCHORED or RIGHT_ANCHORED_HOST.
 *          3  :  url     = In case of failures, the spec member is
 *                          logged and the class freed.
 *          4  :  regex   = Where the compiled regex should be stored.
 *
 * Returns     :  SP_ERR_OK - Success
 *                SP_ERR_MEMORY - Out of memory
 *                SP_ERR_PARSE - Cannot parse regex
 *
 *********************************************************************/
sp_err urlmatch::compile_pattern(const char *pattern, enum regex_anchoring anchoring,
				 url_spec *url, regex_t **regex)
{
   int errcode;
   char rebuf[BUFFER_SIZE];
   const char *fmt = NULL;

   assert(pattern);
   assert(strlen(pattern) < sizeof(rebuf) - 2);

   if (pattern[0] == '\0')
   {
      *regex = NULL;
      return SP_ERR_OK;
   }

   switch (anchoring)
   {
      case NO_ANCHORING:
         fmt = "%s";
         break;
      case RIGHT_ANCHORED:
         fmt = "%s$";
         break;
      case RIGHT_ANCHORED_HOST:
         fmt = "%s\\.?$";
         break;
      case LEFT_ANCHORED:
         fmt = "^%s";
         break;
      default:
         errlog::log_error(LOG_LEVEL_FATAL,
            "Invalid anchoring in compile_pattern %d", anchoring);
   }

   *regex = (regex_t*)calloc(1,sizeof(**regex));
   if (NULL == *regex)
   {
      delete url;
      return SP_ERR_MEMORY;
   }

   snprintf(rebuf, sizeof(rebuf), fmt, pattern);

   errcode = regcomp(*regex, rebuf, (REG_EXTENDED|REG_NOSUB|REG_ICASE));

   if (errcode)
   {
      size_t errlen = regerror(errcode, *regex, rebuf, sizeof(rebuf));
      if (errlen > (sizeof(rebuf) - (size_t)1))
      {
         errlen = sizeof(rebuf) - (size_t)1;
      }
      rebuf[errlen] = '\0';
      errlog::log_error(LOG_LEVEL_ERROR, "error compiling %s from %s: %s",
			pattern, url->_spec, rebuf);
      delete url;

      return SP_ERR_PARSE;
   }

   return SP_ERR_OK;

}


/*********************************************************************
 *
 * Function    :  compile_url_pattern
 *
 * Description :  Compiles the three parts of an URL pattern.
 *
 * Parameters  :
 *          1  :  url = Target url_spec to be filled in.
 *          2  :  buf = The url pattern to compile. Will be messed up.
 *
 * Returns     :  SP_ERR_OK - Success
 *                SP_ERR_MEMORY - Out of memory
 *                SP_ERR_PARSE - Cannot parse regex
 *
 *********************************************************************/
sp_err urlmatch::compile_url_pattern(url_spec *url, char *buf)
{
   char *p = strchr(buf, '/');
      
   if (NULL != p)
   {
      /*
       * Only compile the regex if it consists of more than
       * a single slash, otherwise it wouldn't affect the result.
       */
      if (p[1] != '\0')
      {
         /*
          * XXX: does it make sense to compile the slash at the beginning?
          */
         sp_err err = urlmatch::compile_pattern(p, LEFT_ANCHORED, url, &url->_preg);

         if (SP_ERR_OK != err)
         {
            return err;
         }
      }
      *p = '\0';
   }

   /*
    * IPv6 numeric hostnames can contain colons, thus we need
    * to delimit the hostname before the real port separator.
    * As brackets are already used in the hostname pattern,
    * we use angle brackets ('<', '>') instead.
    */
   if ((buf[0] == '<') && (NULL != (p = strchr(buf + 1, '>'))))
   {
      *p++ = '\0';
      buf++;

      if (*p == '\0')
      {
         /* IPv6 address without port number */
         p = NULL;
      }
      else if (*p != ':')
      {
         /* Garbage after address delimiter */
         return SP_ERR_PARSE;
      }
   }
   else
   {
      p = strchr(buf, ':');
   }

   if (NULL != p)
   {
      *p++ = '\0';
      url->_port_list = strdup(p);
      if (NULL == url->_port_list)
      {
         return SP_ERR_MEMORY;
      }
   }
   else
   {
      url->_port_list = NULL;
   }

   if (buf[0] != '\0')
   {
      return urlmatch::compile_host_pattern(url, buf);
   }

   return SP_ERR_OK;
}


#ifdef FEATURE_EXTENDED_HOST_PATTERNS
/*********************************************************************
 *
 * Function    :  compile_host_pattern
 *
 * Description :  Parses and compiles a host pattern..
 *
 * Parameters  :
 *          1  :  url = Target url_spec to be filled in.
 *          2  :  host_pattern = Host pattern to compile.
 *
 * Returns     :  SP_ERR_OK - Success
 *                SP_ERR_MEMORY - Out of memory
 *                SP_ERR_PARSE - Cannot parse regex
 *
 *********************************************************************/
sp_err urlmatch::compile_host_pattern(url_spec *url, const char *host_pattern)
{
   return urlmatch::compile_pattern(host_pattern, RIGHT_ANCHORED_HOST, url, &url->_host_regex);
}

#else

/*********************************************************************
 *
 * Function    :  compile_host_pattern
 *
 * Description :  Parses and "compiles" an old-school host pattern.
 *
 * Parameters  :
 *          1  :  url = Target url_spec to be filled in.
 *          2  :  host_pattern = Host pattern to parse.
 *
 * Returns     :  SP_ERR_OK - Success
 *                SP_ERR_MEMORY - Out of memory
 *                SP_ERR_PARSE - Cannot parse regex
 *
 *********************************************************************/
sp_err urlmatch::compile_host_pattern(url_spec *url, const char *host_pattern)
{
   char *v[150];
   size_t size;
   char *p;

   /*
    * Parse domain part
    */
   if (host_pattern[strlen(host_pattern) - 1] == '.')
   {
      url->_unanchored |= ANCHOR_RIGHT;
   }
   if (host_pattern[0] == '.')
   {
      url->_unanchored |= ANCHOR_LEFT;
   }

   /* 
    * Split domain into components
    */
   url->_dbuffer = strdup(host_pattern);
   if (NULL == url->_dbuffer)
   {
      delete url;
      return SP_ERR_MEMORY;
   }

   /* 
    * Map to lower case
    */
   for (p = url->_dbuffer; *p ; p++)
   {
      *p = (char)tolower((int)(unsigned char)*p);
   }

   /* 
    * Split the domain name into components
    */
   url->_dcount = miscutil::ssplit(url->_dbuffer, ".", v, SZ(v), 1, 1);
   
   if (url->_dcount < 0)
   {
      delete url;
      return SP_ERR_MEMORY;
   }
   else if (url->_dcount != 0)
   {
      /* 
       * Save a copy of the pointers in dvec
       */
      size = (size_t)url->_dcount * sizeof(*url->_dvec);
      
      url->_dvec = (char **)malloc(size);
      if (NULL == url->_dvec)
      {
         delete url;
         return SP_ERR_MEMORY;
      }

      memcpy(url->_dvec, v, size);
   }
   /*
    * else dcount == 0 in which case we needn't do anything,
    * since dvec will never be accessed and the pattern will
    * match all domains.
    */
   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  simplematch
 *
 * Description :  String matching, with a (greedy) '*' wildcard that
 *                stands for zero or more arbitrary characters and
 *                character classes in [], which take both enumerations
 *                and ranges.
 *
 * Parameters  :
 *          1  :  pattern = pattern for matching
 *          2  :  text    = text to be matched
 *
 * Returns     :  0 if match, else nonzero
 *
 *********************************************************************/
int urlmatch::simplematch(const char *pattern, const char *text)
{
   const unsigned char *pat = (const unsigned char *)pattern;
   const unsigned char *txt = (const unsigned char *)text;
   const unsigned char *fallback = pat; 
   int wildcard = 0;
  
   unsigned char lastchar = 'a';
   unsigned i;
   unsigned char charmap[32];
  
   while (*txt)
   {

      /* EOF pattern but !EOF text? */
      if (*pat == '\0')
      {
         if (wildcard)
         {
            pat = fallback;
         }
         else
         {
            return 1;
         }
      }

      /* '*' in the pattern?  */
      if (*pat == '*') 
      {
     
         /* The pattern ends afterwards? Speed up the return. */
         if (*++pat == '\0')
         {
            return 0;
         }
     
         /* Else, set wildcard mode and remember position after '*' */
         wildcard = 1;
         fallback = pat;
      }

      /* Character range specification? */
      if (*pat == '[')
      {
         memset(charmap, '\0', sizeof(charmap));

         while (*++pat != ']')
         {
            if (!*pat)
            { 
               return 1;
            }
            else if (*pat == '-')
            {
               if ((*++pat == ']') || *pat == '\0')
               {
                  return(1);
               }
               for (i = lastchar; i <= *pat; i++)
               {
                  charmap[i / 8] |= (unsigned char)(1 << (i % 8));
               } 
            }
            else
            {
               charmap[*pat / 8] |= (unsigned char)(1 << (*pat % 8));
               lastchar = *pat;
            }
         }
      } /* -END- if Character range specification */


      /* 
       * Char match, or char range match? 
       */
      if ( (*pat == *txt)
      ||   (*pat == '?')
      ||   ((*pat == ']') && (charmap[*txt / 8] & (1 << (*txt % 8)))) )
      {
         /* 
          * Sucess: Go ahead
          */
         pat++;
      }
      else if (!wildcard)
      {
         /* 
          * No match && no wildcard: No luck
          */
         return 1;
      }
      else if (pat != fallback)
      {
         /*
          * Increment text pointer if in char range matching
          */
         if (*pat == ']')
         {
            txt++;
         }
         /*
          * Wildcard mode && nonmatch beyond fallback: Rewind pattern
          */
         pat = fallback;
         /*
          * Restart matching from current text pointer
          */
         continue;
      }
      txt++;
   }

   /* Cut off extra '*'s */
   if(*pat == '*')  pat++;

   /* If this is the pattern's end, fine! */
   return(*pat);

}


/*********************************************************************
 *
 * Function    :  simple_domaincmp
 *
 * Description :  Domain-wise Compare fqdn's.  The comparison is
 *                both left- and right-anchored.  The individual
 *                domain names are compared with simplematch().
 *                This is only used by domain_match.
 *
 * Parameters  :
 *          1  :  pv = array of patterns to compare
 *          2  :  fv = array of domain components to compare
 *          3  :  len = length of the arrays (both arrays are the
 *                      same length - if they weren't, it couldn't
 *                      possibly be a match).
 *
 * Returns     :  0 => domains are equivalent, else no match.
 *
 *********************************************************************/
int urlmatch::simple_domaincmp(char **pv, char **fv, int len)
{
   int n;

   for (n = 0; n < len; n++)
   {
      if (urlmatch::simplematch(pv[n], fv[n]))
      {
         return 1;
      }
   }

   return 0;
}


/*********************************************************************
 *
 * Function    :  domain_match
 *
 * Description :  Domain-wise Compare fqdn's. Governed by the bimap in
 *                pattern->unachored, the comparison is un-, left-,
 *                right-anchored, or both.
 *                The individual domain names are compared with
 *                simplematch().
 *
 * Parameters  :
 *          1  :  pattern = a domain that may contain a '*' as a wildcard.
 *          2  :  fqdn = domain name against which the patterns are compared.
 *
 * Returns     :  0 => domains are equivalent, else no match.
 *
 *********************************************************************/
int urlmatch::domain_match(const url_spec *pattern, const http_request *fqdn)
{
   char **pv, **fv;  /* vectors  */
   int    plen, flen;
   int unanchored = pattern->_unanchored & (ANCHOR_RIGHT | ANCHOR_LEFT);

   plen = pattern->_dcount;
   flen = fqdn->_dcount;

   if (flen < plen)
   {
      /* fqdn is too short to match this pattern */
      return 1;
   }

   pv   = pattern->_dvec;
   fv   = fqdn->_dvec;

   if (unanchored == ANCHOR_LEFT)
   {
      /*
       * Right anchored.
       *
       * Convert this into a fully anchored pattern with
       * the fqdn and pattern the same length
       */
      fv += (flen - plen); /* flen - plen >= 0 due to check above */
      return urlmatch::simple_domaincmp(pv, fv, plen);
   }
   else if (unanchored == 0)
   {
      /* Fully anchored, check length */
      if (flen != plen)
      {
         return 1;
      }
      return urlmatch::simple_domaincmp(pv, fv, plen);
   }
   else if (unanchored == ANCHOR_RIGHT)
   {
      /* Left anchored, ignore all extra in fqdn */
      return urlmatch::simple_domaincmp(pv, fv, plen);
   }
   else
   {
      /* Unanchored */
      int n;
      int maxn = flen - plen;
      for (n = 0; n <= maxn; n++)
      {
         if (!urlmatch::simple_domaincmp(pv, fv, plen))
         {
            return 0;
         }
         /*
          * Doesn't match from start of fqdn
          * Try skipping first part of fqdn
          */
         fv++;
      }
      return 1;
   }
}
#endif /* def FEATURE_EXTENDED_HOST_PATTERNS */

   
/*********************************************************************
 *
 * Function    :  port_matches
 *
 * Description :  Compares a port against a port list.
 *
 * Parameters  :
 *          1  :  port      = The port to check.
 *          2  :  port_list = The list of port to compare with.
 *
 * Returns     :  TRUE for yes, FALSE otherwise.
 *
 *********************************************************************/
int urlmatch::port_matches(const int port, const char *port_list)
{
   return ((NULL == port_list) || urlmatch::match_portlist(port_list, port));
}


/*********************************************************************
 *
 * Function    :  host_matches
 *
 * Description :  Compares a host against a host pattern.
 *
 * Parameters  :
 *          1  :  url = The URL to match
 *          2  :  pattern = The URL pattern
 *
 * Returns     :  TRUE for yes, FALSE otherwise.
 *
 *********************************************************************/
int urlmatch::host_matches(const http_request *http,
			   const url_spec *pattern)
{
#ifdef FEATURE_EXTENDED_HOST_PATTERNS
   return ((NULL == pattern->_host_regex)
      || (0 == regexec(pattern->_host_regex, http->_host, 0, NULL, 0)));
#else
   int check = ((NULL == pattern->_dbuffer) || (0 == urlmatch::domain_match(pattern, http)));
   return check;
#endif
}

/*********************************************************************
 *
 * Function    :  path_matches
 *
 * Description :  Compares a path against a path pattern.
 *
 * Parameters  :
 *          1  :  path = The path to match
 *          2  :  pattern = The URL pattern
 *
 * Returns     :  TRUE for yes, FALSE otherwise.
 *
 *********************************************************************/
int urlmatch::path_matches(const char *path, const url_spec *pattern)
{
   return ((NULL == pattern->_preg)
      || (0 == regexec(pattern->_preg, path, 0, NULL, 0)));
}


/*********************************************************************
 *
 * Function    :  url_match
 *
 * Description :  Compare a URL against a URL pattern.
 *
 * Parameters  :
 *          1  :  pattern = a URL pattern
 *          2  :  url = URL to match
 *
 * Returns     :  Nonzero if the URL matches the pattern, else 0.
 *
 *********************************************************************/
int urlmatch::url_match(const url_spec *pattern,
			const http_request *http)
{
   if (pattern->_tag_regex != NULL)
   {
      /* It's a tag pattern and shouldn't be matched against URLs */
      return 0;
   } 

   int port_match = urlmatch::port_matches(http->_port, pattern->_port_list);
   int host_match = urlmatch::host_matches(http, pattern);
   int path_match = urlmatch::path_matches(http->_path, pattern);
   
   /*std::cout << "port_match: " << port_match
     << " -- host_match: " << host_match << " -- path_match: " << path_match
     << std::endl; */
   
   return (port_match && host_match && path_match);
}

/*********************************************************************
 *
 * Function    :  match_portlist
 *
 * Description :  Check if a given number is covered by a comma
 *                separated list of numbers and ranges (a,b-c,d,..)
 *
 * Parameters  :
 *          1  :  portlist = String with list
 *          2  :  port = port to check
 *
 * Returns     :  0 => no match
 *                1 => match
 *
 *********************************************************************/
int urlmatch::match_portlist(const char *portlist, int port)
{
   char *min, *max, *next, *portlist_copy;

   min = portlist_copy = strdup(portlist);

   /*
    * Zero-terminate first item and remember offset for next
    */
   if (NULL != (next = strchr(portlist_copy, (int) ',')))
   {
      *next++ = '\0';
   }

   /*
    * Loop through all items, checking for match
    */
   while (NULL != min)
   {
      if (NULL == (max = strchr(min, (int) '-')))
      {
         /*
          * No dash, check for equality
          */
         if (port == atoi(min))
         {
            freez(portlist_copy);
            return(1);
         }
      }
      else
      {
         /*
          * This is a range, so check if between min and max,
          * or, if max was omitted, between min and 65K
          */
         *max++ = '\0';
         if(port >= atoi(min) && port <= (atoi(max) ? atoi(max) : 65535))
         {
            freez(portlist_copy);
            return(1);
         }

      }

      /*
       * Jump to next item
       */
      min = next;

      /*
       * Zero-terminate next item and remember offset for n+1
       */
      if ((NULL != next) && (NULL != (next = strchr(next, (int) ','))))
      {
         *next++ = '\0';
      }
   }

   freez(portlist_copy);
   return 0;
}


/*********************************************************************
 *
 * Function    :  parse_forwarder_address
 *
 * Description :  Parse out the host and port from a forwarder address.
 *
 * Parameters  :
 *          1  :  address = The forwarder address to parse.
 *          2  :  hostname = Used to return the hostname. NULL on error.
 *          3  :  port = Used to return the port. Untouched if no port
 *                       is specified.
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out of memory
 *                SP_ERR_PARSE on malformed address.
 *
 *********************************************************************/
sp_err urlmatch::parse_forwarder_address(char *address, char **hostname, int *port)
{
   char *p = address;

   if ((*address == '[') && (NULL == strchr(address, ']')))
   {
      /* XXX: Should do some more validity checks here. */
      return SP_ERR_PARSE;
   }

   *hostname = strdup(address);
   if (NULL == *hostname)
   {
      return SP_ERR_MEMORY;
   }

   if ((**hostname == '[') && (NULL != (p = strchr(*hostname, ']'))))
   {
      *p++ = '\0';
      memmove(*hostname, (*hostname + 1), (size_t)(p - *hostname));
      if (*p == ':')
      {
         *port = (int)strtol(++p, NULL, 0);
      }
   }
   else if (NULL != (p = strchr(*hostname, ':')))
   {
      *p++ = '\0';
      *port = (int)strtol(p, NULL, 0);
   }

   return SP_ERR_OK;
}

} /* end of namespace. */
