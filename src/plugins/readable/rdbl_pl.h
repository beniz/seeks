/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#ifndef RDBL_PL_H
#define RDBL_PL_H

#include "plugin.h"
#include "interceptor_plugin.h"
#include "readable.h"
#include "sp_exception.h"

using namespace sp;

namespace seeks_plugins
{

  class rdbl_pl : public plugin
  {
    public:
      rdbl_pl();
      virtual ~rdbl_pl();

      virtual void start() {};
      virtual void stop() {};

      static sp_err cgi_readable(client_state *csp,
                                 http_response *rsp,
                                 const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      static sp_err fetch_url_call_readable(const std::string &url,
                                            std::string &content);

      /**
       * \brief main extraction function.
       * @param html the HTML content to process for better readability.
       * @param url the URL the content was grabbed from.
       * @param encoding the document encoding, or empty (from http://xmlsoft.org/html/libxml-HTMLparser.html#htmlReadDoc).
       * @param options:
       *   READABLE_OPTION_STRIP_UNLIKELYS = 1,
       *   READABLE_OPTION_WEIGHT_CLASSES = 1 << 1,
       *   READABLE_OPTION_CHECK_MIN_LENGTH = 1 << 2,
       *   READABLE_OPTION_CLEAN_CONDITIONALLY = 1 << 3,
       *   READABLE_OPTION_REMOVE_IMAGES = 1 << 4,
       *   READABLE_OPTION_LOOK_HARDER_FOR_IMAGES = 1 << 5,
       *   READABLE_OPTION_TRY_MULTIMEDIA_ARTICLE = 1 << 6,
       *   READABLE_OPTION_WRAP_CONTENT = 1 << 7,
       *   READABLE_OPTIONS_DEFAULT = (0xFFFF & ~(READABLE_OPTION_REMOVE_IMAGES | READABLE_OPTION_WRAP_CONTENT))
       */
      static std::string call_readable(const std::string &html, const std::string &url,
                                       const std::string &encoding="",
                                       const int &options=READABLE_OPTIONS_DEFAULT) throw (sp_exception);
  };

  class rdbl_elt : public interceptor_plugin
  {
    public:
      rdbl_elt(plugin *parent);
      ~rdbl_elt() {};

      virtual http_response* plugin_response(client_state *csp);
  };

} /* end of namespace. */

#endif
