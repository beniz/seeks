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

#include "se_parser_redmine.h"
#include "miscutil.h"
#include "urlmatch.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;
using sp::urlmatch;

namespace seeks_plugins
{
  se_parser_redmine::se_parser_redmine(const std::string &url)
    :se_parser(url),_results_flag(false),_date_flag(false),_title_flag(false),_summary_flag(false)
  {
    urlmatch::parse_url_host_and_path(url,_host,_path);
  }

  se_parser_redmine::~se_parser_redmine()
  {
  }

  void se_parser_redmine::start_element(parser_context *pc,
                                        const xmlChar *name,
                                        const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    if (strcasecmp(tag,"dl")==0)
      {
        const char *a_id = se_parser::get_attribute((const char**)attributes,"id");
        if (a_id && strcasecmp(a_id,"search-results")==0)
          _results_flag = true;
      }
    else if (_results_flag && strcasecmp(tag,"dt") == 0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");

        // create new snippet.
        search_snippet *sp = new search_snippet(_count + 1);
        _count++;
        sp->_engine = feeds("redmine",_url);
        if (a_class)
          {
            if (strcasecmp(a_class,"changeset")==0)
              sp->_doc_type = REVISION;
            else if (strcasecmp(a_class,"issue")==0)
              sp->_doc_type = ISSUE;
          }
        pc->_current_snippet = sp;
        pc->_snippets->push_back(pc->_current_snippet);
      }
    else if (_results_flag && strcasecmp(tag,"a")==0)
      {
        const char *a_href = se_parser::get_attribute((const char**)attributes,"href");
        if (a_href)
          {
            pc->_current_snippet->set_url(_host + std::string(a_href));
            _title_flag = true;
          }
      }
    else if (_results_flag && strcasecmp(tag,"span")==0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        if (a_class)
          {
            if (strcasecmp(a_class,"description")==0)
              _summary_flag = true;
            else if (strcasecmp(a_class,"author")==0)
              _date_flag = true;
          }
      }
  }

  void se_parser_redmine::characters(parser_context *pc,
                                     const xmlChar *chars,
                                     int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_redmine::cdata(parser_context *pc,
                                const xmlChar *chars,
                                int length)
  {
    //handle_characters(pc, chars, length);
  }

  void se_parser_redmine::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (_title_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        //miscutil::replace_in_string(a_chars,"<span class=\"highlight token-0\">","");
        //miscutil::replace_in_string(a_chars,"</span>","");
        _title += a_chars;
      }
    else if (!_date_flag && _summary_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        //miscutil::replace_in_string(a_chars,"<span class=\"highlight token-0\">","");
        //miscutil::replace_in_string(a_chars,"</span>","");
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

  void se_parser_redmine::end_element(parser_context *pc,
                                      const xmlChar *name)
  {
    const char *tag = (const char*) name;

    if (_results_flag && strcasecmp(tag,"dl")==0)
      _results_flag = false;
    else if (_title_flag && strcasecmp(tag,"a")==0)
      {
        _title_flag = false;
        pc->_current_snippet->set_title(_title);
        _title = "";
      }
    else if (_summary_flag && strcasecmp(tag,"dd")==0)
      {
        _summary_flag = false;
        _date_flag = false;
        pc->_current_snippet->set_summary(miscutil::chomp_cpp(_summary));
        pc->_current_snippet->set_date(miscutil::chomp_cpp(_date));
        _summary = "";
        _date = "";
      }
  }


} /* end of namespace. */
