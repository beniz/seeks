/*********************************************************************
 * Purpose     :  Simple CGIs to get information about Seeks proxy's
 *                status.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written by and Copyright (C) 2001-2008 the SourceForge
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
 **********************************************************************/

#include "config.h"
#include "mem_utils.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "seeks_proxy.h"
#include "plugin.h"
#include "interceptor_plugin.h"
#include "action_plugin.h"
#include "filter_plugin.h"
#include "cgi.h"
#include "cgisimple.h"
#include "encode.h"
#include "proxy_dts.h"
#include "proxy_configuration.h"
#include "filters.h"
#include "miscutil.h"
#include "parsers.h"
#include "urlmatch.h"
#include "errlog.h"

namespace sp
{

/*********************************************************************
 *
 * Function    :  cgi_default
 *
 * Description :  CGI function that is called for the CGI_SITE_1_HOST
 *                and CGI_SITE_2_HOST/CGI_SITE_2_PATH base URLs.
 *                Boring - only exports the default exports.
 *               
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory
 *
 *********************************************************************/
sp_err cgisimple::cgi_default(client_state *csp,
			      http_response *rsp,
			      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports;

   (void)parameters;

   assert(csp);
   assert(rsp);

   if (NULL == (exports = cgi::default_exports(csp, "")))
   {
      return SP_ERR_MEMORY;
   }

   return cgi::template_fill_for_cgi(csp, "default", csp->_config->_templdir,
				     exports, rsp);
}


/*********************************************************************
 *
 * Function    :  cgi_error_404
 *
 * Description :  CGI function that is called if an unknown action was
 *                given.
 *               
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_error_404(client_state *csp,
				http_response *rsp,
				const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (NULL == (exports = cgi::default_exports(csp, NULL)))
   {
      return SP_ERR_MEMORY;
   }

   rsp->_status = strdup("404 Seeks proxy page not found");
   if (rsp->_status == NULL)
   {
      miscutil::free_map(exports);
      return SP_ERR_MEMORY;
   }

   return cgi::template_fill_for_cgi(csp, "cgi-error-404", csp->_config->_templdir,
				     exports, rsp);
}


#ifdef FEATURE_GRACEFUL_TERMINATION
/*********************************************************************
 *
 * Function    :  cgi_die
 *
 * Description :  CGI function to shut down Seeks proxy.
 *                NOTE: Turning this on in a production build
 *                would be a BAD idea.  An EXTREMELY BAD idea.
 *                In short, don't do it.
 *               
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_die(client_state *csp,
			  http_response *rsp,
			  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   assert(csp);
   assert(rsp);
   assert(parameters);

   /* quit */
   g_terminate = 1;

   /*
    * I don't really care what gets sent back to the browser.
    * Take the easy option - "out of memory" page.
    */

   return SP_ERR_MEMORY;
}
#endif /* def FEATURE_GRACEFUL_TERMINATION */


/*********************************************************************
 *
 * Function    :  cgi_show_request
 *
 * Description :  Show the client's request and what sed() would have
 *                made of it.
 *               
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_show_request(client_state *csp,
				   http_response *rsp,
				   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   char *p;
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (NULL == (exports = cgi::default_exports(csp, "show-request")))
   {
      return SP_ERR_MEMORY;
   }
   
   /*
    * Repair the damage done to the IOB by get_header()
    */
   for (p = csp->_iob._buf; p < csp->_iob._eod; p++)
   {
      if (*p == '\0') *p = '\n';
   }

   /*
    * Export the original client's request and the one we would
    * be sending to the server if this wasn't a CGI call
    */

   if (miscutil::add_map_entry(exports, "client-request", 1, encode::html_encode(csp->_iob._buf), 0))
   {
      miscutil::free_map(exports);
      return SP_ERR_MEMORY;
   }

  if (miscutil::add_map_entry(exports, "processed-request", 1,
			      encode::encode::html_encode_and_free_original(miscutil::list_to_text(&csp->_headers)), 0))
   {
      miscutil::free_map(exports);
      return SP_ERR_MEMORY;
   }

   return cgi::template_fill_for_cgi(csp, "show-request", csp->_config->_templdir,
				     exports, rsp);
}


