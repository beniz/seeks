/**
 * Copyright   :   Modified by Emmanuel Benazera for the Seeks Project,
 *                2009, 2010.
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

#include "blocker.h"

#include "miscutil.h"
#include "encode.h"
#include "cgi.h"
#include "parsers.h"
#include "seeks_proxy.h"
#include "errlog.h"

#include <iostream>

namespace seeks_plugins
{
   /*- blocker. -*/
   blocker::blocker()
     : plugin()
       {
	  _name = "blocker";
	  
	  _version_major = "0";
	  _version_minor = "1";
       
	  _config_filename = "blocker-config";
	  _configuration = NULL;
	    
	  _interceptor_plugin = new blocker_element(this);
       }
   
   /*- blocker_element. -*/
   std::string blocker_element::_bp_filename = "blocker/blocked-patterns";

   std::string blocker_element::_response
     = "<html> \
	  <head> \
	   <title>Request blocked</title> \
	  </head> \
	 <body> \
	 <table cellpadding=\"20\" cellspacing=\"10\" border=\"0\" width=\"100%\">\
    <tr> \
      <td class=\"status\"> \
        BLOCKED \
      </td> \
       </tr> \
	 </body> \
	 ";
   
   blocker_element::blocker_element(plugin *parent)
     :  interceptor_plugin((seeks_proxy::_datadir.empty() ? std::string(plugin_manager::_plugin_repository 
								       + blocker_element::_bp_filename).c_str()
			   : std::string(seeks_proxy::_datadir + "/plugins/" + blocker_element::_bp_filename).c_str()),
			   parent)
       {
       }
   
   http_response* blocker_element::plugin_response(client_state *csp)
     {
	return block_url(csp);
     }
   
   http_response* blocker_element::block_url(client_state *csp)
     {
	http_response *rsp = NULL;
	
	//beware, redirection should overrides blocking...
	//TODO: suggests adding redirection to this plugin.
     
	if (NULL == (rsp = new http_response()))
	  {
	     return cgi::cgi_error_memory();
	  }

	// TODO: image blocking.
	
	/*
	 * Generate an HTML "blocked" message.
	 */
	char *p = NULL;

	/*
	 * Workaround for stupid Netscape bug which prevents
	 * pages from being displayed if loading a referenced
	 * JavaScript or style sheet fails. So make it appear
	 * as if it succeeded.
	 */
	if ( NULL != (p = parsers::get_header_value(&csp->_headers, "User-Agent:"))
	     && !miscutil::strncmpic(p, "mozilla", 7) /* Catch Netscape but */
	     && !strstr(p, "Gecko")         /* save Mozilla, */
	     && !strstr(p, "compatible")    /* MSIE */
	     && !strstr(p, "Opera"))        /* and Opera. */
	  {
	    rsp->_status = strdup("200 Request for blocked URL");
	  }
	else
	  {
	    rsp->_status = strdup("403 Request for blocked URL");
	  }
	
	if (rsp->_status == NULL)
	  {
	    delete rsp;
	    return cgi::cgi_error_memory();
	  }
	rsp->_body = strdup(blocker_element::_response.c_str());
	rsp->_content_length = strlen(rsp->_body);

	//std::cerr << "[Debug]: blocked response length: " << rsp->_content_length << std::endl;
	
	rsp->_is_static = 1;
	rsp->_reason = RSP_REASON_BLOCKED;
	
	// debug
	//std::cout << "[Debug]:block_url: finishing http response\n";
	// debug  
	
	return cgi::finish_http_response(csp, rsp);
     }

  /* auto-registration */
#if defined(ON_OPENBSD) && defined(ON_OSX)
   extern "C"
  {
    plugin* maker()
    {
      return new blocker;
    }
  }
#else
   plugin* makerb()
     {
	return new blocker;
     }
   class proxy_autor_blocker
  {
  public:
    proxy_autor_blocker()
    {
      plugin_manager::_factory["blocker"] = makerb; // beware: default plugin shell with no name.
    }
  };
  
  proxy_autor_blocker _p; // one instance, instanciated when dl-opening. 
#endif
   
} /* end of namespace. */
