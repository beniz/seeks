/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "xsl_serializer.h"
#include "xml_renderer.h"
#include "seeks_proxy.h"
#include "miscutil.h"

#include <string.h>
#include <iostream>

using namespace sp;

namespace seeks_plugins
{

  xsl_serializer::xsl_serializer()
    :plugin()
  {
    _name = "xsl-serializer";
    _version_major = "0";
    _version_minor = "1";
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 1;
  }

  xsl_serializer::~xsl_serializer()
  {
    xsltCleanupGlobals();
    xmlCleanupParser();
  }


  /* public methods */

  /*
    render_xsl_cached_queries
    render_xsl_clustered_results
    render_xsl_engines
    render_xsl_node_options
    render_xsl_recommendations
    render_xsl_results
    render_xsl_snippet
    render_xsl_suggested_queries
    render_xsl_words
  */


  sp_err xsl_serializer::render_xml_cached_queries(const query_context *qc,
						   http_response *rsp,
						   const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
						   const std::string &query,
						   const int &nq)
  {
    sp_err res=SP_ERR_OK;
    xmlDocPtr doc=xmlNewDoc(BAD_CAST "1.0");
    res = xml_renderer::render_xml_cached_queries(query,nq,doc);
    res = res && xsl_serializer::response(rsp,parameters,doc);
    xmlFreeDoc(doc);
    return res;
  }



  sp_err xsl_serializer::render_xml_clustered_results(const query_context *qc,
						      http_response *rsp,
						      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
						      cluster *clusters,
						      const short &K,
						      client_state *csp, 
						      const double &qtime)
  {
    sp_err res=SP_ERR_OK;
    xmlDocPtr doc=xmlNewDoc(BAD_CAST "1.0");
    res = xml_renderer::render_xml_clustered_results(qc, parameters,clusters,K,qtime, doc);
    res = res && xsl_serializer::response(rsp,parameters,doc);
    xmlFreeDoc(doc);
    return res;
  }

  sp_err xsl_serializer::render_xsl_engines(const query_context *qc,
					    http_response *rsp,
					    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					    const feeds &engines)
  {
    sp_err res=SP_ERR_OK;
    xmlDocPtr doc=xmlNewDoc(BAD_CAST "1.0");
    res = xml_renderer::render_xml_engines(engines, doc);
    res = res && xsl_serializer::response(rsp,parameters,doc);
    xmlFreeDoc(doc);
    return res;    
  }


  sp_err xsl_serializer::render_xsl_node_options(const query_context *qc,
						 http_response *rsp,
						 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
						 client_state *csp)						 
  {
    sp_err res=SP_ERR_OK;
    xmlDocPtr doc=xmlNewDoc(BAD_CAST "1.0");
    res=xml_renderer::render_xml_node_options(csp, doc);
    res = res && xsl_serializer::response(rsp,parameters,doc);
    xmlFreeDoc(doc);
    return res;
  }

  sp_err xsl_serializer::render_xsl_recommendations(const query_context *qc,
						    http_response *rsp,
						    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
						    const double &qtime,
						    const int &radius,
						    const std::string &lang)
  {
    sp_err res=SP_ERR_OK;
    xmlDocPtr doc=xmlNewDoc(BAD_CAST "1.0");
    res=xml_renderer::render_xml_recommendations(qc,parameters,qtime,radius,lang,doc);
    res = res && xsl_serializer::response(rsp,parameters,doc);
    xmlFreeDoc(doc);    
    return res;
  }

  sp_err xsl_serializer::render_xsl_results(const query_context *qc,
					    http_response *rsp,
					    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					    client_state *csp, 
					    const std::vector<search_snippet*> &snippets,
					    const double &qtime,
					    const bool &img=false)
  {
    sp_err res=SP_ERR_OK;
    xmlDocPtr doc=xmlNewDoc(BAD_CAST "1.0");
    res=xml_renderer::render_xml_results(qc, parameters,snippets, qtime, img, doc);
    res = res && xsl_serializer::response(rsp,parameters,doc);
    xmlFreeDoc(doc);    
    return res;
  }

  sp_err xsl_serializer::render_xsl_snippet(query_context *qc,
					    http_response *rsp,
					    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					    const search_snippet *sp)
  {
    sp_err res=SP_ERR_OK;
    xmlDocPtr doc=xmlNewDoc(BAD_CAST "1.0");
    res=xml_renderer::render_xml_snippet(qc,sp,doc);
    res = res && xsl_serializer::response(rsp,parameters,doc);
    xmlFreeDoc(doc);    
    return res; 
  }


