/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
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

#ifndef SE_PARSER_CUIL_H
#define SE_PARSER_CUIL_H

#include "se_parser.h"

namespace seeks_plugins
{
   class se_parser_cuil : public se_parser
     {
      public:
	se_parser_cuil();
	~se_parser_cuil();
	
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
	bool _start_results;
	bool _new_link;
	bool _sum_flag;
	bool _cite_flag;
	bool _screening;
	bool _pages;
	
	std::string _summary;
	std::string _cite;
	
      public:
	hash_map<int,std::string> _links_to_pages; // links to other result pages are embedded and
	                                           // generated automatically. Until it is reversed
						   // engineered, we grab them from the html page. 
     };
   
} /* end of namespace. */

#endif
