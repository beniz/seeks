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

#include "se_parser_ggle.h"

#include "miscutil.h"
#include "urlmatch.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;
using sp::urlmatch;

namespace seeks_plugins
{
   se_parser_ggle::se_parser_ggle()
     :se_parser(),_body_flag(false),_h2_flag(false),_h2_sr_flag(false),
      _ol_flag(false),_li_flag(false),_h3_flag(false),
      _div_flag_summary(false),_div_flag_forum(false),_sum_flag(false),
      _link_flag(false),_cite_flag(false),
      _cached_flag(false),_span_cached_flag(false),
      _ff_flag(false),_new_link_flag(false),_spell_flag(false),_sgg_spell_flag(false),
      _end_sgg_spell_flag(false),_rt_flag(false)
       {
       }
   
   se_parser_ggle::~se_parser_ggle()
     {
     }
   
   void se_parser_ggle::start_element(parser_context *pc,
				      const xmlChar *name,
				      const xmlChar **attributes)
     {
	const char *tag = (const char*)name;
	
	if (strcasecmp(tag, "body") == 0)
	  {
	     _body_flag = true;
	  }
	// check for h3 flag -> snippet's title.
	else if (_h2_sr_flag && _li_flag && strcasecmp(tag,"h3") == 0)
	  {
	     const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
	     if (a_class && strcasecmp(a_class,"r") == 0)
	       {
		  _h3_flag = true;
		  _new_link_flag = true;
	       }
	  }
	// check for h2 flag -> search results 'title'.
	else if (_body_flag && !_h2_sr_flag && strcasecmp(tag,"h2") == 0)
	  {
	     _h2_flag = true;
	     const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
	     if (a_class && strcasecmp(a_class,"hd") == 0)
	       _h2_sr_flag = true;
	  }
	// real time results avoidance.
	else if (pc->_current_snippet && _h2_sr_flag && strcasecmp(tag,"span") == 0)
	  {
	     const char *a_id = se_parser::get_attribute((const char**)attributes,"id");
	     if (a_id && strcasecmp(a_id,"rth") == 0)
	       {
		  _rt_flag = true;
		  pc->_current_snippet->_title = "";  // scheduled for removal.
		  _li_flag = false;
		  _h3_flag = false;
	       }
	  }
	else if (pc->_current_snippet && _h3_flag && strcasecmp(tag,"a") == 0)
	  {
	     const char *a_link = se_parser::get_attribute((const char**)attributes,"href");
	     
	     if (a_link)
	       {
		  std::string a_link_str = std::string(a_link);
		  size_t p = miscutil::replace_in_string(a_link_str,"/url?q=",""); // remove query form
		  if (p != 0)
		    {
		       size_t pos = a_link_str.find("&");
		       a_link_str = a_link_str.substr(0,pos);
		    }
		  pc->_current_snippet->set_url(a_link_str);
		  
		  /* std::cerr << "[Debug]:ggle_parser: url id: " << pc->_current_snippet->_id 
		    << " -- url: " << pc->_current_snippet->_url << std::endl; */
	       }
	  }
	else if (_h2_sr_flag && strcasecmp(tag,"ol") == 0)
	  {
	     _ol_flag = true;
	  }
	else if (_h2_sr_flag && strcasecmp(tag,"li") == 0)
	  {
	     const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
	     
	     if (!a_class || strcasecmp(a_class,"g") != 0)
	       return;
	     
	     // assert previous snippet, if any.
	     if (pc->_current_snippet)
	       {
		  if (!pc->_current_snippet->_title.empty() && !pc->_current_snippet->_url.empty())
		    {
		       se_parser_ggle::post_process_snippet(pc->_current_snippet);
		       if (pc->_current_snippet)
			 {
			    pc->_snippets->push_back(pc->_current_snippet);
			    pc->_current_snippet = NULL;
			 }
		    }
		  else // no title, throw the snippet away.
		    {
		       delete pc->_current_snippet;
		       pc->_current_snippet = NULL;
		       _count--;
		    }
	       }
	     	     
	     // create new snippet.
	     search_snippet *sp = new search_snippet(_count+1);
	     _count++;
	     sp->_engine |= std::bitset<NSEs>(SE_GOOGLE);
	     pc->_current_snippet = sp;
	  
	     // reset some variables.
	     _summary = "";
	     
	     _li_flag = true;
	  }
	else if (_h2_sr_flag && _li_flag && _new_link_flag && strcasecmp(tag,"div") == 0)
	  {
	     const char *d_class = se_parser::get_attribute((const char**)attributes,"class");
	     
	     if (d_class && strcasecmp(d_class,"f") == 0)
	       _div_flag_forum = true;
	     else if (d_class && d_class[0] == 's')
	       _div_flag_summary = true;
	  }
	else if (pc->_current_snippet && _li_flag && strcasecmp(tag,"cite") == 0)
	  {
	     _cite_flag = true;
	     
	     // summary, if any, ends here.
	     pc->_current_snippet->set_summary(_summary);
	     _summary = "";
	  }
	else if (_ol_flag && _span_cached_flag && strcasecmp(tag,"a") == 0)
	  {
	     const char *a_cached = se_parser::get_attribute((const char**)attributes,"href");
	     
	     if (a_cached)
	       {
		  _cached_flag = true;
		  _cached = std::string(a_cached);
	       }
	  }
	else if (_h2_sr_flag && strcasecmp(tag,"span") == 0)
	  {
	     const char *span_class = se_parser::get_attribute((const char**)attributes,"class");
	     	     
	     if (span_class)
	       {
		  if (_div_flag_summary)
		    {
		       if (span_class[0] == 'f')
			 _ff_flag = true;
		       else if (strcasecmp(span_class,"gl") == 0)
			 _span_cached_flag = true;
		    }
		  else
		    {
		       if (strcasecmp(span_class,"spell") == 0)
			 _spell_flag = true;
		       else if (strcasecmp(span_class,"med") == 0 && _spell_flag)
			 _spell_flag = false;
		    }
	       }
	  }
	// probably not robust: wait for the next opening tag,
	// before setting the file format.
     	else if (_ff_flag)
	  {
	     _ff_flag = false;
	     _ff = ""; // TODO: file format.
	     // TODO: store in search snippet and reset.
	  }	
	else if (!_end_sgg_spell_flag && _count <= 1 && strcasecmp(tag,"a") == 0)
	  {
	     
	     const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
	     if (a_class && strcasecmp(a_class,"spell") == 0)
	       {
		  _sgg_spell_flag = true;
	       }
	  }
     }
   
