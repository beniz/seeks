/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009-2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#ifndef XSLSERIALIZER_H
#define XSLSERIALIZER_H

#define NO_PERL // we do not use Perl.

#include "wb_err.h"
#include "plugin.h"
#include "search_snippet.h"
#include "query_context.h"
#include "miscutil.h"
#include "mutexes.h"

#include <string>

using sp::client_state;
using sp::http_response;
using sp::plugin;
using sp::miscutil;

namespace seeks_plugins
{
  class xsl_serializer : public plugin
  {
    public:
      xsl_serializer();

      ~xsl_serializer();

      virtual void start();
      virtual void stop();


      /* websearch. */
      /*static void perform_action_threaded(wo_thread_arg *args);

      static sp_err perform_action(client_state *csp,
                                   http_response *rsp,
                                   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                   bool render = true);*/



      /* error handling. */

    public:
      static xslserializer_configuration *_xslconfig;
      static hash_map<uint32_t,query_context*,id_hash_uint> _active_qcontexts;
      static double _cl_sec; // clock ticks per second.

      /* dependent plugins. */
    public:
      static plugin *_qc_plugin; /**< query capture plugin. */
      static bool _qc_plugin_activated;
      static plugin *_cf_plugin; /**< (collaborative) filtering plugin. */
      static bool _cf_plugin_activated;
      
      /* multithreading. */
    public:
      static sp_mutex_t _context_mutex;
  };

} /* end of namespace. */

#endif
