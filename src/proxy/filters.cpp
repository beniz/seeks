/*********************************************************************
 * Purpose     :  Declares functions to parse/crunch headers and pages.
 *                Functions declared include:
 *                   `acl_addr', `add_stats', `block_acl', `block_imageurl',
 *                   `block_url', `url_actions', `domain_split',
 *                   `filter_popups', `forward_url', 'redirect_url',
 *                   `ij_untrusted_url', `intercept_url', `pcrs_filter_respose',
 *                   `ijb_send_banner', `trust_url', `gif_deanimate_response',
 *                   `execute_single_pcrs_command', `rewrite_url',
 *                   `get_last_url'
 *
 * Copyright   :   Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001, 2004-2009 the
 *                Privoxy team. http://www.privoxy.org/
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
#include "filters.h"
#include "mem_utils.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_RFC2553
#include <netdb.h>
#include <sys/socket.h>
#endif /* def HAVE_RFC2553 */

#ifndef _WIN32
#include <unistd.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif /* ndef _WIN32 */

#include "encode.h"
#include "parsers.h"
#include "errlog.h"
#include "spsockets.h"
#include "miscutil.h"
#include "cgi.h"
#include "urlmatch.h"
#include "loaders.h"
#include "proxy_configuration.h"

#ifdef _WIN32
#include "win32.h"
#endif

#include <iostream>

/* Fix a problem with Solaris.  There should be no effect on other
 * platforms.
 * Solaris's isspace() is a macro which uses it's argument directly
 * as an array index.  Therefore we need to make sure that high-bit
 * characters generate +ve values, and ideally we also want to make
 * the argument match the declared parameter type of "int".
 */
#define ijb_isdigit(__X) isdigit((int)(unsigned char)(__X))

