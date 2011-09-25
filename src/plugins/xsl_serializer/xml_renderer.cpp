/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010, 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
 * Copyright (C) 2010 Loic Dachary <loic@dachary.org>
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

#include "xsl_renderer.h"
#include "cgisimple.h"
#include "miscutil.h"
#include "cgi.h"
#include "xsl_renderer_private.h"
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
#include "img_query_context.h"
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

using namespace xml_serializer_private;

namespace seeks_plugins
{
  sp_err xml_renderer::render_engines(const feeds &engines,xmlNodePtr parent)
  {
    hash_map<const char*,feed_url_options,hash<const char*>,eqstr>::const_iterator hit;
    xmlNodePtr eng=NULL;
    std::set<feed_parser,feed_parser::lxn>::const_iterator it
    = engines._feedset.begin();
    while(it!=engines._feedset.end())
      {	
        std::set<std::string>::const_iterator sit = (*it)._urls.begin();
        while(sit!=(*it)._urls.end())
          {
            if ((hit = websearch::_wconfig->_se_options.find((*sit).c_str()))
                != websearch::_wconfig->_se_options.end())
	      {
		eng=xmlNewNode(NULL,"engine");
		xmlNewProp(eng,"value",(*hit).second._id);
		xmlAddChild(parent,eng);
	      }
	    ++sit;
          }
        ++it;
      }
    return SP_ERR_OK;
  }

  sp_err xml_renderer::render_node_options(client_state *csp,
      xmlNodePtr parent)
  {
    // system options.
    hash_map<const char*, const char*, hash<const char*>, eqstr> *exports
    = cgi::default_exports(csp,"");
    const char *value = miscutil::lookup(exports,"version");
    if (value)
      xmlNewProp(parent,"version",value);
    if (websearch::_wconfig->_show_node_ip)
      {
        value = miscutil::lookup(exports,"my-ip-address");
        if (value)
          {
	    xmlNewProp(parent,"my-ip-adress",value);
          }
      }
    value = miscutil::lookup(exports,"code-status");
    if (value)
      {
	xmlNewProp(parent,"code-status",value);
      }
    value = miscutil::lookup(exports,"admin-address");
    if (value)
      {
	xmlNewProp(parent,"admin-address",value);
      }
    xmlNewProp(parent,"url-source-code",csp->_config->_url_source_code);

    miscutil::free_map(exports);

    /*- websearch options. -*/
    // thumbs.
    
    std::string thumbs;
    xmlNewProp(parent,"thumbs",
	       websearch::_wconfig->_thumbs ? "on" :"off");
    xmlNewProp(parent,"content-analysis",
	       websearch::_wconfig->_content_analysis ? "on" : "off");
    xmlNewProp(parent,"clustering",
	       websearch::_wconfig->_clustering ? "on" : "off");

    /* feeds options */
    std::list<std::string> se_options;
    hash_map<const char*,feed_url_options,hash<const char*>,eqstr>::const_iterator hit;
    std::set<feed_parser,feed_parser::lxn>::const_iterator fit
    = websearch::_wconfig->_se_enabled._feedset.begin();
    while(fit!=websearch::_wconfig->_se_enabled._feedset.end())
      {
	feed=xmlNewNode(NULL,"feed");
	xmlNewProp(feed,"name",(*fit)._name);
        std::list<std::string> se_urls;
        std::set<std::string>::const_iterator sit
        = (*fit)._urls.begin();
        while(sit!=(*fit)._urls.end())
          {
	    feed_url=xmlNewNode(NULL,"url");
            std::string url = (*sit);
            if ((hit=websearch::_wconfig->_se_options.find(url.c_str()))
                !=websearch::_wconfig->_se_options.end())
              {
		xmlNewProp(feedurl,"value",(*hit).second._id);
		if ((*hit).second._default)
		  xmlNewProp(feed,"default","yes");
              }
	    xmlAddChild(feed,feed_url);
            ++sit;
          }
	xmlAddChild(parent,feed);
        ++fit;
      }
    
    return SP_ERR_OK;
  }

