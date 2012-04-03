/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010, 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
 * Copyright (C) 2010, 2011 Stéphane Bonhomme <stephane.bonhomme@seeks.pro>
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

#include "xml_renderer.h"
#include "cgisimple.h"
#include "miscutil.h"
#include "cgi.h"
#include "websearch.h"
#include "query_context.h"
#include "xml_renderer_private.h"

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
#include "img_websearch.h"
#include "img_query_context.h"
#include "img_search_snippet.h"
#endif
#include "proxy_configuration.h"
#include "seeks_proxy.h" // for sweepables.
#include "encode.h"

using sp::cgisimple;
using sp::miscutil;
using sp::cgi;
using sp::proxy_configuration;
using sp::seeks_proxy;
using sp::encode;

using namespace xml_renderer_private;

namespace seeks_plugins
{
  sp_err xml_renderer::render_engines(const feeds &engines,
                                      const bool &img,
                                      xmlNodePtr parent)
  {
    sp_err err=SP_ERR_OK;
    hash_map<const char*,feed_url_options,hash<const char*>,eqstr>::const_iterator hit;
    xmlNodePtr eng=NULL;
    std::set<feed_parser,feed_parser::lxn>::const_iterator it
    = engines._feedset.begin();
    while(it!=engines._feedset.end())
      {
        std::set<std::string>::const_iterator sit = (*it)._urls.begin();
        while(sit!=(*it)._urls.end())
          {
            if (!img)
              {
                if ((hit = websearch::_wconfig->_se_options.find((*sit).c_str()))
                    != websearch::_wconfig->_se_options.end())
                  {
                    eng=xmlNewNode(NULL,BAD_CAST "engine");
                    xmlAddChild(parent,eng);
                    xmlNewProp(eng,BAD_CAST "value",BAD_CAST (*hit).second._id.c_str());
                  }
              }
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
            else
              {
                if ((hit = img_websearch::_iwconfig->_se_options.find((*sit).c_str()))
                    != img_websearch::_iwconfig->_se_options.end())
                  {
                    eng=xmlNewNode(NULL,BAD_CAST "engine");
                    xmlAddChild(parent,eng);
                    xmlNewProp(eng,BAD_CAST "value",BAD_CAST (*hit).second._id.c_str());
                  }
              }
#endif
            ++sit;
          }
        ++it;
      }
    return err;
  }

  sp_err xml_renderer::render_node_options(client_state *csp,
      xmlNodePtr parent)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr feed, feed_url;

    // system options.
    hash_map<const char*, const char*, hash<const char*>, eqstr> *exports
    = cgi::default_exports(csp,"");
    const char *value = miscutil::lookup(exports,"version");
    if (value)
      xmlNewProp(parent,BAD_CAST "version",BAD_CAST value);
    if (websearch::_wconfig->_show_node_ip)
      {
        value = miscutil::lookup(exports,"my-ip-address");
        if (value)
          {
            xmlNewProp(parent,BAD_CAST "my-ip-adress",BAD_CAST value);
          }
      }
    value = miscutil::lookup(exports,"code-status");
    if (value)
      {
        xmlNewProp(parent,BAD_CAST "code-status",BAD_CAST value);
      }
    value = miscutil::lookup(exports,"admin-address");
    if (value)
      {
        xmlNewProp(parent,BAD_CAST "admin-address",BAD_CAST value);
      }
    xmlNewProp(parent,BAD_CAST "url-source-code",BAD_CAST csp->_config->_url_source_code.c_str());

    miscutil::free_map(exports);

    /*- websearch options. -*/
    // thumbs.

    std::string thumbs;
    xmlNewProp(parent,BAD_CAST "thumbs",
               BAD_CAST (websearch::_wconfig->_thumbs ? "on" :"off"));
    xmlNewProp(parent,BAD_CAST "content-analysis",
               BAD_CAST (websearch::_wconfig->_content_analysis ? "on" : "off"));
    xmlNewProp(parent,BAD_CAST "clustering",
               BAD_CAST (websearch::_wconfig->_clustering ? "on" : "off"));