namespace sp
{

#ifdef FEATURE_ACL
#ifdef HAVE_RFC2553
/*********************************************************************
 *
 * Function    :  sockaddr_storage_to_ip
 *
 * Description :  Access internal structure of sockaddr_storage
 *
 * Parameters  :
 *          1  :  addr = socket address
 *          2  :  ip   = IP address as array of octets in network order
 *                       (it points into addr)
 *          3  :  len  = length of IP address in octets
 *          4  :  port = port number in network order;
 *
 * Returns     :  0 = no errror; -1 otherwise.
 *
 *********************************************************************/
   int filters::sockaddr_storage_to_ip(const struct sockaddr_storage *addr,
				       uint8_t **ip, unsigned int *len,
				       in_port_t **port)
     {
	if (NULL == addr)
	  {
	     return(-1);
	  }
	
	switch (addr->ss_family)
	  {
	   case AF_INET:
	     if (NULL != len)
	       {
		  *len = 4;
	       }
	     if (NULL != ip)
	       {
		  *ip = (uint8_t *)
		    &(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	       }
	     if (NULL != port)
	       {
		  *port = &((struct sockaddr_in *)addr)->sin_port;
	       }
	     break;
	     
	   case AF_INET6:
	     if (NULL != len)
	       {
		  *len = 16;
	       }
	     if (NULL != ip)
	       {
		  *ip = ((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr;
	       }
	     if (NULL != port)
	       {
		  *port = &((struct sockaddr_in6 *)addr)->sin6_port;
	       }
	     break;
	     
	   default:
	     /* Unsupported address family */
	     return(-1);
	  }
	
	return(0);
     }
   
   
   /*********************************************************************
    *
    * Function    :  match_sockaddr
    *
    * Description :  Check whether address matches network (IP address and port)
    *
    * Parameters  :
    *          1  :  network = socket address of subnework
    *          2  :  netmask = network mask as socket address
    *          3  :  address = checked socket address against given network
    *
    * Returns     :  0 = doesn't match; 1 = does match
    *
    *********************************************************************/
   int filters::match_sockaddr(const struct sockaddr_storage *network,
			       const struct sockaddr_storage *netmask,
			       const struct sockaddr_storage *address)
     {
	uint8_t *network_addr, *netmask_addr, *address_addr;
	unsigned int addr_len;
	in_port_t *network_port, *netmask_port, *address_port;
	
	if (network->ss_family != netmask->ss_family)
	  {
	     /* This should never happen */
	     errlog::log_error(LOG_LEVEL_ERROR,
			       "Internal error at %s:%llu: network and netmask differ in family",
			       __FILE__, __LINE__);
	     return 0;
	  }
	
	filters::sockaddr_storage_to_ip(network, &network_addr, &addr_len, &network_port);
	filters::sockaddr_storage_to_ip(netmask, &netmask_addr, NULL, &netmask_port);
	filters::sockaddr_storage_to_ip(address, &address_addr, NULL, &address_port);
	
	/* Check for family */
	if ((network->ss_family == AF_INET) && (address->ss_family == AF_INET6)
	    && IN6_IS_ADDR_V4MAPPED((struct in6_addr *)address_addr))
	  {
	     /* Map AF_INET6 V4MAPPED address into AF_INET */
	     address_addr += 12;
	     addr_len = 4;
	  }
	else if ((network->ss_family == AF_INET6) && (address->ss_family == AF_INET)
		 && IN6_IS_ADDR_V4MAPPED((struct in6_addr *)network_addr))
	  {
	     /* Map AF_INET6 V4MAPPED network into AF_INET */
	     network_addr += 12;
	     netmask_addr += 12;
	     addr_len = 4;
	  }
	else if (network->ss_family != address->ss_family)
	  {
	     return 0;
	  }
	
	/* XXX: Port check is signaled in netmask */
	if (*netmask_port && *network_port != *address_port)
	  {
	     return 0;
	  }
	
	/* TODO: Optimize by checking by words insted of octets */
	for (size_t j = 0; (j < addr_len) && netmask_addr[j]; j++)
	  {
	     if ((network_addr[j] & netmask_addr[j]) !=
		 (address_addr[j] & netmask_addr[j]))
	       {
		  return 0;
	       }
	  }

	return 1;
     }
#endif /* def HAVE_RFC2553 */
   
   
   /*********************************************************************
    *
    * Function    :  block_acl
    *
    * Description :  Block this request?
    *                Decide yes or no based on ACL file.
    *
    * Parameters  :
    *          1  :  dst = The proxy or gateway address this is going to.
    *                      Or NULL to check all possible targets.
    *          2  :  csp = Current client state (buffers, headers, etc...)
    *                      Also includes the client IP address.
    *
    * Returns     : 0 = FALSE (don't block) and 1 = TRUE (do block)
    *
    *********************************************************************/
   int filters::block_acl(const access_control_addr *dst, const client_state *csp)
     {
	access_control_list *acl = csp->_config->_acl;

	/* if not using an access control list, then permit the connection */
	if (acl == NULL)
	  {
	     return(0);
	  }
	
	/* search the list */
	while (acl != NULL)
	  {
	     if (
#ifdef HAVE_RFC2553
		 filters::match_sockaddr(&acl->_src._addr, &acl->_src._mask, &csp->_tcp_addr)
#else
		 (csp->_ip_addr_long & acl->_src._mask) == acl->_src._addr
#endif
		 )
	       {
		  if (dst == NULL)
		    {
		       /* Just want to check if they have any access */
		       if (acl->_action == ACL_PERMIT)
			 {
			    return(0);
			 }
		    }
		  else if (
#ifdef HAVE_RFC2553
			   /*
			    * XXX: An undefined acl->_dst is full of zeros and should be
			    * considered a wildcard address. sockaddr_storage_to_ip()
			    * fails on such destinations because of unknown sa_familly
			    * (glibc only?). However this test is not portable.
			    *
			    * So, we signal the acl->dst is wildcard in wildcard_dst.
			    */
			   acl->_wildcard_dst ||
			   filters::match_sockaddr(&acl->_dst._addr, &acl->_dst._mask, &dst->_addr)
#else
			   ((dst->_addr & acl->_dst._mask) == acl->_dst._addr)
			   && ((dst->_port == acl->_dst._port) || (acl->_dst._port == 0))
#endif
			   )
         {
            if (acl->_action == ACL_PERMIT)
            {
               return(0);
            }
            else
            {
               return(1);
            }
         }
      }
      acl = acl->_next;
   } /* end while. */
	
   return(1);
}


/*********************************************************************
 *
 * Function    :  acl_addr
 *
 * Description :  Called from `load_config' to parse an ACL address.
 *
 * Parameters  :
 *          1  :  aspec = String specifying ACL address.
 *          2  :  aca = struct access_control_addr to fill in.
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int filters::acl_addr(const char *aspec, access_control_addr *aca)
{
   int masklength;
#ifdef HAVE_RFC2553
   struct addrinfo hints, *result;
   uint8_t *mask_data;
   in_port_t *mask_port;
   unsigned int addr_len;
#else
   long port;
#endif /* def HAVE_RFC2553 */
   char *p;
   char *acl_spec = NULL;

#ifdef HAVE_RFC2553
   /* XXX: Depend on ai_family */
   masklength = 128;
#else
   masklength = 32;
   port       =  0;
#endif

   /*
    * Use a temporary acl spec copy so we can log
    * the unmodified original in case of parse errors.
    */
   acl_spec = strdup(aspec);
   if (acl_spec == NULL)
   {
      /* XXX: This will be logged as parse error. */
      return(-1);
   }

   if ((p = strchr(acl_spec, '/')) != NULL)
   {
      *p++ = '\0';
      if (ijb_isdigit(*p) == 0)
      {
         freez(acl_spec);
	 acl_spec = NULL;
         return(-1);
      }
      masklength = atoi(p);
   }

   if ((masklength < 0) ||
#ifdef HAVE_RFC2553
         (masklength > 128)
#else
         (masklength > 32)
#endif
         )
   {
      freez(acl_spec);
      acl_spec = NULL;
      return(-1);
   }

   if ((*acl_spec == '[') && (NULL != (p = strchr(acl_spec, ']'))))
   {
      *p = '\0';
      std::memmove(acl_spec, acl_spec + 1, (size_t)(p - acl_spec));

      if (*++p != ':')
      {
         p = NULL;
      }
   }
   else
   {
      p = strchr(acl_spec, ':');
   }

#ifdef HAVE_RFC2553
   std::memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   int i = getaddrinfo(acl_spec, ((p) ? ++p : NULL), &hints, &result);
   freez(acl_spec);
   acl_spec = NULL;
   
   if (i != 0)
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Can not resolve [%s]:%s: %s",
			acl_spec, p, gai_strerror(i));
      return(-1);
   }

   /* TODO: Allow multihomed hostnames */
   std::memcpy(&(aca->_addr), result->ai_addr, result->ai_addrlen);
   freeaddrinfo(result);
#else
   if (p != NULL)
   {
      char *endptr;

      *p++ = '\0';
      port = strtol(p, &endptr, 10);

      if (port <= 0 || port > 65535 || *endptr != '\0')
      {
         freez(acl_spec);
         acl_spec = NULL;
	 return(-1);
      }
   }

   aca->_port = (unsigned long)port;

   aca->_addr = ntohl(resolve_hostname_to_ip(acl_spec));
   freez(acl_spec);
   acl_spec = NULL;
   
   if (aca->_addr == INADDR_NONE)
   {
      /* XXX: This will be logged as parse error. */
      return(-1);
   }
#endif /* def HAVE_RFC2553 */

   /* build the netmask */
#ifdef HAVE_RFC2553
   /* Clip masklength according to current family. */
   if ((aca->_addr.ss_family == AF_INET) && (masklength > 32))
   {
      masklength = 32;
   }

   aca->_mask.ss_family = aca->_addr.ss_family;
   if (sockaddr_storage_to_ip(&aca->_mask, &mask_data, &addr_len, &mask_port))
   {
      return(-1);
   }

   if (p)
   {
      /* ACL contains a port number, check ports in the future. */
      *mask_port = 1;
   }
   else ((struct sockaddr_in*)&aca->_mask)->sin_port = 0;
   
   /*
    * XXX: This could be optimized to operate on whole words instead
    * of octets (128-bit CPU could do it in one iteration).
    */
   /*
    * Octets after prefix can be ommitted because of
    * previous initialization to zeros.
    */
   for (size_t i = 0; (i < addr_len) && masklength; i++)
   {
      if (masklength >= 8)
      {
         mask_data[i] = 0xFF;
         masklength -= 8;
      }
      else
      {
         /*
          * XXX: This assumes MSB of octet is on the left side.
          * This should be true for all architectures or solved
          * by the link layer.
          */
         mask_data[i] = (uint8_t)~((1 << (8 - masklength)) - 1);
         masklength = 0;
      }
   }

#else
   aca->mask = 0;
   for (i=1; i <= masklength ; i++)
   {
      aca->mask |= (1U << (32 - i));
   }

   /* now mask off the host portion of the ip address
    * (i.e. save on the network portion of the address).
    */
   aca->addr = aca->addr & aca->mask;
#endif /* def HAVE_RFC2553 */

   return(0);
}
#endif /* def FEATURE_ACL */


/*********************************************************************
 *
 * Function    :  connect_port_is_forbidden
 *
 * Description :  Check to see if CONNECT requests to the destination
 *                port of this request are forbidden. The check is
 *                independend of the actual request method.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  True if yes, false otherwise.
 *
 *********************************************************************/
/*int filters::connect_port_is_forbidden(const client_state *csp)
{
   return ((csp->_action._flags & ACTION_LIMIT_CONNECT) &&
	   !urlmatch::match_portlist(csp->_action._string[ACTION_STRING_LIMIT_CONNECT],
				     csp->_http._port));
}*/

/*********************************************************************
 *
 * Function    :  rewrite_url
 *
 * Description :  Rewrites a URL with a single pcrs command
 *                and returns the result if it differs from the
 *                original and isn't obviously invalid.
 *
 * Parameters  :
 *          1  :  old_url = URL to rewrite.
 *          2  :  pcrs_command = pcrs command formatted as string (s@foo@bar@)
 *
 *
 * Returns     :  NULL if the pcrs_command didn't change the url, or 
 *                the result of the modification.
 *
 *********************************************************************/
char* filters::rewrite_url(char *old_url, const char *pcrs_command)
{
   char *new_url = NULL;
   int hits;

   assert(old_url);
   assert(pcrs_command);

   new_url = pcrs::pcrs_execute_single_command(old_url, pcrs_command, &hits);

   if (hits == 0)
   {
      errlog::log_error(LOG_LEVEL_REDIRECTS,
			"pcrs command \"%s\" didn't change \"%s\".",
			pcrs_command, old_url);
      freez(new_url);
      new_url = NULL;
   }
   else if (hits < 0)
   {
      errlog::log_error(LOG_LEVEL_REDIRECTS,
			"executing pcrs command \"%s\" to rewrite %s failed: %s",
			pcrs_command, old_url, pcrs::pcrs_strerror(hits));
      freez(new_url);
      new_url = NULL;
   }
   else if (miscutil::strncmpic(new_url, "http://", 7) 
	    && miscutil::strncmpic(new_url, "https://", 8))
   {
      errlog::log_error(LOG_LEVEL_ERROR,
			"pcrs command \"%s\" changed \"%s\" to \"%s\" (%u hi%s), "
			"but the result doesn't look like a valid URL and will be ignored.",
			pcrs_command, old_url, new_url, hits, (hits == 1) ? "t" : "ts");
      freez(new_url);
      new_url = NULL;
   }
   else
   {
      errlog::log_error(LOG_LEVEL_REDIRECTS,
			"pcrs command \"%s\" changed \"%s\" to \"%s\" (%u hi%s).",
			pcrs_command, old_url, new_url, hits, (hits == 1) ? "t" : "ts");
   }

   return new_url;
}


#ifdef FEATURE_FAST_REDIRECTS
/*********************************************************************
 *
 * Function    :  get_last_url
 *
 * Description :  Search for the last URL inside a string.
 *                If the string already is a URL, it will
 *                be the first URL found.
 *
 * Parameters  :
 *          1  :  subject = the string to check
 *          2  :  redirect_mode = +fast-redirect{} mode 
 *
 * Returns     :  NULL if no URL was found, or
 *                the last URL found.
 *
 *********************************************************************/
char* filters::get_last_url(char *subject, const char *redirect_mode)
{
   char *new_url = NULL;
   char *tmp;

   assert(subject);
   assert(redirect_mode);

   subject = strdup(subject);
   if (subject == NULL)
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Out of memory while searching for redirects.");
      return NULL;
   }

