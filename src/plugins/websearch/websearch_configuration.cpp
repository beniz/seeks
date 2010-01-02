/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include "websearch_configuration.h"

#include <iostream>

namespace seeks_plugins
{

#define hash_lang                   1526909359ul /* "search-langage" */
#define hash_n                       578814699ul /* "search-results-page" */  
#define hash_se                     1635576913ul /* "search-engine" */
#define hash_qcd                    4118649627ul /* "query-context-delay" */
#define hash_thumbs                  793242781ul /* "enable-thumbs" */
#define hash_js                     3171705030ul /* "enable-js" */
#define hash_content_analysis       1483831511ul /* "enable-content-analysis" */
#define hash_se_transfer_timeout    2056038060ul /* "se-transfer-timeout" */
#define hash_se_connect_timeout     1026838950ul /* "se-connect-timeout" */
#define hash_ct_transfer_timeout    3371661146ul /* "ct-transfer-timeout" */
#define hash_ct_connect_timeout     3817701526ul /* "ct-connect-timeout" */
   
   websearch_configuration::websearch_configuration(const std::string &filename)
     :configuration_spec(filename)
       {
	  _se_enabled = std::bitset<NSEs>(0);
	  load_config();
       }
   
   websearch_configuration::~websearch_configuration()
     {
     }
   
   void websearch_configuration::set_default_config()
     {
	_lang = "en";
	_N = 10;
	_thumbs = 0;
	_se_enabled = std::bitset<NSEs>(000); // ggle only, TODO: change to all...
	_query_context_delay = 300; // in seconds, 5 minutes.
	_js = 0; // default is no javascript, this may change later on.
	_content_analysis = 0;
	_se_connect_timeout = 3;  // in seconds.
	_se_transfer_timeout = 5; // in seconds.
	_ct_connect_timeout = 1; // in seconds.
	_ct_connect_timeout = 3; // in seconds.
     }
   
   void websearch_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
						   char *buf, const unsigned long &linenum)
     {
	switch(cmd_hash)
	  {
	   case hash_lang :
	     _lang = std::string(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Websearch language");
	     break;
	     
	   case hash_n :
	       _N = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Number of websearch results per page");
	     break;
	     
	   case hash_se :
	     if (strcasecmp(arg,"google") == 0)
	       _se_enabled |= std::bitset<NSEs>(SE_GOOGLE);
	     else if (strcasecmp(arg,"cuil") == 0)
	       _se_enabled |= std::bitset<NSEs>(SE_CUIL);
	     else if (strcasecmp(arg,"bing") == 0)
	       _se_enabled |= std::bitset<NSEs>(SE_BING);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Enabled search engine");
	     break;
	     
	   case hash_thumbs:
	      _thumbs = static_cast<bool>(atoi(arg));
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Enable search results webpage thumbnails");
	     break;
	   
	   case hash_qcd :
	     _query_context_delay = strtod(arg,NULL);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Delay in seconds before deletion of cached websearches and results");
	     break;
	   
	   case hash_js:
	     _js = static_cast<bool>(atoi(arg));
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Enable javascript use on the websearch result page");
	     break;
	     
	   case hash_content_analysis:
	     _content_analysis = static_cast<bool>(atoi(arg));
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Enable the background download of webpages pointed to by websearch results and content analysis");
	     break;
	     
	   case hash_se_transfer_timeout:
	     _se_transfer_timeout = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the transfer timeout in seconds for connections to a search engine");
	     break;
	     
	   case hash_se_connect_timeout:
	     _se_connect_timeout = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the connection timeout in seconds for connectinos to a search engine");
	     break;
	     
	   case hash_ct_transfer_timeout:
	     _ct_transfer_timeout = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the transfer timeout in seconds when fetching content for analysis and caching");
	     break;
	     
	   case hash_ct_connect_timeout:
	     _ct_connect_timeout = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the connection timeout in seconds when fetching content for analysis and caching");
	     break;
	     
	   default :
	     break;
	     
	  } // end of switch.
     }
   
   void websearch_configuration::finalize_configuration()
     {
	// TODO.
     }
   
} /* end of namespace. */
