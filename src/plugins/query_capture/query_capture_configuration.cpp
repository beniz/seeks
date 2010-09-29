/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "query_capture_configuration.h"
#include "errlog.h"

using sp::errlog;

namespace seeks_plugins
{
   
#define hash_max_radius                    1988906041ul  /* "query-max-radius" */
#define hash_mode_intercept                1971845008ul  /* "mode-intercept" */
   
   query_capture_configuration* query_capture_configuration::_config = NULL;
   
   query_capture_configuration::query_capture_configuration(const std::string &filename)
     :configuration_spec(filename)
       {
	  if (query_capture_configuration::_config)
	    delete query_capture_configuration::_config;
	  query_capture_configuration::_config = this;
	  load_config();
       }
   
   query_capture_configuration::~query_capture_configuration()
     {
     }

   void query_capture_configuration::set_default_config()
     {
	_max_radius = 5;
	_mode_intercept = "redirect";
     }
      
   void query_capture_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
						       char *buf, const unsigned long &linenum)
     {
	switch(cmd_hash)
	  {
	   case hash_max_radius :
	     _max_radius = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Maximum radius of the query generation halo");
	     break;
	     
	   case hash_mode_intercept :
	     if (strcasecmp(arg,"capture")==0 || strcasecmp(arg,"redirect")==0)
	       _mode_intercept = std::string(arg);
	     else errlog::log_error(LOG_LEVEL_ERROR,"bad value to query_capture plugin option mode-intercept: %",
				    arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Whether to silently capture queries and clicks or to use a redirection from the result page to the proxy instead");
	     break;
	     
	   default:
	     break;
	  }
     }
      
   void query_capture_configuration::finalize_configuration()
     {
     }
      
} /* end of namespace. */
