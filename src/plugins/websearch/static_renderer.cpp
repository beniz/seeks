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

#include "static_renderer.h"
#include "seeks_snippet.h"
#include "mem_utils.h"
#include "plugin_manager.h"
#include "cgi.h"
#include "cgisimple.h"
#include "miscutil.h"
#include "encode.h"
#include "urlmatch.h"
#include "proxy_configuration.h"
#if defined(PROTOBUF) && defined(TC)
#include "query_capture_configuration.h"
#endif
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
#include "img_search_snippet.h"
#endif
#include "seeks_proxy.h"
#include "errlog.h"

#include <assert.h>
#include <ctype.h>
#include <iostream>
#include <algorithm>

using sp::miscutil;
using sp::cgi;
using sp::cgisimple;
using sp::plugin_manager;
using sp::cgi_dispatcher;
using sp::encode;
using sp::urlmatch;
using sp::proxy_configuration;
using sp::seeks_proxy;
using sp::errlog;
using lsh::LSHUniformHashTable;
using lsh::Bucket;

namespace seeks_plugins
{
  void static_renderer::highlight_discr(const search_snippet *sp,
                                        std::string &str, const std::string &base_url_str,
                                        const std::vector<std::string> &query_words)
  {
    // select discriminant words.
    std::set<std::string> words;
    sp->discr_words(query_words,words);

    // highlighting.
    std::set<std::string>::const_iterator sit = words.begin();
    while(sit!=words.end())
      {
        if ((*sit).length() > 2)
          {
            char *wenc = encode::url_encode((*sit).c_str());
            std::string rword = " " + (*sit) + " ";
            std::string bold_str = "<span class=\"highlight\"><a href=\"" + base_url_str + "/search?q=" + sp->_qc->_url_enc_query
                                   + "+" + std::string(wenc) + "&amp;page=1&amp;expansion=1&amp;lang=" + sp->_qc->_auto_lang + "&amp;ui=stat\">" + rword + "</a></span>";
            free(wenc);
            miscutil::ci_replace_in_string(str,rword,bold_str);
          }
        ++sit;
      }
  }

  void static_renderer::render_query(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                     hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
                                     std::string &html_encoded_query,
                                     std::string &url_encoded_query)
  {
    const char *q = miscutil::lookup(parameters,"q");
    char *html_encoded_query_str = encode::html_encode(q);
    html_encoded_query = std::string(html_encoded_query_str);
    free(html_encoded_query_str);
    char *url_encoded_query_str = encode::url_encode(q);
    miscutil::add_map_entry(exports,"$fullquery",1,url_encoded_query_str,1);
    url_encoded_query = std::string(url_encoded_query_str);
    free(url_encoded_query_str);
  }

  void static_renderer::render_clean_query(const std::string &html_encoded_query,
      hash_map<const char*,const char*,hash<const char*>,eqstr> *exports)
  {
    miscutil::add_map_entry(exports,"$qclean",1,html_encoded_query.c_str(),1);
  }

  void static_renderer::render_suggestions(const query_context *qc,
      hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
      const std::string &cgi_base)
  {
    if (!qc->_suggestions.empty())
      {
        std::string used_rqueries = miscutil::to_string(qc->_suggestions.size());
        const char *base_url = miscutil::lookup(exports,"base-url");
        std::string base_url_str = "";
        if (base_url)
          base_url_str = std::string(base_url);
        std::string suggestion_str;
        int k = 0;
        std::multimap<double,std::string,std::less<double> >::const_iterator mit
        = qc->_suggestions.begin();
        while(mit!=qc->_suggestions.end())
          {
            if (k >= websearch::_wconfig->_num_reco_queries)
              break;
            std::string suggested_q_str = (*mit).second;
            char *sugg_html_enc = encode::html_encode(suggested_q_str.c_str());
            std::string sugg_html_enc_str = std::string(sugg_html_enc);
            if (sugg_html_enc_str.size() > 45)
              sugg_html_enc_str =sugg_html_enc_str.substr(0,42) + "...";
            free(sugg_html_enc);
            char *sugg_url_enc = encode::url_encode(suggested_q_str.c_str());
            std::string sugg_url_enc_str = std::string(sugg_url_enc);
            free(sugg_url_enc);
            suggestion_str += "<br><a href=\"" + base_url_str + cgi_base + "?q=";
            suggestion_str += sugg_url_enc_str + "?";
            suggestion_str += "lang=" + qc->_auto_lang + "&amp;";
            suggestion_str += "expansion=1&amp;ui=stat";
            suggestion_str += "\">";
            suggestion_str += sugg_html_enc_str;
            suggestion_str += "</a>";
            ++mit;
            ++k;
          }
        if (!suggestion_str.empty())
          suggestion_str = "Related queries:" + suggestion_str;
        miscutil::add_map_entry(exports,"$xxsugg",1,suggestion_str.c_str(),1);
        miscutil::add_map_entry(exports,"$xxrqueries",1,used_rqueries.c_str(),1);
      }
    else
      {
        miscutil::add_map_entry(exports,"$xxsugg",1,strdup(""),0);
        miscutil::add_map_entry(exports,"$xxrqueries",1,strdup("0"),0);
      }
    miscutil::add_map_entry(exports,"$xxnpeers",1,miscutil::to_string(qc->_npeers).c_str(),1);
  }

