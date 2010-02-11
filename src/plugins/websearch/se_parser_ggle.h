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

#ifndef SE_PARSER_GGLE_H
#define SE_PARSER_GGLE_H

#include "se_parser.h"
#include <string>

namespace seeks_plugins
{
   class se_parser_ggle : public se_parser
     {
      public:
	se_parser_ggle();
	
	~se_parser_ggle();
	
	// virtual fct.
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

	void post_process_snippet(search_snippet *&se);
	
      private:
	bool _body_flag;
	bool _h2_flag;
	bool _h2_sr_flag;
	bool _ol_flag;
	bool _li_flag;
	bool _h3_flag;
	bool _div_flag_summary;
	bool _div_flag_forum;
	bool _sum_flag;
	bool _link_flag;
	bool _cite_flag;
	bool _cached_flag;
	bool _span_cached_flag;
	bool _ff_flag;
	bool _new_link_flag;
	bool _spell_flag;
	bool _sgg_spell_flag;
	bool _end_sgg_spell_flag;
	bool _rt_flag;
	
	std::string _link;
	std::string _cached;
	std::string _ff;
	std::string _h3;
	std::string _cite;
	std::string _summary;
	std::string _forum;
	
      public:
	std::string _suggestion;
     };
   
} /* end of namespace. */

#endif