/*********************************************************************
 *
 * Function    :  cgi_send_banner
 *
 * Description :  CGI function that returns a banner. 
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *           type : Selects the type of banner between "trans", "logo",
 *                  and "auto". Defaults to "logo" if absent or invalid.
 *                  "auto" means to select as if we were image-blocking.
 *                  (Only the first character really counts; b and t are
 *                  equivalent).
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_send_banner(client_state *csp,
				  http_response *rsp,
				  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   char imagetype = miscutil::lookup(parameters, "type")[0];

   /*
    * If type is auto, then determine the right thing
    * to do from the set-image-blocker action
    */
   if (imagetype == 'a') 
   {
      /*
       * Default to pattern
       */
      imagetype = 'p';

#ifdef FEATURE_IMAGE_BLOCKING
      if ((csp->_action._flags & ACTION_IMAGE_BLOCKER) != 0)
      {
         static const char prefix1[] = CGI_PREFIX "send-banner?type=";
         static const char prefix2[] = "http://" CGI_SITE_1_HOST "/send-banner?type=";
         const char *p = csp->_action._string[ACTION_STRING_IMAGE_BLOCKER];

         if (p == NULL)
         {
            /* Use default - nothing to do here. */
         }
         else if (0 == miscutil::strcmpic(p, "blank"))
         {
            imagetype = 'b';
         }
         else if (0 == miscutil::strcmpic(p, "pattern"))
         {
            imagetype = 'p';
         }

         /*
          * If the action is to call this CGI, determine
          * the argument:
          */
         else if (0 == miscutil::strncmpic(p, prefix1, sizeof(prefix1) - 1))
         {
            imagetype = p[sizeof(prefix1) - 1];
         }
         else if (0 == miscutil::strncmpic(p, prefix2, sizeof(prefix2) - 1))
         {
            imagetype = p[sizeof(prefix2) - 1];
         }

         /*
          * Everything else must (should) be a URL to
          * redirect to.
          */
         else
         {
            imagetype = 'r';
         }
      }
#endif /* def FEATURE_IMAGE_BLOCKING */
   }
      
   /*
    * Now imagetype is either the non-auto type we were called with,
    * or it was auto and has since been determined. In any case, we
    * can proceed to actually answering the request by sending a redirect
    * or an image as appropriate:
    */
   if (imagetype == 'r') 
   {
      rsp->_status = strdup("302 Local Redirect from Seeks proxy");
      if (rsp->_status == NULL)
      {
         return SP_ERR_MEMORY;
      }
      if (miscutil::enlist_unique_header(&rsp->_headers, "Location",
					 csp->_action._string[ACTION_STRING_IMAGE_BLOCKER]))
      {
         return SP_ERR_MEMORY;
      }
   }
   else
   {
      if ((imagetype == 'b') || (imagetype == 't')) 
      {
         rsp->_body = miscutil::bindup(cgi::_image_blank_data, cgi::_image_blank_length);
         rsp->_content_length = cgi::_image_blank_length;
      }
      else
      {
         rsp->_body = miscutil::bindup(cgi::_image_pattern_data, cgi::_image_pattern_length);
         rsp->_content_length = cgi::_image_pattern_length;
      }

      if (rsp->_body == NULL)
      {
         return SP_ERR_MEMORY;
      }
      if (miscutil::enlist(&rsp->_headers, "Content-Type: " BUILTIN_IMAGE_MIMETYPE))
      {
         return SP_ERR_MEMORY;
      }

      rsp->_is_static = 1;
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  cgi_transparent_image
 *
 * Description :  CGI function that sends a 1x1 transparent image.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_transparent_image(client_state *csp,
					http_response *rsp,
					const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   (void)csp;
   (void)parameters;

   rsp->_body = miscutil::bindup(cgi::_image_blank_data, cgi::_image_blank_length);
   rsp->_content_length = cgi::_image_blank_length;

   if (rsp->_body == NULL)
   {
      return SP_ERR_MEMORY;
   }

   if (miscutil::enlist(&rsp->_headers, "Content-Type: " BUILTIN_IMAGE_MIMETYPE))
   {
      return SP_ERR_MEMORY;
   }

   rsp->_is_static = 1;

   return SP_ERR_OK;
}

   
/*********************************************************************
 *
 * Function    :  cgi_send_error_favicon
 *
 * Description :  CGI function that sends the favicon for error pages.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_send_error_favicon(client_state *csp,
					 http_response *rsp,
					 const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   static const char error_favicon_data[] =
      "\000\000\001\000\001\000\020\020\002\000\000\000\000\000\260"
      "\000\000\000\026\000\000\000\050\000\000\000\020\000\000\000"
      "\040\000\000\000\001\000\001\000\000\000\000\000\100\000\000"
      "\000\000\000\000\000\000\000\000\000\002\000\000\000\000\000"
      "\000\000\377\377\377\000\000\000\377\000\017\360\000\000\077"
      "\374\000\000\161\376\000\000\161\376\000\000\361\377\000\000"
      "\361\377\000\000\360\017\000\000\360\007\000\000\361\307\000"
      "\000\361\307\000\000\361\307\000\000\360\007\000\000\160\036"
      "\000\000\177\376\000\000\077\374\000\000\017\360\000\000\360"
      "\017\000\000\300\003\000\000\200\001\000\000\200\001\000\000"
      "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
      "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
      "\000\000\200\001\000\000\200\001\000\000\300\003\000\000\360"
      "\017\000\000";
   static const size_t favicon_length = sizeof(error_favicon_data) - 1;

   (void)csp;
   (void)parameters;

   rsp->_body = miscutil::bindup(error_favicon_data, favicon_length);
   rsp->_content_length = favicon_length;

   if (rsp->_body == NULL)
   {
      return SP_ERR_MEMORY;
   }

   if (miscutil::enlist(&rsp->_headers, "Content-Type: image/x-iconn"))
   {
      return SP_ERR_MEMORY;
   }

   rsp->_is_static = 1;

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  cgi_send_stylesheet
 *
 * Description :  CGI function that sends a css stylesheet found
 *                in the cgi-style.css template
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_send_stylesheet(client_state *csp,
				      http_response *rsp,
				      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   sp_err err;
   
   assert(csp);
   assert(rsp);

   (void)parameters;

   err = cgi::template_load(csp, &rsp->_body, "cgi-style.css", 
			    csp->_config->_templdir, 0);

   if (err == SP_ERR_FILE)
   {
      /*
       * No way to tell user; send empty stylesheet
       */
      errlog::log_error(LOG_LEVEL_ERROR, "Could not find cgi-style.css template");
   }
   else if (err)
   {
      return err; /* SP_ERR_MEMORY */
   }

   if (miscutil::enlist(&rsp->_headers, "Content-Type: text/css"))
   {
      return SP_ERR_MEMORY;
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  cgi_send_url_info_osd
 *
 * Description :  CGI function that sends the OpenSearch Description
 *                template for the show-url-info page. It allows to
 *                access the page through "search engine plugins".
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_send_url_info_osd(client_state *csp,
					http_response *rsp,
					const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   sp_err err = SP_ERR_MEMORY;
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports = cgi::default_exports(csp, NULL);

   (void)csp;
   (void)parameters;

   if (NULL != exports)
   {
      err = cgi::template_fill_for_cgi(csp, "url-info-osd.xml", csp->_config->_templdir,
				       exports, rsp);
      if (SP_ERR_OK == err)
      {
         err = miscutil::enlist(&rsp->_headers,
            "Content-Type: application/opensearchdescription+xml");
      }
   }

   return err;

}


/*********************************************************************
 *
 * Function    :  cgi_send_user_manual
 *
 * Description :  CGI function that sends a file in the user
 *                manual directory.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : file=name.html, the name of the HTML file
 *                  (relative to user-manual from config)
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_send_user_manual(client_state *csp,
				       http_response *rsp,
				       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   const char * filename;
   char *full_path;
   sp_err err = SP_ERR_OK;
   size_t length;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (parameters->empty())
   {
      /* requested http://p.p/user-manual (without trailing slash) */
      return cgi::cgi_redirect(rsp, CGI_PREFIX "user-manual/");
   }

   cgi::get_string_param(parameters, "file", &filename);
   /* Check parameter for hack attempts */
   if (filename && strchr(filename, '/'))
   {
      return SP_ERR_CGI_PARAMS;
   }
   if (filename && strstr(filename, ".."))
   {
      return SP_ERR_CGI_PARAMS;
   }

   full_path = seeks_proxy::make_path(csp->_config->_usermanual, filename ? filename : "index.html");
   if (full_path == NULL)
   {
      return SP_ERR_MEMORY;
   }

   err = cgisimple::load_file(full_path, &rsp->_body, &rsp->_content_length);
   if (SP_ERR_OK != err)
   {
      assert((SP_ERR_FILE == err) || (SP_ERR_MEMORY == err));
      if (SP_ERR_FILE == err)
      {
         err = cgi::cgi_error_no_template(csp, rsp, full_path);
      }
      freez(full_path);
      full_path = NULL;
      return err;
   }
   freez(full_path);
   full_path = NULL;
   
   /* Guess correct Content-Type based on the filename's ending */
   if (filename)
   {
      length = strlen(filename);
   }
   else
   {
      length = 0;
   } 
   if((length>=4) && !strcmp(&filename[length-4], ".css"))
   {
      err = miscutil::enlist(&rsp->_headers, "Content-Type: text/css");
   }
   else if((length>=4) && !strcmp(&filename[length-4], ".jpg"))
   {
      err = miscutil::enlist(&rsp->_headers, "Content-Type: image/jpeg");
   }
   else if((length>=4) && !strcmp(&filename[length-4], ".ico"))
   {
      err = miscutil::enlist(&rsp->_headers, "Content-Type: image/x-icon");
   }
   else if((length>=4) && !strcmp(&filename[length-4], ".xml"))
   {
      err = miscutil::enlist(&rsp->_headers, "Content-Type: text/xml");
   }
   else
   {
      err = miscutil::enlist(&rsp->_headers, "Content-Type: text/html");
   }

   return err;
}


