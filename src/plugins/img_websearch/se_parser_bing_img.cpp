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

#include "se_parser_bing_img.h"
#include "img_search_snippet.h"
#include "img_websearch_configuration.h"
#include "miscutil.h"

#include <iostream>

using sp::miscutil;

namespace seeks_plugins
{

  se_parser_bing_img::se_parser_bing_img(const std::string &url)
    :se_parser(url),_results_flag(false),_link_flag(false),_title_flag(false),_safesearch(true)
  {
  }

  se_parser_bing_img::~se_parser_bing_img()
  {
  }

  void se_parser_bing_img::start_element(parser_context *pc,
                                         const xmlChar *name,
                                         const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    //std::cout << "tag: " << tag << std::endl;

    if (strcasecmp(tag,"span") == 0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");

        if (a_class && strcasecmp(a_class,"ic") == 0)
          {
            if (pc->_snippets->empty())
              _results_flag = true;

            // assert previous snippet if any.
            if (pc->_current_snippet)
              {
                if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
                    || pc->_current_snippet->_url.empty()
                    || pc->_current_snippet->_cached.empty())
                  //|| pc->_current_snippet->_summary.empty()
                  //|| pc->_current_snippet->_cite.empty())
                  {
                    delete pc->_current_snippet;
                    pc->_current_snippet = NULL;
                    _count--;
                  }
                else pc->_snippets->push_back(pc->_current_snippet);
              }

            //std::cout << "snippets size: " << pc->_snippets->size() << std::endl;

            // create new snippet.
            //std::cout << "creating new snippet\n";
            img_search_snippet *sp = new img_search_snippet(_count+1);
            sp->_safe = _safesearch;
            _count++;
            sp->_img_engine = feeds("bing_img",_url);
            pc->_current_snippet = sp;
          }
        else if (_results_flag && a_class && strcasecmp(a_class,"md_mu") == 0)
          {
            _link_flag = true;
          }
        else if (_results_flag && a_class && strcasecmp(a_class,"md_de") == 0)
          {
            _title_flag = true;
          }
      }
    else if (_results_flag && strcasecmp(tag,"img")==0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        if (a_class && strcasecmp(a_class,"img_ls_u") == 0)
          {
            const char *a_src = se_parser::get_attribute((const char**)attributes,"src");
            if (a_src)
              {
                pc->_current_snippet->_cached = std::string(a_src);
              }
          }
      }
  }

  void se_parser_bing_img::characters(parser_context *pc,
                                      const xmlChar *chars,
                                      int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_bing_img::cdata(parser_context *pc,
                                 const xmlChar *chars,
                                 int length)
  {
    //handle_characters(pc, chars, length);
  }

  void se_parser_bing_img::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (_link_flag)
      {
        _link.append((char*)chars,length);
      }
    else if (_title_flag)
      {
        std::string a_chars = std::string((char*)chars,length);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        _title += a_chars;
      }
  }

  void se_parser_bing_img::end_element(parser_context *pc,
                                       const xmlChar *name)
  {
    const char *tag = (const char*)name;

    if (!_results_flag)
      return;

    if (_link_flag && strcasecmp(tag,"span") == 0)
      {
        _link_flag = false;
        pc->_current_snippet->set_url(_link);
        _link.clear();
      }
    else if (_title_flag && strcasecmp(tag,"span") == 0)
      {
        _title_flag = false;
        pc->_current_snippet->_title = _title;
        _title.clear();
      }
  }

} /* end of namespace. */