  void static_renderer::render_cached_queries(const std::string &query,
      hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
      const std::string &cgi_base)
  {
    const char *base_url = miscutil::lookup(exports,"base-url");
    std::string base_url_str = "";
    if (base_url)
      base_url_str = std::string(base_url);
    std::string cqueries_str;

    int k = 0;
    std::vector<sweepable*>::const_iterator sit = seeks_proxy::_memory_dust.begin();
    while(sit!=seeks_proxy::_memory_dust.end())
      {
        if (k >= websearch::_wconfig->_num_recent_queries)
          break;
        query_context *qc = dynamic_cast<query_context*>((*sit));
        if (!qc)
          {
            ++sit;
            continue;
          }

        if (qc->_query != query)
          {
            char *html_enc_query = encode::html_encode(qc->_query.c_str());
            char *url_enc_query = encode::url_encode(qc->_query.c_str());
            cqueries_str += "<br><a href=\"" + base_url_str + cgi_base
                            + "?q=" + std::string(url_enc_query) +"?expansion=1&amp;ui=stat\">"
                            + std::string(html_enc_query) + "</a>";
            free(html_enc_query);
            free(url_enc_query);
            ++k;
          }
        ++sit;
      }
    if (!cqueries_str.empty())
      cqueries_str = "Recent queries:" + cqueries_str;
    miscutil::add_map_entry(exports,"$xxqcache",1,cqueries_str.c_str(),1);
  }

  void static_renderer::render_lang(const query_context *qc,
                                    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports)
  {
    miscutil::add_map_entry(exports,"$xxlang",1,qc->_auto_lang.c_str(),1);
  }

  void static_renderer::render_engines(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                       hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
                                       std::string &engines)
  {
    const char *eng = miscutil::lookup(parameters,"engines");
    if (eng)
      {
        engines = std::string(eng);
        miscutil::add_map_entry(exports,"$xxeng",1,eng,1);
      }
    else
      {
        engines = "";
        miscutil::add_map_entry(exports,"$xxeng",1,strdup(""),0);
      }
  }