    /* feeds options */
    std::list<std::string> se_options;
    hash_map<const char*,feed_url_options,hash<const char*>,eqstr>::const_iterator hit;
    std::set<feed_parser,feed_parser::lxn>::const_iterator fit
    = websearch::_wconfig->_se_enabled._feedset.begin();
    while(fit!=websearch::_wconfig->_se_enabled._feedset.end())
      {
        feed=xmlNewNode(NULL,BAD_CAST "feed");
        xmlAddChild(parent,feed);
        xmlNewProp(feed,BAD_CAST "name",BAD_CAST ((*fit)._name.c_str()));
        std::list<std::string> se_urls;
        std::set<std::string>::const_iterator sit
        = (*fit)._urls.begin();
        while(sit!=(*fit)._urls.end())
          {
            feed_url=xmlNewNode(NULL,BAD_CAST "url");
            xmlAddChild(feed,feed_url);
            std::string url = (*sit);
            if ((hit=websearch::_wconfig->_se_options.find(url.c_str()))
                !=websearch::_wconfig->_se_options.end())
              {
                xmlNewProp(feed_url,BAD_CAST "value",BAD_CAST ((*hit).second._id.c_str()));
                if ((*hit).second._default)
                  xmlNewProp(feed_url,BAD_CAST "default",BAD_CAST "yes");
              }
            ++sit;
          }
        ++fit;
      }
    return err;
  }

  sp_err xml_renderer::render_suggested_queries(const query_context *qc,
      const int &nsuggs,
      xmlNodePtr parent)
  {
    sp_err err=SP_ERR_OK;
    if (!qc->_suggestions.empty())
      {
        int k = 0;
        xmlNodePtr sugg = NULL;
        std::multimap<double,std::string,std::less<double> >::const_iterator mit
        = qc->_suggestions.begin();
        while(mit!=qc->_suggestions.end())
          {
            std::string sugg_str = (*mit).second;
            miscutil::replace_in_string(sugg_str,"\\","\\\\");
            miscutil::replace_in_string(sugg_str,"\"","\\\"");
            miscutil::replace_in_string(sugg_str,"\t","");
            miscutil::replace_in_string(sugg_str,"\r","");
            miscutil::replace_in_string(sugg_str,"\n","");
            sugg=xmlNewNode(NULL, BAD_CAST "suggested_query");
            xmlAddChild(parent,sugg);
            xmlNewProp(sugg, BAD_CAST "value", BAD_CAST sugg_str.c_str());
            if (k >= nsuggs-1)
              break;
            ++k;
            ++mit;
          }
      }
    return err;
  }


  sp_err xml_renderer::render_recommendations(const query_context *qc,
      const int &nreco,
      const double &qtime,
      const uint32_t &radius,
      const std::string &lang,
      xmlNodePtr parent)
  {
    sp_err err=SP_ERR_OK;
    std::list<std::string> results;

    // query.
    std::string escaped_query = qc->_query;
    miscutil::replace_in_string(escaped_query,"\"","\\\"");
    xmlNewProp(parent,BAD_CAST "query",BAD_CAST escaped_query.c_str());

    // peers.
    xmlNewProp(parent,BAD_CAST "npeers", BAD_CAST (miscutil::to_string(qc->_npeers)).c_str());

    // date.
    char datebuf[256];
    cgi::get_http_time(0,datebuf,sizeof(datebuf));
    xmlNewProp(parent,BAD_CAST "date",BAD_CAST std::string(datebuf).c_str());

    // processing time.
    xmlNewProp(parent,BAD_CAST "qtime",BAD_CAST miscutil::to_string(qtime).c_str());

    //snippets.

    size_t ssize = qc->_cached_snippets.size();
    int count = 0;
    xmlNodePtr snippet_node;
    for (size_t i=0; i<ssize && !err; i++)
      {
        search_snippet *sp = qc->_cached_snippets.at(i);
        if (sp->_radius > radius)
          continue;
        if (!lang.empty() && sp->_lang != lang)
          continue;
        if (sp->_doc_type == doc_type::REJECTED)
          continue;
        if (!sp->_engine.has_feed("seeks"))
          continue;
        snippet_node=xmlNewNode(NULL,BAD_CAST "snippet");

        err = sp->to_xml(false,qc->_query_words,snippet_node);
        count++;

        if (nreco > 0 && count == nreco)
          {
            break; // end here.
          }
      }
    return err;
  }


  sp_err xml_renderer::render_cached_queries(const std::string &query,
      const int &nq,
      xmlNodePtr parent)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr query_node;
    int i = 0;
    std::list<std::string> suggs;
    std::vector<sweepable*>::const_iterator sit = seeks_proxy::_memory_dust.begin();
    while(sit!=seeks_proxy::_memory_dust.end() && i++<nq)
      {
        query_context *qc = dynamic_cast<query_context*>((*sit));
        if (!qc)
          {
            ++sit;
            continue;
          }
        mutex_lock(&qc->_qc_mutex);
        if (qc->_query != query)
          {
            std::string escaped_query = qc->_query;
            miscutil::replace_in_string(escaped_query,"\"","\\\"");
            miscutil::replace_in_string(escaped_query,"\t","");
            miscutil::replace_in_string(escaped_query,"\r","");
            miscutil::replace_in_string(escaped_query,"\n","");
            query_node=xmlNewNode(NULL,BAD_CAST "query");
            xmlAddChild(parent,query_node);
            xmlSetProp(query_node,BAD_CAST "value",BAD_CAST (escaped_query.c_str()));
          }
        mutex_unlock(&qc->_qc_mutex);
        ++sit;
      }
    return err;
  }

  sp_err xml_renderer::render_img_engines(const query_context *qc,
                                          xmlNodePtr parent)
  {
    sp_err err=SP_ERR_OK;
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    const img_query_context *iqc = static_cast<const img_query_context*>(qc);
    feeds engines = iqc->_img_engines;
    err=xml_renderer::render_engines(engines, false, parent);
#endif
    return err;
  }


  /*sp_err xml_renderer::render_snippet(search_snippet *sp,
  			      const bool &thumbs,
  			      const std::vector<std::string> &query_words,
  			      xmlNodePtr parent)
  {
    sp_err err=SP_ERR_OK;
    xmlSetProp(parent,BAD_CAST "id",        BAD_CAST (miscutil::to_string(sp->_id).c_str()));
    xmlSetProp(parent,BAD_CAST "title",     BAD_CAST (sp->_title).c_str());
    xmlSetProp(parent,BAD_CAST "url",       BAD_CAST (sp->_url.c_str()));
    xmlSetProp(parent,BAD_CAST "summary",   BAD_CAST (sp->_summary).c_str());
    xmlSetProp(parent,BAD_CAST "seeks_meta", BAD_CAST (miscutil::to_string(sp->_meta_rank).c_str()));
    xmlSetProp(parent,BAD_CAST "seeks_score",BAD_CAST (miscutil::to_string(sp->_seeks_rank).c_str()));
    double rank = 0.0;
    if (sp->_engine.size() > 0)
      rank = sp->_rank / static_cast<double>(sp->_engine.size());
    xmlSetProp(parent,BAD_CAST "rank", BAD_CAST (miscutil::to_string(rank).c_str()));
    xmlSetProp(parent,BAD_CAST "cite", BAD_CAST (sp->_cite.empty()?sp->_url:sp->_cite).c_str());
    if (!sp->_cached.empty())
      {
  xmlSetProp(parent,BAD_CAST "cached",BAD_CAST (sp->_cached).c_str());
      }

  #ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    img_search_snippet *isp = NULL;
    if (sp->_doc_type == IMAGE)
      isp = dynamic_cast<img_search_snippet*>(sp);
    if (isp)
      err=xml_renderer::render_engines(sp->_engine,true,parent);
    else
  #endif
      err=xml_renderer::render_engines(sp->_engine,false, parent);

    if (thumbs)
      xmlSetProp(parent,BAD_CAST "thumb",BAD_CAST sprintf(NULL,"http://open.thumbshots.org/image.pxf?url=%s",sp->_url.c_str()));

    std::set<std::string> words;
    sp->discr_words(query_words,words);
    if (!words.empty())
      {
        std::set<std::string>::const_iterator sit = words.begin();
        while(sit!=words.end())
          {
      xmlNewTextChild(parent, NULL, BAD_CAST "word", BAD_CAST (*sit).c_str());
            ++sit;
          }
      }
    xmlSetProp(parent,BAD_CAST "type",BAD_CAST sp->get_doc_type_str().c_str());
    xmlSetProp(parent,BAD_CAST "personalized",BAD_CAST (sp->_personalized?"yes":"no"));
    if (sp->_npeers > 0)
      xmlSetProp(parent,BAD_CAST "snpeers", BAD_CAST miscutil::to_string(sp->_npeers).c_str());
    if (sp->_hits > 0)
      xmlSetProp(parent,BAD_CAST "hits", BAD_CAST miscutil::to_string(sp->_hits).c_str());
    if (!sp->_date.empty())
      xmlSetProp(parent,BAD_CAST "date", BAD_CAST (sp->_date).c_str());
    return err;
    }*/

  sp_err xml_renderer::render_snippets(const std::string &query_clean,
                                       const int &current_page,
                                       const std::vector<search_snippet*> &snippets,
                                       const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                       xmlNodePtr parent)
  {
    sp_err err=SP_ERR_OK;
    bool has_thumbs = false;
    const char *thumbs = miscutil::lookup(parameters,"thumbs");
    if (thumbs && strcmp(thumbs,"on")==0)
      has_thumbs = true;

    if (!snippets.empty())
      {
        xmlNodePtr snippet_node;
        // check whether we're rendering similarity snippets.
        bool similarity = false;
        if (snippets.at(0)->_seeks_ir > 0)
          similarity = true;

        // checks for safe snippets (for now, only used for images).
        const char* safesearch_p = miscutil::lookup(parameters,"safesearch");
        bool safesearch_off = false;
        if (safesearch_p)
          safesearch_off = strcasecmp(safesearch_p,"on") == 0 ? false : true;

        // proceed with rendering.
        const char *rpp_str = miscutil::lookup(parameters,"rpp"); // results per page.
        int rpp = websearch::_wconfig->_Nr;
        if (rpp_str)
          rpp = atoi(rpp_str);
        size_t ssize = snippets.size();
        int ccpage = current_page;
        if (ccpage <= 0)
          ccpage = 1;
        size_t snisize = std::min(ccpage*rpp,(int)snippets.size());
        size_t snistart = (ccpage-1)*rpp;
        size_t count = 0;

        for (size_t i=0; i<ssize && !err; i++)
          {
            if (snippets.at(i)->_doc_type == doc_type::REJECTED)
              continue;
            if (!snippets.at(i)->is_se_enabled(parameters))
              continue;
            if (!safesearch_off && !snippets.at(i)->_safe)
              continue;
            if (!similarity || snippets.at(i)->_seeks_ir > 0)
              {
                if (count >= snistart)
                  {
                    snippet_node=xmlNewNode(NULL,BAD_CAST "snippet");
                    xmlAddChild(parent, snippet_node);
                    err = snippets.at(i)->to_xml(has_thumbs,snippets.at(i)->_qc->_query_words, snippet_node);
                  }
                count++;
              }
            if (count == snisize)
              {
                break; // end here.
              }
          }
      }
    return err;
  }

  sp_err xml_renderer::render_clustered_snippets(const std::string &query_clean,
      hash_map<int,cluster*> *clusters,
      const short &K,
      const query_context *qc,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
      xmlNodePtr parent)

  {
    // render every cluster and snippets within.
    xmlNodePtr cluster_node;
    sp_err err=SP_ERR_OK;
    hash_map<int,cluster*>::const_iterator chit = clusters->begin();
    while(chit!=clusters->end())
      {
        if ((*chit).second->_cpoints.empty())
          {
            ++chit;
            continue;
          }

        cluster *cl = (*chit).second;
        std::vector<search_snippet*> snippets;
        snippets.reserve(cl->_cpoints.size());
        hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit
        = cl->_cpoints.begin();
        while (hit!=cl->_cpoints.end())
          {
            search_snippet *sp = qc->get_cached_snippet((*hit).first);
            snippets.push_back(sp);
            ++hit;
          }
        std::stable_sort(snippets.begin(),snippets.end(),search_snippet::max_seeks_ir);
        cluster_node=xmlNewNode(NULL,BAD_CAST "cluster");
        xmlAddChild(parent,cluster_node);
        xmlSetProp(cluster_node,BAD_CAST "label", BAD_CAST cl->_label.c_str());
        err=xml_renderer::render_snippets(query_clean,0,snippets,parameters,cluster_node);
        ++chit;
      }

    return err;
  }


  /* Public methods*/

  sp_err xml_renderer::render_xml_cached_queries(const std::string &query,
      const int &nq,
      xmlDocPtr doc)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
    =xmlNewNode(NULL,BAD_CAST "queries");
    xmlDocSetRootElement(doc, root);
    err=xml_renderer::render_cached_queries(query,nq,root);
    return err;
  }

  sp_err xml_renderer::render_xml_clustered_results(const query_context *qc,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      hash_map<int,cluster*> *clusters,
      const short &K,
      const double &qtime,
      xmlDocPtr doc)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
    =xmlNewNode(NULL,BAD_CAST "clustered_results");
    xmlDocSetRootElement(doc, root);
    std::string query = qc->_query;

    // search clustered snippets.
    err = collect_xml_results(parameters,qc,qtime, false,root);
    err = err || xml_renderer::render_clustered_snippets(query,clusters,K,qc,parameters,root);
    return err;
  }


  sp_err xml_renderer::render_xml_engines(const feeds &engines,
                                          xmlDocPtr doc)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
    =xmlNewNode(NULL,BAD_CAST "engines");
    xmlDocSetRootElement(doc, root);
    err=xml_renderer::render_engines(engines,false,root);
    return err;
  }


  sp_err xml_renderer::render_xml_node_options(client_state *csp,
      xmlDocPtr doc)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
    =xmlNewNode(NULL,BAD_CAST "options");
    xmlDocSetRootElement(doc, root);
    err = xml_renderer::render_node_options(csp,root);
    return err;
  }


  sp_err xml_renderer::render_xml_peers(std::list<std::string> *peers,
                                        xmlDocPtr doc)
  {
    std::string peer;
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
    =xmlNewNode(NULL,BAD_CAST "peers");
    xmlDocSetRootElement(doc,root);
    std::list<std::string>::iterator lit = peers->begin();
    while (lit!=peers->end())
      {
        xmlNewTextChild(root, NULL, BAD_CAST "peer", BAD_CAST (*lit).c_str());
        ++lit;
      }
    return err;
  }


  sp_err xml_renderer::render_xml_recommendations(const query_context *qc,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
      const double &qtime,
      const int &radius,
      const std::string &lang,
      xmlDocPtr doc)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
    =xmlNewNode(NULL,BAD_CAST "recommendations");
    xmlDocSetRootElement(doc,root);
    int nreco = -1;
    const char *nreco_str = miscutil::lookup(parameters,"nreco");
    if (nreco_str)
      nreco = atoi(nreco_str);
    err = xml_renderer::render_recommendations(qc,nreco,qtime,radius,lang,root);
    return err;
  }


  sp_err xml_renderer::render_xml_results(const query_context *qc,
                                          const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                          const std::vector<search_snippet*> &snippets,
                                          const double &qtime,
                                          const bool &img,
                                          xmlDocPtr doc)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
    =xmlNewNode(NULL,BAD_CAST "result");
    xmlDocSetRootElement(doc, root);
    const char *current_page_str = miscutil::lookup(parameters,"page");
    if (!current_page_str)
      {
        //XXX: no page argument, we default to no page.
        current_page_str = "0";
      }
    int current_page = atoi(current_page_str);

    std::string query = qc->_query;

    // search snippets.
    xmlNodePtr snippets_node=xmlNewNode(NULL,BAD_CAST "snippets");
    xmlAddChild(root,snippets_node);
    err=xml_renderer::render_snippets(query,current_page,snippets,parameters,snippets_node);
    collect_xml_results(parameters,qc,qtime,img, root);
    return err;
  }

  sp_err xml_renderer::render_xml_snippet(query_context *qc,
                                          search_snippet *sp,
                                          xmlDocPtr doc)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
    =xmlNewNode(NULL,BAD_CAST "snippet");
    xmlDocSetRootElement(doc, root);
    std::string query = qc->_query;
    err = sp->to_xml(false,qc->_query_words,root);
    return err;
  }

  sp_err xml_renderer::render_xml_suggested_queries(const query_context *qc,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
      xmlDocPtr doc)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
    =xmlNewNode(NULL,BAD_CAST "suggested_queries");
    xmlDocSetRootElement(doc,root);
    int nsuggs = websearch::_wconfig->_num_reco_queries;
    const char *nsugg_str = miscutil::lookup(parameters,"nsugg");
    if (nsugg_str)
      nsuggs = atoi(nsugg_str);
    err=xml_renderer::render_suggested_queries(qc,nsuggs,root);
    return err;
  }


  sp_err xml_renderer::render_xml_words(const std::set<std::string> &words,
                                        xmlDocPtr doc)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
    =xmlNewNode(NULL,BAD_CAST "words");
    xmlDocSetRootElement(doc, root);
    std::set<std::string>::const_iterator sit = words.begin();
    while(sit!=words.end())
      {
        xmlNewTextChild(root, NULL,BAD_CAST "word",BAD_CAST ((*sit).c_str()));
        ++sit;
      }
    return err;
  }


}; /* end of namespace seeks_plugins */


