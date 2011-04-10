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

#include "se_parser_twitter.h"
#include "miscutil.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;

namespace seeks_plugins
{
  se_parser_twitter::se_parser_twitter(const std::string &service, const std::string &url)
    :se_parser(url),_in_entry(false),_in_title(false),_in_published(false),_in_uri(false),
     _service(service)
  {
  }

  se_parser_twitter::~se_parser_twitter()
  {
  }

  void se_parser_twitter::start_element(parser_context *pc,
                                        const xmlChar *name,
                                        const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    if (strcasecmp(tag, "entry") == 0)
      {
        //std::cout << "<entry>" << std::endl;
        _in_entry = true;
        // create new snippet.
        search_snippet *sp = new search_snippet(_count + 1);
        _count++;
        if (_service.empty() || _service == "twitter")
          sp->_engine = feeds("twitter",_url);
        //sp->_engine |= std::bitset<NSEs>(SE_TWITTER);
        else if (_service == "identica")
          sp->_engine = feeds("identica",_url);
        //sp->_engine |= std::bitset<NSEs>(SE_IDENTICA);
        sp->_doc_type = TWEET;
        pc->_current_snippet = sp;
      }
    else if (_in_entry && strcasecmp(tag, "title") == 0)
      {
        _in_title = true;
      }
    else if (_in_entry && strcasecmp(tag, "link") == 0)
      {
        const char *a_link = se_parser::get_attribute((const char**)attributes, "href");
        if (pc->_current_snippet->_url.empty())
          pc->_current_snippet->set_url(a_link);
        else pc->_current_snippet->_cached = a_link;
      }
    else if (_in_entry && strcasecmp(tag, "published") == 0)
      {
        _in_published = true;
      }
    else if (_in_entry && strcasecmp(tag, "uri") == 0)
      {
        _in_uri = true;
      }
  }

  void se_parser_twitter::characters(parser_context *pc,
                                     const xmlChar *chars,
                                     int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_twitter::cdata(parser_context *pc,
                                const xmlChar *chars,
                                int length)
  {
    //handle_characters(pc, chars, length);
  }

  void se_parser_twitter::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (_in_title)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        miscutil::replace_in_string(a_chars,"-"," ");
        _title += a_chars;
        //std::cout << "    " << _title << std::endl;
      }
    else if (_in_published)
      {
        _published = std::string((char*)chars);
        size_t p = _published.find("T");
        if (p!=std::string::npos)
          _published = _published.substr(0,p);
      }
    else if (_in_uri)
      {
        _uri = std::string((char*)chars);
      }
  }

  void se_parser_twitter::end_element(parser_context *pc,
                                      const xmlChar *name)
  {
    const char *tag = (const char*) name;

    if (_in_entry && strcasecmp(tag, "entry") == 0)
      {
        //std::cout << "</entry>" << std::endl;
        _in_entry = false;

        // assert previous snippet if any.
        if (pc->_current_snippet)
          {
            if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
                || pc->_current_snippet->_url.empty())
              {
                //std::cout << "[snippet fail]" << " title: " << pc->_current_snippet->_title.empty() << " url: " << pc->_current_snippet->_url.empty() << std::endl;
                delete pc->_current_snippet;
                pc->_current_snippet = NULL;
                _count--;
              }
            else pc->_snippets->push_back(pc->_current_snippet);
          }
      }
    else if (_in_entry && _in_title && strcasecmp(tag, "title") == 0)
      {
        //std::cout << "  </title>" << std::endl;
        _in_title = false;
        pc->_current_snippet->_title = _title;
        _title = "";
      }
    else if (_in_entry && _in_published && strcasecmp(tag, "published") == 0)
      {
        _in_published = false;
        pc->_current_snippet->set_date(_published);
        _published = "";
      }
    else if (_in_entry && _in_uri && strcasecmp(tag, "uri") ==0)
      {
        _in_uri = false;
        pc->_current_snippet->_cite = _uri;
        _uri = "";
      }
  }

} /* end of namespace. */