  void static_renderer::render_snippets(const std::string &query_clean,
                                        const int &current_page,
                                        const std::vector<search_snippet*> &snippets,
                                        const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                        hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
                                        bool &not_end,
                                        const bool &img)
  {
    const char *base_url = miscutil::lookup(exports,"base-url");
    std::string base_url_str = "";
    if (base_url)
      base_url_str = std::string(base_url);

    cgi::map_block_killer(exports,"have-clustered-results-head");
    cgi::map_block_killer(exports,"have-clustered-results-body");

    int rpp = websearch::_wconfig->_Nr;
    const char *rpp_str = miscutil::lookup(parameters,"rpp");
    if (rpp_str)
      rpp = atoi(rpp_str);

    // checks for safe snippets (for now, only used for images).
    const char* safesearch_p = miscutil::lookup(parameters,"safesearch");
    bool safesearch_off = false;
    if (safesearch_p)
      safesearch_off = strcasecmp(safesearch_p,"on") == 0 ? false : true;

    not_end = false;
    bool only_tweets = true; // required for rendering the correct 'content_analysi' value.
    std::string snippets_str;
    if (!snippets.empty())
      {
        // check whether we're rendering similarity snippets.
        bool similarity = false;
        if (snippets.at(0)->_seeks_ir > 0)
          similarity = true;

        // proceed with rendering.
        int ssize = snippets.size();
        size_t snisize = std::min(current_page*rpp,(int)ssize);
        size_t snistart = (current_page-1) * rpp;
        size_t count = 0;

        for (int i=0; i<ssize; i++)
          {
            if (snippets.at(i)->_doc_type == doc_type::REJECTED)
              continue;
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
            if (img && snippets.at(i)->_doc_type != seeks_img_doc_type::IMAGE)
              continue;
#endif
            if (!snippets.at(i)->is_se_enabled(parameters))
              continue;
            if (!safesearch_off && !snippets.at(i)->_safe)
              continue;
            if (only_tweets && snippets.at(i)->_doc_type != seeks_doc_type::TWEET)
              only_tweets = false;

            if (!similarity || snippets.at(i)->_seeks_ir > 0)
              {
                if (count >= snistart)
                  snippets_str += snippets.at(i)->to_html(snippets.at(i)->_qc->_query_words,base_url_str,
                                                          parameters);
                count++;
              }
            if (count == snisize)
              {
                if (i < ssize-1)
                  not_end = true;
                break; // end list here.
              }
          }
      }

    miscutil::add_map_entry(exports,"search_snippets",1,snippets_str.c_str(),1);
    if (rpp_str)
      miscutil::add_map_entry(exports,"$xxrpp",1,rpp_str,1);
    else miscutil::add_map_entry(exports,"$xxrpp",1,strdup(""),0);
    miscutil::add_map_entry(exports,"$xxtrpp",1,
                            miscutil::to_string(websearch::_wconfig->_Nr).c_str(),1);

    // content analysis.
    bool content_analysis = websearch::_wconfig->_content_analysis;
    const char *ca = miscutil::lookup(parameters,"content_analysis");
    if (ca && strcasecmp(ca,"on") == 0)
      content_analysis = true;
    if (content_analysis)
      miscutil::add_map_entry(exports,"$xxca",1,"on",1);
    else miscutil::add_map_entry(exports,"$xxca",1,"off",1);
    if (only_tweets)
      miscutil::add_map_entry(exports,"$xxcca",1,"off",1); // xxcca is content_analysis for cluster analysis.
    else miscutil::add_map_entry(exports,"$xxcca",1,content_analysis ? "on" : "off",1);

    // personalization.
    const char *prs = miscutil::lookup(parameters,"prs");
    if (!prs)
      {
        prs = websearch::_wconfig->_personalization ? "on" : "off";
      }
    if (strcasecmp(prs,"on")==0)
      {
        miscutil::add_map_entry(exports,"$xxpers",1,"on",1);
        miscutil::add_map_entry(exports,"$xxnpers",1,"off",1);
      }
    else
      {
        miscutil::add_map_entry(exports,"$xxpers",1,"off",1);
        miscutil::add_map_entry(exports,"$xxnpers",1,"on",1);
      }
  }

  void static_renderer::render_clustered_snippets(const std::string &query_clean,
      const std::string &url_encoded_query,
      const int &current_page,
      hash_map<int,cluster*> *clusters,
      const short &K,
      const query_context *qc,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
      hash_map<const char*,const char*,hash<const char*>,eqstr> *exports)
  {
    static short template_K=8; // max number of clusters available in the template.

    const char *base_url = miscutil::lookup(exports,"base-url");
    std::string base_url_str = "";
    if (base_url)
      base_url_str = std::string(base_url);

    // lookup activated engines.
    feeds se_enabled;
    query_context::fillup_engines(parameters,se_enabled);

    // check for empty cluster, and determine which rendering to use.
    short k = 0;
    hash_map<int,cluster*>::const_iterator chit = clusters->begin();
    while(chit!=clusters->end())
      {
        short cc = 0;
        if (!(*chit).second->_cpoints.empty())
          {
            cluster *cl = (*chit).second;
            hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit
            = cl->_cpoints.begin();
            while (hit!=cl->_cpoints.end())
              {
                search_snippet *sp = qc->get_cached_snippet((*hit).first);
                if (sp->_doc_type == doc_type::REJECTED)
                  {
                    ++hit;
                    continue;
                  }

                feeds band = se_enabled.inter(sp->_engine);
                if (band.size() == 0)
                  {
                    ++hit;
                    continue;
                  }
                ++hit;
                ++cc;
              }
            if (cc > 0)
              k++;
          }
        ++chit;
      }

    std::string rplcnt = "ccluster";
    cgi::map_block_killer(exports,"have-one-column-results-head");
    if (k>1)
      cgi::map_block_killer(exports,"have-one-column-results");
    else
      {
        cgi::map_block_killer(exports,"have-clustered-results-body");
        rplcnt = "search_snippets";
      }

    bool clusterize = false;
    const char *action_str = miscutil::lookup(parameters,"action");
    if (action_str && strcmp(action_str,"clusterize")==0)
      clusterize = true;

    // renders every cluster and snippets within.
    int l = 0;
    chit = clusters->begin();
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
            if (sp->_doc_type == doc_type::REJECTED)
              {
                ++hit;
                continue;
              }
            //std::bitset<NSEs> band = (sp->_engine & se_enabled);
            feeds band = se_enabled.inter(sp->_engine);
            if (band.size() == 0)
              {
                ++hit;
                continue;
              }
            snippets.push_back(sp);
            ++hit;
          }

