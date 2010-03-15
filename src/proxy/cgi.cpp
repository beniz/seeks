/**
 * Purpose     :  Declares functions to intercept request, generate
 *                html or gif answers, and to compose HTTP responses.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001-2009 the SourceForge
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

#include "mem_utils.h"
#include "cgi.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
 
#include "cgi.h"
#include "encode.h"
#include "errlog.h"
#include "filters.h" 
#include "miscutil.h" 
#include "cgisimple.h"
#include "spsockets.h"
 
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "plugin_manager.h"
#include "plugin.h"

#include <iostream>

namespace sp
{
   /*
    * List of CGI functions: name, handler, description
    * Note: Do NOT use single quotes in the description;
    *       this will break the dynamic "blocked" template!
    */
   cgi_dispatcher cgi::_cgi_dispatchers[] = {
     cgi_dispatcher( "",
	  &cgisimple::cgi_default,
	  "Seeks main local page",
	  TRUE ),
#ifdef FEATURE_GRACEFUL_TERMINATION
     cgi_dispatcher(
	"die",
	  &cgisimple::cgi_die,
	  "<b>Shut down</b> - <em class=\"warning\">Do not deploy this build in a production environment, "
	  "this is a one click Denial Of Service attack!!!</em>",
	  FALSE ),
#endif
     cgi_dispatcher(
	"show-status",
	  &cgisimple::cgi_show_status,
	  "View the current configuration",
	  TRUE ),
      cgi_dispatcher(
	"show-plugin-status",
	 &cgisimple::cgi_show_plugin,
	 "View the configuration of a plugin",
	 TRUE ),
      /* cgi_dispatcher( "show-version",
	  &cgisimple::cgi_show_version,
	  "View the source code version numbers",
	  TRUE ) , */
     cgi_dispatcher( "show-request",
	  &cgisimple::cgi_show_request,
	  "View the request headers",
	  TRUE ),
     cgi_dispatcher( "show-url-info",
	  &cgisimple::cgi_show_url_info,
	  "Look up which actions apply to a URL and why",
	  TRUE ),
#ifdef FEATURE_TOGGLE
     cgi_dispatcher("toggle",
	  &cgisimple::cgi_toggle,
	  "Toggle Seeks on or off",
	  FALSE ),
#endif /* def FEATURE_TOGGLE */
     cgi_dispatcher(
	 "error-favicon.ico",
	  &cgisimple::cgi_send_error_favicon,
	  NULL, TRUE /* Sends the favicon image for error pages. */ 
     ),
     cgi_dispatcher(
	  "favicon.ico",
	  &cgisimple::cgi_send_old_school_favicon,
	  NULL, TRUE /* Sends the favicon image for old school navigators, IE? . */
     ),
     cgi_dispatcher( "seeks_favicon_32.png",
	  &cgisimple::cgi_send_default_favicon,
	  NULL, TRUE /* Sends the default favicon image. */ 
     ),
      cgi_dispatcher( "seeks_logo.png",
      &cgisimple::cgi_send_default_logo,
	 NULL, TRUE /* Sends the default Seeks logo png image. */
     ),
     cgi_dispatcher( "robots.txt",
	  &cgisimple::cgi_robots_txt,
	  NULL, TRUE /* Sends a robots.txt file to tell robots to go away. */ 
     ),
     cgi_dispatcher( "send-banner",
	  &cgisimple::cgi_send_banner,
	  NULL, TRUE /* Send a built-in image */ 
	),
     cgi_dispatcher( "send-stylesheet",
	  &cgisimple::cgi_send_stylesheet,
	  NULL, FALSE /* Send templates/cgi-style.css */ 
     ),
     cgi_dispatcher( "t",
	  &cgisimple::cgi_transparent_image,
	  NULL, TRUE /* Send a transparent image (short name) */ 
     ),
     cgi_dispatcher( "url-info-osd.xml",
	  &cgisimple::cgi_send_url_info_osd,
	  NULL, TRUE /* Send templates/url-info-osd.xml */ 
     ),
     cgi_dispatcher( "user-manual",
	  &cgisimple::cgi_send_user_manual,
	  NULL, TRUE /* Send user-manual */ 
     ),
     cgi_dispatcher( NULL, /* NULL Indicates end of list and default page */
	  &cgisimple::cgi_error_404,
	  NULL, TRUE /* Unknown CGI page */ 
     )
};

   cgi_dispatcher cgi::_cgi_file_server 
     = cgi_dispatcher("",
		      &cgisimple::cgi_file_server,
		      NULL, TRUE /* public generic file server */
		      );
   
   cgi_dispatcher cgi::_cgi_plugin_file_server
     = cgi_dispatcher("",
		      &cgisimple::cgi_plugin_file_server,
		      NULL, TRUE /* public plugin file server */
		      );
   
   /**
    * Built-in images for ad replacement
    *
    * Hint: You can encode your own images like this:
    * cat your-image | perl -e 'while (read STDIN, $c, 1) { printf("\\%.3o", unpack("C", $c)); }'
    */
      
#ifdef FEATURE_NO_GIFS
    
   /*
    * Checkerboard pattern, as a PNG.
    */
   const char cgi::_image_pattern_data[] =
     "\211\120\116\107\015\012\032\012\000\000\000\015\111\110\104"
     "\122\000\000\000\004\000\000\000\004\010\006\000\000\000\251"
     "\361\236\176\000\000\000\006\142\113\107\104\000\000\000\000"
     "\000\000\371\103\273\177\000\000\000\033\111\104\101\124\010"
     "\327\143\140\140\140\060\377\377\377\077\003\234\106\341\060"
     "\060\230\063\020\124\001\000\161\021\031\241\034\364\030\143"
     "\000\000\000\000\111\105\116\104\256\102\140\202";
  
   /*
    * 1x1 transparant PNG.
    */
   const char cgi::_image_blank_data[] =
     "\211\120\116\107\015\012\032\012\000\000\000\015\111\110\104\122"
     "\000\000\000\001\000\000\000\001\001\003\000\000\000\045\333\126"
     "\312\000\000\000\003\120\114\124\105\377\377\377\247\304\033\310"
     "\000\000\000\001\164\122\116\123\000\100\346\330\146\000\000\000"
     "\001\142\113\107\104\000\210\005\035\110\000\000\000\012\111\104"
     "\101\124\170\001\143\140\000\000\000\002\000\001\163\165\001\030"
     "\000\000\000\000\111\105\116\104\256\102\140\202";
#else
   /*
    * Checkerboard pattern, as a GIF.
    */
   const char cgi::_image_pattern_data[] =
     "\107\111\106\070\071\141\004\000\004\000\200\000\000\310\310"
     "\310\377\377\377\041\376\016\111\040\167\141\163\040\141\040"
     "\142\141\156\156\145\162\000\041\371\004\001\012\000\001\000"
     "\054\000\000\000\000\004\000\004\000\000\002\005\104\174\147"
     "\270\005\000\073";
   
   /*
    * 1x1 transparant GIF.
    */
   const char cgi::_image_blank_data[] =
     "GIF89a\001\000\001\000\200\000\000\377\377\377\000\000"
     "\000!\371\004\001\000\000\000\000,\000\000\000\000\001"
     "\000\001\000\000\002\002D\001\000;";
#endif
   
size_t cgi::_image_pattern_length = sizeof(cgi::_image_pattern_data)-1;
size_t cgi::_image_blank_length = sizeof(cgi::_image_blank_data)-1;
   
http_response cgi::_cgi_error_memory_response; // beware.

/*********************************************************************
 *
 * Function    :  dispatch_cgi
 *
 * Description :  Checks if a request URL has either the magical
 *                hostname CGI_SITE_1_HOST (usually http://s.s/) or
 *                matches CGI_SITE_2_HOST CGI_SITE_2_PATH (usually
 *                http://config.seeks.org/). If so, it passes
 *                the (rest of the) path onto dispatch_known_cgi, which
 *                calls the relevant CGI handler function.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  http_response if match, NULL if nonmatch or handler fail
 *
 *********************************************************************/
http_response* cgi::dispatch_cgi(client_state *csp)
{
   const char *host = csp->_http._host;
   const char *path = csp->_http._path;
   
   // should we intercept ?
   /* Note: "example.com" and "example.com." are equivalent hostnames. */
   if (((0 == strcmpic(host, CGI_SITE_1_HOST))
	|| (0 == strcmpic(host, CGI_SITE_1_HOST ".")))
       && (path[0] == '/') )
     {
	/* ..then the path will all be for us.  Remove leading '/' */
	path++;
     }
   /* Or it's the host part CGI_SITE_2_HOST, and the path CGI_SITE_2_PATH */
   else if (((0 == strcmpic(host, CGI_SITE_2_HOST ))
	     || (0 == strcmpic(host, CGI_SITE_2_HOST ".")) )
	    && (0 == strncmpic(path, CGI_SITE_2_PATH, strlen(CGI_SITE_2_PATH))) )
     {
	/* take everything following CGI_SITE_2_PATH */
	path += strlen(CGI_SITE_2_PATH);
	if (*path == '/')
	  {
	     /* skip the forward slash after CGI_SITE_2_PATH */
	     path++;
	  }
	else if (*path != '\0')
	  {
	     /*
	      * weirdness: URL is /configXXX, where XXX is some string
	      * Do *NOT* intercept.
	      */
	     return NULL;
	  }
     }
   else
     {
	/* Not a CGI */
	return NULL;
     }
   
   /*
    * This is a CGI call.
    */
   return cgi::dispatch_known_cgi(csp, path);
}