/*********************************************************************
 *
 * Function    :  cgi_show_version
 *
 * Description :  CGI function that returns a a web page describing the
 *                file versions of Seeks proxy.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_show_version(client_state *csp,
				   http_response *rsp,
				   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (NULL == (exports = cgi::default_exports(csp, "show-version")))
   {
      return SP_ERR_MEMORY;
   }

   /* if (miscutil::add_map_entry(exports, "sourceversions", 1, show_rcs(), 0))
   {
      miscutil::free_map(exports);
      return SP_ERR_MEMORY;
   } */

   return cgi::template_fill_for_cgi(csp, "show-version", csp->_config->_templdir,
				     exports, rsp);
}


/*********************************************************************
 *
 * Function    :  cgi_show_status
 *
 * Description :  CGI function that returns a web page describing the
 *                current status of Seeks proxy.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *        file :  Which file to show.  Only first letter is checked,
 *                valid values are:
 *                - "a"ction file
 *                - "r"egex
 *                - "t"rust
 *                Default is to show menu and other information.
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_show_status(client_state *csp,
				  http_response *rsp,
				  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   char *s = NULL;
   int j;

   char buf[BUFFER_SIZE];
#ifdef FEATURE_STATISTICS
   float perc_rej;   /* Percentage of http requests rejected */
   int local_urls_read;
   int local_urls_rejected;
#endif /* ndef FEATURE_STATISTICS */
   sp_err err = SP_ERR_OK;

   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (NULL == (exports = cgi::default_exports(csp, "show-status")))
   {
      return SP_ERR_MEMORY;
   }

   s = strdup("");
   for (j = 0; (s != NULL) && (j < seeks_proxy::_Argc); j++)
   {
      if (!err) err = miscutil::string_join  (&s, encode::html_encode(seeks_proxy::_Argv[j]));
      if (!err) err = miscutil::string_append(&s, " ");
   }
   if (!err) err = miscutil::add_map_entry(exports, "invocation", 1, s, 0);

   //std::cerr << "config args: " << seeks_proxy::_config->_config_args << std::endl;
   
   if (!err) err = miscutil::add_map_entry(exports, "options", 1, csp->_config->_config_args, 1);
   if (!err) err = show_defines(exports);

   if (err) 
   {
      miscutil::free_map(exports);
      return SP_ERR_MEMORY;
   }

