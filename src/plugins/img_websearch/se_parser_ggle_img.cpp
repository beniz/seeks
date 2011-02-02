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

  se_parser_ggle_img::se_parser_ggle_img()
    :se_parser(),_results_flag(false)
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

    if (strcasecmp(tag,"ol") == 0)
      {
        _results_flag = true;
      }
    else if (_results_flag && strcasecmp(tag,"td") == 0)
      {
        // create new snippet.
        img_search_snippet *sp = new img_search_snippet(_count+1);
        _count++;
        sp->_img_engine |= std::bitset<IMG_NSEs>(SE_GOOGLE_IMG);
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
            pc->_current_snippet->_cached = cached_link;
            int pos = cached_link.find(":http:");
            if (pos != std::string::npos)
              {
                std::string url = cached_link.substr(pos+1);
                pc->_current_snippet->set_url(url);

                pos = url.find_last_of("/");
                url = url.substr(pos+1);
                miscutil::replace_in_string(url,"_"," ");
                miscutil::replace_in_string(url,"-"," ");
                miscutil::replace_in_string(url,".jpg",""); // TODO: other extensions.
                char *dec_url = encode::url_decode(url.c_str());
                char *enc_url = encode::html_encode_and_free_original(dec_url);
                pc->_current_snippet->_title = enc_url;
                free(enc_url);
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
    else if (strcasecmp(tag,"td") == 0)
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