  sp_err xml_renderer::render_suggested_queries(const query_context *qc,
						     const int &nsuggs,
						     xmlNodePtr parent)
  {
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
	    node=xmlNewNode(NULL, "suggested_query");
	    xmlNewProp(node, "value", sugg_str);
	    xmlAddChild(parent,sugg);
            if (k >= nsuggs-1)
              break;
            ++k;
            ++mit;
          }
	return SP_ERR_OK;
      }
    return SP_ERR_OK;
  }

  /* entry point method */
  sp_err xml_renderer::render_xml_suggested_queries(const query_context *qc,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    xmlNodePtr root
      =xmlNewNode(NULL,"suggested_queries");
    int nsuggs = websearch::_wconfig->_num_reco_queries;
    const char *nsugg_str = miscutil::lookup(parameters,"nsugg");
    if (nsugg_str)
      nsuggs = atoi(nsugg_str);
    err=xml_renderer::render_suggested_queries(qc,nssuggs,root);
    xsl_response(rsp,root);
    return err;
  }

  sp_err xml_renderer::render_recommendations(const query_context *qc,
						   const int &nreco,
						   const double &qtime,
						   const uint32_t &radius,
						   const std::string &lang,
						   xmlNodePtr parent
						   )
  {
    sp_err err=SP_ERR_OK;

    std::vector<std::string> query_words;
    miscutil::tokenize(qc->_query,query_words," "); // allows to extract most discriminative words not in query.

    std::list<std::string> results;

    // query.
    std::string escaped_query = qc->_query;
    miscutil::replace_in_string(escaped_query,"\"","\\\"");
    xmlNewProp(parent,"query",escaped_query);

    // peers.
    xmlNewProp(parent,"npeers",miscutil::to_string(qc->_npeers));

    // date.
    char datebuf[256];
    cgi::get_http_time(0,datebuf,sizeof(datebuf));
    xmlNewProp(parent,"date",std::string(datebuf));

    // processing time.
    xmlNewProp(parent,"qtime",miscutil::to_string(qtime));

    //snippets.
    
    json_str += ",\"snippets\":[";
    size_t ssize = qc->_cached_snippets.size();
    int count = 0;
    for (size_t i=0; i<ssize && !err; i++)
      {
        search_snippet *sp = qc->_cached_snippets.at(i);
        if (sp->_radius > radius)
          continue;
        if (!lang.empty() && sp->_lang != lang)
          continue;
        if (sp->_doc_type == REJECTED)
          continue;
        if (!sp->_engine.has_feed("seeks"))
          continue;
	snippet=xmlNewNode(NULL,"snippet");

        err = xml_renderer::render_snippet(sp,false,query_words,snippet);
        count++;

        if (nreco > 0 && count == nreco)
          {
            break; // end here.
          }
      }
    return err;
  }

  sp_err xml_renderer::render_xml_recommendations(const query_context *qc,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
      const double &qtime,
      const int &radius,
      const std::string &lang)
  {
    xmlNodePtr root
      =xmlNewNode(NULL,"recommendations");
    sp_err err;

    int nreco = -1;
    const char *nreco_str = miscutil::lookup(parameters,"nreco");
    if (nreco_str)
      nreco = atoi(nreco_str);
    
    err = xml_renderer::render_recommendations(qc,nreco,qtime,radius,lang,root);
    xsl_response(rsp,root);
    return err;
  }

  sp_err xml_renderer::render_cached_queries(const std::string &query,
						  const int &nq,
						  xmlNodePtr parent)
  {
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
	    query=xmlNewNode(NULL,"query");
	    xmlSetProp(query,"value",escaped_query);
	    xmlAddChild(parent,query)
          }
        mutex_unlock(&qc->_qc_mutex);
        ++sit;
      }
    return SP_ERR_OK;
  }

  sp_err xml_renderer::render_img_engines(const query_context *qc
						xmlNodePtr parent)
  {
    sp_err err=SP_ERR_OK;
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    const img_query_context *iqc = static_cast<const img_query_context*>(qc);
    feeds engines = iqc->_img_engines;
    err=xml_renderer::render_engines(engines, parent);
#endif
    return err
  }


  sp_err xml_renderer::render_snippet(const search_snippet *sp,
				   const bool &thumbs,
				   const std::vector<std::string> &query_words
				   xmlNodePtr parent
				   )
  {
    xmlAddProp(parent,"id",miscutil::to_string(sp->_id));
    xmlAddProp(parent,"title",sp->_title.c_str());
    xmlAddProp(parent,"url",sp->_url);
    xmlAddProp(parent,"summary",sp->_summary);
    xmlAddProp(parent,"seeks_meta", miscutil::to_string(sp->_meta_rank));
    xmlAddProp(parent,"seeks_score",miscutil::to_string(sp->_seeks_rank));
    double rank = 0.0;
    if (sp->_engine.size() > 0)
      rank = sp->_rank / static_cast<double>(sp->_engine.size());
    xmlAddProp(parent,"rank",miscutil::to_string(rank));
    xmlAddProp(parent,"cite",sp->_cite.empty()?sp->_url:sp->_cite);
    if (!sp->_cached.empty())
      {
	xmlAddProp(parent,"cached",sp->_cached);
      }
    /*if (_archive.empty())
      set_archive_link();
      xmlAddProp(parent,"archive",archive);*/
    
    err=xml_renderer::render_engines(sp->_engine,parent);
    if (thumbs)
      	xmlAddProp(parent,"thumb","http://open.thumbshots.org/image.pxf?url=" + url);
    std::set<std::string> words;
    sp->discr_words(query_words,words);
    if (!words.empty())
      {
	
        json_str += ",\"words\":[";
        //json_str += miscutil::join_string_list(",",words);
        std::set<std::string>::const_iterator sit = words.begin();
        while(sit!=words.end())
          {
	    word=xmlNewTextChild(NULL,"word",parent,(*sit));
            ++sit;
          }
      }
    xmlAddProp(parent,"type",sp->get_doc_type_str());
    type_str() + "\"";

    xmlAddProp(parent,"personalized",sp->_personalized?"yes":"no");
    if (sp->_npeers > 0)
      xmlAddProp(parent,"snpeers", miscutil::to_string(sp->_npeers));
    if (sp->_hits > 0)
      xmlAddProp(parent,"hits", miscutil::to_string(sp->_hits));
    if (!sp->_date.empty())
      xmlAddProp(parent,"date", sp->_date);
    return SP_ERR_OK;
  }
  
  sp_err xml_renderer::render_snippets(const std::string &query_clean,
                                        const int &current_page,
                                        const std::vector<search_snippet*> &snippets,
                                        std::string &json_str,
                                        const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters
					xmlNodePtr parent)
  {
    sp_err err;
    bool has_thumbs = false;
    const char *thumbs = miscutil::lookup(parameters,"thumbs");
    if (thumbs && strcmp(thumbs,"on")==0)
      has_thumbs = true;

    if (!snippets.empty())
      {
        // check whether we're rendering similarity snippets.
        bool similarity = false;
        if (snippets.at(0)->_seeks_ir > 0)
          similarity = true;

        // grab query words.
        std::vector<std::string> query_words;
        miscutil::tokenize(query_clean,query_words," "); // allows to extract most discriminative words not in query.

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
            if (snippets.at(i)->_doc_type == REJECTED)
              continue;
            if (!snippets.at(i)->is_se_enabled(parameters))
              continue;
            if (!safesearch_off && !snippets.at(i)->_safe)
              continue;
	    snippet=xmlNewNode(NULL,"snippet")
            if (!similarity || snippets.at(i)->_seeks_ir > 0)
              {
                if (count >= snistart)
                  {
                    if (count > snistart && count<snisize)
                      json_str += ",";
                    err = xml_renderer::render_snippet(snippets.at(i),has_thumbs,query_words, snippet);
                  }
                count++;
              }
	    xmlAddChild(parent, snippet);
            if (count == snisize)
              {
                break; // end here.
              }
          }
      }
    return err;
  }

  sp_err xml_renderer::render_clustered_snippets(const std::string &query_clean,
						 cluster *clusters, const short &K,
						 const query_context *qc,
						 std::string &json_str,
						 const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
						 xmlNodePtr parent)
    
  {
    // render every cluster and snippets within.
    bool has_cluster = false;
    for (int c=0; c<K; c++)
      {
        if (clusters[c]._cpoints.empty())
          continue;

        if (has_cluster)
          json_str += ",";
        has_cluster = true;

        std::vector<search_snippet*> snippets;
        snippets.reserve(clusters[c]._cpoints.size());
        hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit
        = clusters[c]._cpoints.begin();
        while (hit!=clusters[c]._cpoints.end())
          {
            search_snippet *sp = qc->get_cached_snippet((*hit).first);
            snippets.push_back(sp);
            ++hit;
          }
        std::stable_sort(snippets.begin(),snippets.end(),search_snippet::max_seeks_ir);
	cluster=xmlNewNode(NULL,"cluster");
        xmlSetProp(cluster,"label", clusters[c]._label);
        xml_renderer::render_snippets(query_clean,0,snippets,json_str,parameters,cluster);
      }

    return SP_ERR_OK;
  }

  sp_err xml_renderer::render_xml_results(const std::vector<search_snippet*> &snippets,
      client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      const query_context *qc,
      const double &qtime,
      const bool &img)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
      =xmlNewNode(NULL,"result");
    const char *current_page_str = miscutil::lookup(parameters,"page");
    if (!current_page_str)
      {
        //XXX: no page argument, we default to no page.
        current_page_str = "0";
      }
    int current_page = atoi(current_page_str);

    std::string query = qc->_query;

    // search snippets.
    snippets_node=xmlNewNode(NULL,"snippets");
    std::string json_snippets;
    err=xml_renderer::render_snippets(query,current_page,snippets,json_snippets,parameters,snippets_node);
    xmlAddChild(root,snippets_node);
    collect_xml_results(parameters,qc,qtime,img, root);
    xsl_response(rsp, root);
    return err;
  }

  sp_err xml_renderer::render_xml_snippet(const search_snippet *sp,
      http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      query_context *qc)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
      =new XmlNodePtr(NULL,"snippet");
    std::string query = qc->_query;
    // grab query words.
    std::vector<std::string> query_words;
    miscutil::tokenize(query,query_words," "); // allows to extract most discriminative words not in query.
    
    err = json_renderer::render_snippet(sp,false,query_words,root);
    xsl_response(rsp,root);
    return err;
  }

  sp_err xml_renderer::render_xml_words(const std::set<std::string> &words,
                                          http_response *rsp,
                                          const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root
      =new XmlNodePtr(NULL,"words");
    
    std::set<std::string>::const_iterator sit = words.begin();
    while(sit!=words.end())
      {
	xmlNewTextChild(NULL,"word",root,(*sit));
        ++sit;
      }
    xsl_response(rsp,root);
    return err;
  }

  sp_err xml_renderer::render_xml_node_options(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root;
    =xmlNewNode(NULL,"options");
    sp_err err = xml_renderer::render_node_options(csp,root);
    xsl_response(rsp,root);
    return err;
  }

  sp_err xml_renderer::render_xml_cached_queries(http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      const std::string &query,
      const int &nq)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root;
    =xmlNewNode(NULL,"cached_queries");
    
    err=xml_renderer::render_cached_queries(query,nq,root);
    xsl_response(rsp,root);
    return err;
  }

  sp_err xml_renderer::render_xml_clustered_results(cluster *clusters,
      const short &K,
      client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      const query_context *qc,
      const double &qtime)
  {
    sp_err err=SP_ERR_OK;
    xmlNodePtr root;
    =xmlNewNode(NULL,"clustered_results");

    std::string query = qc->_query;

    // search clustered snippets.
    std::string json_snippets;
    err = collect_xml_results(results,parameters,qc,qtime,root);
    err = err || xml_renderer::render_clustered_snippets(query,clusters,K,qc,json_snippets,parameters,root);
    
    xsl_response(rsp, root);

    return err;
  }
}; /* end of namespace seeks_plugins */