   if (0 == miscutil::strcmpic(redirect_mode, "check-decoded-url"))
   {  
      errlog::log_error(LOG_LEVEL_REDIRECTS, "Decoding \"%s\" if necessary.", subject);
      new_url = encode::url_decode(subject);
      if (new_url != NULL)
      {
         freez(subject);
	 subject = NULL;
         subject = new_url;
      }
      else
      {
         errlog::log_error(LOG_LEVEL_ERROR, "Unable to decode \"%s\".", subject);
      }
   }

   errlog::log_error(LOG_LEVEL_REDIRECTS, "Checking \"%s\" for redirects.", subject);

   /*
    * Find the last URL encoded in the request
    */
   tmp = subject;
   while ((tmp = strstr(tmp, "http://")) != NULL)
   {
      new_url = tmp++;
   }
   tmp = (new_url != NULL) ? new_url : subject;
   while ((tmp = strstr(tmp, "https://")) != NULL)
   {
      new_url = tmp++;
   }

   if ((new_url != NULL)
      && (  (new_url != subject)
	    || (0 == miscutil::strncmpic(subject, "http://", 7))
	    || (0 == miscutil::strncmpic(subject, "https://", 8))
	    ))
     {
	/*
	 * Return new URL if we found a redirect 
	 * or if the subject already was a URL.
	 *
	 * The second case makes sure that we can
	 * chain get_last_url after another redirection check
	 * (like rewrite_url) without losing earlier redirects.
	 */
	new_url = strdup(new_url);
	freez(subject);
	subject = NULL;
	return new_url;
   }