#ifdef FEATURE_STATISTICS
   local_urls_read     = seeks_proxy::_urls_read;
   local_urls_rejected = seeks_proxy::_urls_rejected;

   /*
    * Need to alter the stats not to include the fetch of this
    * page.
    *
    * Can't do following thread safely! doh!
    *
    * urls_read--;
    * urls_rejected--; * This will be incremented subsequently *
    */

   if (local_urls_read == 0)
   {
      if (!err) err = cgi::map_block_killer(exports, "have-stats");
   }
   else
   {
      if (!err) err = cgi::map_block_killer(exports, "have-no-stats");

      perc_rej = (float)local_urls_rejected * 100.0F /
            (float)local_urls_read;

      snprintf(buf, sizeof(buf), "%d", local_urls_read);
      if (!err) err = miscutil::add_map_entry(exports, "requests-received", 1, buf, 1);

      snprintf(buf, sizeof(buf), "%d", local_urls_rejected);
      if (!err) err = miscutil::add_map_entry(exports, "requests-blocked", 1, buf, 1);

      snprintf(buf, sizeof(buf), "%6.2f", perc_rej);
      if (!err) err = miscutil::add_map_entry(exports, "percent-blocked", 1, buf, 1);
   }

#else /* ndef FEATURE_STATISTICS */
   err = err || cgi::map_block_killer(exports, "statistics");
#endif /* ndef FEATURE_STATISTICS */
   
   if (!err) err = cgi::map_block_killer(exports, "trust-support");

   if (err)
   {
      miscutil::free_map(exports);
      return SP_ERR_MEMORY;
   }

   return cgi::template_fill_for_cgi(csp, "show-status", csp->_config->_templdir,
				     exports, rsp);
}

