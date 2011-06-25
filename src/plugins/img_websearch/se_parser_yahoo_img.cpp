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

#include "se_parser_yahoo_img.h"
#include "img_search_snippet.h"
#include "img_websearch_configuration.h"
#include "miscutil.h"
#include "encode.h"

#include <iostream>

using sp::miscutil;
using sp::encode;

namespace seeks_plugins
{

  se_parser_yahoo_img::se_parser_yahoo_img(const std::string &url)
    :se_parser(url),_results_flag(false),_cite_flag(false),_safesearch(true)
  {
  }

  se_parser_yahoo_img::~se_parser_yahoo_img()
  {
  }

  void se_parser_yahoo_img::start_element(parser_context *pc,
                                          const xmlChar *name,
                                          const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    if (!_results_flag && strcasecmp(tag,"ul") == 0)
      {
        const char *a_id = se_parser::get_attribute((const char**)attributes,"id");
        if (a_id && strcasecmp(a_id,"yschimg") == 0)
          {
            _results_flag = true;
          }
      }
    else if (_results_flag && strcasecmp(tag,"li") == 0)
      {
        // assert previous snippet if any.
        if (pc->_current_snippet)
          {
            if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
                ||  pc->_current_snippet->_url.empty()
                || pc->_current_snippet->_cached.empty())
              {
                delete pc->_current_snippet;
                pc->_current_snippet = NULL;
                _count--;
                pc->_snippets->pop_back();
              }
          }
        img_search_snippet *sp = new img_search_snippet(_count+1);
        sp->_safe = _safesearch;
        _count++;
        sp->_img_engine = feeds("yahoo_img",_url);
        pc->_current_snippet = sp;
        pc->_snippets->push_back(sp);
      }
    else if (_results_flag && strcasecmp(tag,"a") == 0)
      {
        const char *a_href = se_parser::get_attribute((const char**)attributes,"href");
        if (a_href)
          {
            std::string furl = a_href;
            size_t pos = furl.find("imgurl=");
            if (pos != std::string::npos && pos+7 < furl.size())
              {
                std::string imgurl = furl.substr(pos+7);
                char *imgurl_str = encode::url_decode(imgurl.c_str());
                imgurl = imgurl_str;
                free(imgurl_str);
                pos = imgurl.find("&");
                if (pos != std::string::npos)
                  {
                    imgurl = imgurl.substr(0,pos);
                    imgurl = "http://" + imgurl;
                    pc->_current_snippet->set_url(imgurl);
                  }
              }
          }
      }
    else if (_results_flag && strcasecmp(tag,"img") == 0)
      {
        const char *a_src = se_parser::get_attribute((const char**)attributes,"src");
        if (a_src)
          {
            pc->_current_snippet->_cached = a_src;
          }
      }
    else if (_results_flag && strcasecmp(tag,"cite") == 0)
      {
        _cite_flag = true;
      }
  }

  void se_parser_yahoo_img::characters(parser_context *pc,
                                       const xmlChar *chars,
                                       int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_yahoo_img::cdata(parser_context *pc,
                                  const xmlChar *chars,
                                  int length)
  {
    //handle_characters(pc, chars, length);
  }


  void se_parser_yahoo_img::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (_cite_flag)
      {
        std::string cite = std::string((char*)chars,length);
        _title += cite;
      }
  }

  void se_parser_yahoo_img::end_element(parser_context *pc,
                                        const xmlChar *name)
  {
    const char *tag = (const char*)name;

    if (!_results_flag)
      return;

    if (_results_flag && strcasecmp(tag,"ul") == 0)
      _results_flag = false;

    if (_results_flag && strcasecmp(tag,"cite") == 0)
      {
        _cite_flag = false;
        pc->_current_snippet->_title = _title;
        _title.clear();
      }
  }

} /* end of namespace. */