   void se_parser_ggle::characters(parser_context *pc,
				   const xmlChar *chars,
				   int length)
     {
	handle_characters(pc, chars, length);
     }
   
   void se_parser_ggle::cdata(parser_context *pc,
			      const xmlChar *chars,
			      int length)
     {
	//handle_characters(pc, chars, length);
     }
   
   void se_parser_ggle::handle_characters(parser_context *pc,
					  const xmlChar *chars,
					  int length)
     {
	if (_h3_flag)
	  {
	     std::string a_chars = std::string((char*)chars);
	     miscutil::replace_in_string(a_chars,"\n"," ");
	     miscutil::replace_in_string(a_chars,"\r"," ");
	     _h3 += a_chars;	     
	  }
	else if (_cite_flag)
	  {
	     std::string a_chars = std::string((char*)chars);
	     miscutil::replace_in_string(a_chars,"\n"," ");
	     miscutil::replace_in_string(a_chars,"\r"," ");
	     //miscutil::replace_in_string(a_chars,"-"," ");
	     _cite += a_chars;
	  }
	else if (_ff_flag)
	  {
	     std::string a_chars = std::string((char*)chars);
	     miscutil::replace_in_string(a_chars,"\n"," ");
	     miscutil::replace_in_string(a_chars,"\r"," ");
	     _ff += a_chars;
	  }
	else if (_ol_flag && _div_flag_forum)
	  {
	     std::string a_chars = std::string((char*)chars);
	     miscutil::replace_in_string(a_chars,"\n"," ");
	     miscutil::replace_in_string(a_chars,"\r"," ");
	     miscutil::replace_in_string(a_chars,"-"," ");
	     _forum += a_chars;
	  }
	else if (_ol_flag && _div_flag_summary)
	  {
	     std::string a_chars = std::string((char*)chars);
	     miscutil::replace_in_string(a_chars,"\n"," ");
	     miscutil::replace_in_string(a_chars,"\r"," ");
	     //miscutil::replace_in_string(a_chars,"-"," ");
	     _summary += a_chars;
	  }
	else if (_sgg_spell_flag)
	  {
	     std::string a_chars = std::string((char*)chars);
	     miscutil::replace_in_string(a_chars,"\n"," ");
	     miscutil::replace_in_string(a_chars,"\r"," ");
	     miscutil::replace_in_string(a_chars,"-"," ");
	     _suggestion += a_chars;
	  }
     }
   