/*********************************************************************
 *
 * Function    :  cgi_show_url_info
 *
 * Description :  CGI function that determines and shows which plugins
 *                Seeks proxy will perform for a given url, and which
 *                matches starting from the defaults have lead to that.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *            url : The url whose plugins are to be determined.
 *                  If url is unset, the url-given conditional will be
 *                  set, so that all but the form can be suppressed in
 *                  the template.
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_show_url_info(client_state *csp,
				    http_response *rsp,
				    const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   char *url_param;
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports;
   char buf[150];

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (NULL == (exports = cgi::default_exports(csp, "show-url-info")))
   {
      return SP_ERR_MEMORY;
   }

   /*
    * Get the url= parameter (if present) and remove any leading/trailing spaces.
    */
   url_param = strdup(miscutil::lookup(parameters, "url"));
   if (url_param == NULL)
   {
      miscutil::free_map(exports);
      return SP_ERR_MEMORY;
   }
   miscutil::chomp(url_param);

   /*
    * Handle prefixes.  4 possibilities:
    * 1) "http://" or "https://" prefix present and followed by URL - OK
    * 2) Only the "http://" or "https://" part is present, no URL - change
    *    to empty string so it will be detected later as "no URL".
    * 3) Parameter specified but doesn't contain "http(s?)://" - add a
    *    "http://" prefix.
    * 4) Parameter not specified or is empty string - let this fall through
    *    for now, next block of code will handle it.
    */
   if (0 == strncmp(url_param, "http://", 7))
   {
      if (url_param[7] == '\0')
      {
         /*
          * Empty URL (just prefix).
          * Make it totally empty so it's caught by the next if()
          */
         url_param[0] = '\0';
      }
   }
   else if (0 == strncmp(url_param, "https://", 8))
   {
      if (url_param[8] == '\0')
      {
         /*
          * Empty URL (just prefix).
          * Make it totally empty so it's caught by the next if()
          */
         url_param[0] = '\0';
      }
   }
   else if ((url_param[0] != '\0') && (NULL == strstr(url_param, "://")))
   {
      /* No prefix - assume http:// */
      char *url_param_prefixed = strdup("http://");

      if (SP_ERR_OK != miscutil::string_join(&url_param_prefixed, url_param))
      {
         miscutil::free_map(exports);
         return SP_ERR_MEMORY;
      }
      url_param = url_param_prefixed;
   }

   /*
    * Hide "toggle off" warning if Seeks proxy is toggled on.
    */
   if (
#ifdef FEATURE_TOGGLE
       (seeks_proxy::_global_toggle_state == 1) &&
#endif /* def FEATURE_TOGGLE */
       cgi::map_block_killer(exports, "privoxy-is-toggled-off")
      )
   {
      miscutil::free_map(exports);
      return SP_ERR_MEMORY;
   }

   if (url_param[0] == '\0')
   {
      /* URL parameter not specified, display query form only. */
      freez(url_param);
      if (cgi::map_block_killer(exports, "url-given")
        || miscutil::add_map_entry(exports, "url", 1, "", 1))
      {
         miscutil::free_map(exports);
         return SP_ERR_MEMORY;
      }
   }
   else
   {
      /* Given a URL, so query it. */
      sp_err err;
      char *matches;
      int hits = 0;
      http_request url_to_query;
      current_action_spec action;
      int i = 0;
      
      if (miscutil::add_map_entry(exports, "url", 1, encode::html_encode(url_param), 0))
      {
         freez(url_param);
         miscutil::free_map(exports);
         return SP_ERR_MEMORY;
      }
      
      //seeks: deactivated.
      /* if (miscutil::add_map_entry(exports, "default", 1, actions::current_action_to_html(csp, &action), 0))
      {
         freez(url_param);
         miscutil::free_map(exports);
         return SP_ERR_MEMORY;
      } */

      err = urlmatch::parse_http_url(url_param, &url_to_query, REQUIRE_PROTOCOL);
      assert((err != SP_ERR_OK) || (url_to_query._ssl == !miscutil::strncmpic(url_param, "https://", 8)));

      freez(url_param);

      if (err == SP_ERR_MEMORY)
      {
         miscutil::free_map(exports);
         return SP_ERR_MEMORY;
      }
      else if (err)
      {
         /* Invalid URL */
         err = miscutil::add_map_entry(exports, "matches", 1, "<b>[Invalid URL specified!]</b>" , 1);
         if (!err) err = miscutil::add_map_entry(exports, "final", 1, miscutil::lookup(exports, "default"), 1);
         if (!err) err = cgi::map_block_killer(exports, "valid-url");

         if (err)
         {
            miscutil::free_map(exports);
            return SP_ERR_MEMORY;
         }

         return cgi::template_fill_for_cgi(csp, "show-url-info", csp->_config->_templdir,
					   exports, rsp);
      }

      /*
       * We have a warning about SSL paths.  Hide it for unencrypted sites.
       */
      if (!url_to_query._ssl)
      {
         if (cgi::map_block_killer(exports, "https"))
         {
	    miscutil::free_map(exports);
	    return SP_ERR_MEMORY;
         }
      }

      matches = strdup("<table summary=\"\" class=\"transparent\">");

      std::vector<plugin*>::const_iterator sit = plugin_manager::_plugins.begin();
      while(sit!=plugin_manager::_plugins.end())
	{
	   plugin *p = (*sit);
	   miscutil::string_append(&matches, "<tr><th>By plugin: ");
	   miscutil::string_join  (&matches, encode::html_encode(p->get_name_cstr()));
	   snprintf(buf, sizeof(buf), " <a class=\"cmd\" href=\"/show-status?index=%d\">", i); // TODO.
	   miscutil::string_append(&matches, buf);
	   miscutil::string_append(&matches, "View</a>");
	   miscutil::string_append(&matches, "</th></tr>\n");
	   hits = 0;
	   
	   // TODO.
	   if (p->_interceptor_plugin)
	     {
	     }
	   
	   if (p->_action_plugin)
	     {
	     }
	   
	   if (p->_filter_plugin)
	     {
		
	     }
	   
	   
	   ++sit;
	}
      
      miscutil::string_append(&matches, "</table>\n");
      
      /*
       * Fill in forwarding settings.
       *
       * The possibilities are:
       *  - no forwarding
       *  - http forwarding only
       *  - socks4(a) forwarding only
       *  - socks4(a) and http forwarding.
       *
       * XXX: Parts of this code could be reused for the
       * "forwarding-failed" template which currently doesn't
       * display the proxy port and an eventual second forwarder.
       */
     /* {
         const forward_spec *fwd = filters::forward_url(csp, &url_to_query);

         if ((fwd->_gateway_host == NULL) && (fwd->_forward_host == NULL))
         {
            if (!err) err = cgi::map_block_killer(exports, "socks-forwarder");
            if (!err) err = cgi::map_block_killer(exports, "http-forwarder");
         }
         else
         {
            char port[10]; */ /* We save proxy ports as int but need a string here */

/*            if (!err) err = cgi::map_block_killer(exports, "no-forwarder");

            if (fwd->_gateway_host != NULL)
            {
               char *socks_type = NULL;

               switch (fwd->_type)
               {
                  case SOCKS_4:
		  socks_type = (char*)"socks4";
                     break;
                  case SOCKS_4A:
                     socks_type = (char*)"socks4a";
                     break;
                  case SOCKS_5:
                     socks_type = (char*)"socks5";
                     break;
                  default:
                     errlog::log_error(LOG_LEVEL_FATAL, "Unknown socks type: %d.", fwd->_type);
               }

               if (!err) err = miscutil::add_map_entry(exports, "socks-type", 1, socks_type, 1);
               if (!err) err = miscutil::add_map_entry(exports, "gateway-host", 1, fwd->_gateway_host, 1);
               snprintf(port, sizeof(port), "%d", fwd->_gateway_port);
               if (!err) err = miscutil::add_map_entry(exports, "gateway-port", 1, port, 1);
            }
            else
            {
               if (!err) err = cgi::map_block_killer(exports, "socks-forwarder");
            }

            if (fwd->_forward_host != NULL)
            {
               if (!err) err = miscutil::add_map_entry(exports, "forward-host", 1, fwd->_forward_host, 1);
               snprintf(port, sizeof(port), "%d", fwd->_forward_port);
               if (!err) err = miscutil::add_map_entry(exports, "forward-port", 1, port, 1);
            }
            else
            {
               if (!err) err = cgi::map_block_killer(exports, "http-forwarder");
            }
         }
      } */
      
      if (err || matches == NULL)
      {
	 miscutil::free_map(exports);
         return SP_ERR_MEMORY;
      }

      /*
       * If zlib support is available, if no content filters
       * are enabled or if the prevent-compression action is enabled,
       * suppress the "compression could prevent filtering" warning.
       */
#ifndef FEATURE_ZLIB
      if (!content_filters_enabled(action) ||
         (action->_flags & ACTION_NO_COMPRESSION))
#endif
      {
         if (!err) err = cgi::map_block_killer(exports, "filters-might-be-ineffective");
      }

      if (err || miscutil::add_map_entry(exports, "matches", 1, matches , 0))
      {
	 miscutil::free_map(exports);
         return SP_ERR_MEMORY;
      }

      /* s = actions::current_action_to_html(csp, &action);

      if (miscutil::add_map_entry(exports, "final", 1, s, 0))
      {
         miscutil::free_map(exports);
         return SP_ERR_MEMORY;
      } */
   }

   return cgi::template_fill_for_cgi(csp, "show-url-info", csp->_config->_templdir,
				     exports, rsp);
}


/*********************************************************************
 *
 * Function    :  cgi_robots_txt
 *
 * Description :  CGI function to return "/robots.txt".
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::cgi_robots_txt(client_state *csp,
				 http_response *rsp,
				 const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   char buf[100];
   sp_err err;

   (void)csp;
   (void)parameters;

   rsp->_body = strdup(
      "# This is the Seeks proxy control interface.\n"
      "# It isn't very useful to index it, and you're likely to break stuff.\n"
      "# So go away!\n"
      "\n"
      "User-agent: *\n"
      "Disallow: /\n"
      "\n");
   if (rsp->_body == NULL)
   {
      return SP_ERR_MEMORY;
   }

   err = miscutil::enlist_unique(&rsp->_headers, "Content-Type: text/plain", 13);

   rsp->_is_static = 1;

   cgi::get_http_time(7 * 24 * 60 * 60, buf, sizeof(buf)); /* 7 days into future */
   if (!err) err = miscutil::enlist_unique_header(&rsp->_headers, "Expires", buf);

   return (err ? SP_ERR_MEMORY : SP_ERR_OK);
}


