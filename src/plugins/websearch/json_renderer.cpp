/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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
#include "encode.h"
#include "miscutil.h"
#include "cgi.h"
#include "json_renderer_private.h"
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
#include "img_query_context.h"
#endif
#include "proxy_configuration.h"

using sp::cgisimple;
using sp::miscutil;
using sp::cgi;
using sp::encode;
using sp::proxy_configuration;
using namespace json_renderer_private;

namespace seeks_plugins
{
  std::string json_renderer::render_engines(const std::bitset<NSEs> &engines)
  {
    std::string json_str_eng = "";
    if (engines.to_ulong()&SE_GOOGLE)
      json_str_eng += "\"google\"";
    if (engines.to_ulong()&SE_BING)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"bing\"";
      }
    if (engines.to_ulong()&SE_YAUBA)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"yauba\"";
      }
    if (engines.to_ulong()&SE_YAHOO)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"yahoo\"";
      }
    if (engines.to_ulong()&SE_EXALEAD)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"exalead\"";
      }
    if (engines.to_ulong()&SE_TWITTER)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"twitter\"";
      }
    if (engines.to_ulong()&SE_IDENTICA)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"identica\"";
      }
    if (engines.to_ulong()&SE_DAILYMOTION)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"dailymotion\"";
      }
    if (engines.to_ulong()&SE_YOUTUBE)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"youtube\"";
      }
    return json_str_eng;
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

  std::string json_renderer::render_img_engines(const query_context *qc)
  {
    std::string json_str_eng = "";
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    const img_query_context *iqc = static_cast<const img_query_context*>(qc);
    std::bitset<IMG_NSEs> engines = iqc->_img_engines;
    if (engines.to_ulong()&SE_BING_IMG)
      json_str_eng += "\"bing\"";
    if (engines.to_ulong()&SE_FLICKR)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"flickr\"";
      }
    if (engines.to_ulong()&SE_GOOGLE_IMG)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"google\"";
      }
    if (engines.to_ulong()&SE_WCOMMONS)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"wcommons\"";
      }
    if (engines.to_ulong()&SE_YAHOO_IMG)
      {
        if (!json_str_eng.empty())
          json_str_eng += ",";
        json_str_eng += "\"yahoo\"";
      }
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

        // proceed with rendering.
        const char *rpp_str = miscutil::lookup(parameters,"rpp"); // results per page.
        int rpp = websearch::_wconfig->_Nr;
        if (rpp_str)
          rpp = atoi(rpp_str);
        size_t ssize = snippets.size();
        size_t snisize = ssize;
        size_t snistart = 0;
        if (current_page == 0) // page is not taken into account, return all results.
          {
          }
        else
          {
            snisize = std::min(current_page*rpp,(int)snippets.size());
            snistart = (current_page-1)*rpp;
          }
        size_t count = 0;
        for (size_t i=0; i<ssize; i++)
          {
            if (snippets.at(i)->_doc_type == REJECTED)
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

    std::string query = query_clean(miscutil::lookup(parameters,"q"));

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
    std::string query = query_clean(miscutil::lookup(parameters,"q"));

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
  std::string query_clean(const std::string& q)
  {
    char *html_encoded_query_str = encode::html_encode(q.c_str());
    std::string html_encoded_query = std::string(html_encoded_query_str);
    free(html_encoded_query_str);
    return se_handler::no_command_query(html_encoded_query);
  }

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
    results.push_back("\"query\":\"" + query_clean(miscutil::lookup(parameters,"q")) + "\"");

    // language.
    results.push_back("\"lang\":\"" + qc->_auto_lang + "\"");

    // personalization.
    const char *prs = miscutil::lookup(parameters,"prs");
    if (!prs || (miscutil::strcmpic(prs,"on")!=0 && miscutil::strcmpic(prs,"off")!=0))
      prs = websearch::_wconfig->_personalization ? "on" : "off";
    results.push_back("\"pers\":\"" + std::string(prs) + "\"");

    // expansion.
    results.push_back("\"expansion\":\"" + miscutil::to_string(qc->_page_expansion) + "\"");

    // suggestion.
    if (!qc->_suggestions.empty())
      results.push_back("\"suggestion\":\"" + qc->_suggestions.at(0) + "\"");

    // engines.
    if (qc->_engines.to_ulong() > 0)
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
