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
 **/

#ifndef SE_PARSER_BING_H
#define SE_PARSER_BING_H

#include "se_parser.h"
#include <string>

namespace seeks_plugins
{
   class se_parser_bing : public se_parser
     {
      public:
	se_parser_bing();
	~se_parser_bing();
	
	// virtual.
	void start_element(parser_context *pc,
			   const xmlChar *name,
			   const xmlChar **attributes);
	
	void end_element(parser_context *pc,
			 const xmlChar *name);
	
	void characters(parser_context *pc,
			const xmlChar *chars,
			int length);
	
	void cdata(parser_context *pc,
		   const xmlChar *chars,
		   int length);
	
	// local.
	void handle_characters(parser_context *pc,
			       const xmlChar *chars,
			       int length);
     
      private:
	bool _h1_flag;
	bool _h1_sr_flag;
	bool _results_flag;
	bool _h3_flag;
	bool _link_flag;
	bool _p_flag;
	bool _cite_flag;
	bool _cached_flag;
	
	std::string _h3;
	std::string _link;
	std::string _summary;
	std::string _cite;
	std::string _cached;
	
	static std::string _sr_string_en;
	static std::string _bing_stupid[2];
     };
   
};

#endif
