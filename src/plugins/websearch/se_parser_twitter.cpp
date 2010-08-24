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
        se_parser_twitter::se_parser_twitter()
                :se_parser(),_in_entry(false),_in_title(false)
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
                        std::cout << "<entry>" << std::endl;
                        _in_entry = true;
                        // create new snippet.
                        search_snippet *sp = new search_snippet(_count + 1);
                        _count++;
                        sp->_engine |= std::bitset<NSEs>(SE_TWITTER);
                        pc->_current_snippet = sp;
                }
                if (_in_entry && strcasecmp(tag, "title") == 0)
                {
                        std::cout << "  <title>" << std::endl;
                        _in_title = true;
                }
                if (_in_entry && strcasecmp(tag, "link") == 0)
                {
                        std::cout << "  <link />" << std::endl;
                        const char *a_link = se_parser::get_attribute((const char**)attributes, "href");
                        pc->_current_snippet->_url = std::string(a_link);
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
                handle_characters(pc, chars, length);
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
                        std::cout << "    " << _title << std::endl;
                }
        }

        void se_parser_twitter::end_element(parser_context *pc,
                        const xmlChar *name)
        {
                const char *tag = (const char*) name;

                if (_in_entry && strcasecmp(tag, "entry") == 0)
                {
                        std::cout << "</entry>" << std::endl;
                        _in_entry = false;

                        // assert previous snippet if any.
                        if (pc->_current_snippet)
                        {
                                if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
                                        || pc->_current_snippet->_url.empty())
                                {
                                        std::cout << "[snippet fail]" << " title: " << pc->_current_snippet->_title.empty() << " url: " << pc->_current_snippet->_url.empty() << std::endl;
                                        delete pc->_current_snippet;
                                        pc->_current_snippet = NULL;
                                        _count--;
                                }
                                else pc->_snippets->push_back(pc->_current_snippet);
                        }
                }
                if (_in_entry && _in_title && strcasecmp(tag, "title") == 0)
                {
                        std::cout << "  </title>" << std::endl;
                        _in_title = false;
                        pc->_current_snippet->_title = _title;
                        _title = "";
                }
        }


} /* end of namespace. */
