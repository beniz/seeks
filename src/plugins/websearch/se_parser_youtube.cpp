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

#include "se_parser_youtube.h"
#include "miscutil.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;

namespace seeks_plugins
{
        se_parser_youtube::se_parser_youtube()
                :se_parser(),_in_item(false),_in_title(false),_in_link(false),_in_date(false),_in_description(false)
        {
        }

        se_parser_youtube::~se_parser_youtube()
        {
        }

        void se_parser_youtube::start_element(parser_context *pc,
                        const xmlChar *name,
                        const xmlChar **attributes)
        {
                const char *tag = (const char*)name;

                if (strcasecmp(tag, "item") == 0)
                {
                        //std::cout << "<item>" << std::endl;
                        _in_item = true;
                        // create new snippet.
		   search_snippet *sp = new search_snippet(_count + 1);
                        _count++;
		   sp->_engine |= std::bitset<NSEs>(SE_YOUTUBE);
		   sp->_doc_type = VIDEO;
                        pc->_current_snippet = sp;
                        //const char *a_link = se_parser::get_attribute((const char**)attributes, "rdf:about");
                        //pc->_current_snippet->_url = std::string(a_link);
                }
                if (_in_item && strcasecmp(tag, "title") == 0)
                {
                        //std::cout << "  <title>" << std::endl;
                        _in_title = true;
                }
                if (_in_item && strcasecmp(tag, "pubDate") == 0)
                {
                        //std::cout << "  <pubDate>" << std::endl;
                        _in_date = true;
                }
                if (_in_item && strcasecmp(tag, "link") == 0)
                {
                        //std::cout << "  <link>" << std::endl;
                        _in_link = true;
                }
                if (_in_item && strcasecmp(tag, "description") == 0)
                {
                        //std::cout << "  <description>" << std::endl;
                        _in_description = true;
                }
        }

        void se_parser_youtube::characters(parser_context *pc,
                        const xmlChar *chars,
                        int length)
        {
                handle_characters(pc, chars, length);
        }

        void se_parser_youtube::cdata(parser_context *pc,
                        const xmlChar *chars,
                        int length)
        {
	   //handle_characters(pc, chars, length);
        }

        void se_parser_youtube::handle_characters(parser_context *pc,
                        const xmlChar *chars,
                        int length)
        {
                if (_in_description)
                {
		   std::string a_chars = std::string((char*)chars);
                        miscutil::replace_in_string(a_chars,"\n"," ");
                        miscutil::replace_in_string(a_chars,"\r"," ");
                        _description += a_chars;
                }
	   else if (_in_link)
	     {
		//std::cout << "    in link" << std::endl;
		_link.append((char*)chars,length);
		}
                else if (_in_date)
                {
		   _date.append((char*)chars,length);
                }
                else if (_in_title)
                {
		   _title.append((char*)chars,length);
                }
        }

        void se_parser_youtube::end_element(parser_context *pc,
                        const xmlChar *name)
        {
                const char *tag = (const char*) name;

                if (_in_item && strcasecmp(tag, "description") == 0)
                {
                        // get image src, don't like this code
                        // url will maybe need to be unquoted
                        int start = _description.find("src=\"");
                        int end = _description.find(".jpg\"", start + 5);
                        _description = _description.substr(start + 5, end - start - 1);
                        //std::cout << "    " << _description << std::endl;

		   //std::cout << "  </description>" << std::endl;
                        _in_description = false;
                        pc->_current_snippet->_cached = _description;
                        _description = "";
                }
                else if (_in_item && strcasecmp(tag, "item") == 0)
                {
		   //std::cout << "</item>" << std::endl;
                        _in_item = false;

                        // assert previous snippet if any.
                        if (pc->_current_snippet)
                        {
			   if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
			       || pc->_current_snippet->_url.empty()
                                        || pc->_current_snippet->_cached.empty()
                                        || pc->_current_snippet->_date.empty())
                                {
				   std::cout << "[snippet fail]" << " title: " << pc->_current_snippet->_title.empty() << " url: " << pc->_current_snippet->_url.empty() << std::endl;
                                        delete pc->_current_snippet;
                                        pc->_current_snippet = NULL;
                                        _count--;
                                }
                                else pc->_snippets->push_back(pc->_current_snippet);
                        }
                }
                else if (_in_item && _in_date && strcasecmp(tag, "pubDate") == 0)
                {
		   //std::cout << "    " << _date << std::endl;
		   //std::cout << "  </pubDate>" << std::endl;
                        _in_date = false;
                        pc->_current_snippet->_date = _date;
                        _date = "";
                }
                else if (_in_item && _in_title && strcasecmp(tag, "title") == 0)
                {
                        //std::cout << "    " << _title << std::endl;
		   //std::cout << "  </title>" << std::endl;
                        _in_title = false;
                        pc->_current_snippet->_title = _title;
                        _title = "";
                }
                else if (_in_item && _in_link && strcasecmp(tag, "link") == 0)
                {
		   //std::cout << "  </link>" << std::endl;
		   miscutil::replace_in_string(_link,"&feature=youtube_gdata",""),
		   pc->_current_snippet->set_url(_link);
		   _in_link = false;
		   _link = "";
                }
        }


} /* end of namespace. */