/**************************************************************
 *
 * Function    :  grep_cgi_referrer
 *
 * Description :  Ugly provisorical fix that greps the value of the
 *                referer HTTP header field out of a list of
 *                strings like found at csp->headers. 
 *
 *                FIXME: csp->headers ought to be csp->_http.headers
 *                FIXME: Parsing all client header lines should
 *                       happen right after the request is received!
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  pointer to value (no copy!), or NULL if none found.
 *
 *********************************************************************/
const char *cgi::grep_cgi_referrer(const client_state *csp)
{
   std::list<const char*>::const_iterator lit = csp->_headers.begin();
   while(lit!=csp->_headers.end())
     {
	if ((*lit) == NULL)
	  {
	     ++lit;
	     continue;
	  }
	if (strncmpic((*lit),"Referer: ", 9) == 0)
	  return (*lit)+9; // beware...
	++lit;
     }
   return NULL;
}
   
/*********************************************************************
 *
 * Function    :  referrer_is_safe
 *
 * Description :  Decides whether we trust the Referer for
 *                CGI pages which are only meant to be reachable
 *                through Seeks's web interface directly.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  TRUE  if the referrer is safe, or
 *                FALSE if the referrer is unsafe or not set.
 *
 *********************************************************************/
int cgi::referrer_is_safe(const client_state *csp)
{
   const char *referrer;
   static const char alternative_prefix[] = "http://" CGI_SITE_1_HOST "/";
   referrer = cgi::grep_cgi_referrer(csp);
   if (NULL == referrer)
     {
	/* No referrer, no access  */
	errlog::log_error(LOG_LEVEL_ERROR, "Denying access to %s. No referrer found.",
			  csp->_http._url);
     }
   else if ((0 == strncmp(referrer, CGI_PREFIX, sizeof(CGI_PREFIX)-1)
	     || (0 == strncmp(referrer, alternative_prefix, strlen(alternative_prefix)))))
     {
	/* Trustworthy referrer */
	errlog::log_error(LOG_LEVEL_CGI, "Granting access to %s, referrer %s is trustworthy.",
			  csp->_http._url, referrer);
	return TRUE;
     }
   else
     {
	/* Untrustworthy referrer */
	errlog::log_error(LOG_LEVEL_ERROR, "Denying access to %s, referrer %s isn't trustworthy.",
			  csp->_http._url, referrer);
     }
   return FALSE;
}

/*********************************************************************
 *
 * Function    :  dispatch_known_cgi
 *
 * Description :  Processes a CGI once dispatch_cgi has determined that
 *                it matches one of the magic prefixes. Parses the path
 *                as a cgi name plus query string, prepares a map that
 *                maps CGI parameter names to their values, initializes
 *                the http_response class, and calls the relevant CGI
 *                handler function.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  path = Path of CGI, with the CGI prefix removed.
 *                       Should not have a leading "/".
 *
 * Returns     :  http_response, or NULL on handler failure or out of
 *                memory.
 *
 *********************************************************************/
http_response* cgi::dispatch_known_cgi(client_state *csp, const char *path)
{
   const cgi_dispatcher *d;
   hash_map<const char*,const char*,hash<const char*>,eqstr> *param_list;
   http_response *rsp;
   char *query_args_start;
   char *path_copy;
      
   if (NULL == (path_copy = strdup(path)))
     {
	return cgi_error_memory();
     }
   
   /**
    * Two special cgi calls for serving files from a built-in directory.
    * - public for seeks generic files.
    * - plugins/<plugin_name>/public for every plugin's files.
    */
   bool cgi_file_server = false;
   bool cgi_plugin_file_server = false;
   if (strncmpic(path_copy,CGI_SITE_FILE_SERVER,strlen(CGI_SITE_FILE_SERVER)) == 0)
     cgi_file_server = true;
   else if (strncmpic(path_copy,CGI_SITE_PLUGIN_FILE_SERVER,strlen(CGI_SITE_PLUGIN_FILE_SERVER)) == 0)
     cgi_plugin_file_server = true; // plugin name check comes further down.
   
   query_args_start = path_copy;
   while (*query_args_start && *query_args_start != '?' && *query_args_start != '/')
     {
	query_args_start++;
     }
   
   if (*query_args_start == '/')
     {
	*query_args_start++ = '\0';
	if ((param_list = new hash_map<const char*,const char*,hash<const char*>,eqstr>()))
	  {
	     miscutil::add_map_entry(param_list, "file", 1, 
				     encode::url_decode(query_args_start), 0);
	  }
     }
   else
     {
	if (*query_args_start == '?')
	  {
	     *query_args_start++ = '\0';
	  }
	
	if (NULL == (param_list = cgi::parse_cgi_parameters(query_args_start)))
	  {
	     freez(path_copy);
	     return cgi::cgi_error_memory();
	  }	
     }
   
   /*
    * At this point:
    * path_copy        = CGI call name
    * param_list       = CGI params, as map
    */
   
   /* Get mem for response or fail*/
   if (NULL == (rsp = new http_response()))
     {
	freez(path_copy);
	delete param_list;
	return cgi::cgi_error_memory();
     }
   
   /**
    * Is it a call to the public file service.
    */
   if (cgi_file_server)
     {
	return cgi::dispatch(&cgi::_cgi_file_server, path_copy, csp, param_list, rsp);
     }
      
   /**
    * Else is it a call to a plugin's file service.
    */
   if (cgi_plugin_file_server)
     {
	std::vector<plugin*>::const_iterator sit = plugin_manager::_plugins.begin();
	while(sit!=plugin_manager::_plugins.end())
	  {
	     std::string pname_public = (*sit)->get_name() + "/public";
	     if (strncmpic(query_args_start,pname_public.c_str(),strlen(pname_public.c_str())) == 0)
	       return cgi::dispatch(&cgi::_cgi_plugin_file_server, path_copy, csp, param_list, rsp);
	     ++sit;
	  }
     }
      
   /**
    *  Else find and start the right generic CGI function.
    */
   d = cgi::_cgi_dispatchers;
   for (;;)
     {
	if (d->_name == NULL)
	  break;
	
	if ((strcmp(path_copy, d->_name) == 0))
	  {
	     return cgi::dispatch(d, path_copy, csp, param_list, rsp);
	  }
	
	d++;
     }
   
   /**
    * Else find and start the right plugin CGI function.
    */
   d = plugin_manager::find_plugin_cgi_dispatcher(path_copy);
   if (d)
     {
	return cgi::dispatch(d, path_copy, csp, param_list, rsp);
     }
	
   return NULL; // beware.
}
   
/**
 * Dispatches a built-in response.
 */
http_response* cgi::dispatch(const cgi_dispatcher *d, char* path_copy,
			     client_state *csp,
			     hash_map<const char*,const char*,hash<const char*>,eqstr> *param_list,
			     http_response *rsp)
{
   sp_err err;
   
   /*
    * If the called CGI is either harmless, or referred
    * from a trusted source, start it.
    */
   if (d->_harmless || cgi::referrer_is_safe(csp))
     {
	err = (d->_handler)(csp, rsp, param_list);
     }
   else
     {
	/*
	 * Else, modify toggle calls so that they only display
	 * the status, and deny all other calls.
	 */
	if (0 == strcmp(path_copy, "toggle"))
	  {
	     miscutil::unmap(param_list, "set");
	     err = (d->_handler)(csp, rsp, param_list);
	  }
	else
	  {
	     err = cgi::cgi_error_disabled(csp, rsp);
	  }
     }
   
   freez(path_copy);
   miscutil::free_map(param_list);
   
   if (err == SP_ERR_CGI_PARAMS)
     {
	err = cgi::cgi_error_bad_param(csp, rsp);
     }
   
   if (err && (err != SP_ERR_MEMORY))
     {
	/* Unexpected error! Shouldn't get here */
	errlog::log_error(LOG_LEVEL_ERROR, 
			  "Unexpected CGI error %d in top-level handler.  Please file a bug report!", err);
	err = cgi::cgi_error_unknown(csp, rsp, err);
     }
   
   if (!err)
     {
	/* It worked */
	rsp->_reason = RSP_REASON_CGI_CALL;
	return cgi::finish_http_response(csp, rsp);
     }
   else
     {
	/* Error in handler, probably out-of-memory */
	delete rsp;
	return cgi::cgi_error_memory();
     }
}
   
   
/*********************************************************************
 *
 * Function    :  parse_cgi_parameters
 *
 * Description :  Parse a URL-encoded argument string into name/value
 *                pairs and store them in a struct map list.
 *
 * Parameters  :
 *          1  :  argstring = string to be parsed.  Will be trashed.
 *
 * Returns     :  pointer to param list, or NULL if out of memory.
 *
 *********************************************************************/
