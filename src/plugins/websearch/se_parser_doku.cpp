/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
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

#include "se_parser_doku.h"
#include "miscutil.h"
#include "urlmatch.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;
using sp::urlmatch;

namespace seeks_plugins
{
  std::string se_parser_doku::_doku_stupid[2] =
  { "Document title", "Titre du document / Document title" };

  se_parser_doku::se_parser_doku(const std::string &url)
    :se_parser(url),_results_flag(false),_link_flag(false),_search_div(false),_search_snippet(false),_sn(NULL)
  {
    urlmatch::parse_url_host_and_path(url,_host,_path);
    if (miscutil::strncmpic(url.c_str(), "http://",7) == 0)
      _host = "http://" + _host;
    else if (miscutil::strncmpic(url.c_str(), "https://",8) == 0)
      _host = "https://" + _host;
  }

  se_parser_doku::~se_parser_doku()
  {
  }

  void se_parser_doku::start_element(parser_context *pc,
                                     const xmlChar *name,
                                     const xmlChar **attributes)
  {
    const char *tag = (const char*)name;
    if (strcasecmp(tag,"div") == 0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes,"class");
        if (_results_flag && a_class && strcasecmp(a_class, "search_snippet") == 0)
          {
            _search_snippet = true;
          }
        else if (a_class && strcasecmp(a_class,"search_result") == 0)
          {
            // assert previous snippet if any.
            if (pc->_current_snippet)
              {
                if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
                    || pc->_current_snippet->_url.empty()
                    || pc->_current_snippet->_summary.empty()
                    || _sn->_cite.empty())
                  {
                    delete pc->_current_snippet;
                    pc->_current_snippet = NULL;
                    _count--;
                  }
                else pc->_snippets->push_back(pc->_current_snippet);
              }

            // create new snippet.
            _sn = new seeks_snippet(_count+1);
            _count++;
            _sn->_engine = feeds("dokuwiki",_url);
            pc->_current_snippet = _sn;
            _results_flag = true;

            //_cached_flag = false; // in case previous snippet did not close the cached flag.
          }
      }

    if (_results_flag && strcasecmp(tag,"a") == 0)
      {
        _link_flag = true;
        const char *a_link = se_parser::get_attribute((const char**)attributes,"href");

        if (a_link)
          {
            _link = std::string(a_link);
            _cite = std::string(a_link);
            //std::cout << _cite << std::endl;
          }
        const char *snip_title = se_parser::get_attribute((const char**)attributes,"title");

        if (snip_title)
          {
            _title = std::string(snip_title);
          }
      }
  }

  void se_parser_doku::characters(parser_context *pc,
                                  const xmlChar *chars,
                                  int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_doku::cdata(parser_context *pc,
                             const xmlChar *chars,
                             int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_doku::handle_characters(parser_context *pc,
                                         const xmlChar *chars,
                                         int length)
  {
    if (_search_snippet)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        miscutil::replace_in_string(a_chars,se_parser_doku::_doku_stupid[1],"");
        miscutil::replace_in_string(a_chars,se_parser_doku::_doku_stupid[0],"");
        _summary += a_chars;

      }
    /*else if (_cite_flag)
      {
         std::string a_chars = std::string((char*)chars);
         miscutil::replace_in_string(a_chars,"\n"," ");
         miscutil::replace_in_string(a_chars,"\r"," ");
         //miscutil::replace_in_string(a_chars,"-"," ");
         _cite += a_chars;
      }
    else if (_title_flag)
      {
         std::string a_chars = std::string((char*)chars);
         miscutil::replace_in_string(a_chars,"\n"," ");
         miscutil::replace_in_string(a_chars,"\r"," ");
         _h3 += a_chars;
      }*/


  }

  void se_parser_doku::end_element(parser_context *pc,
                                   const xmlChar *name)
  {
    const char *tag = (const char*) name;

    if (!_results_flag)
      return;

    if (strcasecmp(tag,"a") == 0)
      {
        _link_flag = false;
        _sn->_cite = _host + _cite;
        pc->_current_snippet->set_url_no_decode(_host + _link);
        _link = "";
        _cite = "";
        pc->_current_snippet->set_title(_title);
        _title = "";
      }
    else if (_search_snippet && strcasecmp(tag,"div") == 0)
      {
        _search_snippet = false;
        //std::cout << "/_search_snippet" << std::endl;
        pc->_current_snippet->set_summary(_summary);
        _summary = "";
      }
    else if (_results_flag && strcasecmp(tag,"div") == 0)
      {
        //std::cout << "/_results_flag" << std::endl;
        _results_flag = false; // getting ready for the Cached link, if any.
      }
  }


} /* end of namespace. */