using namespace seeks_plugins;

namespace xml_renderer_private
{

  /* move to xsl_serializer */
  /*
    void xsl_response(http_response *rsp, const std::string& json_str)
  {
    rsp->_body = strdup(json_str.c_str());
    rsp->_content_length = json_str.size();
    miscutil::enlist(&rsp->_headers, "Content-Type: application/json");
    rsp->_is_static = 1;
  }
  */
  sp_err collect_xml_results(std::list<std::string> &results,
			     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
			     const query_context *qc,
			     const double &qtime,
			     const bool &img,
			     xmlNodePtr parent)
  {
    xmlNodePtr context
      =xmlNewNode(NULL,"query_context");
    /*- query info. -*/
    // query.

    xmlNewProp(context,"query",qc->_query);

    // language.
    xmlNewProp(context,"lang",qc->_auto_lang);

    // personalization.
    const char *prs = miscutil::lookup(parameters,"prs");
    if (!prs || (miscutil::strcmpic(prs,"on")!=0 && miscutil::strcmpic(prs,"off")!=0))
      prs = websearch::_wconfig->_personalization ? "on" : "off";
    xmlNewProp(context,"pers",std::string(prs));

    // expansion.
    xmlNewProp(context,"expansion",miscutil::to_string(qc->_page_expansion));

    // peers.
    xmlNewProp(context,"npeers",miscutil::to_string(qc->_npeers));

    // suggested queries.
    xml_renderer::render_suggested_queries(qc,websearch::_wconfig->_num_reco_queries, context);

    // engines.
    if (qc->_engines.size() > 0)
      {
	if (img) 
	  xml_renderer::render_img_engines(qc, context);
	else
	  xml_renderer::render_engines(qc->_engines, context);
      }

    // render date & exec time.
    char datebuf[256];
    cgi::get_http_time(0,datebuf,sizeof(datebuf));

    xmlNewProp(context,"date",std::string(datebuf));
    xmlNewProp(context,"qtime",miscutil::to_string(qtime));
    return SP_ERR_OK;
  }

} /* end of namespace json_renderer_private */