hash_map<const char*,const char*,hash<const char*>,eqstr>* cgi::parse_cgi_parameters(char *argstring)
{
   char *p;
   char *vector[BUFFER_SIZE];
   int pairs, i;
   hash_map<const char*,const char*,hash<const char*>,eqstr> *cgi_params;
   
   if (NULL == (cgi_params = new hash_map<const char*,const char*,hash<const char*>,eqstr>()))
     {
	return NULL;
     }
   
   /*
    * IE 5 does, of course, violate RFC 2316 Sect 4.1 and sends
    * the fragment identifier along with the request, so we must
    * cut it off here, so it won't pollute the CGI params:
    */
   if (NULL != (p = strchr(argstring, '#')))
     {
	*p = '\0';
     }
   
   pairs = miscutil::ssplit(argstring, "?&", vector, SZ(vector), 1, 1);
   
   for (i = 0; i < pairs; i++)
     {
	if ((NULL != (p = strchr(vector[i], '='))) && (*(p+1) != '\0'))
	  {
	     *p = '\0';
	     if (miscutil::add_map_entry(cgi_params, 
					 encode::url_decode(vector[i]), 0, 
					 encode::url_decode(++p), 0))
	       {
		  miscutil::free_map(cgi_params);
		  return NULL;
	       }
	  }
     }
   
   return cgi_params;
}

/*********************************************************************
 *
 * Function    :  get_char_param
 *
 * Description :  Get a single-character parameter passed to a CGI
 *                function.
 *
 * Parameters  :
 *          1  :  parameters = map of cgi parameters
 *          2  :  param_name = The name of the parameter to read
 *
 * Returns     :  Uppercase character on success, '\0' on error.
 *
 *********************************************************************/
char cgi::get_char_param(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
			 const char *param_name)
{
   char ch;
   
   assert(parameters);
   assert(param_name);
   
   ch = *(miscutil::lookup(parameters, param_name));  // beware: this is a lookup for a single item.
   if ((ch >= 'a') && (ch <= 'z'))
     {
	ch = (char)(ch - 'a' + 'A');
     }
   return ch;
}
   
/*********************************************************************
 *
 * Function    :  get_string_param
 *
 * Description :  Get a string parameter, to be used as an
 *                ACTION_STRING or ACTION_MULTI parameter.
 *                Validates the input to prevent stupid/malicious
 *                users from corrupting their action file.
 *
 * Parameters  :
 *          1  :  parameters = map of cgi parameters
 *          2  :  param_name = The name of the parameter to read
 *          3  :  pparam = destination for paramater.  Allocated as
 *                part of the map "parameters", so don't free it.
 *                Set to NULL if not specified.
 *
 * Returns     :  SP_ERR_OK         on success, or if the paramater
 *                                  was not specified.
 *                SP_ERR_MEMORY     on out-of-memory.
 *                SP_ERR_CGI_PARAMS if the paramater is not valid.
 *
 *********************************************************************/
sp_err cgi::get_string_param(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
			     const char *param_name,
			     const char **pparam)
{
   const char *param;
   const char *s;
   char ch;
   
   assert(parameters);
   assert(param_name);
   assert(pparam);
   
   *pparam = NULL;

   param = miscutil::lookup(parameters,param_name);
   if (!*param)
     {
	return SP_ERR_OK;
     }
   
   if (strlen(param) >= CGI_PARAM_LEN_MAX)
     {
	/*
	 * Too long.
	 *
	 * Note that the length limit is arbitrary, it just seems
	 * sensible to limit it to *something*.  There's no
	 * technical reason for any limit at all.
	 */
	return SP_ERR_CGI_PARAMS;
     }
   
   /* Check every character to see if it's legal */
   s = param;
   while ((ch = *s++) != '\0')
     {
	if ( ((unsigned char)ch < (unsigned char)' ')
	     || (ch == '}') )
	  {
	     /* Probable hack attempt, or user accidentally used '}'. */
	     return SP_ERR_CGI_PARAMS;
	  }
     }
   
   /* Success */
   *pparam = param;
   
   return SP_ERR_OK;
}

/*********************************************************************
 *
 * Function    :  get_number_param
 *
 * Description :  Get a non-negative integer from the parameters
 *                passed to a CGI function.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  parameters = map of cgi parameters
 *          3  :  name = Name of CGI parameter to read
 *          4  :  pvalue = destination for value.
 *                         Set to -1 on error.
 *
 * Returns     :  SP_ERR_OK         on success
 *                SP_ERR_MEMORY     on out-of-memory
 *                SP_ERR_CGI_PARAMS if the parameter was not specified
 *                                  or is not valid.
 *
 *********************************************************************/
sp_err cgi::get_number_param(client_state *csp,
			     const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
			     char *name, unsigned *pvalue)
{
   const char *param;
   char ch;
   unsigned value;
   
   assert(csp);
   assert(parameters);
   assert(name);
   assert(pvalue);
   
   *pvalue = 0;
   
   param = miscutil::lookup(parameters, name);
   if (!*param)
     {
	return SP_ERR_CGI_PARAMS;
     }
   
   /* We don't use atoi because I want to check this carefully... */
   value = 0;
   while ((ch = *param++) != '\0')
     {
	if ((ch < '0') || (ch > '9'))
	  {
	     
	     return SP_ERR_CGI_PARAMS;
	  }
	ch = (char)(ch - '0');

	/* Note:
	 *
	 * <limits.h> defines UINT_MAX
	 *
	 * (UINT_MAX - ch) / 10 is the largest number that
	 *     can be safely multiplied by 10 then have ch added.
	 */
	if (value > ((UINT_MAX - (unsigned)ch) / 10U))
	  {
	     return SP_ERR_CGI_PARAMS;
	  }
	value = value * 10 + (unsigned)ch;
     }
   /* Success */
   *pvalue = value;

   return SP_ERR_OK;
}

/*********************************************************************
 *
 * Function    :  error_response
 *
 * Description :  returns an http_response that explains the reason
 *                why a request failed.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  templatename = Which template should be used for the answer
 *
  * Returns     :  A http_response.  If we run out of memory, this
 *                will be cgi_error_memory().
 *
 *********************************************************************/
http_response* cgi::error_response(client_state *csp,
				   const char *templatename)
{
   sp_err err;
   http_response *rsp;
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports = cgi::default_exports(csp,NULL);
   char *path = NULL;
   
   if (exports == NULL)
     {
	return cgi::cgi_error_memory();
     }
   
   if (NULL == (rsp = new http_response()))
     {
	miscutil::free_map(exports);
	return cgi::cgi_error_memory();
     }
   
#ifdef FEATURE_FORCE_LOAD
   if (csp->_flags & CSP_FLAG_FORCED)
     {
	path = strdup(FORCE_PREFIX);
     }
   else
#endif /* def FEATURE_FORCE_LOAD */
     {
	path = strdup("");
     }
   
   err = miscutil::string_append(&path, csp->_http._path);
   
   if (!err) 
     err = miscutil::add_map_entry(exports, "host", 1, encode::html_encode(csp->_http._host), 0);
   if (!err) 
     err = miscutil::add_map_entry(exports, "hostport", 1, encode::html_encode(csp->_http._hostport), 0);
   if (!err) 
     err = miscutil::add_map_entry(exports, "path", 1, encode::html_encode_and_free_original(path), 0);
   if (!err) 
     err = miscutil::add_map_entry(exports, "protocol", 1, csp->_http._ssl ? "https://" : "http://", 1);
   if (!err)
     {
	err = miscutil::add_map_entry(exports, "host-ip", 1, encode::html_encode(csp->_http._host_ip_addr_str), 0);
	if (err)
	  {
	     /* Some failures, like "404 no such domain", don't have an IP address. */
	     err = miscutil::add_map_entry(exports, "host-ip", 1, encode::html_encode(csp->_http._host), 0);
	  }
     }
      
   if (err)
     {
	miscutil::free_map(exports);
	delete rsp;
	return cgi::cgi_error_memory();
     }
      
   if (!strcmp(templatename, "no-such-domain"))
     {
	rsp->_status = strdup("404 No such domain");
	rsp->_reason = RSP_REASON_NO_SUCH_DOMAIN;
     }
/*   else if (!strcmp(templatename, "forwarding-failed"))
     {
	const forward_spec *fwd = filters::forward_url(csp, &csp->_http);
	char *socks_type = NULL;
	if (fwd == NULL)
	  {
	     errlog::log_error(LOG_LEVEL_FATAL, "gateway spec is NULL. This shouldn't happen!"); */
	     /* Never get here - LOG_LEVEL_FATAL causes program exit */
/*	  } */
		
	/*
	 * XXX: While the template is called forwarding-failed,
	 * it currently only handles socks forwarding failures.
	 */
/*	assert(fwd != NULL);
	assert(fwd->_type != SOCKS_NONE); */
	
	/*
	 * Map failure reason, forwarding type and forwarder.
	 */
/*	if (NULL == csp->_error_message)
	  { */
	     /*
	      * Either we forgot to record the failure reason,
	      * or the memory allocation failed.
	      */
/*	     errlog::log_error(LOG_LEVEL_ERROR, "Socks failure reason missing.");
	     csp->_error_message = strdup("Failure reason missing. Check the log file for details.");
	  }
	
	if (!err) err = miscutil::add_map_entry(exports, "gateway", 1, fwd->_gateway_host, 1); */
	// below: in cgisimple
	/*
	 * XXX: this is almost the same code as in cgi_show_url_info()
	 * and thus should be factored out and shared.
	 */
/*	switch (fwd->_type)
	  {
	   case SOCKS_4:
	     socks_type = (char*)"socks4-";
	     break;
	   case SOCKS_4A:
	     socks_type = (char*)"socks4a-";
	     break;
	   case SOCKS_5:
	     socks_type = (char*)"socks5-";
	     break;
	   default:
	     errlog::log_error(LOG_LEVEL_FATAL, "Unknown socks type: %d.", fwd->_type);
	  }
	
	if (!err) 
	  err = miscutil::add_map_entry(exports, "forwarding-type", 1, socks_type, 1);
	if (!err) 
	  err = miscutil::add_map_entry(exports, "error-message", 1, encode::html_encode(csp->_error_message), 0);
	if ((NULL == csp->_error_message) || err)
	  {
	     miscutil::free_map(exports);
	     delete rsp;
	     return cgi::cgi_error_memory();
	  }
		
	rsp->_status = strdup("503 Forwarding failure");
	rsp->_reason = RSP_REASON_FORWARDING_FAILED;
     } */
   else if (!strcmp(templatename, "connect-failed"))
     {
	rsp->_status = strdup("503 Connect failed");
	rsp->_reason = RSP_REASON_CONNECT_FAILED;
     }
   else if (!strcmp(templatename, "connection-timeout"))
     {
	rsp->_status = strdup("504 Connection timeout");
	rsp->_reason = RSP_REASON_CONNECTION_TIMEOUT;
     }
   else if (!strcmp(templatename, "no-server-data"))
     {
	rsp->_status = strdup("502 No data received from server or forwarder");
	rsp->_reason = RSP_REASON_NO_SERVER_DATA;
     }
   
   if (rsp->_status == NULL)
     {
	miscutil::free_map(exports);
	delete rsp;
	return cgi::cgi_error_memory();
     }
   
   err = cgi::template_fill_for_cgi(csp, templatename, csp->_config->_templdir, 
				    exports, rsp);
   if (err)
     {
	delete rsp;
	return cgi::cgi_error_memory();
     }
   
   return cgi::finish_http_response(csp, rsp);
}