   freez(subject);
   subject = NULL;
   return NULL;
}
#endif /* def FEATURE_FAST_REDIRECTS */


/*********************************************************************
 *
 * Function    :  redirect_url
 *
 * Description :  Checks if Seeks proxy should answer the request with
 *                a HTTP redirect and generates the redirect if
 *                necessary.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  NULL if the request can pass, HTTP redirect otherwise.
 *
 *********************************************************************/
/*http_response* filters::redirect_url(client_state *csp)
{
   http_response *rsp;
#ifdef FEATURE_FAST_REDIRECTS */
   /*
    * XXX: Do we still need FEATURE_FAST_REDIRECTS
    * as compile-time option? The user can easily disable
    * it in his action file.
    */
/*   char * redirect_mode;
#endif*/ /* def FEATURE_FAST_REDIRECTS */
/*   char *old_url = NULL;
   char *new_url = NULL;
   char *redirection_string;

   if ((csp->_action._flags & ACTION_REDIRECT))
   {
      redirection_string = csp->_action._string[ACTION_STRING_REDIRECT]; */

      /*
       * If the redirection string begins with 's',
       * assume it's a pcrs command, otherwise treat it as
       * properly formatted URL and use it for the redirection
       * directly.
       *
       * According to RFC 2616 section 14.30 the URL
       * has to be absolute and if the user tries:
       * +redirect{shit/this/will/be/parsed/as/pcrs_command.html}
       * she would get undefined results anyway.
       *
       */
