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

#ifndef SE_PARSER_FLICKR_IMG_H
#define SE_PARSER_FLICKR_IMG_H

#include "se_parser.h"
#include <string>

namespace seeks_plugins
{
   
   class se_parser_flickr : public se_parser
     {
      public:
	se_parser_flickr();
	~se_parser_flickr();
	
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
	bool _results_flag;
	bool _span_flag;
		
	std::string _link;
	std::string _title;
     };
      
} /* end of namespace. */

#endif
