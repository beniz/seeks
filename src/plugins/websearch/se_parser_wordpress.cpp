/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#include "se_parser_wordpress.h"
#include "miscutil.h"
#include "urlmatch.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;
using sp::urlmatch;

namespace seeks_plugins
{
  se_parser_wordpress::se_parser_wordpress(const std::string &url)
    :se_parser(url),_link_flag(false),_summary_flag(false),_date_flag(false),_title_flag(false)
  {
  }

  se_parser_wordpress::~se_parser_wordpress()
  {
  }

  void se_parser_wordpress::start_element(parser_context *pc,
                                          const xmlChar *name,
                                          const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    if (strcasecmp(tag,"div") == 0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        std::string cl_str;
        if (a_class)
          cl_str = a_class;
        if (cl_str.find("type-post")!=std::string::npos)
          {
            // create new snippet.
            search_snippet *sp = new search_snippet(_count + 1);
            _count++;
            sp->_engine = feeds("wordpress",_url);
            sp->_doc_type = POST;
            pc->_current_snippet = sp;
            pc->_snippets->push_back(pc->_current_snippet);
          }
        else if (cl_str == "post-content")
          {
            _summary_flag = true;
          }
      }
    else if (strcasecmp(tag,"h2")==0)
      {
        _title_flag = true;
      }
    else if (_title_flag && strcasecmp(tag,"a")==0)
      {
        const char *a_href = se_parser::get_attribute((const char**)attributes,"href");
        pc->_current_snippet->set_url(a_href);
      }
    else if (strcasecmp(tag,"span")==0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        if (a_class && strcasecmp(a_class,"post-date")==0)
          _date_flag = true;
      }
  }

  void se_parser_wordpress::characters(parser_context *pc,
                                       const xmlChar *chars,
                                       int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_wordpress::cdata(parser_context *pc,
                                  const xmlChar *chars,
                                  int length)
  {
    //handle_characters(pc, chars, length);
  }

  void se_parser_wordpress::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (_title_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        _title += a_chars;
      }
    else if (_summary_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        _summary += a_chars;
      }
    else if (_date_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        _date += a_chars;
      }
  }

  void se_parser_wordpress::end_element(parser_context *pc,
                                        const xmlChar *name)
  {
    const char *tag = (const char*) name;

    if (_title_flag && strcasecmp(tag,"h2")==0)
      {
        _title_flag = false;
        pc->_current_snippet->set_title(_title);
        _title = "";
      }
    else if (_summary_flag && strcasecmp(tag,"div")==0)
      {
        _summary_flag = false;
        pc->_current_snippet->set_summary(_summary);
        _summary = "";
      }
    else if (_date_flag && strcasecmp(tag,"span")==0)
      {
        _date_flag = false;
        pc->_current_snippet->set_date(_date);
        _date = "";
      }
  }


} /* end of namespace. */