/********************************************************************
 *
 * Function    :  cgi_error_disabled
 *
 * Description :  CGI function that is called to generate an error
 *                response if the actions editor or toggle CGI are
 *                accessed despite having being disabled at compile-
 *                or run-time, or if the user followed an untrusted link
 *                to access a unsafe CGI feature that is only reachable
 *                through Privoxy directly.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *
 * CGI Parameters : none
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err cgi::cgi_error_disabled(const client_state *csp, http_response *rsp)
{
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports;
   
   assert(csp);
   assert(rsp);
   
   if (NULL == (exports = cgi::default_exports(csp, "cgi-error-disabled")))
     {
	return SP_ERR_MEMORY;
     }
   
   if (miscutil::add_map_entry(exports, "url", 1, encode::html_encode(csp->_http._url), 0))
     {
	/* Not important enough to do anything */
	errlog::log_error(LOG_LEVEL_ERROR, "Failed to fill in url.");
     }
      
   return cgi::template_fill_for_cgi(csp, "cgi-error-disabled", 
				     csp->_config->_templdir, exports, rsp);
}

/*********************************************************************
 *
 * Function    :  cgi_init_error_messages
 *
 * Description :  Call at the start of the program to initialize
 *                the error message used by cgi_error_memory().
 *
 * Parameters  :  N/A
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void cgi::cgi_init_error_messages()
{
//   memset(cgi_error_memory_response, '\0', sizeof(*cgi_error_memory_response));
   cgi::_cgi_error_memory_response._head =
     (char*)"HTTP/1.0 500 Internal Seeks proxy Error\r\n"
     "Content-Type: text/html\r\n"
     "\r\n";
   cgi::_cgi_error_memory_response._body = (char*)
     "<html>\r\n"
     "<head>\r\n"
           " <title>500 Internal Seeks proxy Error</title>\r\n"
           " <link rel=\"shortcut icon\" href=\"" CGI_PREFIX "error-favicon.ico\" type=\"image/x-icon\">"
           "</head>\r\n"
           "<body>\r\n"
           "<h1>500 Internal Seeks proxy Error</h1>\r\n"
           "<p>Privoxy <b>ran out of memory</b> while processing your request.</p>\r\n"
           "<p>Please contact your proxy administrator, or try again later</p>\r\n"
           "</body>\r\n"
           "</html>\r\n";
   
   cgi::_cgi_error_memory_response._head_length =
     strlen(cgi::_cgi_error_memory_response._head);
   cgi::_cgi_error_memory_response._content_length =
     strlen(cgi::_cgi_error_memory_response._body);
   cgi::_cgi_error_memory_response._reason = RSP_REASON_OUT_OF_MEMORY;
}

/*********************************************************************
 *
 * Function    :  cgi_error_memory
 *
 * Description :  Called if a CGI function runs out of memory.
 *                Returns a statically-allocated error response.
 *
 * Parameters  :  N/A
 *
 * Returns     :  http_response data structure for output.  This is
 *                statically allocated, for obvious reasons.
 *
 *********************************************************************/
http_response* cgi::cgi_error_memory()
{
    /* assert that it's been initialized. */
   assert(cgi::_cgi_error_memory_response._head);
   
   return &cgi::_cgi_error_memory_response;
}

/*********************************************************************
 *
 * Function    :  cgi_error_no_template
 *
 * Description :  Almost-CGI function that is called if a template
 *                cannot be loaded.  Note this is not a true CGI,
 *                it takes a template name rather than a map of
 *                parameters.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  template_name = Name of template that could not
 *                                be loaded.
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err cgi::cgi_error_no_template(const client_state *csp, http_response *rsp,
				  const char *template_name)
{
   static const char status[] =
     "500 Internal Seeks proxy Error";
   static const char body_prefix[] =
     "<html>\r\n"
     "<head>\r\n"
     " <title>500 Internal Seeks proxy Error</title>\r\n"
     " <link rel=\"shortcut icon\" href=\"" CGI_PREFIX "error-favicon.ico\" type=\"image/x-icon\">"
     "</head>\r\n"
     "<body>\r\n"
     "<h1>500 Internal Seeks proxy Error</h1>\r\n"
     "<p>Seeks proxy encountered an error while processing your request:</p>\r\n"
     "<p><b>Could not load template file <code>";
   static const char body_suffix[] =
     "</code> or one of its included components.</b></p>\r\n"
     "<p>Please contact your proxy administrator.</p>\r\n"
     "<p>If you are the proxy administrator, please put the required file(s)"
     "in the <code><i>(confdir)</i>/templates</code> directory.  The "
     "location of the <code><i>(confdir)</i></code> directory "
     "is specified in the main Seeks' <code>config</code> "
     "file.  (It's typically the Seeks install directory"
#ifndef _WIN32
     ", or <code>/etc/seeks/</code>"
#endif /* ndef _WIN32 */
     ").</p>\r\n"
     "</body>\r\n"
     "</html>\r\n";
   const size_t body_size = strlen(body_prefix) + strlen(template_name) + strlen(body_suffix) + 1;
   
   assert(csp);
   assert(rsp);
   assert(template_name);
   
   /* Reset rsp, if needed */
   rsp->reset();
   
   rsp->_body = (char*) std::malloc(body_size);
   if (rsp->_body == NULL)
     {
	return SP_ERR_MEMORY;
     }
   
   strlcpy(rsp->_body, body_prefix, body_size);
   strlcat(rsp->_body, template_name, body_size);
   strlcat(rsp->_body, body_suffix, body_size);
   
   rsp->_status = strdup(status);
   if (rsp->_status == NULL)
     {
	return SP_ERR_MEMORY;
     }
   
   return SP_ERR_OK;
}

/*********************************************************************
 *
 * Function    :  cgi_error_unknown
 *
 * Description :  Almost-CGI function that is called if an unexpected
 *                error occurs in the top-level CGI dispatcher.
 *                In this context, "unexpected" means "anything other
 *                than SP_ERR_MEMORY or SP_ERR_CGI_PARAMS" - CGIs are
 *                expected to handle all other errors internally,
 *                since they can give more relavent error messages
 *                that way.
 *
 *                Note this is not a true CGI, it takes an error
 *                code rather than a map of parameters.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  error_to_report = Error code to report.
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err cgi::cgi_error_unknown(const client_state *csp,
			      http_response *rsp,
			      sp_err error_to_report)
{
   static const char status[] =
     "500 Internal Seeks proxy Error";
   static const char body_prefix[] =
     "<html>\r\n"
     "<head>\r\n"
     " <title>500 Internal Seeks proxy Error</title>\r\n"
     " <link rel=\"shortcut icon\" href=\"" CGI_PREFIX "error-favicon.ico\" type=\"image/x-icon\">"
     "</head>\r\n"
     "<body>\r\n"
     "<h1>500 Internal Seeks proxy Error</h1>\r\n"
     "<p>Seeks encountered an error while processing your request:</p>\r\n"
     "<p><b>Unexpected internal error: ";
   static const char body_suffix[] =
     "</b></p>\r\n"
     "<p>Please "
     "<a href=\"http://sourceforge.net/tracker/?group_id=165896\">"
     "file a bug report</a>.</p>\r\n"
     "</body>\r\n"
     "</html>\r\n";
   char errnumbuf[30];
   /*
    * Due to sizeof(errnumbuf), body_size will be slightly
    * bigger than necessary but it doesn't really matter.
    */
   const size_t body_size = strlen(body_prefix) + sizeof(errnumbuf) + strlen(body_suffix) + 1;
   assert(csp);
   assert(rsp);
   
   rsp->reset();
   rsp->_reason = RSP_REASON_INTERNAL_ERROR;
   
   snprintf(errnumbuf, sizeof(errnumbuf), "%d", error_to_report);
   
   rsp->_body = (char*) std::malloc(body_size);
   if (rsp->_body == NULL)
     {
	return SP_ERR_MEMORY;
     }
   
   strlcpy(rsp->_body, body_prefix, body_size);
   strlcat(rsp->_body, errnumbuf,   body_size);
   strlcat(rsp->_body, body_suffix, body_size);
   
   rsp->_status = strdup(status);
   if (rsp->_status == NULL)
     {
	return SP_ERR_MEMORY;
     }
   
   return SP_ERR_OK;
}

