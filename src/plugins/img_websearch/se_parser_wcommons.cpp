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

#include "se_parser_wcommons.h"
#include "img_search_snippet.h"
#include "img_websearch_configuration.h"
#include "miscutil.h"

#include <iostream>

using sp::miscutil;

namespace seeks_plugins
{

  se_parser_wcommons::se_parser_wcommons(const std::string &url)
    :se_parser(url),_results_flag(false),_sr_flag(false)
  {
  }

  se_parser_wcommons::~se_parser_wcommons()
  {
  }

  void se_parser_wcommons::start_element(parser_context *pc,
                                         const xmlChar *name,
                                         const xmlChar **attributes)
  {
    const char *tag = (const char*)name;
    if (strcasecmp(tag,"ul") == 0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        if (a_class && strcasecmp(a_class,"mw-search-results") == 0)
          {
            _sr_flag = true;
          }
      }
    else if (_sr_flag && strcasecmp(tag,"table") == 0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        if (a_class && strcasecmp(a_class,"searchResultImage") == 0)
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
                    pc->_snippets->pop_back();
                  }
              }

            // create new snippet.
            img_search_snippet *sp = new img_search_snippet(_count+1);
            _count++;
            sp->_img_engine = feeds("wcommons",_url);
            pc->_current_snippet = sp;
            pc->_snippets->push_back(sp);

            if (!_results_flag)
              _results_flag = true;
          }
      }
    else if (_results_flag && strcasecmp(tag,"img") == 0)
      {
        const char *a_src = se_parser::get_attribute((const char**)attributes,"src");
        if (a_src)
          {
            pc->_current_snippet->_cached = std::string(a_src);
          }
      }
    else if (_results_flag && strcasecmp(tag,"a") == 0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        if (a_class)
          {
            const char *a_href = se_parser::get_attribute((const char**)attributes,"href");
            if (a_href)
              {
                pc->_current_snippet->set_url("http://commons.wikipedia.org" + std::string(a_href));
              }
          }
        else
          {
            const char *a_title = se_parser::get_attribute((const char**)attributes,"title");
            if (a_title)
              {
                std::string title = a_title;
                miscutil::replace_in_string(title,"File:","");
                pc->_current_snippet->_title = title;
              }
          }
      }
  }

  void se_parser_wcommons::characters(parser_context *pc,
                                      const xmlChar *chars,
                                      int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_wcommons::cdata(parser_context *pc,
                                 const xmlChar *chars,
                                 int length)
  {
    //handle_characters(pc, chars, length);
  }

  void se_parser_wcommons::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {

  }

  void se_parser_wcommons::end_element(parser_context *pc,
                                       const xmlChar *name)
  {
    const char *tag = (const char*)name;
    if (!_results_flag)
      return;

    if (_results_flag && strcasecmp(tag,"ul") == 0)
      _results_flag = false; // end of image list.
  }


} /* end of namespace. */
