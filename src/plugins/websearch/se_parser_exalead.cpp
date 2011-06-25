/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009, 2010
 *  sileht, theli48@gmail.com
 *  Emmanuel Benazera, juban@free.fr
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

#include "errlog.h"
#include "se_parser_exalead.h"
#include "miscutil.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;
using sp::errlog;

namespace seeks_plugins
{
  se_parser_exalead::se_parser_exalead(const std::string &url)
    :se_parser(url),_result_flag(false),_title_flag(false),_p_flag(false),
     _summary_flag(false),_cite_flag(false),_cached_flag(false),_b_title_flag(false),
     _b_summary_flag(false),_ignore_flag(false)
  {
  }

  se_parser_exalead::~se_parser_exalead()
  {
  }

  void se_parser_exalead::start_element(parser_context *pc,
                                        const xmlChar *name,
                                        const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    if (strcasecmp(tag,"div") == 0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        if (a_class && strcasecmp(a_class,"resultContent") == 0)
          {
            // assert previous snippet, if any.
            if (pc->_current_snippet)
              {
                if (bad_snippet(pc->_current_snippet))
                  {
                    delete pc->_current_snippet;
                    pc->_current_snippet = NULL;
                    _count--;
                    pc->_snippets->pop_back();
                  }
              }

            _result_flag = true;
            search_snippet *sp = new search_snippet(_count+1);
            _count++;
            sp->_engine = feeds("exalead",_url);
            pc->_current_snippet = sp;
            pc->_snippets->push_back(pc->_current_snippet);
          }
      }
    else if (_result_flag)
      {
        if (strcasecmp(tag,"p") == 0)
          {
            _p_flag = true;
          }
        else if (_p_flag && strcasecmp(tag,"span") == 0)
          {
            const char *a_class = se_parser::get_attribute((const char**)attributes,"class");

            if (!_summary_flag && !a_class)
              _summary_flag = true;
            if (_summary_flag && a_class && strcmp(a_class,"bookmarkLinks") == 0)
              {
                _ignore_flag = true;
              }
          }
        else if (strcasecmp(tag,"a") == 0)
          {
            const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
            if (a_class && strcasecmp(a_class,"url") == 0)
              {
                _cite_flag = true;
              }
            else if (a_class && strcasecmp(a_class,"title") == 0)
              {
                //_title_flag = true;
                const char *a_link = se_parser::get_attribute((const char**)attributes,"href");
                if (a_link)
                  pc->_current_snippet->set_url(std::string(a_link));
                const char *tt_link = se_parser::get_attribute((const char**)attributes,"title");
                if (tt_link)
                  {
                    std::string a_chars = std::string(tt_link);
                    size_t i=0;
                    while (i<a_chars.length() && isspace(a_chars[i]))
                      {
                        i++;
                      }
                    try
                      {
                        a_chars = a_chars.substr(i);
                      }
                    catch (std::exception &e)
                      {
                        return;
                      }
                    miscutil::replace_in_string(a_chars,"\n"," ");
                    miscutil::replace_in_string(a_chars,"\r"," ");
                    _title = a_chars;
                    pc->_current_snippet->set_title(_title);
                    _title = "";
                  }
              }
            else if (a_class && strcasecmp(a_class,"cache") == 0)
              {
                _cached_flag = true;
                const char *a_cached = se_parser::get_attribute((const char**)attributes,"href");
                if (a_cached)
                  {
                    _cached = std::string(a_cached);
                    pc->_current_snippet->_cached = "http://www.exalead.com" + _cached; // beware: check on the host.
                    _cached = "";
                  }
              }
          }
        /* else if (_title_flag && strcasecmp(tag,"b") == 0)
          {
        _b_title_flag = true;
          } */
        else if (_summary_flag && strcasecmp(tag,"b") == 0)
          {
            _b_summary_flag = true;
          }
      }
  }

  void se_parser_exalead::characters(parser_context *pc,
                                     const xmlChar *chars,
                                     int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_exalead::cdata(parser_context *pc,
                                const xmlChar *chars,
                                int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_exalead::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (!chars)
      return;

    if (!_ignore_flag && _summary_flag)
      {
        std::string a_chars = std::string((char*)chars);
        size_t i=0;
        while (i<a_chars.length() && isspace(a_chars[i]))
          {
            i++;
          }
        try
          {
            a_chars = a_chars.substr(i);
          }
        catch (std::exception &e)
          {
            return;
          }
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        if (_b_summary_flag)
          _summary += " ";
        _summary += a_chars;
        if (_b_summary_flag)
          _summary += " ";
      }
    else if (_cite_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        _cite += a_chars;
      }
    /* else if (_title_flag)
     {
        std::string a_chars = std::string((char*)chars);
        size_t i=0;
        while(i<a_chars.length() && isspace(a_chars[i]))
          {
    	 i++;
          }
        try
          {
    	 a_chars = a_chars.substr(i);
          }
        catch(std::exception &e)
          {
    	 return;
          }
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        if (_b_title_flag)
          _title += " ";
        _title += a_chars;
        if (_b_title_flag)
          _title += " ";
      } */
  }

  void se_parser_exalead::end_element(parser_context *pc,
                                      const xmlChar *name)
  {
    const char *tag = (const char*) name;
    if (_result_flag)
      {
        if (strcasecmp(tag,"div") == 0)
          {
            _result_flag = false;
            _title_flag = false;
            _p_flag = false;
            _summary_flag = false;
            _cite_flag = false;
            _cached_flag = false;
          }
        else if (strcasecmp(tag,"span") == 0)
          {
            if (!_ignore_flag && _summary_flag)
              {
                pc->_current_snippet->set_summary(_summary);
                _summary = "";
                _summary_flag = false;
              }
            else if (_ignore_flag)
              _ignore_flag = false;
          }
        else if ( _cite_flag && strcasecmp(tag,"a") == 0)
          {
            pc->_current_snippet->_cite = _cite;
            _cite = "";
            _cite_flag = false;
          }
        /* else if (_title_flag && strcasecmp(tag,"a") == 0)
          {
        pc->_current_snippet->_title = _title;
        _title = "";
        _title_flag = false;
          }
        else if (_title_flag && strcasecmp(tag,"b") == 0)
          _b_title_flag = false; */
        else if (_summary_flag && strcasecmp(tag,"b") == 0)
          _b_summary_flag = false;
      }
  }

  bool se_parser_exalead::bad_snippet(search_snippet *sp)
  {
    if (sp->_url.empty()
        || sp->_title.empty()
        || sp->_title.find("%visible_url%")!=std::string::npos)
      return true;
    else return false;
  }

} /* end of namespace. */