/*      if (*redirection_string == 's')
      {
         old_url = csp->_http._url;
         new_url = filters::rewrite_url(old_url, redirection_string);
      }
      else
      {
         errlog::log_error(LOG_LEVEL_REDIRECTS,
			   "No pcrs command recognized, assuming that \"%s\" is already properly formatted.",
			   redirection_string);
         new_url = strdup(redirection_string);
      }
   }

#ifdef FEATURE_FAST_REDIRECTS
   if ((csp->_action._flags & ACTION_FAST_REDIRECTS))
   {
      redirect_mode = csp->_action._string[ACTION_STRING_FAST_REDIRECTS]; */

      /*
       * If it exists, use the previously rewritten URL as input
       * otherwise just use the old path.
       */
/*      old_url = (new_url != NULL) ? new_url : strdup(csp->_http._path);
      new_url = get_last_url(old_url, redirect_mode);
      freez(old_url);
   } */
   
   /*
    * Disable redirect checkers, so that they
    * will be only run more than once if the user
    * also enables them through tags.
    *
    * From a performance point of view
    * it doesn't matter, but the duplicated
    * log messages are annoying.
    */
/*   csp->_action._flags &= ~ACTION_FAST_REDIRECTS;
#endif*/ /* def FEATURE_FAST_REDIRECTS */
   //csp->_action._flags &= ~ACTION_REDIRECT;

   /* Did any redirect action trigger? */   
   /* if (new_url)
   {
      if (0 == miscutil::strcmpic(new_url, csp->_http._url))
      {
         errlog::log_error(LOG_LEVEL_ERROR,
			   "New URL \"%s\" and old URL \"%s\" are the same. Redirection loop prevented.",
			   csp->_http._url, new_url);
	 freez(new_url);
      }
      else
      {
         errlog::log_error(LOG_LEVEL_REDIRECTS, "New URL is: %s", new_url);

         if (NULL == (rsp = new http_response()))
         {
            freez(new_url);
	    return cgi::cgi_error_memory();
         }

         if ( miscutil::enlist_unique_header(&rsp->_headers, "Location", new_url)
           || (NULL == (rsp->_status = strdup("302 Local Redirect from Seeks proxy"))) )
         {
            freez(new_url);
            delete rsp;
            return cgi::cgi_error_memory();
         }
         rsp->_reason = RSP_REASON_REDIRECTED;
         freez(new_url);
	 
         return cgi::finish_http_response(csp, rsp);
      }
   } */

   /* Only reached if no redirect is required */
   /* return NULL;
} */


#ifdef FEATURE_IMAGE_BLOCKING
/*********************************************************************
 *
 * Function    :  is_imageurl
 *
 * Description :  Given a URL, decide whether it is an image or not,
 *                using either the info from a previous +image action
 *                or, #ifdef FEATURE_IMAGE_DETECT_MSIE, and the browser
 *                is MSIE and not on a Mac, tell from the browser's accept
 *                header.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  True (nonzero) if URL is an image, false (0)
 *                otherwise
 *
 *********************************************************************/
