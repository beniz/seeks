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

#include "se_parser_flickr.h"
#include "img_search_snippet.h"
#include "img_websearch_configuration.h"
#include "miscutil.h"

#include <iostream>

using sp::miscutil;

namespace seeks_plugins
{
   
   se_parser_flickr::se_parser_flickr()
     :se_parser(),_results_flag(false),_span_flag(false)
       {
       }
   
   se_parser_flickr::~se_parser_flickr()
     {
     }
      
   void se_parser_flickr::start_element(parser_context *pc,
					const xmlChar *name,
					const xmlChar **attributes)
     {
	const char *tag = (const char*)name;
	
	if (strcasecmp(tag,"div") == 0)
	  {
	     const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
	     
	     if (!_results_flag && a_class && strcasecmp(a_class,"ResultsThumbs") == 0)
	       {
		  _results_flag = true;
	       }
	     else if (_results_flag && a_class && strcasecmp(a_class,"ResultsThumbsChild") == 0)
	       {
		  // assert previous snippet if any.
		  if (pc->_current_snippet)
		    {
		       if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
			   || pc->_current_snippet->_url.empty()
			   || pc->_current_snippet->_cached.empty())
			 {
			    delete pc->_current_snippet;
			    pc->_current_snippet = NULL;
			    _count--;
			 }
		       
		       else pc->_snippets->push_back(pc->_current_snippet);
		    }
		  
		  // create new snippet.
		  img_search_snippet *sp = new img_search_snippet(_count+1);
		  _count++;
		  sp->_img_engine |= std::bitset<IMG_NSEs>(SE_FLICKR);
		  pc->_current_snippet = sp;
	       }
	  }
	else if (_results_flag && !_span_flag && strcasecmp(tag,"span") == 0)
	  {
	     _span_flag = true;
	  }
	else if (_results_flag && _span_flag && strcasecmp(tag,"a") == 0)
	  {
	     const char *a_ref = se_parser::get_attribute((const char**)attributes,"href");
	     if (a_ref)
	       {
		  std::string url = "http://www.flickr.com" + std::string(a_ref);
		  pc->_current_snippet->set_url(url);
	       }
	     const char *a_title = se_parser::get_attribute((const char**)attributes,"title");
	     if (a_title)
	       {
		  pc->_current_snippet->_title = a_title;
	       }
	  }
	else if (_span_flag && strcasecmp(tag,"img") == 0)
	  {
	     const char *a_src = se_parser::get_attribute((const char**)attributes,"src");
	     if (a_src)
	       {
		  pc->_current_snippet->_cached = a_src;
	       }
	  }
     }
   
   void se_parser_flickr::characters(parser_context *pc,
				     const xmlChar *chars,
				     int length)
     {
	handle_characters(pc, chars, length);
     }
   
   void se_parser_flickr::cdata(parser_context *pc,
				const xmlChar *chars,
				int length)
     {
	//handle_characters(pc, chars, length);
     }
   
   void se_parser_flickr::handle_characters(parser_context *pc,
					    const xmlChar *chars,
					    int length)
     {
	/* if (_link_flag)
	  {
	     _link.append((char*)chars,length);
	  }
	else if (_title_flag)
	  {
	     std::string a_chars = std::string((char*)chars,length);
	     miscutil::replace_in_string(a_chars,"\n"," ");
	     miscutil::replace_in_string(a_chars,"\r"," ");
	     _title += a_chars;
	  } */
     }
   
   void se_parser_flickr::end_element(parser_context *pc,
				      const xmlChar *name)
     {
	const char *tag = (const char*)name;
	
	if (!_results_flag)
	  return;
	
	if (_span_flag && strcasecmp(tag,"span") == 0)
	  _span_flag = false;
     }
   
} /* end of namespace. */
  
