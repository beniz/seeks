/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "img_websearch_configuration.h"

#include <iostream>

namespace seeks_plugins
{
   
#define hash_img_se     3083524283ul /* "img-search-engine" */
#define hash_img_ca     3745160171ul /* "img-content-analysis" */
#define hash_img_n      3311685141ul /* "img-per-page" */
   
   img_websearch_configuration *img_websearch_configuration::_img_wconfig = NULL;
   
   img_websearch_configuration::img_websearch_configuration(const std::string &filename)
     :configuration_spec(filename)
     {
	if (img_websearch_configuration::_img_wconfig == NULL)
	  img_websearch_configuration::_img_wconfig = this;
	
	_img_se_enabled = std::bitset<IMG_NSEs>(0);
	load_config();
     }
   
   img_websearch_configuration::~img_websearch_configuration()
     {
     }
   
   void img_websearch_configuration::set_default_config()
     {
	_img_se_enabled.set(); // all engines is default.
	_img_content_analysis = false; // no download of image thumbnails is default.
	_N = 30; // default number of images per page.
     }
   
   void img_websearch_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
						       char *buf, const unsigned long &linenum)
     {
	switch(cmd_hash)
	  {
	   case hash_img_se:
	     if (_img_se_enabled.count() == IMG_NSEs) // all bits set is default, so now reset to 0.
	       _img_se_enabled.reset();
	     if (strcasecmp(arg,"google") == 0)
	       _img_se_enabled |= std::bitset<IMG_NSEs>(SE_GOOGLE_IMG);
	     else if (strcasecmp(arg,"bing") == 0)
	       _img_se_enabled |= std::bitset<IMG_NSEs>(SE_BING_IMG);
	     else if (strcasecmp(arg,"flickr") == 0)
	       _img_se_enabled |= std::bitset<IMG_NSEs>(SE_FLICKR);
	     break;
	     
	   case hash_img_ca:
	     _img_content_analysis = static_cast<bool>(atoi(arg));
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Enable the downloading of image snippets and comparison operations to detect identical images");
	     break;
	     
	   case hash_img_n:
	     _N = atoi(arg);
	     configuration_spec::html_table_row(_config_args,cmd,arg,
						"Number of images per page");
	     break;
	     
	   default:
	     break;
	  }
     }
   
   void img_websearch_configuration::finalize_configuration()
     {
     }
      
} /* end of namespace. */
