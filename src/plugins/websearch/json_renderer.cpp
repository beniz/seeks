/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 - 2012 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#include "json_renderer.h"
#include "cgisimple.h"
#include "miscutil.h"
#include "cgi.h"
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

namespace seeks_plugins
{
  Json::Value json_renderer::render_engines(const feeds &engines,
					    const bool &img)
  {
    Json::Value res;
    hash_map<const char*,feed_url_options,hash<const char*>,eqstr>::const_iterator hit;
    std::list<std::string> engs;
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
		  res.append((*hit).second._id);
              }
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
            else
              {
                if ((hit = img_websearch::_iwconfig->_se_options.find((*sit).c_str()))
                    != img_websearch::_iwconfig->_se_options.end())
		  res.append((*hit).second._id);
	      }
#endif
            ++sit;
          }
        ++it;
      }
    return res;
  }

  Json::Value json_renderer::render_tags(const hash_map<const char*,float,hash<const char*>,eqstr> &tags)
  {
    std::multimap<float,std::string,std::greater<float> > otags;
    hash_map<const char*,float,hash<const char*>,eqstr>::const_iterator tit
      = tags.begin();
    while(tit!=tags.end())
      {
	otags.insert(std::pair<float,std::string>((*tit).second,std::string((*tit).first)));
	++tit;
      }
    return json_renderer::render_tags(otags);
  }
  
  Json::Value json_renderer::render_tags(const std::multimap<float,std::string,std::greater<float> > &tags)
  {
    Json::Value root,jtags;
    std::multimap<float,std::string,std::greater<float> >::const_iterator mit
      = tags.begin();
    while(mit!=tags.end())
      {
	root["tags"][(*mit).second]["weight"] = (*mit).first;
	++mit;
      }
    return root;
  }

  Json::Value json_renderer::render_node_options(client_state *csp)
  {
    Json::Value res;
    
    // system options.
    hash_map<const char*, const char*, hash<const char*>, eqstr> *exports
    = cgi::default_exports(csp,"");
    const char *value = miscutil::lookup(exports,"version");
    if (value)
      {
	Json::Value jv;
	jv["version"] = value;
	res.append(jv);
      }
    if (websearch::_wconfig->_show_node_ip)
      {
        value = miscutil::lookup(exports,"my-ip-address");
        if (value)
          {
	    Json::Value jip;
	    jip["my-ip-address"] = value;
	    res.append(jip);
          }
      }
    value = miscutil::lookup(exports,"code-status");
    if (value)
      {
	Json::Value jcs;
	jcs["code-status"] = value;
        res.append(jcs);
      }
    value = miscutil::lookup(exports,"admin-address");
    if (value)
      {
	Json::Value jaa;
	jaa["admin-address"] = value;
        res.append(jaa);
      }
    Json::Value jusc;
    jusc["url-source-code"] = csp->_config->_url_source_code;
    res.append(jusc);

    miscutil::free_map(exports);

    /*- websearch options. -*/
    // thumbs.
    Json::Value jth;
    jth["thumbs"] = websearch::_wconfig->_thumbs ? "on" : "off";
    res.append(jth);
    
    Json::Value jca;
    jca["content-analysis"] = websearch::_wconfig->_content_analysis ? "on" : "off";
    res.append(jca);
    
    Json::Value jcl;
    jcl["clustering"] = websearch::_wconfig->_clustering ? "on" : "off";
    res.append(jcl);

    /* feeds options */
    Json::Value jopts;
    std::list<std::string> se_options;
    hash_map<const char*,feed_url_options,hash<const char*>,eqstr>::const_iterator hit;
    std::set<feed_parser,feed_parser::lxn>::const_iterator fit
      = websearch::_wconfig->_se_enabled._feedset.begin();
    while(fit!=websearch::_wconfig->_se_enabled._feedset.end())
      {
        std::string fp_name = (*fit)._name;
	Json::Value jfpname;
        std::list<std::string> se_urls;
        std::set<std::string>::const_iterator sit
	  = (*fit)._urls.begin();
        while(sit!=(*fit)._urls.end())
          {
            std::string url = (*sit);
            if ((hit=websearch::_wconfig->_se_options.find(url.c_str()))
                !=websearch::_wconfig->_se_options.end())
              {
		Json::Value jdef;
		jdef["default"] = (*hit).second._default ? "true" : "false";
		Json::Value jopt;
		jopt[(*hit).second._id] = jdef;
		jfpname.append(jopt);
	      }
            ++sit;
          }
        jopts.append(jfpname);
	++fit;
      }
    Json::Value jparsers;
    jparsers["txt-parsers"] = jopts;
    res.append(jparsers);
    return res;
  }

  Json::Value json_renderer::render_suggested_queries(const query_context *qc,
						      const int &nsuggs)
  {
    Json::Value jqsuggs;
    if (!qc->_suggestions.empty())
      {
        int k = 0;
        std::multimap<double,std::string,std::less<double> >::const_iterator mit
        = qc->_suggestions.begin();
        while(mit!=qc->_suggestions.end())
          {
	    jqsuggs.append((*mit).second);
            if (k >= nsuggs-1)
              break;
            ++k;
            ++mit;
          }
      }
    return jqsuggs;
  }

  void json_renderer::render_json_suggested_queries(const query_context *qc,
						    http_response *rsp,
						    const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    int nsuggs = websearch::_wconfig->_num_reco_queries;
    const char *nsugg_str = miscutil::lookup(parameters,"nsugg");
    if (nsugg_str)
      nsuggs = atoi(nsugg_str);
    Json::FastWriter writer;
    Json::Value jq;
    jq["suggestions"] = json_renderer::render_suggested_queries(qc,nsuggs);
    std::string json_str = writer.write(jq);
    const std::string body = jsonp(json_str, miscutil::lookup(parameters,"callback"));
    response(rsp,body);
  }

  Json::Value json_renderer::render_recommendations(const query_context *qc,
						    const int &nreco,
						    const double &qtime,
						    const uint32_t &radius,
						    const std::string &lang)
  {
    Json::Value jres;
    
    // query.
    jres["query"] = qc->_query;
    
    // peers.
    jres["npeers"] = qc->_npeers;
    
    // date.
    char datebuf[256];
    cgi::get_http_time(0,datebuf,sizeof(datebuf));
    jres["date"] = datebuf;

    // processing time.
    jres["qtime"] = qtime;

    //snippets.
    Json::Value jsnippets;
    size_t ssize = qc->_cached_snippets.size();
    int count = 0;
    for (size_t i=0; i<ssize; i++)
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
        jsnippets.append(sp->to_json(false,qc->_query_words));
        count++;

        if (nreco > 0 && count == nreco)
          {
            break; // end here.
          }
      }
    jres["snippets"] = jsnippets;
    return jres;
  }

  void json_renderer::render_json_recommendations(const query_context *qc,
						  http_response *rsp,
						  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
						  const double &qtime,
						  const int &radius,
						  const std::string &lang)
  {
    int nreco = -1;
    const char *nreco_str = miscutil::lookup(parameters,"nreco");
    if (nreco_str)
      nreco = atoi(nreco_str);
    Json::FastWriter writer;
    std::string json_str = writer.write(json_renderer::render_recommendations(qc,nreco,qtime,radius,lang));
    const std::string body = jsonp(json_str, miscutil::lookup(parameters,"callback"));
    response(rsp,body);
  }

  Json::Value json_renderer::render_cached_queries(const std::string &query,
						   const int &nq)
  {
    Json::Value jsuggs,jqsuggs;
    int i = 0;
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
	    jqsuggs.append(qc->_query);
          }
        mutex_unlock(&qc->_qc_mutex);
        ++sit;
      }
    jsuggs["queries"] = jqsuggs;
    return jsuggs;
  }

  Json::Value json_renderer::render_img_engines(const query_context *qc)
  {
    Json::Value jres;
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    const img_query_context *iqc = static_cast<const img_query_context*>(qc);
    feeds engines = iqc->_img_engines;
    jres = render_engines(engines);
#endif
    return jres;
  }
  
  Json::Value json_renderer::render_snippets(const std::string &query_clean,
					     const int &current_page,
					     const std::vector<search_snippet*> &snippets,
					     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    Json::Value jres,jsnippets;
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

        for (size_t i=0; i<ssize; i++)
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
                    jsnippets.append(snippets.at(i)->to_json(has_thumbs,
							     snippets.at(i)->_qc->_query_words));
                  }
                count++;
              }
            if (count == snisize)
              {
                break; // end here.
              }
          }
      }
    jres["snippets"] = jsnippets;
    return jres;
  }

  Json::Value json_renderer::render_clustered_snippets(const std::string &query_clean,
						       hash_map<int,cluster*> *clusters, const short &K,
						       const query_context *qc,
						       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    Json::Value jres,jfcls;
    
    // render every cluster and snippets within.
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
        std::stable_sort(snippets.begin(),snippets.end(),search_snippet::max_seeks_rank);
	
	Json::Value jcl,jcls;
	jcl["label"] = cl->_label;
	jcls.append(jcl);
	jcls.append(json_renderer::render_snippets(query_clean,0,snippets,parameters));
	++chit;
	jfcls.append(jcls);
      }
    jres["clusters"] = jfcls;
    return jres;
  }
  
  void json_renderer::render_json_results(const std::vector<search_snippet*> &snippets,
					  client_state *csp, http_response *rsp,
					  const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					  const query_context *qc,
					  const double &qtime,
					  const bool &img)
  {
    const char *current_page_str = miscutil::lookup(parameters,"page");
    if (!current_page_str)
      {
        //XXX: no page argument, we default to no page.
        current_page_str = "0";
      }
    int current_page = atoi(current_page_str);

    std::string query = qc->_query;

    // search snippets.
    Json::Value jsnippets = json_renderer::render_snippets(query,current_page,snippets,parameters);

    collect_json_results(jsnippets,parameters,qc,qtime,img);
    Json::FastWriter writer;
    const std::string body = jsonp(writer.write(jsnippets),miscutil::lookup(parameters,"callback"));
    response(rsp, body);
  }

  void json_renderer::render_json_snippet(search_snippet *sp,
					  http_response *rsp,
					  const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					  query_context *qc)
  {
    std::string query = qc->_query;
    Json::Value js = sp->to_json(false,qc->_query_words);
    Json::FastWriter writer;
    const std::string body = jsonp(writer.write(js),miscutil::lookup(parameters,"callback"));
    response(rsp,body);
  }

  void json_renderer::render_json_words(const std::set<std::string> &words,
					http_response *rsp,
					const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    Json::Value jw;
    std::set<std::string> qwords;
    std::set<std::string>::const_iterator sit = words.begin();
    while(sit!=words.end())
      {
	jw.append((*sit));
        ++sit;
      }
    Json::Value jws;
    jws["words"] = jw;
    Json::FastWriter writer;
    const std::string body = jsonp(writer.write(jws),
                                   miscutil::lookup(parameters,"callback"));
    response(rsp,body);
  }

  void json_renderer::render_json_node_options(client_state *csp, http_response *rsp,
					       const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    Json::FastWriter writer;
    std::string json_str = writer.write(json_renderer::render_node_options(csp));
    const std::string body = jsonp(json_str, miscutil::lookup(parameters,"callback"));
    response(rsp,body);
  }

  void json_renderer::render_cached_queries(http_response *rsp,
					    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					    const std::string &query,
					    const int &nq)
  {
    Json::FastWriter writer;
    std::string json_str = writer.write(json_renderer::render_cached_queries(query,nq));
    const std::string body = jsonp(json_str, miscutil::lookup(parameters,"callback"));
    response(rsp,body);
  }

  void json_renderer::render_clustered_json_results(hash_map<int,cluster*> *clusters,
						    const short &K,
						    client_state *csp, http_response *rsp,
						    const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
						    const query_context *qc,
						    const double &qtime)
  {
    std::string query = qc->_query;

    // search clustered snippets.
    Json::Value jsnippets = json_renderer::render_clustered_snippets(query,clusters,K,qc,parameters);

    collect_json_results(jsnippets,parameters,qc,qtime);
    Json::FastWriter writer;
    const std::string results_string = writer.write(jsnippets);
    const std::string body = jsonp(results_string, miscutil::lookup(parameters,"callback"));
    response(rsp, body);
  }
  
  std::string json_renderer::jsonp(const std::string& input, const char* callback)
  {
    std::string output;
    if (callback == NULL)
      return input;
    else
      return std::string(callback) + "(" + input + ")";
  }

  void json_renderer::response(http_response *rsp, const std::string& json_str)
  {
    rsp->_body = strdup(json_str.c_str());
    rsp->_content_length = json_str.size();
    miscutil::enlist(&rsp->_headers, "Content-Type: application/json");
    rsp->_is_static = 1;
  }

  void json_renderer::collect_json_results(Json::Value &results,
					   const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					   const query_context *qc,
					   const double &qtime,
					   const bool &img)
  {
    // query.
    results["query"] = qc->_query;
    
    // language.
    results["lang"] = qc->_auto_lang;
    
    // personalization.
    const char *prs = miscutil::lookup(parameters,"prs");
    if (!prs || (miscutil::strcmpic(prs,"on")!=0 && miscutil::strcmpic(prs,"off")!=0))
      prs = websearch::_wconfig->_personalization ? "on" : "off";
    results["pers"] = prs;

    // expansion.
    results["expansion"] = qc->_page_expansion;
    
    // peers.
    results["npeers"] = qc->_npeers;
    
    // suggested queries.
    results["suggestions"] = json_renderer::render_suggested_queries(qc,websearch::_wconfig->_num_reco_queries);
    
    // engines.
    if (qc->_engines.size() > 0)
      {
	results["engines"] = img ? json_renderer::render_img_engines(qc) : json_renderer::render_engines(qc->_engines);
      }
    else results["engines"] = Json::nullValue;

    // render date & exec time.
    char datebuf[256];
    cgi::get_http_time(0,datebuf,sizeof(datebuf));
    results["date"] = datebuf;
    results["qtime"] = qtime;
  }

} /* end of namespace json_renderer */
