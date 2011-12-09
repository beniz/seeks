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

#include "se_parser_ggle_img.h"
#include "img_search_snippet.h"
#include "img_websearch_configuration.h"
#include "miscutil.h"
#include "encode.h"

#include <iostream>

using sp::miscutil;
using sp::encode;

namespace seeks_plugins
{

  se_parser_ggle_img::se_parser_ggle_img(const std::string &url)
    :se_parser(url),_results_flag(false)
  {
  }

  se_parser_ggle_img::~se_parser_ggle_img()
  {
  }

  void se_parser_ggle_img::start_element(parser_context *pc,
                                         const xmlChar *name,
                                         const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    if (!_results_flag && strcasecmp(tag,"div") == 0)
      {
        const char *id = se_parser::get_attribute((const char**)attributes,"id");
        if (id && strcasecmp(id,"res") == 0)
          {
            _results_flag = true;
          }
      }
    else if (_results_flag && strcasecmp(tag,"td") == 0)
      {
        // create new snippet.
        img_search_snippet *sp = new img_search_snippet(_count+1);
        _count++;
        sp->_img_engine = feeds("google_img",_url);
        pc->_current_snippet = sp;
      }
    else if (_results_flag && strcasecmp(tag,"img") == 0)
      {
        // set url and cached link.
        const char *a_src = se_parser::get_attribute((const char**)attributes,"src");
        if (a_src)
          {
            std::string cached_link = std::string(a_src);
            miscutil::replace_in_string(cached_link,"\n","");
            miscutil::replace_in_string(cached_link,"\r","");
            static_cast<img_search_snippet*>(pc->_current_snippet)->_cached = cached_link;
          }
      }
    else if (_results_flag && strcasecmp(tag,"a") == 0)
      {
        const char *a_href = se_parser::get_attribute((const char**)attributes,"href");
        if (a_href)
          {
            std::string furl = a_href;
            miscutil::replace_in_string(furl,"/imgres?imgurl=","");
            size_t pos = furl.find("imgrefurl");
            if (pos != std::string::npos)
              {
                std::string url = furl.substr(0,pos-1);
                pc->_current_snippet->set_url(url);
                pos = url.find_last_of("/");
                if (pos != std::string::npos && pos+1 != std::string::npos)
                  {
                    std::string imgfile = url.substr(pos+1);
                    char *dec_url = encode::url_decode(imgfile.c_str());
                    char *enc_url = encode::html_encode_and_free_original(dec_url);
                    pc->_current_snippet->_title = enc_url;
                    free(enc_url);
                  }
              }
          }
      }
  }

  void se_parser_ggle_img::characters(parser_context *pc,
                                      const xmlChar *chars,
                                      int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_ggle_img::cdata(parser_context *pc,
                                 const xmlChar *chars,
                                 int length)
  {
  }

  void se_parser_ggle_img::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    std::string a_chars = std::string((char*)chars);
  }

  void se_parser_ggle_img::end_element(parser_context *pc,
                                       const xmlChar *name)
  {
    const char *tag = (const char*)name;

    if (!_results_flag)
      return;

    if (strcasecmp(tag,"ol") == 0)
      {
        _results_flag = false;
      }
    if (_results_flag && strcasecmp(tag,"td") == 0)
      {
        if (pc->_current_snippet)
          {
            if (pc->_current_snippet->_url.empty())
              {
                delete pc->_current_snippet;
                pc->_current_snippet = NULL;
                _count--;
              }
            else pc->_snippets->push_back(pc->_current_snippet);
          }
      }
  }

} /* end of namespace. */
