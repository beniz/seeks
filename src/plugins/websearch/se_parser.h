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
 
#ifndef SE_PARSER_H
#define SE_PARSER_H

#include "search_snippet.h"
#include <libxml/HTMLparser.h>

namespace seeks_plugins
{
   class se_parser;
   
   struct parser_context
     {
	se_parser *_parser;
	search_snippet *_current_snippet;
	std::vector<search_snippet*> *_snippets;
     };
   
   class se_parser
     {
      public:
	se_parser();
	
	virtual ~se_parser();
	
	void parse_output(char *output, std::vector<search_snippet*> *snippets,
			  const int &count_offset);
     
	// handlers.
	virtual void start_element(parser_context *pc,
				   const xmlChar *name,
				   const xmlChar **attributes) {};
	
	virtual void end_element(parser_context *pc,
				 const xmlChar *name) {};
	
	virtual void characters(parser_context *pc,
				const xmlChar *chars,
				int length) {};
	
	virtual void cdata(parser_context *pc,
			   const xmlChar *chars,
			   int length) {};
	
	// libxml related tools.
	static const char* get_attribute(const char **attributes,
					 const char *name);
	

	// multithreaded use requires initialization.
	static void libxml_init();
	
      protected:
	int _count; // number of snippets.
     };

} /* end of namespace. */
 
#endif
