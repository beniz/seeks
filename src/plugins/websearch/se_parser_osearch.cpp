/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, <ebenazer@seeks-project.info>
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

#include "se_parser_osearch.h"

#include "encode.h"
#include "miscutil.h"
#include "mem_utils.h"

#include <strings.h>
#include <iostream>

using sp::encode;
using sp::miscutil;

namespace seeks_plugins
{
  /*- se_parser_osearch -*/
  se_parser_osearch::se_parser_osearch()
    :_feed_flag(false),_entry_flag(false),_entry_title_flag(false),
     _updated_flag(false),_content_flag(false),_gen_title_flag(false)
  {
  }

  se_parser_osearch::~se_parser_osearch()
  {
  }

  /*- se_parser_osearch_atom -*/
  se_parser_osearch_atom::se_parser_osearch_atom()
    :se_parser(),se_parser_osearch()
  {
  }

  se_parser_osearch_atom::~se_parser_osearch_atom()
  {
  }

  void se_parser_osearch_atom::start_element(parser_context *pc,
      const xmlChar *name,
      const xmlChar **attribute)
  {
    const char *tag = (const char*)name;
    if (!_feed_flag && strcasecmp(tag,"feed") == 0)
      {
        _feed_flag = true;
      }
    else if (_feed_flag && strcasecmp(tag,"entry") == 0)
      {
        // create new snippet.
        search_snippet *sp = new search_snippet(++_count);
        sp->_engine |= std::bitset<NSEs>(SE_OPENSEARCH);
        pc->_current_snippet = sp;
        pc->_snippets->push_back(pc->_current_snippet);
        _entry_flag = true;
      }
    else if (_entry_flag && strcasecmp(tag,"title") == 0)
      {
        _entry_title_flag = true;
      }
    else if (_entry_flag && strcasecmp(tag,"link") == 0)
      {
        const char *a_link = se_parser::get_attribute((const char**)attribute,"href");
        if (a_link && pc->_current_snippet)
          {
            pc->_current_snippet->set_url(a_link);
          }
      }
    else if (_entry_flag && strcasecmp(tag,"updated") == 0)
      {
        _updated_flag = true;
      }
    else if (_entry_flag && strcasecmp(tag,"content") == 0)
      {
        const char *a_type = se_parser::get_attribute((const char**)attribute,"type");
        if (a_type)
          _entry_type = a_type;
        _content_flag = true;
      }
    else if (_feed_flag && strcasecmp(tag,"title") == 0)
      {
        _gen_title_flag = true;
      }
  }

  void se_parser_osearch_atom::characters(parser_context *pc,
                                          const xmlChar *chars,
                                          int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_osearch_atom::cdata(parser_context *pc,
                                     const xmlChar *chars,
                                     int length)
  {
    //handle_characters(pc, chars, length);
  }

  void se_parser_osearch_atom::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (_entry_title_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        _entry_title += a_chars;
      }
    else if (_updated_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        _entry_date += a_chars;
      }
    else if (_content_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        _entry_content += a_chars;
      }
    else if (_gen_title_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        _gen_title += a_chars;
      }
  }

  void se_parser_osearch_atom::end_element(parser_context *pc,
      const xmlChar *name)
  {
    const char *tag = (const char*)name;
    if (!_feed_flag || !pc->_current_snippet)
      return;
    else if (_entry_flag && strcasecmp(tag,"entry") == 0)
      {
        // validate snippet.
        validate_current_snippet(pc);
        _entry_flag = false;
      }
    else if (_entry_title_flag && strcasecmp(tag,"title") == 0)
      {
        pc->_current_snippet->set_title(_entry_title);
        _entry_title_flag = false;
        _entry_title = "";
      }
    else if (_updated_flag && strcasecmp(tag,"updated") == 0)
      {
        pc->_current_snippet->set_date(_entry_date);
        _updated_flag = false;
        _entry_date = "";
      }
    else if (_content_flag && strcasecmp(tag,"content") == 0)
      {
        pc->_current_snippet->set_summary(_entry_content);
        _content_flag = false;
        _entry_content = "";
      }
    else if (_gen_title_flag && strcasecmp(tag,"title") == 0)
      {
        //TODO: useless for now.
        _gen_title_flag = false;
      }
    else if (_feed_flag && strcasecmp(tag,"feed") == 0)
      {
        _feed_flag = false;
      }
  }

