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

#include "se_parser_delicious.h"

#include "encode.h"
#include "miscutil.h"
#include "mem_utils.h"

#include <strings.h>
#include <iostream>

using sp::encode;
using sp::miscutil;

namespace seeks_plugins
{

  se_parser_delicious::se_parser_delicious(const std::string &url)
    :se_parser(url),_title_flag(false)
  {
  }

  se_parser_delicious::~se_parser_delicious()
  {
  }

  void se_parser_delicious::start_element(parser_context *pc,
                                          const xmlChar *name,
                                          const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    if (strcasecmp(tag,"li") == 0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        if (a_class && strncasecmp(a_class,"post",4)==0)
          {
            // check on previous snippet, if any.
            if (pc->_current_snippet)
              {
                //post_process_snippet(pc->_current_snippet);
                if (pc->_current_snippet)
                  {
                    pc->_current_snippet = NULL;
                  }
                else pc->_snippets->pop_back();
              }

            // create new snippet.
            search_snippet *sp = new search_snippet(_count++);
            sp->_engine = feeds("delicious",_url);
            pc->_current_snippet = sp;
            pc->_snippets->push_back(pc->_current_snippet);
          }
      }
    else if (strcasecmp(tag,"a")==0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        const char *a_rel = se_parser::get_attribute((const char**)attributes,"rel");
        if (a_class && strcasecmp(a_class,"taggedlink ")==0)
          {
            const char *a_href = se_parser::get_attribute((const char**)attributes,"href");
            if (a_href)
              {
                std::string url = a_href;
                pc->_current_snippet->set_url(url);
                _title_flag = true;
              }
          }
      }
  }

  void se_parser_delicious::characters(parser_context *pc,
                                       const xmlChar *chars,
                                       int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_delicious::cdata(parser_context *pc,
                                  const xmlChar *chars,
                                  int length)
  {
    //handle_characters(pc, chars, length);
  }

  void se_parser_delicious::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (_title_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"<b>"," ");
        miscutil::replace_in_string(a_chars,"</b>"," ");
        _title += a_chars;
      }
  }

  void se_parser_delicious::end_element(parser_context *pc,
                                        const xmlChar *name)
  {
    const char *tag = (const char*)name;
    if (_title_flag && strcasecmp(tag,"a") == 0)
      {
        _title_flag = false;
        pc->_current_snippet->set_title(_title);
        _title = "";
      }
  }

} /* end of namespace. */