/*********************************************************************
 *
 * Function    :  cgi_error_bad_param
 *
 * Description :  CGI function that is called if the parameters
 *                (query string) for a CGI were wrong.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *
 * CGI Parameters : none
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err cgi::cgi_error_bad_param(const client_state *csp,
				http_response *rsp)
{
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports;
   
   assert(csp);
   assert(rsp);
   
   if (NULL == (exports = cgi::default_exports(csp, NULL)))
     {
	return SP_ERR_MEMORY;
     }
   
   return cgi::template_fill_for_cgi(csp, "cgi-error-bad-param", 
				     csp->_config->_templdir, exports, rsp);
}

/********************************************************************
 *
 * Function    :  cgi_redirect
 *
 * Description :  CGI support function to generate a HTTP redirect
 *                message
 *
 * Parameters  :
 *          1  :  rsp = http_response data structure for output
 *          2  :  target = string with the target URL
 *
 * CGI Parameters : None
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err cgi::cgi_redirect (http_response * rsp, const char *target)
{
   sp_err err;
   
   assert(rsp);
   assert(target);
   
   err = miscutil::enlist_unique_header(&rsp->_headers, "Location", target);
   
   rsp->_status = strdup("302 Local Redirect from Seeks proxy");
   if (rsp->_status == NULL)
     {
	return SP_ERR_MEMORY;
     }
   return err;
}

//TODO
/*********************************************************************
 *
 * Function    :  add_help_link
 *
 * Description :  Produce a copy of the string given as item,
 *                embedded in an HTML link to its corresponding
 *                section (item name in uppercase) in the actions
 *                chapter of the user manual, (whose URL is given in
 *                the config and defaults to our web site).
 *
 *                FIXME: I currently only work for actions, and would
 *                       like to be generalized for other topics.
 *
 * Parameters  :
 *          1  :  item = item (will NOT be free()d.)
 *                       It is assumed to be HTML-safe.
 *          2  :  config = The current configuration.
 *
 * Returns     :  String with item embedded in link, or NULL on
 *                out-of-memory
 *
 *********************************************************************/
char* cgi::add_help_link(const char *item, proxy_configuration *config)
{
   char *result;
   
   if (!item) return NULL;
   
   result = strdup("<a href=\"");
   if (!miscutil::strncmpic(config->_usermanual, "file://", 7) ||
       !miscutil::strncmpic(config->_usermanual, "http", 4))
     {
	miscutil::string_append(&result, config->_usermanual);
     }
   else
     {
	miscutil::string_append(&result, "http://");
	miscutil::string_append(&result, CGI_SITE_2_HOST);
	miscutil::string_append(&result, "/user-manual/");
     }
   
   miscutil::string_append(&result, ACTIONS_HELP_PREFIX);
   miscutil::string_join(&result, miscutil::string_toupper(item));
   miscutil::string_append(&result, "\">");
   miscutil::string_append(&result, item);
   miscutil::string_append(&result, "</a> ");
   
   return result;
}

/*********************************************************************
 *
 * Function    :  get_http_time
 *
 * Description :  Get the time in a format suitable for use in a
 *                HTTP header - e.g.:
 *                "Sun, 06 Nov 1994 08:49:37 GMT"
 *
 * Parameters  :
 *          1  :  time_offset = Time returned will be current time
 *                              plus this number of seconds.
 *          2  :  buf = Destination for result.
 *          3  :  buffer_size = Size of the buffer above. Must be big
 *                              enough to hold 29 characters plus a
 *                              trailing zero.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void cgi::get_http_time(int time_offset, char *buf, size_t buffer_size)
{
   static const char day_names[7][4] = 
     { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
   static const char month_names[12][4] =
     { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
   
   struct tm *t;
   time_t current_time;
#if defined(HAVE_GMTIME_R)
   struct tm dummy;
#endif
   
   assert(buf);
   assert(buffer_size > (size_t)29);
   
   time(&current_time);
   
   current_time += time_offset;
   
   /* get and save the gmt */
#if HAVE_GMTIME_R
   t = gmtime_r(&current_time, &dummy);
#elif defined(MUTEX_LOCKS_AVAILABLE)
   seeks_proxy::mutex_lock(&gmtime_mutex);
   t = gmtime(&current_time);
   seeks_proxy::mutex_unlock(&gmtime_mutex);
#else
   t = gmtime(&current_time);
#endif
   
   /* Format: "Sun, 06 Nov 1994 08:49:37 GMT" */
   snprintf(buf, buffer_size,
	    "%s, %02d %s %4d %02d:%02d:%02d GMT",
	    day_names[t->tm_wday],
	    t->tm_mday,
	    month_names[t->tm_mon],
	    t->tm_year + 1900,
	    t->tm_hour,
	    t->tm_min,
	    t->tm_sec
	    );
}

/*********************************************************************
 *
 * Function    :  get_locale_time
 *
 * Description :  Get the time in a date(1)-like format
 *                according to the current locale - e.g.:
 *                "Fri Aug 29 19:37:12 CEST 2008"
 *
 *                XXX: Should we allow the user to change the format?
 *
 * Parameters  :
 *          1  :  buf         = Destination for result.
 *          2  :  buffer_size = Size of the buffer above. Must be big
 *                              enough to hold 29 characters plus a
 *                              trailing zero.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void cgi::get_locale_time(char *buf, size_t buffer_size)
{
   struct tm *timeptr;
   time_t current_time;
#if defined(HAVE_LOCALTIME_R)
   struct tm dummy;
#endif
   
   assert(buf);
   assert(buffer_size > (size_t)29);
   
   time(&current_time);
   
#if HAVE_LOCALTIME_R
   timeptr = localtime_r(&current_time, &dummy);
#elif defined(MUTEX_LOCKS_AVAILABLE)
   seeks_proxy::mutex_lock(&localtime_mutex);
   timeptr = localtime(&current_time);
   seeks_proxy::mutex_unlock(&localtime_mutex);
#else
   timeptr = localtime(&current_time);
#endif
   
   strftime(buf, buffer_size, "%a %b %d %X %Z %Y", timeptr);
}

/*********************************************************************
 *
 * Function    :  finish_http_response
 *
 * Description :  Fill in the missing headers in an http response,
 *                and flatten the headers to an http head.
 *                For HEAD requests the body is freed once
 *                the Content-Length header is set.
 *
 * Parameters  :
 *          1  :  rsp = pointer to http_response to be processed
 *
 * Returns     :  A http_response, usually the rsp parameter.
 *                On error, free()s rsp and returns cgi_error_memory()
 *
 *********************************************************************/
