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

#include "se_parser_blekko.h"
#include "miscutil.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;

namespace seeks_plugins
{
   se_parser_blekko::se_parser_blekko()
     :se_parser(),_in_entry(false),_in_title(false),_in_uri(false),_in_description(false)
       {
       }

   se_parser_blekko::~se_parser_blekko()
     {
     }

   void se_parser_blekko::start_element(parser_context *pc,
                                         const xmlChar *name,
                                         const xmlChar **attributes)
     {
        const char *tag = (const char*)name;

        if (strcasecmp(tag, "item") == 0)
          {
             // std::cout << "<item>" << std::endl;
             _in_entry = true;
             // create new snippet.
             search_snippet *sp = new search_snippet(_count + 1);
             _count++;
             sp->_engine |= std::bitset<NSEs>(SE_BLEKKO);
             pc->_current_snippet = sp;
          }
        else if (_in_entry && strcasecmp(tag, "title") == 0)
          {
             // std::cout << "  <title>" << std::endl;
             _in_title = true;
          }
        else if (_in_entry && strcasecmp(tag, "guid") == 0)
          {
             // std::cout << "  <link>" << std::endl;
             _in_uri = true;
          }
        else if (_in_entry && strcasecmp(tag, "description") == 0)
             {
             // std::cout << "  <description>" << std::endl;
             _in_description = true;
             }
     }

        void se_parser_blekko::characters(parser_context *pc,
                                           const xmlChar *chars,
                                           int length)
     {
        handle_characters(pc, chars, length);
     }

   void se_parser_blekko::cdata(parser_context *pc,
                                 const xmlChar *chars,
                                 int length)
     {
        //handle_characters(pc, chars, length);
     }

   void se_parser_blekko::handle_characters(parser_context *pc,
                                             const xmlChar *chars,
                                             int length)
     {
        if (_in_description)
          {
             std::string a_chars = std::string((char*)chars);
             miscutil::replace_in_string(a_chars,"\n"," ");
             miscutil::replace_in_string(a_chars,"\r"," ");
             miscutil::replace_in_string(a_chars,"-"," ");
             _description += a_chars;
             // std::cout << "    " << _description << std::endl;
          }
        else if (_in_title)
          {
             std::string a_chars = std::string((char*)chars);
             miscutil::replace_in_string(a_chars,"\n"," ");
             miscutil::replace_in_string(a_chars,"\r"," ");
             miscutil::replace_in_string(a_chars,"-"," ");
             _title += a_chars;
             // std::cout << "    " << _title << std::endl;
          }
        else if (_in_uri)
          {
             //std::string a_chars = std::string((char*)chars);
             //miscutil::replace_in_string(a_chars,"\n"," ");
             //miscutil::replace_in_string(a_chars,"\r"," ");
             //miscutil::replace_in_string(a_chars,"-"," ");
             //_uri += a_chars;
             // //std::cout << "    " << _uri << std::endl;
             _uri.append((char*)chars, length);
             // std::cout << "    " << _uri << std::endl;
          }
     }

   void se_parser_blekko::end_element(parser_context *pc,
                                       const xmlChar *name)
     {
        const char *tag = (const char*) name;

        if (_in_entry && strcasecmp(tag, "item") == 0)
          {
             // std::cout << "</item>" << std::endl;
             _in_entry = false;

             // assert previous snippet if any.
             if (pc->_current_snippet)
               {
                  if (pc->_current_snippet->_title.empty()  // consider the parsing did fail on the snippet.
                      || pc->_current_snippet->_url.empty()
                      || pc->_current_snippet->_summary.empty())
                    {
                       // std::cout << "[snippet fail]" << " title: " << pc->_current_snippet->_title.empty() << " description: " << pc->_current_snippet->_summary.empty() << " url: " << pc->_current_snippet->_url.empty() << std::endl;
                       delete pc->_current_snippet;
                       pc->_current_snippet = NULL;
                       _count--;
                    }
                  else pc->_snippets->push_back(pc->_current_snippet);
               }
          }
        else if (_in_entry && _in_title && strcasecmp(tag, "title") == 0)
          {
             // std::cout << "  </title>" << std::endl;
             _in_title = false;
             pc->_current_snippet->set_title(_title);
             _title = "";
          }
        else if (_in_entry && _in_description && strcasecmp(tag, "description") == 0)
          {
             // std::cout << "  </description>" << std::endl;
             // not a good solution, their is a quote/unquote problem nearby
             miscutil::replace_in_string(_description, "&#39;", "'");
             _in_description = false;
             pc->_current_snippet->set_summary(_description);
             _description = "";
          }
        else if (_in_entry && _in_uri && strcasecmp(tag, "guid") ==0)
          {
             // std::cout << "  </link>" << std::endl;
             _in_uri = false;
             pc->_current_snippet->set_url(_uri);
             _uri = "";
          }
     }

} /* end of namespace. */
