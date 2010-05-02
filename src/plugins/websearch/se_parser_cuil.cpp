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

#include "se_parser_cuil.h"
#include "miscutil.h"
                   
#include <strings.h>
#include <iostream>
        
using sp::miscutil;
                               
namespace seeks_plugins
{
   se_parser_cuil::se_parser_cuil()
     :se_parser(),_start_results(false),_new_link(false),_sum_flag(false),
      _cite_flag(false),_screening(false),_pages(false)
       {
       }
   
   se_parser_cuil::~se_parser_cuil()
     {
     }
   
   void se_parser_cuil::start_element(parser_context *pc,
				      const xmlChar *name,
				      const xmlChar **attributes)
     {
	const char *tag = (const char*)name;
	
	if (strcasecmp(tag,"div") == 0)
	  {
	     const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
	     const char *a_id = se_parser::get_attribute((const char**)attributes,"id");
	     
	     if (!_start_results && a_class && strcasecmp(a_class,"result web_result") == 0)
	       {
		  _start_results = true;
	       }
	     
	     if (a_id && _start_results && strncasecmp(a_id,"result_",7) == 0)
	       {
		  // check on previous snippet.
		  if (pc->_current_snippet)
		    {
		       if (pc->_current_snippet->_title != "")
			 {
			    // no cache on cuil, so we give a link to an archived page.
			    pc->_current_snippet->set_archive_link();
			    pc->_snippets->push_back(pc->_current_snippet);
			 }
		       else
			 {
			    delete pc->_current_snippet;
			    pc->_current_snippet = NULL;
			 }
		    }
		  
		  // create new snippet.
		  std::string count_str = std::string(a_id);
		  try
		    {
		       count_str = count_str.substr(7);
		    }
		  catch(std::exception &e)
		    {
		       return; // stop here, do not create the snippet.
		    }
		  int count = atoi(count_str.c_str());
		  search_snippet *sp = new search_snippet(count-1);
		  sp->_engine |= std::bitset<NSEs>(SE_CUIL);
		  pc->_current_snippet = sp;
		  _screening = true;
	       }
	     else if (a_class && _start_results && _screening && strcasecmp(a_class,"clear") == 0)
	       _screening = false; // specify to stop parsing snippets until told otherwise (aka 'cut the crap').
	     else if (a_class && _start_results && _new_link && strcasecmp(a_class,"url-wrap") == 0)
	       {
		  // sets summary.
		  _sum_flag = false;
		  pc->_current_snippet->set_summary(_summary);
		  _summary = "";
		  
		  _new_link = false;
		  _cite_flag = true;
	       }
	     else if (a_id && _start_results && !_screening && strcasecmp(a_id,"pages") == 0)
	       {
		  _pages = true;
	       }
	  }
	else if (_start_results && strcasecmp(tag,"a") == 0)
	  {
	     const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
	     
	     if (a_class && _screening && strcasecmp(a_class,"title_link") == 0)
	       {
		  // sets the link.
		  const char *a_link = se_parser::get_attribute((const char**)attributes,"href");
		  const char *a_title = se_parser::get_attribute((const char**)attributes,"title");
		  
		  if (a_link)
		    {
		       pc->_current_snippet->set_url(std::string(a_link));
		       _new_link = true;
		    }
		  if (a_title)
		    {
		       pc->_current_snippet->_title = std::string(a_title);
		    }
	       }
	     else if (a_class && _pages)
	       {
		  if (a_class && strcasecmp(a_class,"pagelink ") == 0)
		    {
		       const char *a_link = se_parser::get_attribute((const char**)attributes,"href");
		       if (a_link)
			 {
			    std::string a_link_str = std::string(a_link);
			    size_t ppos = a_link_str.find("&p=");
			    size_t epos = a_link_str.find("_");
			    std::string pnum_str;
			    try
			      {
				 pnum_str = a_link_str.substr(ppos+3,epos-ppos-3);
			      }
			    catch(std::exception &e)
			      {
				 return; // do not save this link.
			      }
			    int pnum = atoi(pnum_str.c_str());
			    try
			      {
				 _links_to_pages.insert(std::pair<int,std::string>(pnum,a_link_str.substr(ppos)));
			      }
			    catch(std::exception &e)
			      {
				 return; // do not save this link.
			      }
			    /* std::cout << "added page link: " << a_link_str.substr(ppos) << std::endl;
			     std::cout << "pnum_str: " << pnum_str << std::endl; */
			 }
		    }
	       }
	  }
	else if (_start_results && _screening && _new_link && strcasecmp(tag,"p") == 0)
	  {
	     _sum_flag = true;
	  }
     }
   
   void se_parser_cuil::characters(parser_context *pc,
				   const xmlChar *chars,
				   int length)
     {
	handle_characters(pc, chars, length);
     }
   
   void se_parser_cuil::cdata(parser_context *pc,
			      const xmlChar *chars,
			      int length)
     {
	handle_characters(pc, chars, length);
     }
   
   void se_parser_cuil::handle_characters(parser_context *pc,
					  const xmlChar *chars,
					  int length)
     {
	if (_start_results && !_screening)
	  return;
	else if (_sum_flag)
	  {
	     std::string a_chars = std::string((char*)chars);
	     miscutil::replace_in_string(a_chars,"\n"," ");
	     miscutil::replace_in_string(a_chars,"\r"," ");
	     //miscutil::replace_in_string(a_chars,"-"," ");
	     _summary += a_chars;
	  }
	else if (_cite_flag)
	  {
	     std::string a_chars = std::string((char*)chars);
	     miscutil::replace_in_string(a_chars,"\n"," ");
	     miscutil::replace_in_string(a_chars,"\r"," ");
	     _cite += a_chars;
	  }
     }
   
   void se_parser_cuil::end_element(parser_context *pc,
				    const xmlChar *name)
     {
	if (_start_results && !_screening)
	  return;
	
	const char *tag = (const char*)name;
	
	if (_cite_flag && strcasecmp(tag,"a") == 0)
	  {
	     _cite_flag = false;
	     pc->_current_snippet->_cite = _cite;
	     _cite = "";
	  }
	else if (_pages && strcasecmp(tag,"div") == 0)
	  _pages = false;
     }
   
} /* end of namespace. */