http_response* cgi::finish_http_response(const client_state *csp, http_response *rsp)
{
   char buf[BUFFER_SIZE];
   sp_err err;
   
   /* Special case - do NOT change this statically allocated response,
    * which is ready for output anyway.
    */
   if (rsp == &cgi::_cgi_error_memory_response)
     {
	return rsp;
     }
      
   /*
    * Fill in the HTTP Status, using HTTP/1.1
    * unless the client asked for HTTP/1.0.
    */
   snprintf(buf, sizeof(buf), "%s %s",
	    strcmpic(csp->_http._ver, "HTTP/1.0") ? "HTTP/1.1" : "HTTP/1.0",
	    rsp->_status ? rsp->_status : "200 OK");
   err = miscutil::enlist_first(&rsp->_headers, buf);
   
   /*
    * Set the Content-Length
    */
   if (rsp->_content_length == 0)
     {
	rsp->_content_length = rsp->_body ? strlen(rsp->_body) : 0;
     }
   
   if (!err)
     {
	snprintf(buf, sizeof(buf), "Content-Length: %d", (int)rsp->_content_length);
	err = miscutil::enlist(&rsp->_headers, buf);
     }
   
   if (0 == strcmpic(csp->_http._gpc, "head"))
     {
	/*
	 * The client only asked for the head. Dispose
	 * the body and log an offensive message.
	 *
	 * While it may seem to be a bit inefficient to
	 * prepare the body if it isn't needed, it's the
	 * only way to get the Content-Length right for
	 * dynamic pages. We could have disposed the body
	 * earlier, but not without duplicating the
	 * Content-Length setting code above.
	 */
	errlog::log_error(LOG_LEVEL_CGI, "Preparing to give head to %s.", csp->_ip_addr_str);
	freez(rsp->_body);
	rsp->_body = NULL;
	rsp->_content_length = 0;
     }
   
   if (strncmpic(rsp->_status, "302", 3))
     {
	/*
	 * If it's not a redirect without any content,
	 * set the Content-Type to text/html if it's
	 * not already specified.
	 */
	if (!err) 
	  {
	     if (csp->_content_type == CT_CSS)
	       err =  miscutil::enlist_unique(&rsp->_headers, "Content-Type: text/css", 13);
	     else if (csp->_content_type == CT_XML)
	       err = miscutil::enlist_unique(&rsp->_headers, "Content-Type: application/xml", 13);
	     /* Other response types come here... */
	     else
	       err = miscutil::enlist_unique(&rsp->_headers, "Content-Type: text/html; charset=UTF-8", 13);
	  }
     }
      
   /*
    * Fill in the rest of the default headers:
    *
    * Date: set to current date/time.
    * Last-Modified: set to date/time the page was last changed.
    * Expires: set to date/time page next needs reloading.
    * Cache-Control: set to "no-cache" if applicable.
    *
    * See http://www.w3.org/Protocols/rfc2068/rfc2068
    */
   if (rsp->_is_static)
     {
	/*
	 * Set Expires to about 10 min into the future so it'll get reloaded
	 * occasionally, e.g. if Seeks' proxy gets upgraded.
	 */
	if (!err)
	  {
	     cgi::get_http_time(0, buf, sizeof(buf));
	     err = miscutil::enlist_unique_header(&rsp->_headers, "Date", buf);
	  }
	
	/* Some date in the past. */
	if (!err) err = miscutil::enlist_unique_header(&rsp->_headers, 
						       "Last-Modified", "Sat, 17 Jun 2000 12:00:00 GMT");
	
	if (!err)
	  {
	     cgi::get_http_time(10 * 60, buf, sizeof(buf)); /* 10 * 60sec = 10 minutes */
	     err = miscutil::enlist_unique_header(&rsp->_headers, "Expires", buf);
	  }
     }
   else if (!strncmpic(rsp->_status, "302", 3))
     {
	cgi::get_http_time(0, buf, sizeof(buf));
	if (!err) 
	  err = miscutil::enlist_unique_header(&rsp->_headers, "Date", buf);
     }
   else
     {
	/*
	 * Setting "Cache-Control" to "no-cache" and  "Expires" to
	 * the current time doesn't exactly forbid caching, it just
	 * requires the client to revalidate the cached copy.
	 *
	 * If a temporary problem occurs and the user tries again after
	 * getting Seeks proxy's error message, a compliant browser may set the
	 * If-Modified-Since header with the content of the error page's
	 * Last-Modified header. More often than not, the document on the server
	 * is older than Seeks proxy's error message, the server would send status code
	 * 304 and the browser would display the outdated error message again and again.
	 *
	 * For documents delivered with status code 403, 404 and 503 we set "Last-Modified"
	 * to Tim Berners-Lee's birthday, which predates the age of any page on the web
	 * and can be safely used to "revalidate" without getting a status code 304.
	 *
	 * There is no need to let the useless If-Modified-Since header reach the
	 * server, it is therefore stripped by client_if_modified_since in parsers.c.
	 */
	if (!err) 
	  err = miscutil::enlist_unique_header(&rsp->_headers, "Cache-Control", "no-cache");
	
	cgi::get_http_time(0, buf, sizeof(buf));
	if (!err) err = miscutil::enlist_unique_header(&rsp->_headers, "Date", buf);
	if (!miscutil::strncmpic(rsp->_status, "403", 3)
	    || !miscutil::strncmpic(rsp->_status, "404", 3)
	    || !miscutil::strncmpic(rsp->_status, "502", 3)
	    || !miscutil::strncmpic(rsp->_status, "503", 3)
	    || !miscutil::strncmpic(rsp->_status, "504", 3))
	  {
	     if (!err) 
	       err = miscutil::enlist_unique_header(&rsp->_headers, "Last-Modified", "Wed, 08 Jun 1955 12:00:00 GMT");
	  }
	else
	  {
	     if (!err) 
	       err = miscutil::enlist_unique_header(&rsp->_headers, "Last-Modified", buf);
	  }
	
	if (!err) err = miscutil::enlist_unique_header(&rsp->_headers, "Expires", "Sat, 17 Jun 2000 12:00:00 GMT");
	if (!err) err = miscutil::enlist_unique_header(&rsp->_headers, "Pragma", "no-cache");
     }
   
   
   /*
    * Quoting RFC 2616:
    *
    * HTTP/1.1 applications that do not support persistent connections MUST
    * include the "close" connection option in every message.
    */
   if (!err) err = miscutil::enlist_unique_header(&rsp->_headers, "Connection", "close");
   
   /*
    * Write the head
    */
   if (err || (NULL == (rsp->_head = miscutil::list_to_text(&rsp->_headers))))
     {
	delete rsp;
	return cgi::cgi_error_memory();
     }
   
   rsp->_head_length = strlen(rsp->_head);
   
   return rsp;
}

/*********************************************************************
 *
 * Function    :  template_load
 *
 * Description :  CGI support function that loads a given HTML
 *                template, ignoring comment lines and following
 *                #include statements up to a depth of 1.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  template_ptr = Destination for pointer to loaded
 *                               template text.
 *          3  :  templatename = name of the HTML template to be used
 *          4  :  recursive = Flag set if this function calls itself
 *                            following an #include statement
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.
 *                SP_ERR_FILE if the template file cannot be read
 *
 *********************************************************************/
sp_err cgi::template_load(const client_state *csp, char **template_ptr,
			  const char *templatename, 
			  const char *templatedir,
			  int recursive)
{
   sp_err err;
   const char *templates_dir_path;
   char *full_path;
   char *file_buffer;
   char *included_module;
   //const char *p;
   FILE *fp;
   char buf[BUFFER_SIZE];
   
   assert(csp);
   assert(template_ptr);
   assert(templatename);
   
   *template_ptr = NULL;
   
   /* Validate template name.  Paranoia. */
   /* for (p = templatename; *p != 0; p++)
     {
	if ( ((*p < 'a') || (*p > 'z'))
	     && ((*p < 'A') || (*p > 'Z'))
	     && ((*p < '0') || (*p > '9'))
	     && (*p != '-')
	     && (*p != '.'))
	  { */
	     /* Illegal character */
	 /*    return SP_ERR_FILE;
	  }
     } */
   
   /*
    * Generate full path using either templdir
    * or confdir/templates as base directory.
    */
/*   if (NULL != csp->_config->_templdir)
     {
	templates_dir_path = strdup(csp->_config->_templdir);
     }
   else
     {
	templates_dir_path = miscutil::make_path(csp->_config->_confdir, "templates");
     } */
   
   templates_dir_path = strdup(templatedir);
   
   if (templates_dir_path == NULL)
     {
	errlog::log_error(LOG_LEVEL_ERROR, "Out of memory while generating template path for %s.",
			  templatename);
	return SP_ERR_MEMORY;
     }
   
   full_path = seeks_proxy::make_path(templates_dir_path, templatename);
   
   if (full_path == NULL)
     {
	errlog::log_error(LOG_LEVEL_ERROR, "Out of memory while generating full template path for %s.",
			  templatename);
	return SP_ERR_MEMORY;
     }
	     
   /* Allocate buffer */
   file_buffer = strdup("");
   if (file_buffer == NULL)
     {
	errlog::log_error(LOG_LEVEL_ERROR, "Not enough free memory to buffer %s.", full_path);
	freez(full_path);
	return SP_ERR_MEMORY;
     }
      
   /* Open template file */
   if (NULL == (fp = fopen(full_path, "r")))
     {
	if (!recursive)
	  errlog::log_error(LOG_LEVEL_ERROR, "Cannot open template file %s: %E", full_path);
	freez(full_path);
	freez(file_buffer);
	return SP_ERR_FILE;
     }
   
   freez(full_path);
   
   /*
    * Read the file, ignoring comments, and honoring #include
    * statements, unless we're already called recursively.
    *
    * XXX: The comment handling could break with lines lengths > sizeof(buf).
    *      This is unlikely in practise.
    */
   while (fgets(buf, sizeof(buf), fp))
     {
	if (!recursive && !strncmp(buf, "#include ", 9))
	  {
	     // try locally
	     if (SP_ERR_OK != (err = cgi::template_load(csp, &included_module,
							miscutil::chomp(buf + 9), 
							templatedir,
							1)))
	       {
		  // try the general template repository.
		  if (SP_ERR_OK != (err = cgi::template_load(csp, &included_module, 
							     miscutil::chomp(buf + 9), 
							     csp->_config->_templdir,
							     1)))
		    {
		       errlog::log_error(LOG_LEVEL_ERROR, "Cannot open included template file %s: %E", full_path);
		       freez(file_buffer);
		       fclose(fp);
		       return err;
		    }
	       }
	     
	     
	     if (miscutil::string_join(&file_buffer, included_module))
	       {
		  fclose(fp);
		  return SP_ERR_MEMORY;
	       }
	     
	     continue;
	  }
	
	/* skip lines starting with '#' for certain file types */
	if (csp->_content_type != CT_CSS) // other types with # comments come here.
	  {
	     if (*buf == '#')
	       {
		  continue;
	       }
	  }
	
	if (miscutil::string_append(&file_buffer, buf))
	  {
	     fclose(fp);
	     return SP_ERR_MEMORY;
	  }
     }
   
   fclose(fp);
   
   *template_ptr = file_buffer;
   
   return SP_ERR_OK;
}

/*********************************************************************
 *
 * Function    :  template_fill
 *
 * Description :  CGI support function that fills in a pre-loaded
 *                HTML template by replacing @name@ with value using
 *                pcrs, for each item in the output map.
 *
 *                Note that a leading '$' character in the export map's
 *                values will be stripped and toggle on backreference
 *                interpretation.
 *
 * Parameters  :
 *          1  :  template_ptr = IN: Template to be filled out.
 *                                   Will be free()d.
 *                               OUT: Filled out template.
 *                                    Caller must free().
 *          2  :  exports = map with fill in symbol -> name pairs
 *
 * Returns     :  SP_ERR_OK on success (and for uncritical errors)
 *                SP_ERR_MEMORY on out-of-memory error
 *
 *********************************************************************/