  sp_err xsl_serializer::render_xsl_suggested_queries(const query_context *qc,
						      http_response *rsp,
						      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    sp_err res=SP_ERR_OK;
    xmlDocPtr doc=xmlNewDoc(BAD_CAST "1.0");
    res=xml_renderer::render_xml_suggested_queries(qc,parameters,doc);
    res = res && xsl_serializer::response(rsp,parameters,doc);
    xmlFreeDoc(doc);    
    return res;
  }
    
  sp_err xsl_serializer::render_xsl_words(const query_context *qc,
					  http_response *rsp,
					  const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					  const std::set<std::string> &words)
  {
    sp_err res=SP_ERR_OK;
    xmlDocPtr doc=xmlNewDoc(BAD_CAST "1.0");
    res=xml_renderer::render_xml_words(words,doc);
    res = res && xsl_serializer::response(rsp,parameters,doc);
    xmlFreeDoc(doc);    
    return res;
  }
  

  /* private */
  
  sp_err xsl_serializer::response(http_response *rsp,
				const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				xmlDocPtr doc) {
    
    sp_err res=SP_ERR_OK;

    // check for parameter stylesheet
    const char *stylesheet = miscutil::lookup(parameters,"stylesheet");
    int buffersize;

    if (!stylesheet) {
      xmlChar *xmlbuff;
      xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
      miscutil::enlist(&rsp->_headers, "Content-Type: text/xml");
      rsp->_body = strdup((char *)xmlbuff);
      rsp->_content_length = buffersize;
      xmlFree(xmlbuff);
    } else {
      xsl_serializer::transform(rsp,doc,stylesheet);
    }
    rsp->_is_static = 1;
    return res;
  }

  void xsl_serializer::transform(http_response *rsp, 
				 xmlDocPtr doc, 
				 const std::string stylesheet) {
    xsltStylesheetPtr cur;
    xmlDocPtr stylesheet_doc,res_doc;
    xmlNodePtr root_element, cur_node;
    char *ct=NULL;
    xmlChar *buffer;
    int length;
      
    // parse the stylesheet as libxml2 Document
    stylesheet_doc=xsl_serializer::get_stylesheet(stylesheet);
    
    // get the content type
    // Get the root element node 
    root_element = xmlDocGetRootElement(stylesheet_doc);
    for (cur_node = root_element->children; cur_node; cur_node = cur_node->next) {
      if (cur_node->type == XML_PI_NODE && !strcmp((char *)(cur_node->name), "Content-Type:")) {
	ct=strdup( (char *)cur_node->name);
	strcat(ct, (char *)cur_node->content);
	miscutil::enlist(&rsp->_headers, ct);
      }
    }
    
    if (!ct) {
      miscutil::enlist(&rsp->_headers, "Content-Type: text/xml");
    }

    // prepare the xslt
    cur = xsltParseStylesheetDoc(stylesheet_doc);

    // apply the stylesheet
    const char **params=NULL;
    res_doc = xsltApplyStylesheet(cur, doc, params);
    xsltSaveResultToString(&buffer, &length, res_doc, cur);
    rsp->_content_length = length;
    rsp->_body = strdup((char *)buffer);
    free(buffer);
    xsltFreeStylesheet(cur);
    xmlFreeDoc(res_doc);
    xmlFreeDoc(stylesheet_doc);
  }

   
  xmlDocPtr xsl_serializer::get_stylesheet(const std::string stylesheet)
  {
    std::string stylesheet_path;
    xmlDocPtr xslt;
    // TODO : manage a cache of stylesheets
    if (seeks_proxy::_datadir.empty())
      stylesheet_path = plugin_manager::_plugin_repository + "websearch/stylesheets/" + stylesheet + ".xsl";
    else
      stylesheet_path = seeks_proxy::_datadir + "/plugins/websearch/stylesheets/" + stylesheet + ".xsl";
    xslt=xmlParseFile(stylesheet_path.c_str());
    return xslt;
  }

  /* plugin registration. */
  extern "C"
  {
    plugin* maker()
    {
      return new xsl_serializer;
    }
  }

} /* end of namespace */
