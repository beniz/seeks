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

#ifndef IMG_WEBSEARCH_CONFIGURATION_H
#define IMG_WEBSEARCH_CONFIGURATION_H

#include "configuration_spec.h"

#include <bitset>

using sp::configuration_spec;

namespace seeks_plugins
{

/* engines in alphabetical order. */
#define IMG_NSEs 5 // number of image engines.
   
#define SE_BING_IMG            1U
#define SE_FLICKR              2U
#define SE_GOOGLE_IMG          4U
#define SE_WCOMMONS            8U
#define SE_YAHOO_IMG          16U
   
   class img_websearch_configuration : public configuration_spec
     {
      public:
	img_websearch_configuration(const std::string &filename);
	
	~img_websearch_configuration();
	
	// virtual.
	virtual void set_default_config();
	
	virtual void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
				       char *buf, const unsigned long &linenum);
	
	virtual void finalize_configuration();
     
	// main options.
	std::bitset<IMG_NSEs> _img_se_enabled; /**< enabled image search engines. */
	bool _img_content_analysis; /**< whether to download image thumbnails to detect identical images, or not. */
	int _Nr; /**< number of images per page. */
	bool _safe_search; /**< whether 'safe' image search is activated, or not. */
	
	// configuration object.
	static img_websearch_configuration *_img_wconfig;
     };

} /* end of namespace. */

#endif
