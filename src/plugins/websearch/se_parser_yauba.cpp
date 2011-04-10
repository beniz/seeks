/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Laurent Peuch  <cortex@worlddomination.be>
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

#include "se_parser_yauba.h"
#include "miscutil.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;

namespace seeks_plugins
{
  se_parser_yauba::se_parser_yauba(const std::string &url)
    :se_parser(url),_in_item(false),_in_title(false),_in_result(false),_in_summary(false),_in_cite(false)
  {
  }

  se_parser_yauba::~se_parser_yauba()
  {
  }

  void se_parser_yauba::start_element(parser_context *pc,
                                      const xmlChar *name,
                                      const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    if (strcasecmp(tag, "div") == 0)
      {
        const char *div_attr = se_parser::get_attribute((const char**)attributes, "class");
        if (div_attr && strcasecmp(div_attr, "imageblock") == 0)
          {
            _in_result = true;
            // create new snippet.
            search_snippet *sp = new search_snippet(_count + 1);
            _count++;
            //sp->_engine |= std::bitset<NSEs>(SE_YAUBA);
            sp->_engine = feeds("yauba",_url);
            pc->_current_snippet = sp;
          }
      }
    if (_in_result && strcasecmp(tag, "h1") == 0)
      {
        _in_title = true;
      }
    if (_in_result && strcasecmp(tag, "a") == 0 && pc->_current_snippet->_url.empty())
      {
        const char *url = se_parser::get_attribute((const char**)attributes, "href");
        if (url)
          pc->_current_snippet->set_url(std::string(url));
      }
    if (_in_result && strcasecmp(tag, "p") == 0)
      {
        _in_summary = true;
      }
    if (_in_result && strcasecmp(tag, "li") == 0)
      {
        const char *li_class = se_parser::get_attribute((const char**)attributes, "class");
        if (li_class && strcasecmp(li_class, "bluecolor") == 0)
          {
            _in_cite = true;
          }
      }
  }

  void se_parser_yauba::characters(parser_context *pc,
                                   const xmlChar *chars,
                                   int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_yauba::cdata(parser_context *pc,
                              const xmlChar *chars,
                              int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_yauba::handle_characters(parser_context *pc,
                                          const xmlChar *chars,
                                          int length)
  {
    if (_in_cite)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        _cite += a_chars;
      }
    if (_in_summary)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        _summary += a_chars;
      }
    if (_in_title)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        _title += a_chars;
      }
  }

  void se_parser_yauba::end_element(parser_context *pc,
                                    const xmlChar *name)
  {
    const char *tag = (const char*) name;

    if (strcasecmp(tag, "ul") == 0)
      {
        _in_result = false;

        // assert previous snippet if any.
        if (pc->_current_snippet)
          {
            if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
                || pc->_current_snippet->_cite.empty()
                || pc->_current_snippet->_url.empty())
              {
                delete pc->_current_snippet;
                pc->_current_snippet = NULL;
                _count--;
              }
            else pc->_snippets->push_back(pc->_current_snippet);
            pc->_current_snippet = NULL;
          }
      }
    if (_in_result && _in_title && strcasecmp(tag, "h1") == 0)
      {
        _in_title = false;
        pc->_current_snippet->_title = _title;
        _title = "";
      }
    if (_in_result && _in_summary && strcasecmp(tag, "p") == 0)
      {
        _in_summary = false;
        pc->_current_snippet->_summary = _summary;
        _summary = "";
      }
    if (_in_cite && strcasecmp(tag, "li") == 0)
      {
        _in_cite = false;
        pc->_current_snippet->_cite = _cite;
        _cite = "";
      }
  }


} /* end of namespace. */
