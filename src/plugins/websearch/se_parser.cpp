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
 
#include "se_parser.h"
#include "errlog.h"

#include <string.h>
#include <iostream>

using sp::errlog;

void start_element_wrapper(void *context,
			   const xmlChar *name,
			   const xmlChar **attributes)
{
   using seeks_plugins::parser_context;
   parser_context *pc = static_cast<parser_context*>(context);
   pc->_parser->start_element(pc,name,attributes);
}

void end_element_wrapper(void *context,
			 const xmlChar *name)
{
   using seeks_plugins::parser_context;
   parser_context *pc = static_cast<parser_context*>(context);
   pc->_parser->end_element(pc,name);
}

void characters_wrapper(void *context,
			const xmlChar *chars,
			int length)
{
   using seeks_plugins::parser_context;
   parser_context *pc = static_cast<parser_context*>(context);
   pc->_parser->characters(pc,chars,length);
}

void cdata_wrapper(void *context,
		   const xmlChar *chars,
		   int length)
{
   using seeks_plugins::parser_context;
   parser_context *pc = static_cast<parser_context*>(context);
   pc->_parser->cdata(pc,chars,length);
}
  
namespace seeks_plugins
{
   se_parser::se_parser()
     :_count(0)
     {
     }

   se_parser::~se_parser()
     {
     }
      
   void se_parser::parse_output(char *output, 
				std::vector<search_snippet*> *snippets,
				const int &count_offset)
     {
	_count = count_offset;
	
	htmlParserCtxtPtr ctxt;
	parser_context pc;
	pc._parser = this;
	pc._snippets = snippets;
	pc._current_snippet = NULL;
	
	htmlSAXHandler saxHandler =
	  {
	     NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       start_element_wrapper,
	       end_element_wrapper,
	       NULL,
	       characters_wrapper,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       NULL,
	       cdata_wrapper,
	       NULL
	  };

	try 
	  {
	     ctxt = htmlCreatePushParserCtxt(&saxHandler, &pc, "", 0, "",
					     XML_CHAR_ENCODING_UTF8); // encoding here.
	  }
	catch (std::exception e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"Error %s in parsing html search results.",
			       e.what());
	  }
		
	//std::cout << "parsing chunk...\n";
	
	htmlParseChunk(ctxt,output,strlen(output),0);
	
	//std::cout << "parsing done...\n";
	
	htmlFreeParserCtxt(ctxt);
     }
   
   // static.
   const char* se_parser::get_attribute(const char **attributes,
					const char *name)
     {
	if (!attributes)
	  return NULL;
	
	int i=0;
	while(attributes[i] != NULL)
	  {
	     if (strcasecmp(attributes[i],name) == 0)
	       return attributes[i+1];
	     i++;
	  }
	return NULL;
     }

   void se_parser::libxml_init()
     {
	xmlInitParser();
     }
      
} /* end of namespace. */