int filters::is_imageurl(const client_state *csp)
{
#ifdef FEATURE_IMAGE_DETECT_MSIE
   char *tmp;

   tmp = parsers::get_header_value(&csp->_headers, "User-Agent:");
   if (tmp && strstr(tmp, "MSIE") && !strstr(tmp, "Mac_"))
   {
      tmp = parsers::get_header_value(&csp->_headers, "Accept:");
      if (tmp && strstr(tmp, "image/gif"))
      {
         /* Client will accept HTML.  If this seems counterintuitive,
          * blame Microsoft.
          */
         return(0);
      }
      else
      {
         return(1);
      }
   }
#endif /* def FEATURE_IMAGE_DETECT_MSIE */

   return ((csp->_action._flags & ACTION_IMAGE) != 0);
}
#endif /* def FEATURE_IMAGE_BLOCKING */

/*********************************************************************
 *
 * Function    :  remove_chunked_transfer_coding
 *
 * Description :  In-situ remove the "chunked" transfer coding as defined
 *                in rfc2616 from a buffer.
 *
 * Parameters  :
 *          1  :  buffer = Pointer to the text buffer
 *          2  :  size =  In: Number of bytes to be processed,
 *                       Out: Number of bytes after de-chunking.
 *                       (undefined in case of errors)
 *
 * Returns     :  sp_err_OK for success,
 *                sp_err_PARSE otherwise
 *
 *********************************************************************/
sp_err filters::remove_chunked_transfer_coding(char *buffer, size_t *size)
{
   size_t newsize = 0;
   unsigned int chunksize = 0;
   char *from_p, *to_p;

   assert(buffer);
   from_p = to_p = buffer;

   if (sscanf(buffer, "%x", &chunksize) != 1)
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Invalid first chunksize while stripping \"chunked\" transfer coding");
      return SP_ERR_PARSE;
   }

   while (chunksize > 0U)
   {
      if (NULL == (from_p = strstr(from_p, "\r\n")))
      {
         errlog::log_error(LOG_LEVEL_ERROR, "Parse error while stripping \"chunked\" transfer coding");
         return SP_ERR_PARSE;
      }

      if ((newsize += chunksize) >= *size)
      {
         errlog::log_error(LOG_LEVEL_ERROR,
			   "Chunk size %d exceeds buffer size %d in  \"chunked\" transfer coding",
			   chunksize, *size);
         return SP_ERR_PARSE;
      }
      from_p += 2;

      std::memmove(to_p, from_p, (size_t) chunksize);
      to_p = buffer + newsize;
      from_p += chunksize + 2;

      if (sscanf(from_p, "%x", &chunksize) != 1)
      {
         errlog::log_error(LOG_LEVEL_INFO, "Invalid \"chunked\" transfer encoding detected and ignored.");
         break;
      }
   }
   
   /* XXX: Should get its own loglevel. */
   errlog::log_error(LOG_LEVEL_RE_FILTER, "De-chunking successful. Shrunk from %d to %d", *size, newsize);

   *size = newsize;

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  prepare_for_filtering
 *
 * Description :  If necessary, de-chunks and decompresses
 *                the content so it can get filterd.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  sp_err_OK for success,
 *                sp_err_PARSE otherwise
 *
 *********************************************************************/
sp_err filters::prepare_for_filtering(client_state *csp)
{
   sp_err err = SP_ERR_OK;

   /*
    * If the body has a "chunked" transfer-encoding,
    * get rid of it, adjusting size and iob->eod
    */
   if (csp->_flags & CSP_FLAG_CHUNKED)
   {
      size_t size = (size_t)(csp->_iob._eod - csp->_iob._cur);

      errlog::log_error(LOG_LEVEL_RE_FILTER, "Need to de-chunk first");
      err = filters::remove_chunked_transfer_coding(csp->_iob._cur, &size);
      if (SP_ERR_OK == err)
      {
         csp->_iob._eod = csp->_iob._cur + size;
         csp->_flags |= CSP_FLAG_MODIFIED;
      }
      else
      {
         return SP_ERR_PARSE;
      }
   }

#ifdef FEATURE_ZLIB
   /*
    * If the body has a supported transfer-encoding,
    * decompress it, adjusting size and iob->eod.
    */
   if (csp->_content_type & (CT_GZIP|CT_DEFLATE))
   {
      if (0 == csp->_iob._eod - csp->_iob._cur)
      {
         /* Nothing left after de-chunking. */
         return SP_ERR_OK;
      }

      err = parsers::decompress_iob(csp);

      if (SP_ERR_OK == err)
      {
         csp->_flags |= CSP_FLAG_MODIFIED;
         csp->_content_type &= ~CT_TABOO;
      }
      else
      {
         /*
          * Unset CT_GZIP and CT_DEFLATE to remember not
          * to modify the Content-Encoding header later.
          */
         csp->_content_type &= ~CT_GZIP;
         csp->_content_type &= ~CT_DEFLATE;
      }
   }
#endif

   return err;
}