/*********************************************************************
 *
 * Function    :  show_defines
 *
 * Description :  Add to a map the state od all conditional #defines
 *                used when building
 *
 * Parameters  :
 *          1  :  exports = map to extend
 *
 * Returns     :  SP_ERR_OK on success
 *                SP_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
sp_err cgisimple::show_defines(hash_map<const char*,const char*,hash<const char*>,eqstr> *exports)
{
   sp_err err = SP_ERR_OK;

#ifdef FEATURE_ACL
   if (!err) err = cgi::map_conditional(exports, "FEATURE_ACL", 1);
#else /* ifndef FEATURE_ACL */
   if (!err) err = cgi::map_conditional(exports, "FEATURE_ACL", 0);
#endif /* ndef FEATURE_ACL */

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   if (!err) err = cgi::map_conditional(exports, "FEATURE_CONNECTION_KEEP_ALIVE", 1);
#else 
   if (!err) err = cgi::map_conditional(exports, "FEATURE_CONNECTION_KEEP_ALIVE", 0);
#endif /* ndef FEATURE_CONNECTION_KEEP_ALIVE */

#ifdef FEATURE_FAST_REDIRECTS
   if (!err) err = cgi::map_conditional(exports, "FEATURE_FAST_REDIRECTS", 1);
#else /* ifndef FEATURE_FAST_REDIRECTS */
   if (!err) err = cgi::map_conditional(exports, "FEATURE_FAST_REDIRECTS", 0);
#endif /* ndef FEATURE_FAST_REDIRECTS */

/*#ifdef FEATURE_FORCE_LOAD
   if (!err) err = cgi::map_conditional(exports, "FEATURE_FORCE_LOAD", 1);
   if (!err) err = miscutil::add_map_entry(exports, "FORCE_PREFIX", 1, FORCE_PREFIX, 1);
#else*/ /* ifndef FEATURE_FORCE_LOAD */
/*   if (!err) err = cgi::map_conditional(exports, "FEATURE_FORCE_LOAD", 0);
   if (!err) err = miscutil::add_map_entry(exports, "FORCE_PREFIX", 1, "(none - disabled)", 1);
#endif*/ /* ndef FEATURE_FORCE_LOAD */

#ifdef FEATURE_GRACEFUL_TERMINATION
   if (!err) err = cgi::map_conditional(exports, "FEATURE_GRACEFUL_TERMINATION", 1);
#else /* ifndef FEATURE_GRACEFUL_TERMINATION */
   if (!err) err = cgi::map_conditional(exports, "FEATURE_GRACEFUL_TERMINATION", 0);
#endif /* ndef FEATURE_GRACEFUL_TERMINATION */

/*#ifdef FEATURE_IMAGE_BLOCKING
   if (!err) err = cgi::map_conditional(exports, "FEATURE_IMAGE_BLOCKING", 1);
#else*/ /* ifndef FEATURE_IMAGE_BLOCKING */
   /*if (!err) err = cgi::map_conditional(exports, "FEATURE_IMAGE_BLOCKING", 0);
#endif */ /* ndef FEATURE_IMAGE_BLOCKING */

#ifdef HAVE_RFC2553
   if (!err) err = cgi::map_conditional(exports, "FEATURE_IPV6_SUPPORT", 1);
#else /* ifndef HAVE_RFC2553 */
   if (!err) err = cgi::map_conditional(exports, "FEATURE_IPV6_SUPPORT", 0);
#endif /* ndef HAVE_RFC2553 */

/* #ifdef FEATURE_NO_GIFS
   if (!err) err = cgi::map_conditional(exports, "FEATURE_NO_GIFS", 1);
#else */ /* ifndef FEATURE_NO_GIFS */
/*   if (!err) err = cgi::map_conditional(exports, "FEATURE_NO_GIFS", 0);
#endif */ /* ndef FEATURE_NO_GIFS */

#ifdef FEATURE_PTHREAD
   if (!err) err = cgi::map_conditional(exports, "FEATURE_PTHREAD", 1);
#else /* ifndef FEATURE_PTHREAD */
   if (!err) err = cgi::map_conditional(exports, "FEATURE_PTHREAD", 0);
#endif /* ndef FEATURE_PTHREAD */

#ifdef FEATURE_STATISTICS
   if (!err) err = cgi::map_conditional(exports, "FEATURE_STATISTICS", 1);
#else /* ifndef FEATURE_STATISTICS */
   if (!err) err = cgi::map_conditional(exports, "FEATURE_STATISTICS", 0);
#endif /* ndef FEATURE_STATISTICS */

#ifdef FEATURE_TOGGLE
   if (!err) err = cgi::map_conditional(exports, "FEATURE_TOGGLE", 1);
#else /* ifndef FEATURE_TOGGLE */
   if (!err) err = cgi::map_conditional(exports, "FEATURE_TOGGLE", 0);
#endif /* ndef FEATURE_TOGGLE */

#ifdef FEATURE_ZLIB
   if (!err) err = cgi::map_conditional(exports, "FEATURE_ZLIB", 1);
#else /* ifndef FEATURE_ZLIB */
   if (!err) err = cgi::map_conditional(exports, "FEATURE_ZLIB", 0);
