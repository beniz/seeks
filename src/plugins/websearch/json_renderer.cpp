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

#include "json_renderer.h"
#include "cgisimple.h"
#include "miscutil.h"
#include "cgi.h"
#include "json_renderer_private.h"
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
#include "img_query_context.h"
#endif
#include "proxy_configuration.h"
#include "seeks_proxy.h" // for sweepables.

using sp::cgisimple;
using sp::miscutil;
using sp::cgi;
using sp::proxy_configuration;
using sp::seeks_proxy;
using namespace json_renderer_private;

namespace seeks_plugins
{
  std::string json_renderer::render_engines(const feeds &engines)
  {
    hash_map<const char*,feed_url_options,hash<const char*>,eqstr>::const_iterator hit;
    std::list<std::string> engs;
    std::set<feed_parser,feed_parser::lxn>::const_iterator it
    = engines._feedset.begin();
    while(it!=engines._feedset.end())
      {
        std::set<std::string>::const_iterator sit = (*it)._urls.begin();
        while(sit!=(*it)._urls.end())
          {
            if ((hit = websearch::_wconfig->_se_options.find((*sit).c_str()))
                != websearch::_wconfig->_se_options.end())
              engs.push_back("\"" + (*hit).second._id + "\"");
            ++sit;
          }
        ++it;
      }
    return miscutil::join_string_list(",",engs);
  }

  sp_err json_renderer::render_node_options(client_state *csp,
      std::list<std::string> &opts)
  {
    // system options.
    hash_map<const char*, const char*, hash<const char*>, eqstr> *exports
    = cgi::default_exports(csp,"");
    const char *value = miscutil::lookup(exports,"version");
    if (value)
      opts.push_back("\"version\":\"" + std::string(value) + "\"");
    if (websearch::_wconfig->_show_node_ip)
      {
        value = miscutil::lookup(exports,"my-ip-address");
        if (value)
          {
            opts.push_back("\"my-ip-address\":\"" + std::string(value) + "\"");
          }
      }
    value = miscutil::lookup(exports,"code-status");
    if (value)
      {
        opts.push_back("\"code-status\":\"" + std::string(value) + "\"");
      }
    value = miscutil::lookup(exports,"admin-address");
    if (value)
      {
        opts.push_back("\"admin-address\":\"" + std::string(value) + "\"");
      }
    opts.push_back("\"url-source-code\":\"" + csp->_config->_url_source_code + "\"");

    miscutil::free_map(exports);

    /*- websearch options. -*/
    // thumbs.
    std::string opt = "\"thumbs\":";
    websearch::_wconfig->_thumbs ? opt += "\"on\"" : opt += "\"off\"";
    opts.push_back(opt);
    opt = "\"content-analysis\":";
    websearch::_wconfig->_content_analysis ? opt += "\"on\"" : opt += "\"off\"";
    opts.push_back(opt);
    opt = "\"clustering\":";
    websearch::_wconfig->_clustering ? opt += "\"on\"" : opt += "\"off\"";
    opts.push_back(opt);

    return SP_ERR_OK;
  }

  std::string json_renderer::render_related_queries(const query_context *qc)
  {
    if (!qc->_suggestions.empty())
      {
        int k = 0;
        std::list<std::string> suggs;
        std::multimap<double,std::string,std::less<double> >::const_iterator mit
        = qc->_suggestions.begin();
        while(mit!=qc->_suggestions.end())
          {
            std::string sugg = (*mit).second;
            miscutil::replace_in_string(sugg,"\\","\\\\");
            miscutil::replace_in_string(sugg,"\"","\\\"");
            suggs.push_back("\"" + sugg + "\"");
            if (k > websearch::_wconfig->_num_reco_queries)
              break;
            ++k;
            ++mit;
          }
        return "\"suggestions\":[" + miscutil::join_string_list(",",suggs) + "]"
               + ",\"rqueries\":" + miscutil::to_string(qc->_suggestions.size()); // XXX: rqueries seem useless.
      }
    return "";
  }