using namespace seeks_plugins;

namespace xml_renderer_private
{


  sp_err collect_xml_results(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                             const query_context *qc,
                             const double &qtime,
                             const bool &img,
                             xmlNodePtr parent)
  {
    xmlNodePtr context
    =xmlNewNode(NULL,BAD_CAST "query_context");
    xmlAddChild(parent,context);

    /*- query info. -*/
    // query.

    xmlNewProp(context,BAD_CAST "query",BAD_CAST (qc->_query).c_str());

    // language.
    xmlNewProp(context,BAD_CAST "lang",BAD_CAST (qc->_auto_lang).c_str());

    // personalization.
    const char *prs = miscutil::lookup(parameters,"prs");
    if (!prs || (miscutil::strcmpic(prs,"on")!=0 && miscutil::strcmpic(prs,"off")!=0))
      prs = websearch::_wconfig->_personalization ? "on" : "off";
    xmlNewProp(context,BAD_CAST "pers",BAD_CAST prs);

    // expansion.
    xmlNewProp(context,BAD_CAST "expansion",BAD_CAST miscutil::to_string(qc->_page_expansion).c_str());

    // peers.
    xmlNewProp(context,BAD_CAST "npeers",BAD_CAST miscutil::to_string(qc->_npeers).c_str());

    // suggested queries.
    xml_renderer::render_suggested_queries(qc,websearch::_wconfig->_num_reco_queries, context);

    // engines.
    if (qc->_engines.size() > 0)
      {
        if (img)
          xml_renderer::render_img_engines(qc, context);
        else
          xml_renderer::render_engines(qc->_engines, false, context);
      }

    // render date & exec time.
    char datebuf[256];
    cgi::get_http_time(0,datebuf,sizeof(datebuf));

    xmlNewProp(context,BAD_CAST "date",BAD_CAST datebuf);
    xmlNewProp(context,BAD_CAST "qtime",BAD_CAST miscutil::to_string(qtime).c_str());
    return SP_ERR_OK;
  }

} /* end of namespace xml_renderer_private */