#endif /* ndef FEATURE_ZLIB */

   return err;
}

sp_err cgisimple::cgi_show_plugin(client_state *csp,
				  http_response *rsp,
				  const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
{
   unsigned i = 0;
   sp_err err = cgi::get_number_param(csp, parameters, (char*)"index", &i);
   if (err == SP_ERR_CGI_PARAMS)
     return err;
   
   unsigned c = 0;
   
   plugin *pl = NULL;
   const char *plugin_name = NULL;
   const char *plugin_description = NULL;
   
   std::vector<plugin*>::const_iterator sit = plugin_manager::_plugins.begin();
   while(sit!=plugin_manager::_plugins.end())
     {
	if (c == i)  // plugin has been found.
	  {
	     pl = (*sit);
	     plugin_name = pl->get_name_cstr();
	     plugin_description = pl->get_description_cstr();
	     break;
	  }
	
	++c;
	++sit;
     }
   
   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
     = cgi::default_exports(csp,"show-plugin-status");
   
   if (!exports)
     return SP_ERR_MEMORY;
   
   if (!plugin_name)
     {
	/* should not reach here... */
	miscutil::add_map_entry(exports, "plugin_name", 1, "UNKNOWN PLUGIN!", 1);
     }
   else
     {
	//s = encode::html_encode_and_free_original(s);
	plugin_name = encode::html_encode(plugin_name);
	plugin_description = encode::html_encode(plugin_description);
	
	if (NULL == plugin_name)
	  {
	     return SP_ERR_MEMORY;
	  }
	
	/* if (miscutil::add_map_entry(exports, "contents", 1, s, 0))
	  {
	     miscutil::free_map(exports);
	     return SP_ERR_MEMORY;
	  } */
	
	if (miscutil::add_map_entry(exports,"plugin_name",1,plugin_name,0))
	  {
	     miscutil::free_map(exports);
	     return SP_ERR_MEMORY;
	  }
	
	if (miscutil::add_map_entry(exports,"plugin_description",1,plugin_description,0))
	  {
	     miscutil::free_map(exports);
	     return SP_ERR_MEMORY;
	  }
	
	if (pl->_configuration &&
	    miscutil::add_map_entry(exports,"options",1,pl->_configuration->_config_args,1))
	  {
	     miscutil::free_map(exports);
	     return SP_ERR_MEMORY;
	  }
     }
   return cgi::template_fill_for_cgi(csp, "show-status-plugin", csp->_config->_templdir,
				     exports, rsp);
}
   
   
 /*********************************************************************
 *
 * Function    :  load_file
 *
 * Description :  Loads a file into a buffer.
 *
 * Parameters  :
 *          1  :  filename = Name of the file to be loaded.
 *          2  :  buffer   = Used to return the file's content.
 *          3  :  length   = Used to return the size of the file.
 *
 * Returns     :  SP_ERR_OK in case of success,
 *                SP_ERR_FILE in case of ordinary file loading errors
 *                            (fseek() and ftell() errors are fatal)
 *                SP_ERR_MEMORY in case of out-of-memory.
 *
 *********************************************************************/
sp_err cgisimple::load_file(const char *filename, char **buffer, size_t *length)
{
   FILE *fp;
   long ret;
   sp_err err = SP_ERR_OK;

   fp = fopen(filename, "rb");
   if (NULL == fp)
   {
      return SP_ERR_FILE;
   }

   /* Get file length */
   if (fseek(fp, 0, SEEK_END))
   {
      errlog::log_error(LOG_LEVEL_FATAL,
         "Unexpected error while fseek()ing to the end of %s: %E",
         filename);
   }
   ret = ftell(fp);
   if (-1 == ret)
   {
      errlog::log_error(LOG_LEVEL_FATAL,
         "Unexpected ftell() error while loading %s: %E",
         filename);
   }
   *length = (size_t)ret;

   /* Go back to the beginning. */
   if (fseek(fp, 0, SEEK_SET))
   {
      errlog::log_error(LOG_LEVEL_FATAL,
         "Unexpected error while fseek()ing to the beginning of %s: %E",
         filename);
   }

   *buffer = (char*) std::malloc(*length+1);
   if (NULL == *buffer)
   {
      err = SP_ERR_MEMORY;
   }
   else if (!fread(*buffer, *length, 1, fp))
   {
      /*
       * May happen if the file size changes between fseek() and
       * fread(). If it does, we just log it and serve what we got.
       */
      errlog::log_error(LOG_LEVEL_ERROR,
         "Couldn't completely read file %s.", filename);
      err = SP_ERR_FILE;
   }

   fclose(fp);
   
   return err;

}

#ifdef FEATURE_TOGGLE
/*********************************************************************
 *
 * Function    :  cgi_toggle
 *
 * Description :  CGI function that adds a new empty section to
 *                an actions file.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *         set : If present, how to change toggle setting:
 *               "enable", "disable", "toggle", or none (default).
 *        mini : If present, use mini reply template.
 *
 * Returns     :  SP_ERR_OK     on success
 *                SP_ERR_MEMORY on out-of-memory
 *
 *********************************************************************/
sp_err cgisimple::cgi_toggle(client_state *csp,
			     http_response *rsp,
			     const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
	{
	   
	   hash_map<const char*,const char*,hash<const char*>,eqstr> *exports;
	   char mode;    
	   const char *template_name;
	   
	   assert(csp);
	   assert(rsp);
	   assert(parameters);
	   
	   if (0 == (csp->_config->_feature_flags & RUNTIME_FEATURE_CGI_TOGGLE))
	     {
		return cgi::cgi_error_disabled(csp, rsp);
	     }
	   
	   mode = cgi::get_char_param(parameters, "set");
	   
	   if (mode == 'E')
	     {
		/* Enable */
		seeks_proxy::_global_toggle_state = 1;
	     }
	   else if (mode == 'D')
	     {
		/* Disable */
		seeks_proxy::_global_toggle_state = 0;
	     }
	   else if (mode == 'T')
	     {
		/* Toggle */
		seeks_proxy::_global_toggle_state = !seeks_proxy::_global_toggle_state;
	     }
	   
	   if (NULL == (exports = cgi::default_exports(csp, "toggle")))
	     {
		
		return SP_ERR_MEMORY;
	     }
	   
	   template_name = (cgi::get_char_param(parameters, "mini")
			    ? "toggle-mini"
			    : "toggle");
	   
	   return cgi::template_fill_for_cgi(csp, template_name, csp->_config->_templdir,
					     exports, rsp);
	}
#endif /* def FEATURE_TOGGLE */

/**
* This could be automated for a wider set of content by
* using a dedicated library.
*/
void cgisimple::file_response_content_type(const std::string &ext_str, http_response *rsp)
{     
   if (strcmpic(ext_str.c_str(),"css") == 0)
     miscutil::enlist_unique(&rsp->_headers, "Content-Type: text/css", 13);
   else if (strcmpic(ext_str.c_str(),"jpg") == 0
	    || strcmpic(ext_str.c_str(),"jpeg") == 0)
     miscutil::enlist_unique(&rsp->_headers, "Content-Type: image/jpeg", 13);
   else if (strcmpic(ext_str.c_str(),"png") == 0)
     miscutil::enlist_unique(&rsp->_headers, "Content-Type: image/png", 13);
   else if (strcmpic(ext_str.c_str(),"ico") == 0)
     miscutil::enlist_unique(&rsp->_headers, "Content-Type: image/x-icon", 13);
   else if (strcmpic(ext_str.c_str(),"gif") == 0)
     miscutil::enlist_unique(&rsp->_headers, "Content-Type: image/gif", 13);
   else if (strcmpic(ext_str.c_str(),"js") == 0)
     // should be application/javascript but IE8 and earlier wouldn't eat it.
     miscutil::enlist_unique(&rsp->_headers, "Content-Type: text/javascript", 13);
   else if (strcmpic(ext_str.c_str(),"jso") == 0)
     miscutil::enlist_unique(&rsp->_headers, "Content-Type: application/json", 13);
   else if (strcmpic(ext_str.c_str(),"xml") == 0)
     miscutil::enlist_unique(&rsp->_headers, "Content-Type: text/xml", 13);
   else miscutil::enlist_unique(&rsp->_headers, "Content-Type: text/html; charset=UTF-8", 13);
}   
   
sp_err cgisimple::cgi_file_server(client_state *csp,
				  http_response *rsp,
				  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   const char *path_file = miscutil::lookup(parameters,"file");
   if (!path_file)
     {
	errlog::log_error(LOG_LEVEL_ERROR, "Could not find path to public file.");
	return cgisimple::cgi_error_404(csp,rsp,parameters);
     }
   
   std::string path_file_str;
   if (seeks_proxy::_datadir.empty())
     path_file_str = std::string(seeks_proxy::_basedir);
   else path_file_str = std::string(seeks_proxy::_datadir);
   path_file_str += "/" + std::string(CGI_SITE_FILE_SERVER) + "/" + std::string(path_file);
   
   sp_err err = cgisimple::load_file(path_file_str.c_str(),&rsp->_body,&rsp->_content_length);

   /**
    * Lookup file extention for setting content-type.
    */
   if (!err)
     {
	size_t epos = path_file_str.find_last_of(".");
	std::string ext_str;
	try
	  {
	     ext_str = path_file_str.substr(epos+1);
	  }
	catch(std::exception &e)
	  {
	     ext_str = "";
	  }
	cgisimple::file_response_content_type(ext_str,rsp);
     }
      
   if (err != SP_ERR_OK)
     {
	errlog::log_error(LOG_LEVEL_ERROR, "Could not load %s in public repository.",
			  path_file_str.c_str());
	return cgisimple::cgi_error_404(csp,rsp,parameters);
     }
   
   rsp->_is_static = 1;
   
   return SP_ERR_OK;
}
   
sp_err cgisimple::cgi_plugin_file_server(client_state *csp,
					 http_response *rsp,
					 const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
{
   const char *path_file = miscutil::lookup(parameters,"file");
   if (!path_file)
     {
	errlog::log_error(LOG_LEVEL_ERROR, "Could not find path to public file.");
	return cgisimple::cgi_error_404(csp,rsp,parameters);
     }
   
   std::string path_file_str;
   if (seeks_proxy::_datadir.empty())
     path_file_str = plugin_manager::_plugin_repository + "/" + std::string(path_file);
   else path_file_str = seeks_proxy::_datadir + "plugins/" + std::string(path_file);
   
   sp_err err = cgisimple::load_file(path_file_str.c_str(),&rsp->_body,&rsp->_content_length);
   
   /**
    * Lookup file extention for setting content-type.
    */
   if (!err)
     {
	size_t epos = path_file_str.find_last_of(".");
	std::string ext_str;
	try
	  {
	     ext_str = path_file_str.substr(epos+1);
	  }
	catch(std::exception &e)
	  {
	     ext_str = "";
	  }
	cgisimple::file_response_content_type(ext_str,rsp);
     }
        
   if (err != SP_ERR_OK)
     {
	errlog::log_error(LOG_LEVEL_ERROR, "Could not load %s in public repository.",
			  path_file_str.c_str());
	return cgisimple::cgi_error_404(csp,rsp,parameters);
     }
   
   rsp->_is_static = 1;
   
   return SP_ERR_OK;
}
   
   
} /* end of namespace. */
