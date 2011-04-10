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

#ifndef SE_PARSER_OSEARCH
#define SE_PARSER_OSEARCH

#include "se_parser.h"

namespace seeks_plugins
{
  class se_parser_osearch
  {
    public:
      se_parser_osearch();

      ~se_parser_osearch();

      virtual void validate_current_snippet(parser_context *pc) = 0;

    protected:
      bool _feed_flag;
      bool _entry_flag;
      bool _entry_title_flag;
      bool _updated_flag;
      bool _content_flag;
      bool _gen_title_flag;

      std::string _entry_type;
      std::string _entry_title;
      std::string _entry_date;
      std::string _entry_content;
      std::string _gen_title;
  };

  class se_parser_osearch_atom : public se_parser, public se_parser_osearch
  {
    public:
      se_parser_osearch_atom(const std::string &url);

      ~se_parser_osearch_atom();

      // virtual fct.
      void start_element(parser_context *pc,
                         const xmlChar *name,
                         const xmlChar **attributes);

      void end_element(parser_context *pc,
                       const xmlChar *name);

      void characters(parser_context *pc,
                      const xmlChar *chars,
                      int length);

      void cdata(parser_context *pc,
                 const xmlChar *chars,
                 int length);

      // local.
      void handle_characters(parser_context *pc,
                             const xmlChar *chars,
                             int length);

      virtual void validate_current_snippet(parser_context *pc);
  };


  class se_parser_osearch_rss : public se_parser, public se_parser_osearch
  {
    public:
      se_parser_osearch_rss(const std::string &url);

      ~se_parser_osearch_rss();

      // virtual fct.
      void start_element(parser_context *pc,
                         const xmlChar *name,
                         const xmlChar **attributes);

      void end_element(parser_context *pc,
                       const xmlChar *name);

      void characters(parser_context *pc,
                      const xmlChar *chars,
                      int length);

      void cdata(parser_context *pc,
                 const xmlChar *chars,
                 int length);

      // local.
      void handle_characters(parser_context *pc,
                             const xmlChar *chars,
                             int length);

      virtual void validate_current_snippet(parser_context *pc);

    private:
      bool _link_flag;
  };

#endif

} /* end of namespace. */