        std::stable_sort(snippets.begin(),snippets.end(),search_snippet::max_seeks_rank);

        if (!snippets.empty())
          {
            std::string cluster_str;
            if (!clusterize)
              cluster_str = static_renderer::render_cluster_label(cl);
            else cluster_str = static_renderer::render_cluster_label_query_link(url_encoded_query,
                                 cl,exports);
            size_t nsps = snippets.size();
            for (size_t i=0; i<nsps; i++)
              cluster_str += snippets.at(i)->to_html(snippets.at(i)->_qc->_query_words,
                                                     base_url,parameters);
            cluster_str += "</ol><div class=\"clear\"></div>";

            std::string cl = rplcnt;
            if (k>1)
              cl += miscutil::to_string(l++);
            miscutil::add_map_entry(exports,cl.c_str(),1,cluster_str.c_str(),1);
          }
        ++chit;
      }

    // kill remaining cluster slots.
    if (k>1)
      {
        for (short c=l; c<=template_K; c++)
          {
            std::string hcl = "have-cluster" + miscutil::to_string(c);
            cgi::map_block_killer(exports,hcl.c_str());
          }
      }
    //else miscutil::add_map_entry(exports,rplcnt.c_str(),1,"",1);

    const char *rpp_str = miscutil::lookup(parameters,"rpp");
    if (rpp_str)
      miscutil::add_map_entry(exports,"$xxrpp",1,rpp_str,1);
    else miscutil::add_map_entry(exports,"$xxrpp",1,strdup(""),0);
    /*miscutil::add_map_entry(exports,"$xxtrpp",1,
    			miscutil::to_string(websearch::_wconfig->_Nr).c_str(),1); */

    // personalization.
    const char *prs = miscutil::lookup(parameters,"prs");
    if (!prs)
      {
        prs = websearch::_wconfig->_personalization ? "on" : "off";
      }
    if (strcasecmp(prs,"on")==0)
      {
        miscutil::add_map_entry(exports,"$xxpers",1,"on",1);
        miscutil::add_map_entry(exports,"$xxnpers",1,"off",1);
      }
    else
      {
        miscutil::add_map_entry(exports,"$xxpers",1,"off",1);
        miscutil::add_map_entry(exports,"$xxnpers",1,"on",1);
      }
  }

  std::string static_renderer::render_cluster_label(const cluster *cl)
  {
    const char *clabel_encoded = encode::html_encode(cl->_label.c_str());
    std::string slabel = "(" + miscutil::to_string(cl->_cpoints.size()) + ")";
    const char *slabel_encoded = encode::html_encode(slabel.c_str());
    std::string html_label = "<h2>" + std::string(clabel_encoded)
                             + " <font size=\"2\">" + std::string(slabel_encoded) + "</font></h2><br><ol>";
    free_const(clabel_encoded);
    free_const(slabel_encoded);
    return html_label;
  }

  std::string static_renderer::render_cluster_label_query_link(const std::string &url_encoded_query,
      cluster *cl,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
      const std::string &cgi_base)
  {
    const char *base_url = miscutil::lookup(exports,"base-url");
    std::string base_url_str = "";
    if (base_url)
      base_url_str = std::string(base_url);
    char *clabel_url_enc = encode::url_encode(cl->_label.c_str());
    char *clabel_html_enc = encode::html_encode(cl->_label.c_str());
    std::string clabel_url_enc_str = std::string(clabel_url_enc);
    free(clabel_url_enc);
    std::string clabel_html_enc_str = std::string(clabel_html_enc);
    free(clabel_html_enc);

    std::string slabel = "(" + miscutil::to_string(cl->_cpoints.size()) + ")";
    char *slabel_html_enc = encode::html_encode(slabel.c_str());
    std::string slabel_html_enc_str = std::string(slabel_html_enc);
    free(slabel_html_enc);

    std::string label_query = url_encoded_query + "+" + clabel_url_enc_str;
    std::string html_label = "<h2><a class=\"label\" href=\"" + base_url_str + cgi_base + "?q=" + label_query
                             + "?page=1&amp;expansion=1&amp;ui=stat\">" + clabel_html_enc_str
                             + "</a><font size=\"2\"> " + slabel_html_enc_str + "</font></h2><br><ol>";
    return html_label;
  }

  void static_renderer::render_current_page(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
      int &current_page)
  {
    const char *current_page_str = miscutil::lookup(parameters,"page");
    if (!current_page_str)
      current_page = 0;
    else current_page = atoi(current_page_str);
    if (current_page == 0) current_page = 1;
    std::string cp_str = miscutil::to_string(current_page);
    miscutil::add_map_entry(exports,"$xxpage",1,cp_str.c_str(),1);
  }

  void static_renderer::render_expansion(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                         hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
                                         std::string &expansion)
  {
    const char *expansion_str = miscutil::lookup(parameters,"expansion");
    if (!expansion_str)
      expansion_str = "1"; // default.
    miscutil::add_map_entry(exports,"$xxexp",1,expansion_str,1);
    int expn = atoi(expansion_str)+1;
    std::string expn_str = miscutil::to_string(expn);
    miscutil::add_map_entry(exports,"$xxexpn",1,expn_str.c_str(),1);
    expansion = std::string(expansion_str);
  }

  void static_renderer::render_next_page_link(const int &current_page,
      const size_t &snippets_size,
      const std::string &url_encoded_query,
      const std::string &expansion,
      const std::string &engines,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
      hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
      const query_context *qc,
      const std::string &cgi_base, const bool &not_end)
  {
    if (!not_end)
      {
        miscutil::add_map_entry(exports,"$xxnext",1,strdup("<a id=\"search_page_end\">&nbsp;</a>"),0);
        return;
      }

    int rpp = websearch::_wconfig->_Nr;
    const char *rpp_str = miscutil::lookup(parameters,"rpp");
    if (rpp_str)
      rpp = atoi(rpp_str);
    double nl = snippets_size / static_cast<double>(rpp);

    if (current_page < nl)
      {
        const char *base_url = miscutil::lookup(exports,"base-url");
        std::string base_url_str;
        if (base_url)
          base_url_str = std::string(base_url);
        std::string rpp_s;
        if (rpp_str)
          rpp_s = rpp_str;
        std::string np_str = miscutil::to_string(current_page+1);
        bool content_analysis = websearch::_wconfig->_content_analysis;
        const char *ca = miscutil::lookup(parameters,"content_analysis");
        if (ca && strcasecmp(ca,"on") == 0)
          content_analysis = true;
        std::string ca_s = content_analysis ? "on" : "off";
        const char *prs = miscutil::lookup(parameters,"prs");
        if (!prs)
          prs = websearch::_wconfig->_personalization ? "on" : "off";
        std::string np_link = "<a href=\"" + base_url_str + cgi_base + "?q="
                              + url_encoded_query + "?page=" + np_str + "&amp;expansion=" + expansion + "&amp;engines="
                              + engines + "&amp;rpp=" + rpp_s + "&amp;action=page&amp;content_analysis=" + ca_s
                              + "&amp;prs=" + std::string(prs) + "&amp;lang=" + qc->_auto_lang
                              + "&amp;ui=stat\"  id=\"search_page_next\" title=\"Next (ctrl+&gt;)\">&nbsp;</a>";
        miscutil::add_map_entry(exports,"$xxnext",1,np_link.c_str(),1);
      }
    else miscutil::add_map_entry(exports,"$xxnext",1,strdup("<a id=\"search_page_end\">&nbsp;</a>"),0);
  }

  void static_renderer::render_prev_page_link(const int &current_page,
      const size_t &snippets_size,
      const std::string &url_encoded_query,
      const std::string &expansion,
      const std::string &engines,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
      hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
      const query_context *qc,
      const std::string &cgi_base)
  {
    if (current_page > 1)
      {
        std::string pp_str = miscutil::to_string(current_page-1);
        const char *base_url = miscutil::lookup(exports,"base-url");
        std::string base_url_str = "";
        if (base_url)
          base_url_str = std::string(base_url);
        std::string rpp_s;
        const char *rpp_str = miscutil::lookup(parameters,"rpp");
        if (rpp_str)
          rpp_s = rpp_str;
        bool content_analysis = websearch::_wconfig->_content_analysis;
        const char *ca = miscutil::lookup(parameters,"content_analysis");
        if (ca && strcasecmp(ca,"on") == 0)
          content_analysis = true;
        const char *prs = miscutil::lookup(parameters,"prs");
        if (!prs)
          prs = websearch::_wconfig->_personalization ? "on" : "off";
        std::string ca_s = content_analysis ? "on" : "off";
        std::string pp_link = "<a href=\"" + base_url_str + cgi_base + "?q=" + url_encoded_query
                              + "?page=" + pp_str + "&amp;action=page&amp;expansion=" + expansion + "&amp;engines="
                              + engines + "&amp;rpp=" + rpp_s + "&amp;content_analysis=" + ca_s
                              + "&amp;prs=" + std::string(prs) + "&amp;lang=" + qc->_auto_lang
                              + "&amp;ui=stat\"  id=\"search_page_prev\" title=\"Previous (ctrl+&lt;)\">&nbsp;</a>";
        miscutil::add_map_entry(exports,"$xxprev",1,pp_link.c_str(),1);
      }
    else miscutil::add_map_entry(exports,"$xxprev",1,strdup(""),0);
  }

  void static_renderer::render_nclusters(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                         hash_map<const char*,const char*,hash<const char*>,eqstr> *exports)
  {
    if (!websearch::_wconfig->_clustering)
      {
        cgi::map_block_killer(exports,"have-clustering");
        return;
      }
    else cgi::map_block_killer(exports,"not-have-clustering");

    const char *nclusters_str = miscutil::lookup(parameters,"clusters");

    if (!nclusters_str)
      miscutil::add_map_entry(exports,"$xxnclust",1,strdup("10"),0); // default number of clusters is 10.
    else
      {
        miscutil::add_map_entry(exports,"$xxclust",1,nclusters_str,1);
        int nclust = atoi(nclusters_str)+1;
        std::string nclust_str = miscutil::to_string(nclust);
        miscutil::add_map_entry(exports,"$xxnclust",1,nclust_str.c_str(),1);
      }
  }

  hash_map<const char*,const char*,hash<const char*>,eqstr>* static_renderer::websearch_exports(client_state *csp,
      const std::vector<std::pair<std::string,std::string> > *param_exports)
  {
    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
    = cgi::default_exports(csp,"");

    // we need to inject a remote base location for remote web access.
    // the injected header, if it exists is X-Seeks-Remote-Location
    std::string base_url = query_context::detect_base_url_http(csp);
    miscutil::add_map_entry(exports,"base-url",1,base_url.c_str(),1);

    if (!websearch::_wconfig->_js) // no javascript required
      {
        cgi::map_block_killer(exports,"websearch-have-js");
      }

    /* if (websearch::_wconfig->_thumbs)
      miscutil::add_map_entry(exports,"websearch-thumbs",1,"1",1);
    else miscutil::add_map_entry(exports,"websearch-thumbs",1,strdup(""),0); */

    /* if (websearch::_wconfig->_clustering)
      miscutil::add_map_entry(exports,"websearch-clustering",1,"1",1);
    else miscutil::add_map_entry(exports,"websearch-clustering",1,strdup(""),0); */

    // image websearch, if available.
#ifndef FEATURE_IMG_WEBSEARCH_PLUGIN
    cgi::map_block_killer(exports,"img-websearch");
#endif

    // node IP rendering.
    if (!websearch::_wconfig->_show_node_ip)
      cgi::map_block_killer(exports,"have-show-node-ip");

    // message rendering.
    if (websearch::_wconfig->_result_message.empty())
      cgi::map_block_killer(exports,"have-result-message");
    else miscutil::add_map_entry(exports,"$xxmsg",1,
                                   websearch::_wconfig->_result_message.c_str(),1);

    // theme.
    miscutil::add_map_entry(exports,"$xxtheme",1,
                            websearch::_wconfig->_ui_theme.c_str(),1);

    // other parameters.
    if (param_exports)
      {
        for (size_t i=0; i<param_exports->size(); i++)
          {
            miscutil::add_map_entry(exports,param_exports->at(i).first.c_str(),1,
                                    param_exports->at(i).second.c_str(),1);
          }
      }

    return exports;
  }

  /*- rendering. -*/
  sp_err static_renderer::render_hp(client_state *csp,http_response *rsp)
  {
    std::string hp_tmpl_name = "websearch/templates/themes/"
                               + websearch::_wconfig->_ui_theme + "/seeks_ws_hp.html";

    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
    = static_renderer::websearch_exports(csp);

    sp_err err = cgi::template_fill_for_cgi(csp,hp_tmpl_name.c_str(),
                                            (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
                                                : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
                                            exports,rsp);

    return err;
  }

  sp_err static_renderer::render_result_page_static(const std::vector<search_snippet*> &snippets,
      client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      const query_context *qc)
  {
    std::string result_tmpl_name = "websearch/templates/themes/"
                                   + websearch::_wconfig->_ui_theme + "/seeks_result_template.html";
    return static_renderer::render_result_page_static(snippets,csp,rsp,parameters,qc,result_tmpl_name);
  }

  sp_err static_renderer::render_result_page_static(const std::vector<search_snippet*> &snippets,
      client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      const query_context *qc,
      const std::string &result_tmpl_name,
      const std::string &cgi_base,
      const std::vector<std::pair<std::string,std::string> > *param_exports,
      const bool &img)
  {
    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
    = static_renderer::websearch_exports(csp,param_exports);

    // query.
    std::string html_encoded_query;
    std::string url_encoded_query;
    static_renderer::render_query(parameters,exports,html_encoded_query,
                                  url_encoded_query);

    // clean query.
    static_renderer::render_clean_query(html_encoded_query,exports);

    // current page.
    int current_page = -1;
    static_renderer::render_current_page(parameters,exports,current_page);

    // suggestions.
    static_renderer::render_suggestions(qc,exports,cgi_base);

    // recommended URLs.
    //static_renderer::render_recommendations(qc,exports,cgi_base);

    // queries in cache.
    static_renderer::render_cached_queries(html_encoded_query,exports,cgi_base);

    // language.
    static_renderer::render_lang(qc,exports);

    // engines.
    std::string engines;
    static_renderer::render_engines(parameters,exports,engines);

    // TODO: check whether we have some results.
    /* if (snippets->empty())
      {
    // no results found.
    std::string no_results = "";
    return SP_ERR_OK;
      } */

    // search snippets.
    bool not_end = false;
    static_renderer::render_snippets(html_encoded_query,current_page,snippets,parameters,exports,not_end,img);

    // expand button.
    std::string expansion;
    static_renderer::render_expansion(parameters,exports,expansion);

    // next link.
    static_renderer::render_next_page_link(current_page,snippets.size(),
                                           url_encoded_query,expansion,engines,parameters,
                                           exports,qc,cgi_base,not_end);

    // previous link.
    static_renderer::render_prev_page_link(current_page,snippets.size(),
                                           url_encoded_query,expansion,engines,
                                           parameters,exports,qc,cgi_base);

    // cluster link.
    static_renderer::render_nclusters(parameters,exports);

    // rendering.
    sp_err err = cgi::template_fill_for_cgi(csp,result_tmpl_name.c_str(),
                                            (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
                                                : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
                                            exports,rsp);

    return err;
  }

  sp_err static_renderer::render_clustered_result_page_static(hash_map<int,cluster*> *clusters,
      const short &K,
      client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      const query_context *qc,
      const std::string &cgi_base)
  {
    std::string result_tmpl_name = "websearch/templates/themes/"
                                   + websearch::_wconfig->_ui_theme + "/seeks_result_template.html"; // XXX: for using this along with the image plugin, we would need to externalize the the theme choice.

    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
    = static_renderer::websearch_exports(csp);

    // query.
    std::string html_encoded_query;
    std::string url_encoded_query;
    static_renderer::render_query(parameters,exports,html_encoded_query,
                                  url_encoded_query);

    // clean query.
    static_renderer::render_clean_query(html_encoded_query,exports);

    // current page.
    int current_page = -1;
    static_renderer::render_current_page(parameters,exports,current_page);

    // suggestions.
    static_renderer::render_suggestions(qc,exports,cgi_base);

    // recommended URLs.
    //static_renderer::render_recommendations(qc,exports,cgi_base);

    // queries in cache.
    static_renderer::render_cached_queries(html_encoded_query,exports,cgi_base);

    // language.
    static_renderer::render_lang(qc,exports);

    // engines.
    std::string engines;
    static_renderer::render_engines(parameters,exports,engines);

    // search snippets.
    static_renderer::render_clustered_snippets(html_encoded_query,
        url_encoded_query,current_page,
        clusters,K,qc,parameters,
        exports);

    // expand button.
    std::string expansion;
    static_renderer::render_expansion(parameters,exports,expansion);

    // next link.
    /* static_renderer::render_next_page_link(current_page,snippets.size(),
    				       html_encoded_query,expansion,exports); */

    // previous link.
    /* static_renderer::render_prev_page_link(current_page,snippets.size(),
    				       html_encoded_query,expansion,exports); */

    // cluster link.
    static_renderer::render_nclusters(parameters,exports);

    // rendering.
    sp_err err = cgi::template_fill_for_cgi(csp,result_tmpl_name.c_str(),
                                            (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
                                                : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
                                            exports,rsp);

    return err;
  }

  // mode: urls = 0, titles = 1.
  sp_err static_renderer::render_neighbors_result_page(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      query_context *qc, const int &mode)
  {
    if (mode > 1)
      return SP_ERR_OK; // wrong mode, do nothing.

    hash_map<uint32_t,search_snippet*,id_hash_uint> hsnippets;
    hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit;
    size_t nsnippets = qc->_cached_snippets.size();

    /**
     * Instanciate an LSH uniform hashtable. Parameters are adhoc, based on experience.
     */
    LSHSystemHamming *lsh_ham = new LSHSystemHamming(7,30);
    LSHUniformHashTableHamming ulsh_ham(lsh_ham,nsnippets);

    for (size_t i=0; i<nsnippets; i++)
      {
        search_snippet *sp = qc->_cached_snippets.at(i);
        if (mode == 0)
          {
            std::string surl = urlmatch::strip_url(sp->_url);
            ulsh_ham.add(surl,lsh_ham->_Ld);
          }
        else if (mode == 1)
          {
            std::string lctitle = sp->_title;
            miscutil::to_lower(lctitle);
            ulsh_ham.add(lctitle,lsh_ham->_Ld);
          }
      }

    int k=nsnippets;
    for (size_t i=0; i<nsnippets; i++)
      {
        search_snippet *sp = qc->_cached_snippets.at(i);
        if ((hit=hsnippets.find(sp->_id))==hsnippets.end())
          {
            sp->_seeks_ir = k--;
            hsnippets.insert(std::pair<uint32_t,search_snippet*>(sp->_id,sp));
          }
        else continue;

        //std::cerr << "original url: " << sp->_url << std::endl;

        /**
         * get similar results. Right now we're not using the probability, but we could
         * in the future, e.g. to prune some outliers.
         */
        std::map<double,const std::string,std::greater<double> > mres;
        if (mode == 0)
          {
            std::string surl = urlmatch::strip_url(sp->_url);
            mres = ulsh_ham.getLEltsWithProbabilities(surl,lsh_ham->_Ld);
          }
        else if (mode == 1)
          {
            std::string lctitle = sp->_title;
            miscutil::to_lower(lctitle);
            mres = ulsh_ham.getLEltsWithProbabilities(lctitle,lsh_ham->_Ld);
          }
        std::map<double,const std::string,std::greater<double> >::const_iterator mit = mres.begin();
        while (mit!=mres.end())
          {
            //std::cerr << "lsh url: " << (*mit).second << " -- prob: " << (*mit).first << std::endl;

            search_snippet *sp2 = NULL;
            if (mode == 0)
              sp2 = qc->get_cached_snippet((*mit).second);
            else if (mode == 1)
              sp2 = qc->get_cached_snippet_title((*mit).second.c_str());

            if ((hit=hsnippets.find(sp2->_id))==hsnippets.end())
              {
                sp2->_seeks_ir = k--;
                hsnippets.insert(std::pair<uint32_t,search_snippet*>(sp2->_id,sp2));
              }
            ++mit;
          }
      }
    std::vector<search_snippet*> snippets;
    snippets.reserve(hsnippets.size());
    hit = hsnippets.begin();
    while (hit!=hsnippets.end())
      {
        snippets.push_back((*hit).second);
        ++hit;
      }
    std::sort(snippets.begin(),snippets.end(),search_snippet::max_seeks_ir);

    // destroy lsh db.
    delete lsh_ham;

    // static rendering.
    return static_renderer::render_result_page_static(snippets,csp,rsp,parameters,qc);
  }

} /* end of namespace. */
