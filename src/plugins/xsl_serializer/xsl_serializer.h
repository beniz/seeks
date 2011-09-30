/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009-2011 Emmanuel Benazera <ebenazer@seeks-project.info>
 * Copyright (C) 2011 Stéphane Bonhomme <stephane.bonhomme@seeks.pro>
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
#include "xsl_serializer_configuration.h"
#include "miscutil.h"
#include "mutexes.h"
#include "clustering.h"

#include <string>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>


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
    
    static sp_err render_xml_cached_queries(const query_context *qc,
					    http_response *rsp,
					    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					    const std::string &query,
					    const int &nq);


    static sp_err render_xml_clustered_results(const query_context *qc,
					       http_response *rsp,
					       const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					       cluster *clusters,
					       const short &K,
					       client_state *csp, 
					       const double &qtime);

    static sp_err render_xsl_engines(const query_context *qc,
				     http_response *rsp,
				     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				     const feeds &engines);

    static sp_err render_xsl_node_options(const query_context *qc,
					  http_response *rsp,
					  const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					  client_state *csp);

    static sp_err render_xsl_recommendations(const query_context *qc,
					     http_response *rsp,
					     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					     const double &qtime,
					     const int &radius,
					     const std::string &lang);

    static sp_err render_xsl_results(const query_context *qc,
				     http_response *rsp,
				     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				     client_state *csp, 
				     const std::vector<search_snippet*> &snippets,
				     const double &qtime,
				     const bool &img);

    static sp_err render_xsl_snippet(query_context *qc,
				     http_response *rsp,
				     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				     const search_snippet *sp);

    static sp_err render_xsl_suggested_queries(const query_context *qc,
					       http_response *rsp,
					       const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);
    
    static sp_err render_xsl_words(const query_context *qc,
				   http_response *rsp,
				   const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				   const std::set<std::string> &words);
  /* private */
  private:
    static sp_err response(http_response *rsp,
			 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
			 xmlDocPtr doc);
    static void transform(http_response *rsp, 
			  xmlDocPtr doc, 
			  const std::string stylesheet);

    static xmlDocPtr get_stylesheet(const std::string stylesheet);


    public:
      static xsl_serializer_configuration *_xslconfig;
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
