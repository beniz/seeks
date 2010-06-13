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

#ifndef SE_PARSER_H
#define SE_PARSER_H

#include "search_snippet.h"
#include "mutexes.h"
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

	// parser mutex.
	static sp_mutex_t _se_parser_mutex;
	
      protected:
	int _count; // number of snippets.
     };

} /* end of namespace. */
 
#endif