sp_err cgi::template_fill(char **template_ptr,
			  const hash_map<const char*,const char*,hash<const char*>,eqstr> *exports)
{
   pcrs_job *job;
   char buf[BUFFER_SIZE];
   char *tmp_out_buffer;
   char *file_buffer;
   size_t size;
   int error;
   const char *flags;
   
   assert(template_ptr);
   assert(*template_ptr);
   assert(exports);
   
   file_buffer = *template_ptr;
   size = strlen(file_buffer) + 1;
   
   /*
    * Assemble pcrs joblist from exports map
    */
   hash_map<const char*,const char*,hash<const char*>,eqstr>::const_iterator mit = exports->begin();
   while(mit!=exports->end())
     {
	const char *name = (*mit).first; // beware.. const.
	const char *value = (*mit).second; // beware.. const.
	
	if (*name == '$')
	  {
	     /*
	      * First character of name is '$', so remove this flag
	      * character and allow backreferences ($1 etc) in the
	      * "replace with" text.
	      */
	     snprintf(buf, sizeof(buf), "%s", (*mit).first + 1);
	     flags = "sigU";
   
	  }
	else
	  {
	     /*
	      * Treat the "replace with" text as a literal string -
	      * no quoting needed, no backreferences allowed.
	      * ("Trivial" ['T'] flag).
	      */
	     flags = "sigTU";
	     
	     /* Enclose name in @@ */
	     snprintf(buf, sizeof(buf), "@%s@", (*mit).first);
	  }
	
	errlog::log_error(LOG_LEVEL_CGI, "Substituting: s/%s/%s/%s", buf, value, flags);
	
	/* Make and run job. */
	job = pcrs::pcrs_compile(buf, value, flags,  &error);
	if (job == NULL)
	  {
	     if (error == PCRS_ERR_NOMEM)
	       {
		  freez(file_buffer);
		  *template_ptr = NULL;
		  return SP_ERR_MEMORY;
	       }
	     else
	       {
		  errlog::log_error(LOG_LEVEL_ERROR, "Error compiling template fill job %s: %d", name, error);
		  /* Hope it wasn't important and silently ignore the invalid job */
	       }
	  }
	else
	  {
	     error = pcrs::pcrs_execute(job, file_buffer, size, &tmp_out_buffer, &size);
	     
	     delete job;  // TODO.
	     if (NULL == tmp_out_buffer)
	       {
		  *template_ptr = NULL;
		  return SP_ERR_MEMORY;
	       }
	     
	     if (error < 0)
	       {
		  /*
		   * Substitution failed, keep the original buffer,
		   * log the problem and ignore it.
		   *
		   * The user might see some unresolved @CGI_VARIABLES@,
		   * but returning a special CGI error page seems unreasonable
		   * and could mask more important error messages.
		   */
		  freez(tmp_out_buffer);
		  errlog::log_error(LOG_LEVEL_ERROR, "Failed to execute s/%s/%s/%s. %s",
				    buf, value, flags, pcrs::pcrs_strerror(error));
	       }
	     else
	       {
		  /* Substitution succeeded, use modified buffer. */
		  freez(file_buffer);
		  file_buffer = tmp_out_buffer;
	       }
	  }
	++mit;
     }
   
   /*
    * Return
    */
   *template_ptr = file_buffer;
   return SP_ERR_OK;
}

sp_err cgi::template_fill_str(char **template_ptr,
			      const hash_map<const char*,const char*,hash<const char*>,eqstr> *exports)
{     
   std::string buffer_str = std::string(*template_ptr);
   freez(*template_ptr); // beware.
   
   hash_map<const char*,const char*,hash<const char*>,eqstr>::const_iterator mit = exports->begin();
   while(mit!=exports->end())
     {
	const char *name = (*mit).first;
	const char *value = (*mit).second;
	
	std::string name_str = std::string(name);
	if (*name == '$')
	  {
	     name_str = name_str.substr(1);
	  }
	miscutil::replace_in_string(buffer_str,name_str,std::string(value));
	++mit;
     }
   
   *template_ptr = strdup(buffer_str.c_str());
   return SP_ERR_OK;
}   
   
/*********************************************************************
 *
 * Function    :  template_fill_for_cgi
 *
 * Description :  CGI support function that loads a HTML template
 *                and fills it in.  Handles file-not-found errors
 *                by sending a HTML error message.  For convenience,
 *                this function also frees the passed "exports" map.
 *
 * Parameters  :
 *          1  :  csp = Client state
 *          2  :  templatename = name of the HTML template to be used
 *          3  :  exports = map with fill in symbol -> name pairs.
 *                          Will be freed by this function.
 *          4  :  rsp = Response structure to fill in.
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error
 *
 *********************************************************************/
sp_err cgi::template_fill_for_cgi(const client_state *csp,
				  const char *templatename,
				  const char *templatedir,
				  hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
				  http_response *rsp)
{
   sp_err err;
   
   assert(csp);
   assert(templatename);
   assert(exports);
   assert(rsp);
   
   err = cgi::template_load(csp, &rsp->_body, templatename, templatedir, 0);
   if (err == SP_ERR_FILE)
     {
	miscutil::free_map(exports);
	return cgi::cgi_error_no_template(csp, rsp, templatename);
     }
   else if (err)
     {
	miscutil::free_map(exports);
	return err; /* SP_ERR_MEMORY */
     }
   err = cgi::template_fill(&rsp->_body, exports);
   miscutil::free_map(exports);
   return err;
}

sp_err cgi::template_fill_for_cgi_str(const client_state *csp,
				      const char *templatename,
				      const char *templatedir,
				      hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
				      http_response *rsp)
{
   sp_err err;
   
   assert(csp);
   assert(templatename);
   assert(exports);
   assert(rsp);
   
   err = cgi::template_load(csp, &rsp->_body, templatename, templatedir, 0);
   if (err == SP_ERR_FILE)
     {
	miscutil::free_map(exports);
	return cgi::cgi_error_no_template(csp, rsp, templatename);
     }
   else if (err)
     {
	miscutil::free_map(exports);
	return err; /* SP_ERR_MEMORY */
     }
	
   err = cgi::template_fill_str(&rsp->_body,exports);
   miscutil::free_map(exports);
   return err;
}   
   
/*********************************************************************
 *
 * Function    :  default_exports
 *
 * Description :  returns a hash_map that contains exports
 *                which are common to all CGI functions.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  caller = name of CGI who calls us and which should
 *                         be excluded from the generated menu. May be
 *                         NULL.
 * Returns     :  NULL if no memory, else a new map.  Caller frees.
 *
 *********************************************************************/
hash_map<const char*,const char*,hash<const char*>,eqstr>* cgi::default_exports(const client_state *csp,
								   const char *caller)
{
   char buf[30];
   sp_err err;
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports;
   int local_help_exists = 0;
   char *ip_address = NULL;
   char *hostname = NULL;
   
   assert(csp);
   
   exports = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
   if (exports == NULL)
     return NULL;
   
   if (csp->_config->_hostname)
     {
	spsockets::get_host_information(csp->_cfd, &ip_address, NULL);
	hostname = strdup(csp->_config->_hostname);
     }
   else
     {
	spsockets::get_host_information(csp->_cfd, &ip_address, &hostname);
     }
   
   err = miscutil::add_map_entry(exports, "version", 1, encode::html_encode(VERSION), 0);
   if (!err) err = miscutil::add_map_entry(exports, "package-version", 1, encode::html_encode(PACKAGE_VERSION), 0);
   cgi::get_locale_time(buf, sizeof(buf));
   if (!err) err = miscutil::add_map_entry(exports, "time",          1, encode::html_encode(buf), 0);
   if (!err) err = miscutil::add_map_entry(exports, "my-ip-address", 1, 
					    encode::html_encode(ip_address ? ip_address : "unknown"), 0);
   freez(ip_address); ip_address = NULL;
   if (!err) err = miscutil::add_map_entry(exports, "my-hostname",   1, 
					   encode::html_encode(hostname ? hostname : "unknown"), 0);
   freez(hostname); hostname = NULL;
   if (!err) err = miscutil::add_map_entry(exports, "homepage",      1, encode::html_encode(HOME_PAGE_URL), 0);
   if (!err) err = miscutil::add_map_entry(exports, "default-cgi",   1, encode::html_encode(CGI_PREFIX), 0);
   if (!err) err = miscutil::add_map_entry(exports, "menu",          1, 
					   cgi::make_menu(caller, csp->_config->_feature_flags), 0);
   if (!err) err = miscutil::add_map_entry(exports, "plugins-list",  1,
					   cgi::make_plugins_list(), 0);
   if (!err) err = miscutil::add_map_entry(exports, "code-status",   1, SEEKS_CODE_STATUS, 1);
   if (!miscutil::strncmpic(csp->_config->_usermanual, "file://", 7) ||
       !miscutil::strncmpic(csp->_config->_usermanual, "http", 4))
     {
	/* Manual is located somewhere else, just link to it. */
	if (!err) err = miscutil::add_map_entry(exports, "user-manual", 1, 
						encode::html_encode(csp->_config->_usermanual), 0);
     }
   else
     {
	/* Manual is delivered by Seeks proxy. */
	if (!err) err = miscutil::add_map_entry(exports, "user-manual", 1, 
						encode::html_encode(CGI_PREFIX"user-manual/"), 0);
     }
   
   if (!err) err = miscutil::add_map_entry(exports, "actions-help-prefix", 1, ACTIONS_HELP_PREFIX ,1);
#ifdef FEATURE_TOGGLE
   if (!err) err = cgi::map_conditional(exports, "enabled-display", seeks_proxy::_global_toggle_state);
#else
   if (!err) err = cgi::map_block_killer(exports, "can-toggle");
#endif
   
   snprintf(buf, sizeof(buf), "%d", csp->_config->_hport);
   if (!err) err = miscutil::add_map_entry(exports, "my-port", 1, buf, 1);
   
   if(!strcmp(SEEKS_CODE_STATUS, "stable"))
     {
	if (!err) err = cgi::map_block_killer(exports, "unstable");
     }
      
   if (csp->_config->_admin_address != NULL)
     {
	if (!err) err = miscutil::add_map_entry(exports, "admin-address", 1, 
						encode::html_encode(csp->_config->_admin_address), 0);
	local_help_exists = 1;
     }
   else
     {
	if (!err) err = cgi::map_block_killer(exports, "have-adminaddr-info");
     }
   
   if (csp->_config->_proxy_info_url != NULL)
     {
	if (!err) err = miscutil::add_map_entry(exports, "proxy-info-url", 1, 
						encode::html_encode(csp->_config->_proxy_info_url), 0);
	local_help_exists = 1;
     }
   else
     {
	if (!err) err = cgi::map_block_killer(exports, "have-proxy-info");
     }
   
   
   if (local_help_exists == 0)
     {
	if (!err) err = cgi::map_block_killer(exports, "have-help-info");
     }
      
   if (err)
     {
	miscutil::free_map(exports);
	return NULL;
     }
   
   return exports;
}

