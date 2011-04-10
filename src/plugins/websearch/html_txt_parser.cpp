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

#include "html_txt_parser.h"

namespace seeks_plugins
{
  html_txt_parser::html_txt_parser(const std::string &url)
    : se_parser(url),_txt("")
  {
  }

  html_txt_parser::~html_txt_parser()
  {
  }

  void html_txt_parser::characters(parser_context *pc,
                                   const xmlChar *chars,
                                   int length)
  {
    handle_characters(pc, chars, length);
  }

  void html_txt_parser::cdata(parser_context *pc,
                              const xmlChar *chars,
                              int length)
  {
    return;
  }

  void html_txt_parser::handle_characters(parser_context *pc,
                                          const xmlChar *chars,
                                          int length)
  {
    if (chars)
      _txt += std::string((char*)chars);
  }


} /* end of namespace. */