/*********************************************************************
 *
 * Function    :  execute_content_filter
 *
 * Description :  Executes a given content filter.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  content_filter = The filter function to execute
 *
 * Returns     :  Pointer to the modified buffer, or
 *                NULL if filtering failed or wasn't necessary.
 *
 *********************************************************************/
 char* filters::execute_content_filter(client_state *csp, filter_function_ptr content_filter)
{
   if (0 == csp->_iob._eod - csp->_iob._cur)
   {
      /*
       * No content (probably status code 301, 302 ...),
       * no filtering necessary.
       */
      return NULL;
   }

   if (SP_ERR_OK != filters::prepare_for_filtering(csp))
   {
      /*
       * failed to de-chunk or decompress.
       */
      return NULL;
   }

   if (0 == csp->_iob._eod - csp->_iob._cur)
   {
      /*
       * Clown alarm: chunked and/or compressed nothing delivered.
       */
      return NULL;
   }

   return ((*content_filter)(csp));
}

/*********************************************************************
 *
 * Function    :  get_forward_override_settings
 *
 * Description :  Returns forward settings as specified with the
 *                forward-override{} action. forward-override accepts
 *                forward lines similar to the one used in the
 *                configuration file, but without the URL pattern.
 *
 *                For example:
 *
 *                   forward / .
 *
 *                in the configuration file can be replaced with
 *                the action section:
 *
 *                 {+forward-override{forward .}}
 *                 /
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Pointer to forwarding structure in case of success.
 *                Invalid syntax is fatal.
 *
 *********************************************************************/
/*forward_spec* filters::get_forward_override_settings(client_state *csp)
{
   const char *forward_override_line = csp->_action._string[ACTION_STRING_FORWARD_OVERRIDE];
   char forward_settings[BUFFER_SIZE];
   char *http_parent = NULL; */
   
   /* variable names were chosen for consistency reasons. */
/*   forward_spec *fwd = NULL;
   int vec_count;
   char *vec[3];

   assert(csp->_action._flags & ACTION_FORWARD_OVERRIDE); */
   /* Should be enforced by load_one_actions_file() */
/*   assert(strlen(forward_override_line) < sizeof(forward_settings) - 1); */

   /* Create a copy ssplit can modify */
/*   strlcpy(forward_settings, forward_override_line, sizeof(forward_settings));

   if (NULL != csp->_fwd)
   { */
      /*
       * XXX: Currently necessary to prevent memory
       * leaks when the show-url-info cgi page is visited.
       */
/*      delete csp->_fwd;
   } */

   /*
    * allocate a new forward node, valid only for
    * the lifetime of this request. Save its location
    * in csp as well, so sweep() can free it later on.
    */
/*   fwd = csp->_fwd = new forward_spec();
   if (NULL == fwd)
     {
	errlog::log_error(LOG_LEVEL_FATAL,
			  "can't allocate memory for forward-override{%s}", forward_override_line); */
	/* Never get here - LOG_LEVEL_FATAL causes program exit */
/*	return NULL;
     }

   vec_count = miscutil::ssplit(forward_settings, " \t", vec, SZ(vec), 1, 1);
   if ((vec_count == 2) && !strcasecmp(vec[0], "forward"))
   {
      fwd->_type = SOCKS_NONE; */

      /* Parse the parent HTTP proxy host:port */
/*      http_parent = vec[1];

   }
   else if (vec_count == 3)
   {
      char *socks_proxy = NULL;

      if  (!strcasecmp(vec[0], "forward-socks4"))
      {
         fwd->_type = SOCKS_4;
         socks_proxy = vec[1];
      }
      else if (!strcasecmp(vec[0], "forward-socks4a"))
      {
         fwd->_type = SOCKS_4A;
         socks_proxy = vec[1];
      }
      else if (!strcasecmp(vec[0], "forward-socks5"))
      {
         fwd->_type = SOCKS_5;
         socks_proxy = vec[1];
      }

      if (NULL != socks_proxy)
      { */
         /* Parse the SOCKS proxy host[:port] */