/*********************************************************************
 *
 * Function    :  map_block_killer
 *
 * Description :  Convenience function.
 *                Adds a "killer" for the conditional HTML-template
 *                block <name>, i.e. a substitution of the regex
 *                "if-<name>-start.*if-<name>-end" to the given
 *                export list.
 *
 * Parameters  :
 *          1  :  exports = map to extend
 *          2  :  name = name of conditional block
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.
 *
   *********************************************************************/
sp_err cgi::map_block_killer(hash_map<const char*,const char*,hash<const char*>,eqstr> *exports, 
			     const char *name)
{
   char buf[1000]; /* Will do, since the names are hardwired */
   
   assert(exports);
   assert(name);
   assert(strlen(name) < (size_t)490);
   
   snprintf(buf, sizeof(buf), "if-%s-start.*if-%s-end", name, name);
   return miscutil::add_map_entry(exports, buf, 1, "", 1);
}

/*********************************************************************
 *
 * Function    :  map_block_keep
 *
 * Description :  Convenience function.  Removes the markers used
 *                by map-block-killer, to save a few bytes.
 *                i.e. removes "@if-<name>-start@" and "@if-<name>-end@"
 *
 * Parameters  :
 *          1  :  exports = map to extend
 *          2  :  name = name of conditional block
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err cgi::map_block_keep(hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
			   const char *name)
{
   sp_err err;
   char buf[500]; /* Will do, since the names are hardwired */
   
   assert(exports);
   assert(name);
   assert(strlen(name) < (size_t)490);
   
   snprintf(buf, sizeof(buf), "if-%s-start", name);
   err = miscutil::add_map_entry(exports, buf, 1, "", 1);
   
   if (err)
     {
	return err;
     }
   
   snprintf(buf, sizeof(buf), "if-%s-end", name);
   return miscutil::add_map_entry(exports, buf, 1, "", 1);
}

/*********************************************************************
 *
 * Function    :  map_conditional
 *
 * Description :  Convenience function.
 *                Adds an "if-then-else" for the conditional HTML-template
 *                block <name>, i.e. a substitution of the form:
 *                @if-<name>-then@
 *                   True text
 *                @else-not-<name>@
 *                   False text
 *                @endif-<name>@
 *
 *                The control structure and one of the alternatives
 *                will be hidden.
 *
 * Parameters  :
 *          1  :  exports = map to extend
 *          2  :  name = name of conditional block
 *          3  :  choose_first = nonzero for first, zero for second.
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err cgi::map_conditional(hash_map<const char*,const char*,hash<const char*>,eqstr> *exports, 
			    const char *name, int choose_first)
{
   char buf[1000]; /* Will do, since the names are hardwired */
   sp_err err;
   
   assert(exports);
   assert(name);
   assert(strlen(name) < (size_t)480);
   
   snprintf(buf, sizeof(buf), (choose_first
			       ? "else-not-%s@.*@endif-%s"
			       : "if-%s-then@.*@else-not-%s"),
	    name, name);
   
   err = miscutil::add_map_entry(exports, buf, 1, "", 1);
   if (err)
     {
	return err;
     }
   
   snprintf(buf, sizeof(buf), (choose_first ? "if-%s-then" : "endif-%s"), name);
   return miscutil::add_map_entry(exports, buf, 1, "", 1);
}

/*********************************************************************
 *
 * Function    :  make_menu
 *
 * Description :  Returns an HTML-formatted menu of the available
 *                unhidden CGIs, excluding the one given in <self>
 *                and the toggle CGI if toggling is disabled.
 *
 * Parameters  :
 *          1  :  self = name of CGI to leave out, can be NULL for
 *                complete listing.
 *          2  :  feature_flags = feature bitmap from csp->config
 *
 *
 * Returns     :  menu string, or NULL on out-of-memory error.
 *
 *********************************************************************/
char* cgi::make_menu(const char *self, const unsigned feature_flags)
{
   cgi_dispatcher *d;
   char *result = strdup("");
   
   if (self == NULL)
     {
	self = "NO-SUCH-CGI!";
     }
   
   /* List available unhidden CGI's and export as "other-cgis" */
   for (d = cgi::_cgi_dispatchers; d->_name; d++)
     {
#ifdef FEATURE_TOGGLE
	if (!(feature_flags & RUNTIME_FEATURE_CGI_TOGGLE) && !strcmp(d->_name, "toggle"))
	  {
	     /*
	      * Suppress the toggle link if remote toggling is disabled.
	      */
	     continue;
	  }
#endif /* def FEATURE_TOGGLE */
	
	if (d->_description && strcmp(d->_name, self))
	  {
	     char *html_encoded_prefix;
	     
	     /*
	      * Line breaks would be great, but break
	      * the "blocked" template's JavaScript.
	      */
	     miscutil::string_append(&result, "<li><a href=\"");
	     html_encoded_prefix = encode::html_encode(CGI_PREFIX);
	     if (html_encoded_prefix == NULL)
	       {
		  return NULL;
	       }
	     else
	       {
		  miscutil::string_append(&result, html_encoded_prefix);
		  freez(html_encoded_prefix);
	       }
	     
	     miscutil::string_append(&result, d->_name);
	     miscutil::string_append(&result, "\">");
	     miscutil::string_append(&result, d->_description);
	     miscutil::string_append(&result, "</a></li>");
	  }
     }
   return result;
}

/********************************************************************
 *
 * Function    :  make_plugins_list
 *
 * Description :  Returns an HTML-formatted list of loaded plugins.
 *
 * Parameters  : 
 *
 * Returns     :  list string, or NULL on out-of-memory error.
 *
 *********************************************************************/
char* cgi::make_plugins_list()
{
   /* lists all loaded plugins. */
   char buf[BUFFER_SIZE];
   char *s = strdup("");
   int k=0;
   std::vector<plugin*>::const_iterator sit = plugin_manager::_plugins.begin();
   while(sit!=plugin_manager::_plugins.end())
     {
	miscutil::string_append(&s, "<li>");
	miscutil::string_join(&s, encode::html_encode((*sit)->get_name_cstr()));
	snprintf(buf, sizeof(buf),
		 "<a class=\"buttons\" href=\"/show-plugin-status?index=%u\">\tView</a>", k);
	miscutil::string_append(&s, buf);
	miscutil::string_append(&s, "</li>\n");
	++k;
	++sit;
     }
   
   if (s[0] == '\0')
     {
	s = strdup("None specified");
     }
   return s;
}

/*********************************************************************
 *
 * Function    :  dump_map
 *
 * Description :  HTML-dump a map for debugging (as table)
 *
 * Parameters  :
 *          1  :  the_map = map to dump
 *
 * Returns     :  string with HTML
 *
 *********************************************************************/
char* cgi::dump_map(const hash_map<const char*,const char*,hash<const char*>,eqstr> *the_map)
{
   char *ret = strdup("");		  
   miscutil::string_append(&ret, "<table>\n");
		  
   hash_map<const char*,const char*,hash<const char*>,eqstr>::const_iterator mit = the_map->begin();
   while(mit!=the_map->end())
     {
	miscutil::string_append(&ret, "<tr><td><b>");
	miscutil::string_join  (&ret, encode::html_encode((*mit).first));
	miscutil::string_append(&ret, "</b></td><td>");
	miscutil::string_join  (&ret, encode::html_encode((*mit).second));
	miscutil::string_append(&ret, "</td></tr>\n");

	++mit;
     }
   
   miscutil::string_append(&ret, "</table>\n");
   return ret;
}
	     

} /* end of namespace. */