  void se_parser_osearch_atom::validate_current_snippet(parser_context *pc)
  {
    if (pc->_current_snippet)
      {
        if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
            || pc->_current_snippet->_url.empty())
          {
            delete pc->_current_snippet;
            pc->_current_snippet = NULL;
            _count--;
            pc->_snippets->pop_back();
          }
      }
  }

  /*- se_parser_osearch_rss -*/
  se_parser_osearch_rss::se_parser_osearch_rss()
    :se_parser(),se_parser_osearch(),_link_flag(false)
  {
  }

  se_parser_osearch_rss::~se_parser_osearch_rss()
  {
  }

  void se_parser_osearch_rss::start_element(parser_context *pc,
      const xmlChar *name,
      const xmlChar **attribute)
  {
    const char *tag = (const char*)name;
    if (!_feed_flag && strcasecmp(tag,"channel") == 0)
      {
        _feed_flag = true;
      }
    else if (_feed_flag && strcasecmp(tag,"item") == 0)
      {
        // create new snippet.
        search_snippet *sp = new search_snippet(++_count);
        sp->_engine |= std::bitset<NSEs>(SE_OPENSEARCH);
        pc->_current_snippet = sp;
        pc->_snippets->push_back(pc->_current_snippet);
        _entry_flag = true;
      }
    else if (_entry_flag && strcasecmp(tag,"title") == 0)
      {
        _entry_title_flag = true;
      }
    else if (_entry_flag && strcasecmp(tag,"link") == 0)
      {
        std::cerr << "got link\n";
        _link_flag = true;
      }
    /*else if (_entry_flag && strcasecmp(tag,"updated") == 0)
      {
    	_updated_flag = true;
    	}*/
    else if (_entry_flag && strcasecmp(tag,"description") == 0)
      {
        const char *a_type = se_parser::get_attribute((const char**)attribute,"type");
        if (a_type)
          _entry_type = a_type;
        _content_flag = true;
      }
    else if (_feed_flag && strcasecmp(tag,"title") == 0)
      {
        _gen_title_flag = true;
      }
  }

  void se_parser_osearch_rss::characters(parser_context *pc,
                                         const xmlChar *chars,
                                         int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_osearch_rss::cdata(parser_context *pc,
                                    const xmlChar *chars,
                                    int length)
  {
    //handle_characters(pc, chars, length);
  }

  void se_parser_osearch_rss::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (_entry_title_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        _entry_title += a_chars;
      }
    /*else if (_updated_flag)
      {
    	std::string a_chars = std::string((char*)chars);
    	miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
    	_entry_date += a_chars;
    	}*/
    if (_link_flag)
      {
        std::string a_chars = std::string((char*)chars);
        std::cerr << "link a_chars: " << a_chars << std::endl;
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        pc->_current_snippet->set_url(a_chars);
        _link_flag = false;
      }
    else if (_content_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        _entry_content += a_chars;
      }
    else if (_gen_title_flag)
      {
        std::string a_chars = std::string((char*)chars);
        miscutil::replace_in_string(a_chars,"\n"," ");
        miscutil::replace_in_string(a_chars,"\r"," ");
        _gen_title += a_chars;
      }
  }

  void se_parser_osearch_rss::end_element(parser_context *pc,
                                          const xmlChar *name)
  {
    const char *tag = (const char*)name;
    if (!_feed_flag || !pc->_current_snippet)
      return;
    else if (_entry_flag && strcasecmp(tag,"item") == 0)
      {
        // validate snippet.
        validate_current_snippet(pc);
        _entry_flag = false;
      }
    else if (_entry_title_flag && strcasecmp(tag,"title") == 0)
      {
        pc->_current_snippet->set_title(_entry_title);
        _entry_title_flag = false;
        _entry_title = "";
      }
    /*else if (_updated_flag && strcasecmp(tag,"updated") == 0)
      {
    	pc->_current_snippet->set_date(_entry_date);
    	_updated_flag = false;
    	}*/
    else if (_content_flag && strcasecmp(tag,"description") == 0)
      {
        pc->_current_snippet->set_summary(_entry_content);
        _content_flag = false;
        _entry_content = "";
      }
    /* else if (_link_flag && strcasecmp(tag,"link") == 0)
      {
    	pc->_current_snippet->set_url(_link);
    	_link_flag = false;
    	} */ // weird bug, seems to be triggered right after open bracket detection.
    else if (_gen_title_flag && strcasecmp(tag,"title") == 0)
      {
        //TODO: useless for now.
        _gen_title_flag = false;
      }
    else if (_feed_flag && strcasecmp(tag,"channel") == 0)
      {
        _feed_flag = false;
      }
  }

  void se_parser_osearch_rss::validate_current_snippet(parser_context *pc)
  {
    if (pc->_current_snippet)
      {
        if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
            || pc->_current_snippet->_url.empty())
          {
            delete pc->_current_snippet;
            pc->_current_snippet = NULL;
            _count--;
            pc->_snippets->pop_back();
          }
      }
  }


} /* end of namespace. */