/*         fwd->_gateway_port = 1080;
         urlmatch::parse_forwarder_address(socks_proxy,
					   &fwd->_gateway_host, &fwd->_gateway_port);

         http_parent = vec[2];
      }
   }

   if (NULL == http_parent)
   {
      errlog::log_error(LOG_LEVEL_FATAL,
			"Invalid forward-override syntax in: %s", forward_override_line); */
      /* Never get here - LOG_LEVEL_FATAL causes program exit */
/*   } */

   /* Parse http forwarding settings */
/*   if (strcmp(http_parent, ".") != 0)
   {
      fwd->_forward_port = 8000;
      urlmatch::parse_forwarder_address(http_parent,
					&fwd->_forward_host, &fwd->_forward_port);
   }

   assert (NULL != fwd);

   errlog::log_error(LOG_LEVEL_CONNECT,
		     "Overriding forwarding settings based on \'%s\'", forward_override_line);

   return fwd;
} */


/*********************************************************************
 *
 * Function    :  forward_url
 *
 * Description :  Should we forward this to another proxy?
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  http = http_request request for current URL
 *
 * Returns     :  Pointer to forwarding information.
 *
 *********************************************************************/
const forward_spec* filters::forward_url(client_state *csp,
					 const http_request *http)
{
   static const forward_spec fwd_default[1] = { forward_spec() };
   forward_spec *fwd = csp->_config->_forward;

   /*if (csp->_action._flags & ACTION_FORWARD_OVERRIDE)  // DEPRECATED
   {
      return filters::get_forward_override_settings(csp);
   } */

   if (fwd == NULL)
   {
      return fwd_default;
   }

   while (fwd != NULL)
   {
      if (urlmatch::url_match(fwd->_url, http))
      {
         return fwd;
      }
      fwd = fwd->_next;
   }

   return fwd_default;
}


/*********************************************************************
 *
 * Function    :  direct_response 
 *
 * Description :  Check if Max-Forwards == 0 for an OPTIONS or TRACE
 *                request and if so, return a HTTP 501 to the client.
 *
 *                FIXME: I have a stupid name and I should handle the
 *                requests properly. Still, what we do here is rfc-
 *                compliant, whereas ignoring or forwarding are not.
 *
 * Parameters  :  
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  http_response if , NULL if nonmatch or handler fail
 *
 *********************************************************************/
http_response* filters::direct_response(client_state *csp)
{
   http_response *rsp;
   
   if ((0 == miscutil::strcmpic(csp->_http._gpc, "trace"))
      || (0 == miscutil::strcmpic(csp->_http._gpc, "options")))
   {
      std::list<const char*>::const_iterator lit = csp->_headers.begin();
      while(lit!=csp->_headers.end())
	{
	   char *str = strdup((*lit));
	   if (!miscutil::strncmpic("Max-Forwards:", str, 13))
	     {
		unsigned int max_forwards;
		
		/*
		 * If it's a Max-Forwards value of zero,
		 * we have to intercept the request.
		 */
		if (1 == sscanf(str+12, ": %u", &max_forwards) && max_forwards == 0)
		  {
		     /*
		      * FIXME: We could handle at least TRACE here,
		      * but that would require a verbatim copy of
		      * the request which we don't have anymore
		      */
		     errlog::log_error(LOG_LEVEL_HEADER,
				       "Detected header \'%s\' in OPTIONS or TRACE request. Returning 501.",
				       str);
		     
		     /* Get mem for response or fail*/
		     if (NULL == (rsp = new http_response()))
		       {
			  return cgi::cgi_error_memory();
		       }
		     
		     if (NULL == (rsp->_status = strdup("501 Not Implemented")))
		       {
			  delete rsp;
			  return cgi::cgi_error_memory();
		       }

		     rsp->_is_static = 1;
		     rsp->_reason = RSP_REASON_UNSUPPORTED;
		     
		     freez(str);
		     
		     return(cgi::finish_http_response(csp, rsp));
		  }
	     }
	   freez(str);
	   ++lit;
	}
   }
   return NULL;
}
   

/*********************************************************************
 *
 * Function    :  content_filters_enabled
 *
 * Description :  Checks whether there are any content filters
 *                enabled for the current request.
 *
 * Parameters  :  
 *          1  :  action = Action spec to check.
 *
 * Returns     :  TRUE for yes, FALSE otherwise
 *
 *********************************************************************/
int filters::content_filters_enabled(const current_action_spec *action)
{
   return (!action->_multi[ACTION_MULTI_FILTER].empty());
}

} /* end of namespace. */
