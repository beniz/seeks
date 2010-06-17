/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009, 2010 Emmanuel Benazera, juban@free.fr
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "websearch_configuration.h"

#include <iostream>

namespace seeks_plugins
{

#define hash_lang                   3579134231ul /* "search-langage" */
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
#define hash_clustering             2382120344ul /* "enable-clustering" */
#define hash_max_expansions         3838821776ul /* "max-expansions" */
#define hash_extended_highlight     2722091897ul /* "extended-highlight" */   

#ifdef WITH_DHT
#define hash_sg_subscription        3242804198ul /* "sg-subscription" */
#define hash_sg_update_delay        1703788085ul /* "sg-update-delay" */
#define hash_sg_retry_delay         1941414272ul /* "sg-retry-delay" */
#endif
   
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
	_lang = "auto";
	_N = 10;
	_thumbs = false;
	_se_enabled.set(); // all engines is default.
	_query_context_delay = 300; // in seconds, 5 minutes.
	_js = false; // default is no javascript, this may change later on.
	_content_analysis = false;
	_clustering = false;
	_se_connect_timeout = 3;  // in seconds.
	_se_transfer_timeout = 5; // in seconds.
	_ct_connect_timeout = 1; // in seconds.
	_ct_transfer_timeout = 3; // in seconds.
	_max_expansions = 100;
	_extended_highlight = false;
	
#ifdef WITH_DHT
	_sg_subscription = false;
	_sg_update_delay = 60; // in seconds.
	_sg_retry_delay = 5; // in seconds;
#endif
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
	     if (_se_enabled.count() == NSEs) // all bits set is default, so now reset to 0.
	       _se_enabled.reset();
	     
	     if (strcasecmp(arg,"google") == 0)
	       _se_enabled |= std::bitset<NSEs>(SE_GOOGLE);
	     else if (strcasecmp(arg,"cuil") == 0)
	       _se_enabled |= std::bitset<NSEs>(SE_CUIL);
	     else if (strcasecmp(arg,"bing") == 0)
	       _se_enabled |= std::bitset<NSEs>(SE_BING);
	     else if (strcasecmp(arg,"yahoo") == 0)
	       _se_enabled |= std::bitset<NSEs>(SE_YAHOO);
	     else if (strcasecmp(arg,"exalead") == 0)
	       _se_enabled |= std::bitset<NSEs>(SE_EXALEAD);
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
						"Sets the connection timeout in seconds for connections to a search engine");
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
	     
	   case hash_clustering:
	     _clustering = static_cast<bool>(atoi(arg));
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Enables the clustering from the UI");
	     break;
	     
	   case hash_max_expansions:
	     _max_expansions = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets the maximum number of query expansions");
	     break;

	   case hash_extended_highlight:
	     _extended_highlight = static_cast<bool>(atoi(arg));
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Enables a more discriminative word highlight scheme");
	     break;
	   
#ifdef WITH_DHT
	   case hash_sg_subscription:
	     _sg_subscription = static_cast<bool>(atoi(arg));
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Enables automatic subscription to search groups");
	     break;

	   case hash_sg_update_delay:
	     _sg_update_delay = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets delay between two calls for update to the same search group");
	     break;

	   case hash_sg_retry_delay:
	     _sg_retry_delay = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Sets delay before retrying failed or timed out calls to search groups");
	     break;
#endif
	     
	   default:
	     break;
	     
	  } // end of switch.
     }
   
   void websearch_configuration::finalize_configuration()
     {
     }
   
} /* end of namespace. */
