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

#include "se_parser_dailymotion.h"
#include "miscutil.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;

namespace seeks_plugins
{
        se_parser_dailymotion::se_parser_dailymotion()
                :se_parser(),_in_item(false),_in_title(false),_in_link(false),_in_pubdate(false),_in_summary(false)
        {
        }

        se_parser_dailymotion::~se_parser_dailymotion()
        {
        }

        void se_parser_dailymotion::start_element(parser_context *pc,
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
		   sp->_engine |= std::bitset<NSEs>(SE_DAILYMOTION);
                   sp->_doc_type = VIDEO;
		   pc->_current_snippet = sp;
                }
	   else if (_in_item && strcasecmp(tag, "title") == 0)
                {
		   //std::cout << "  <title>" << std::endl;
                        _in_title = true;
                }
                else if (_in_item && strcasecmp(tag, "guid") == 0)
                {
		   //std::cout << "  <link>" << std::endl;
                        _in_link = true;
                }
	   else if (_in_item && strcasecmp(tag, "pubDate") == 0)
                {
		   //std::cout << "  <pubDate>" << std::endl;
                        _in_pubdate = true;
                }
	   /* else if (_in_item && strcasecmp(tag, "itunes:keywords") == 0)
	     {
                        std::cout << "  <keywords>" << std::endl;
                        _in_keywords = true;
                } */
                else if (_in_item && strcasecmp(tag, "itunes:summary") == 0)
                {
		   // std::cout << "  <summary>" << std::endl;
                        _in_summary = true;
                }
	   else if (_in_item && strcasecmp(tag, "media:thumbnail") == 0)
	     {
		const char *a_url = se_parser::get_attribute((const char**)attributes,"url");
		if (a_url)
		  {
		     pc->_current_snippet->_cached = a_url;
		  }
	     }
	}

        void se_parser_dailymotion::characters(parser_context *pc,
                        const xmlChar *chars,
                        int length)
        {
                handle_characters(pc, chars, length);
        }

        void se_parser_dailymotion::cdata(parser_context *pc,
                        const xmlChar *chars,
                        int length)
        {
                //handle_characters(pc, chars, length);
        }

        void se_parser_dailymotion::handle_characters(parser_context *pc,
                        const xmlChar *chars,
                        int length)
        {
                if (_in_item && _in_title)
                {
		   _title.append((char*)chars,length);
                }
	   else if (_in_item && _in_link)
                {
		   _link.append((char*)chars,length);
                }
	   /* if (_in_keywords)
	     {
                        _keywords += a_chars;
                } */
                else if (_in_item && _in_pubdate)
                {
		   _date.append((char*)chars,length);
                }
                /* else if (_in_item && _in_summary)
                {
		   _summary.append((char*)chars,length);
		   } */
        }

        void se_parser_dailymotion::end_element(parser_context *pc,
                        const xmlChar *name)
        {
                const char *tag = (const char*) name;

                if (_in_item && strcasecmp(tag, "item") == 0)
                {
		   //std::cout << "</item>" << std::endl;
                        _in_item = false;

                        // assert previous snippet if any.
                        if (pc->_current_snippet)
                        {
                                if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
                                        || pc->_current_snippet->_url.empty()
				    //|| pc->_current_snippet->_summary.empty()
                                        //|| pc->_current_snippet->_cached.empty()
                                        || pc->_current_snippet->_date.empty())
                                {
				   //std::cout << "[snippet fail]" << " title: " << pc->_current_snippet->_title.empty() << " url: " << pc->_current_snippet->_url.empty() << std::endl;
                                        delete pc->_current_snippet;
                                        pc->_current_snippet = NULL;
                                        _count--;
                                }
                                else pc->_snippets->push_back(pc->_current_snippet);
                        }
                }
	   else if (_in_item && _in_title && strcasecmp(tag, "title") == 0)
                {
		   //std::cout << "    " << _title << std::endl;
                        //std::cout << "  </title>" << std::endl;
                        _in_title = false;
                        pc->_current_snippet->_title = _title;
                        _title = "";
                }
                else if (_in_item && _in_link && strcasecmp(tag, "guid") == 0)
                {
		   //std::cout << "    " << _link << std::endl;
		   //std::cout << "  </link>" << std::endl;
                        _in_link = false;
                        pc->_current_snippet->set_url(_link);
                        _link = "";
                }
	   /* else if (_in_item && _in_keywords && strcasecmp(tag, "itunes:keywords") == 0)
                {
                        std::cout << "    " << _date << std::endl;
                        std::cout << "</keywords>" << std::endl;
                        _in_keywords = false;
                        pc->_current_snippet->_cached = _keywords;
                        _keywords = "";
                } */
                else if (_in_item && _in_summary && strcasecmp(tag, "itunes:summary") == 0)
                {
		   //std::cout << "    " << _date << std::endl;
		   //std::cout << "</summary>" << std::endl;
                        _in_summary = false;
                        //pc->_current_snippet->set_summary(_summary);
                        //_summary = "";
                }
	   else if (_in_item && _in_pubdate && strcasecmp(tag, "pubDate") == 0)
                {
		   //std::cout << "    " << _date << std::endl;
		   //std::cout << "</pubDate>" << std::endl;
                        _in_pubdate = false;
                        pc->_current_snippet->set_date(_date);
                        _date = "";
                }
        }


} /* end of namespace. */
