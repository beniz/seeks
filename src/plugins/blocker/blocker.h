/**
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

#ifndef PRIVOXY_BLOCK_H
#define PRIVOXY_BLOCK_H

#define NO_PERL // we do not use Perl.

#include "plugin.h"
#include "interceptor_plugin.h"

using namespace sp;

namespace seeks_plugins
{
   class blocker : public plugin
     {
      public:
	blocker();
	
	~blocker() {};
	
	virtual void start() {};
	
	virtual void stop() {};
	
      private:
     };
   
   class blocker_element : public interceptor_plugin
     {
      public:
	blocker_element(plugin *parent);
	
	~blocker_element() {};
	
	// virtual from plugin_element.
	http_response* plugin_response(client_state *csp);

      private:
	http_response* block_url(client_state *csp);
     
	static std::string _bp_filename; // blocked patterns filename = "blocked-patterns";
	static std::string _response; // minimal response (fast).
     };
    
} /* end of namespace. */

#endif
