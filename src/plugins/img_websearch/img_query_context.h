/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#ifndef IMG_QUERY_CONTEXT_H
#define IMG_QUERY_CONTEXT_H

#include "query_context.h"
#include "img_websearch_configuration.h"

namespace seeks_plugins
{

  class img_query_context : public query_context
  {
    public:
      img_query_context();

      img_query_context(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                        const std::list<const char*> &http_headers);

      virtual bool sweep_me();

      virtual void register_qc();

      virtual void unregister();

      virtual ~img_query_context();

      virtual void generate(client_state *csp,
                            http_response *rsp,
                            const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                            bool &expanded) throw (sp_exception);

      void expand_img(client_state *csp,
                      http_response *rsp,
                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                      const int &page_start, const int &page_end,
                      const feeds &se_enabled) throw (sp_exception);

      static void fillup_img_engines(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                     feeds &engines);

    public:
      feeds _img_engines;
      unsigned short _exp_safesearch_on;
      unsigned short _exp_safesearch_off;
      bool _safesearch;
  };

} /* end of namespace. */

#endif