  std::string json_renderer::render_recommendations(const std::string &query_clean,
      const query_context *qc)
  {
    if (!qc->_recommended_snippets.empty())
      {
        std::list<std::string> suggs;

        std::vector<std::string> query_words;
        miscutil::tokenize(query_clean,query_words," ");

        int k = 0;
        hash_map<uint32_t,search_snippet*,id_hash_uint>::const_iterator hit
        = qc->_recommended_snippets.begin();
        while(hit!=qc->_recommended_snippets.end())
          {
            suggs.push_back((*hit).second->to_json(false,query_words));
            if (k > websearch::_wconfig->_num_reco_queries)
              break;
            ++k;
            ++hit;
          }
        return "\"recommendations\":[" + miscutil::join_string_list(",",suggs) + "]";
      }
    return "";
  }

  std::string json_renderer::render_cached_queries(const std::string &query)
  {
    std::list<std::string> suggs;
    std::vector<sweepable*>::const_iterator sit = seeks_proxy::_memory_dust.begin();
    while(sit!=seeks_proxy::_memory_dust.end())
      {
        query_context *qc = dynamic_cast<query_context*>((*sit));
        if (!qc)
          {
            ++sit;
            continue;
          }
        if (qc->_query != query)
          {
            std::string escaped_query = qc->_query;
            miscutil::replace_in_string(escaped_query,"\"","\\\"");
            miscutil::replace_in_string(escaped_query,"\\t","");
            miscutil::replace_in_string(escaped_query,"\\r","");
            miscutil::replace_in_string(escaped_query,"\n","");
            suggs.push_back("\"" + escaped_query + "\"");
          }
        ++sit;
      }
    if (!suggs.empty())
      return "\"queries\":[" + miscutil::join_string_list(",",suggs) + "]";
    else return "";
  }

  std::string json_renderer::render_img_engines(const query_context *qc)
  {
    std::string json_str_eng = "";
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    const img_query_context *iqc = static_cast<const img_query_context*>(qc);
    feeds engines = iqc->_img_engines;
    json_str_eng += render_engines(engines);
#endif
    return json_str_eng;
  }

  sp_err json_renderer::render_snippets(const std::string &query_clean,
                                        const int &current_page,
                                        const std::vector<search_snippet*> &snippets,
                                        std::string &json_str,
                                        const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    json_str += "\"snippets\":[";
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

        for (size_t i=0; i<ssize; i++)
          {
            if (snippets.at(i)->_doc_type == REJECTED)
              continue;
            if (!snippets.at(i)->is_se_enabled(parameters))
              continue;
            if (!safesearch_off && !snippets.at(i)->_safe)
              continue;
            if (!similarity || snippets.at(i)->_seeks_ir > 0)
              {
                if (count >= snistart)
                  {
                    if (count > snistart && count<snisize)
                      json_str += ",";
                    json_str += snippets.at(i)->to_json(has_thumbs,query_words);
                  }
                count++;
              }
            if (count == snisize)
              {
                break; // end here.
              }
          }
      }
    json_str += "]";
    return SP_ERR_OK;
  }

  sp_err json_renderer::render_clustered_snippets(const std::string &query_clean,
      cluster *clusters, const short &K,
      const query_context *qc,
      std::string &json_str,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    json_str += "\"clusters\":[";

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

        json_str += "{";
        json_str += "\"label\":\"" + clusters[c]._label + "\",";
        json_renderer::render_snippets(query_clean,0,snippets,json_str,parameters);
        json_str += "}";
      }

    json_str += "]";
    return SP_ERR_OK;
  }

  sp_err json_renderer::render_json_results(const std::vector<search_snippet*> &snippets,
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
    std::string json_snippets;
    json_renderer::render_snippets(query,current_page,snippets,json_snippets,parameters);

    std::list<std::string> results;
    collect_json_results(results,parameters,qc,qtime,img);
    results.push_back(json_snippets);
    const std::string results_string = "{" + miscutil::join_string_list(",",results) + "}";
    const std::string body = jsonp(results_string, miscutil::lookup(parameters,"callback"));
    response(rsp, body);

    return SP_ERR_OK;
  }

  sp_err json_renderer::render_json_node_options(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    std::list<std::string> opts;
    sp_err err = json_renderer::render_node_options(csp,opts);
    std::string json_str = "{" + miscutil::join_string_list(",",opts) + "}";
    const std::string body = jsonp(json_str, miscutil::lookup(parameters,"callback"));
    response(rsp,body);
    return err;
  }

  sp_err json_renderer::render_clustered_json_results(cluster *clusters,
      const short &K,
      client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      const query_context *qc,
      const double &qtime)
  {
    std::string query = qc->_query;

    // search clustered snippets.
    std::string json_snippets;
    json_renderer::render_clustered_snippets(query,clusters,K,qc,json_snippets,parameters);

    std::list<std::string> results;
    collect_json_results(results,parameters,qc,qtime);
    results.push_back(json_snippets);
    const std::string results_string = "{" + miscutil::join_string_list(",",results) + "}";
    const std::string body = jsonp(results_string, miscutil::lookup(parameters,"callback"));
    response(rsp, body);

    return SP_ERR_OK;
  }
}; /* end of namespace seeks_plugins */

