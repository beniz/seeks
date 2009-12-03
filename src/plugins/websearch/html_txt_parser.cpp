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
 
#include "html_txt_parser.h"

namespace seeks_plugins
{
   html_txt_parser::html_txt_parser()
     : se_parser()
       {
       }
   
   html_txt_parser::~html_txt_parser()
     {
     }
   
   void html_txt_parser::characters(parser_context *pc,
				    const xmlChar *chars,
				    int length)
     {
	handle_characters(pc, chars, length);
     }
   
   void html_txt_parser::cdata(parser_context *pc,
			       const xmlChar *chars,
			       int length)
     {
     }
   
   void html_txt_parser::handle_characters(parser_context *pc,
					   const xmlChar *chars,
					   int length)
     {
	_txt += std::string((char*)chars);
     }
   
   
} /* end of namespace. */
