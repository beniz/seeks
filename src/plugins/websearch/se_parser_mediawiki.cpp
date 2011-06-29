/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Laurent Peuch <cortex@worlddomination.be>
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

#include "se_parser_mediawiki.h"
#include "miscutil.h"
#include "urlmatch.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;
using sp::urlmatch;

namespace seeks_plugins
{
  se_parser_mediawiki::se_parser_mediawiki(const std::string &url,
      const std::string &lang)
    :se_parser(url),_li_sr_flag(false),_a_sr_flag(false),_search_result(false),_end_search(true)
  {
    urlmatch::parse_url_host_and_path(url,_host,_path);
    if (miscutil::strncmpic(url.c_str(), "http://",7) == 0)
      _host = "http://" + _host;
    else if (miscutil::strncmpic(url.c_str(), "https://",8) == 0)
      _host = "https://" + _host;
    miscutil::replace_in_string(_host,"%lang",lang);
  }

  se_parser_mediawiki::~se_parser_mediawiki()
  {
  }

  void se_parser_mediawiki::start_element(parser_context *pc,
                                          const xmlChar *name,
                                          const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    if (strcasecmp(tag, "ul") == 0)
      {
        const char *a_class = se_parser::get_attribute((const char**)attributes, "class");
        if (a_class && strcasecmp(a_class, "mw-search-results") == 0)
          {
            _end_search = false;
          }
      }
    if (!_end_search && strcasecmp(tag, "li") == 0)
      {
        _li_sr_flag = true;

        // create new snippet.
        search_snippet *sp = new search_snippet(_count + 1);
        _count++;
        sp->_engine = feeds("mediawiki",_url);
        pc->_current_snippet = sp;
      }
    if (!_end_search && strcasecmp(tag, "a") == 0 && _li_sr_flag)
      {
        const char *a_link = se_parser::get_attribute((const char**)attributes,"href");
        _link = std::string(a_link);
        _a_sr_flag = true;

        if (pc->_snippets->empty())
          _results_flag = true;
      }
    if (!_end_search && _li_sr_flag && strcasecmp(tag, "div") == 0)
      {
        const char *a_link = se_parser::get_attribute((const char**)attributes,"class");
        if (strcasecmp(a_link, "searchresult") == 0)
          {
            _search_result = true;
          }
      }
  }

  void se_parser_mediawiki::characters(parser_context *pc,
                                       const xmlChar *chars,
                                       int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_mediawiki::cdata(parser_context *pc,
                                  const xmlChar *chars,
                                  int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_mediawiki::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (_a_sr_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        _title += a_chars;
      }
    if (_search_result)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        _summary += a_chars;
      }
  }

  void se_parser_mediawiki::end_element(parser_context *pc,
                                        const xmlChar *name)
  {
    const char *tag = (const char*) name;

    if (!_end_search && strcasecmp(tag, "ul") == 0)
      _end_search = true;

    if (!_results_flag)
      return;

    if (_li_sr_flag && strcasecmp(tag,"li") == 0)
      {
        _li_sr_flag = false;

        // assert previous snippet if any.
        if (pc->_current_snippet)
          {
            if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
                || pc->_current_snippet->_url.empty()
                || pc->_current_snippet->_summary.empty()
                || pc->_current_snippet->_cite.empty())
              {
                //std::cout << "[snippet fail]" << " title: " << pc->_current_snippet->_title.empty() << " url: " << pc->_current_snippet->_url.empty() << " summary: " << pc->_current_snippet->_summary.empty() << " cite de mes deux: " << pc->_current_snippet->_cite.empty() << std::endl;
                delete pc->_current_snippet;
                pc->_current_snippet = NULL;
                _count--;
              }
            else pc->_snippets->push_back(pc->_current_snippet);
          }

      }

    if (_a_sr_flag && strcasecmp(tag,"a") == 0)
      {
        pc->_current_snippet->_title = _title;
        _title = "";
        pc->_current_snippet->set_url(_host + _link);
        pc->_current_snippet->_cite = _host + _link;
        _link = "";
        _a_sr_flag = false;
      }

    if (_search_result && strcasecmp(tag, "div") == 0)
      {
        // some time, mediawiki don't give a body to it's result
        if (_summary == "")
          _summary = " ";

        pc->_current_snippet->set_summary(_summary);
        _summary = "";
        _cite = "";
        _search_result = false;
      }
  }


} /* end of namespace. */