using namespace seeks_plugins;

namespace json_renderer_private
{
  /*std::string query_clean(const std::string& q)
  {
    char *html_encoded_query_str = encode::html_encode(q.c_str());
    std::string html_encoded_query = std::string(html_encoded_query_str);
    free(html_encoded_query_str);
    return se_handler::no_command_query(html_encoded_query);
    }*/

  std::string jsonp(const std::string& input, const char* callback)
  {
    std::string output;
    if (callback == NULL)
      return input;
    else
      return std::string(callback) + "(" + input + ")";
  }

  void response(http_response *rsp, const std::string& json_str)
  {
    rsp->_body = strdup(json_str.c_str());
    rsp->_content_length = json_str.size();
    miscutil::enlist(&rsp->_headers, "Content-Type: application/json");
    rsp->_is_static = 1;
  }

  sp_err collect_json_results(std::list<std::string> &results,
                              const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                              const query_context *qc,
                              const double &qtime,
                              const bool &img)
  {
    /*- query info. -*/
    // query.
    std::string escaped_query = qc->_query;
    miscutil::replace_in_string(escaped_query,"\"","\\\"");
    results.push_back("\"query\":\"" + escaped_query + "\"");

    // language.
    results.push_back("\"lang\":\"" + qc->_auto_lang + "\"");

    // personalization.
    const char *prs = miscutil::lookup(parameters,"prs");
    if (!prs || (miscutil::strcmpic(prs,"on")!=0 && miscutil::strcmpic(prs,"off")!=0))
      prs = websearch::_wconfig->_personalization ? "on" : "off";
    results.push_back("\"pers\":\"" + std::string(prs) + "\"");

    // expansion.
    results.push_back("\"expansion\":\"" + miscutil::to_string(qc->_page_expansion) + "\"");

    // peers.
    results.push_back("\"npeers\":\"" + miscutil::to_string(qc->_npeers) + "\"");

    // related queries.
    std::string related_queries = json_renderer::render_related_queries(qc);
    if (!related_queries.empty())
      results.push_back(related_queries);

    // related URLs.
    std::string reco = json_renderer::render_recommendations(qc->_query,qc);
    if (!reco.empty())
      results.push_back(reco);

    // queries in cache.
    std::string cached_queries = json_renderer::render_cached_queries(qc->_query);
    if (!cached_queries.empty())
      results.push_back(cached_queries);

    // engines.
    if (qc->_engines.size() > 0)
      {
        results.push_back("\"engines\":[" +
                          (img ? json_renderer::render_img_engines(qc)
                           : json_renderer::render_engines(qc->_engines)) +
                          "]");
      }

    // render date & exec time.
    char datebuf[256];
    cgi::get_http_time(0,datebuf,sizeof(datebuf));
    results.push_back("\"date\":\"" + std::string(datebuf) + "\"");
    results.push_back("\"qtime\":" + miscutil::to_string(qtime));

    return SP_ERR_OK;
  }

} /* end of namespace json_renderer_private */
