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

#ifndef WEBSEARCH_CONFIGURATION_H
#define WEBSEARCH_CONFIGURATION_H

#include "configuration_spec.h"
#include "se_handler.h" // NSEs.

#include <bitset>

using sp::configuration_spec;

namespace seeks_plugins
{

/* engines in alphabetical order. */
#define SE_BING               1U
#define SE_CUIL               2U
#define SE_EXALEAD            4U
#define SE_GOOGLE             8U
#define SE_YAHOO              16U
#define SE_MEDIAWIKI          32U
   
   class websearch_configuration : public configuration_spec
     {
      public:
	websearch_configuration(const std::string &filename);
	
	~websearch_configuration();
	
	// virtual
	virtual void set_default_config();
	
	virtual void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
				       char *buf, const unsigned long &linenum);
	
	virtual void finalize_configuration();
	
	// main options.
	std::string _lang; /**< langage of the search results. */	
	int _Nr; /**< max number of search results per page. */
	std::bitset<NSEs> _se_enabled; /**< enabled search engines. */
	bool _thumbs; /**< enabled thumbs */
	bool _js; /**< enabled js */
	bool _content_analysis; /**< enables advanced ranking with background fetch of webpage content. */
	bool _clustering; /**< whether to enable clustering in the UI. XXX: probably always on when dev is finished. */
	
	// others.
	double _query_context_delay; /**< delay for query context before deletion, in seconds. */

	long _se_transfer_timeout; /**< transfer timeout when connecting to a search engine. */
	long _se_connect_timeout; /**< connection timeout when connecting to a search engine. */

	long _ct_transfer_timeout; /**< transfer timeout when fetching content for analysis & caching. */
	long _ct_connect_timeout;  /**< connection timeout when fetching content for analysis & caching. */
	int _max_expansions; /**< max number of allowed expansions. Prevents attacks. */

	bool _extended_highlight;
	
	std::string _background_proxy_addr; /**< address of a proxy through which to fetch URLs. */
	int _background_proxy_port; /** < proxy port. */
	bool _show_node_ip; /**< whether to show the node IP address when rendering the info bar. */
     };
   
} /* end of namespace. */

#endif