   void se_parser_ggle::end_element(parser_context *pc,
				    const xmlChar *name)
     {
	const char *tag = (const char*) name;
	
	if (pc->_current_snippet && _li_flag && _h3_flag && strcasecmp(tag,"h3")==0)
	  {
	     _h3_flag = false;
	     pc->_current_snippet->_title = _h3;
	     _h3 = "";
	  }
	else if (strcasecmp(tag,"h2") == 0)
	  {
	     _h2_flag = false;
	  }
	else if ((_div_flag_summary || _div_flag_forum) && strcasecmp(tag,"div") == 0)
	  {
	     // beware: order matters.
	     if (pc->_current_snippet && _div_flag_forum)
	       {
		  _div_flag_forum = false;
		  pc->_current_snippet->_forum_thread_info = _forum;
		  pc->_current_snippet->_doc_type = FORUM;
		  _forum = "";
	       }
	     else if (_div_flag_summary)
	       {
		  // summary was already added, turn the flag off.
		  _div_flag_summary = false;
	       }
	  }
	else if (pc->_current_snippet && _cite_flag && strcasecmp(tag,"cite") == 0)
	  {
	     _cite_flag = false;
	     pc->_current_snippet->_cite = _cite;
	     _cite = "";
	     _new_link_flag = false;
	  }
	else if (pc->_current_snippet && _cached_flag && strcasecmp(tag,"a") == 0)
	  {
	     _span_cached_flag = false; // no need to catch the /span tag.
	     _cached_flag = false;
	     if (!_cached.empty())
	       pc->_current_snippet->_cached = _cached;
	     _cached = "";
	  }
	else if (_sgg_spell_flag && strcasecmp(tag,"a") == 0)
	  {
	     _sgg_spell_flag = false;
	     _end_sgg_spell_flag = true;
	  }
	else if (_h2_sr_flag && _li_flag && strcasecmp(tag,"ol") == 0)
	  {
	     if (_rt_flag)
	       {
		  _rt_flag =false;
	       }
	     else if (pc->_current_snippet)
	       {
		  if (pc->_current_snippet->_title.empty())
		    {
		       delete pc->_current_snippet;
		       pc->_current_snippet = NULL;
		    }
		  else 
		    {
		       se_parser_ggle::post_process_snippet(pc->_current_snippet);
		       if (pc->_current_snippet)
			 {
			    pc->_snippets->push_back(pc->_current_snippet);
			    pc->_current_snippet = NULL;
			 }
		    }
	       }
	  }
     }
   
   void se_parser_ggle::post_process_snippet(search_snippet *&se)
     {
	// some buggy ggle output generates malformed urls and snippets.
	std::string surl = urlmatch::strip_url(se->_url);
	if (surl == se->_url) // missing http or https.
	  {
	     delete se;
	     se = NULL;
	     _count--;
	     return;
	  }
		
	// fix to summary (others are to be added here accordingly). XXX: language specific...
	size_t r = miscutil::replace_in_string(se->_summary,"Your browser may not have a PDF reader available. Google recommends visiting our text version of this document.",					       "");
	r = miscutil::replace_in_string(se->_summary,"Quick View","");
	r = miscutil::replace_in_string(se->_summary,"View as HTML","");
			
	// remove certain unwanted results (ggle image, video, news & shopping).
	if ((r = se->_url.find("/products?q="))!=std::string::npos
	    || (r = se->_url.find("/videosearch?q="))!=std::string::npos
	    || (r = se->_url.find("news.google"))!=std::string::npos
	    || (r = se->_url.find("maps.google"))!=std::string::npos
	    || (r = se->_url.find("books.google"))!=std::string::npos
	    || (r = se->_url.find("/images?q="))!=std::string::npos)
	  {
	     delete se;
	     se = NULL;
	     _count--;
	  }
     }
   
} /* end of namespace. */
