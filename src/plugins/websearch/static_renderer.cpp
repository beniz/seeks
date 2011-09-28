/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009, 2010 Emmanuel Benazera, juban@free.fr
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

  std::string static_renderer::render_snippet(search_snippet *sp,
      std::vector<std::string> &words,
      const std::string &base_url_str,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    // check for URL redirection for capture & personalization of results.
    bool prs = true;
    const char *pers = miscutil::lookup(parameters,"prs");
    if (!pers)
      prs = websearch::_wconfig->_personalization;
    else
      {
        if (strcasecmp(pers,"on") == 0)
          prs = true;
        else if (strcasecmp(pers,"off") == 0)
          prs = false;
        else prs = websearch::_wconfig->_personalization;
      }

    std::string url = sp->_url;
    char *url_encp = encode::url_encode(url.c_str());
    std::string url_enc = std::string(url_encp);
    free(url_encp);

#if defined(PROTOBUF) && defined(TC)
    if (prs && websearch::_qc_plugin && websearch::_qc_plugin_activated
        && query_capture_configuration::_config)
      {
        url = base_url_str + "/qc_redir?q=" + sp->_qc->_url_enc_query + "&amp;url=" + url_enc
              + "&amp;lang=" + sp->_qc->_auto_lang;
      }
#endif

    std::string html_content = "<li class=\"search_snippet";
    if (sp->_doc_type == VIDEO_THUMB)
      html_content += " search_snippet_vid";
    else if (sp->_doc_type == IMAGE)
      html_content += " search_snippet_img";
    html_content += "\">";
    const char *thumbs = miscutil::lookup(parameters,"thumbs");
    bool has_thumbs = websearch::_wconfig->_thumbs;
    if (thumbs && strcasecmp(thumbs,"on") == 0)
      has_thumbs = true;
    if (sp->_doc_type != TWEET && sp->_doc_type != IMAGE && sp->_doc_type != VIDEO_THUMB && has_thumbs)
      {
        html_content += "<a href=\"" + url + "\">";
        html_content += "<img class=\"preview\" src=\"http://open.thumbshots.org/image.pxf?url=";
        html_content += sp->_url;
        html_content += "\" /></a>";
      }
    if (sp->_doc_type == TWEET)
      {
        html_content += "<a href=\"" + sp->_cite + "\">";
        html_content += "<img class=\"tweet_profile\" src=\"" + sp->_cached + "\" ></a>"; // _cached contains the profile's image.
      }
    if (sp->_doc_type == VIDEO_THUMB)
      {
        html_content += "<a href=\"";
        html_content += url + "\"><img class=\"video_profile\" src=\"";
        html_content += sp->_cached;
        html_content += "\"></a><div>";
      }
    else if (sp->_doc_type == IMAGE)
      {
        html_content += "<a href=\"";
        html_content += url + "\"><img src=\"";
        html_content += sp->_cached;
        html_content += "\"></a><div>";
      }

    if (prs && sp->_personalized && !sp->_engine.has_feed("seeks"))
      {
        html_content += "<h3 class=\"personalized_result personalized\" title=\"personalized result\">";
      }
    else html_content += "<h3>";
    html_content += "<a href=\"";
    html_content += url;
    html_content += "\">";

    std::string title_enc = encode::html_decode(sp->_title);
    html_content += title_enc;
    html_content += "</a>";

    std::string se_icon = "<span class=\"search_engine icon\" title=\"setitle\"><a href=\"" + base_url_str
                          + "/search/q=@query@?page=1&amp;expansion=1&amp;engines=seeng&amp;lang="
                          + sp->_qc->_auto_lang + "&amp;ui=stat\">&nbsp;</a></span>";
    if (sp->_engine.has_feed("google"))
      {
        std::string ggle_se_icon = se_icon;
        miscutil::replace_in_string(ggle_se_icon,"icon","search_engine_google");
        miscutil::replace_in_string(ggle_se_icon,"setitle","Google");
        miscutil::replace_in_string(ggle_se_icon,"seeng","google");
        miscutil::replace_in_string(ggle_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += ggle_se_icon;
      }
    if (sp->_engine.has_feed("bing"))
      {
        std::string bing_se_icon = se_icon;
        miscutil::replace_in_string(bing_se_icon,"icon","search_engine_bing");
        miscutil::replace_in_string(bing_se_icon,"setitle","Bing");
        miscutil::replace_in_string(bing_se_icon,"seeng","bing");
        miscutil::replace_in_string(bing_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += bing_se_icon;
      }
    if (sp->_engine.has_feed("blekko"))
      {
        std::string blekko_se_icon = se_icon;
        miscutil::replace_in_string(blekko_se_icon,"icon","search_engine_blekko");
        miscutil::replace_in_string(blekko_se_icon,"setitle","blekko!");
        miscutil::replace_in_string(blekko_se_icon,"seeng","blekko");
        miscutil::replace_in_string(blekko_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += blekko_se_icon;
      }
    if (sp->_engine.has_feed("yauba"))
      {
        std::string yauba_se_icon = se_icon;
        miscutil::replace_in_string(yauba_se_icon,"icon","search_engine_yauba");
        miscutil::replace_in_string(yauba_se_icon,"setitle","yauba!");
        miscutil::replace_in_string(yauba_se_icon,"seeng","yauba");
        miscutil::replace_in_string(yauba_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += yauba_se_icon;
      }
    if (sp->_engine.has_feed("yahoo"))
      {
        std::string yahoo_se_icon = se_icon;
        miscutil::replace_in_string(yahoo_se_icon,"icon","search_engine_yahoo");
        miscutil::replace_in_string(yahoo_se_icon,"setitle","Yahoo!");
        miscutil::replace_in_string(yahoo_se_icon,"seeng","yahoo");
        miscutil::replace_in_string(yahoo_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += yahoo_se_icon;
      }
    if (sp->_engine.has_feed("exalead"))
      {
        std::string exalead_se_icon = se_icon;
        miscutil::replace_in_string(exalead_se_icon,"icon","search_engine_exalead");
        miscutil::replace_in_string(exalead_se_icon,"setitle","Exalead");
        miscutil::replace_in_string(exalead_se_icon,"seeng","exalead");
        miscutil::replace_in_string(exalead_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += exalead_se_icon;
      }
    if (sp->_engine.has_feed("twitter"))
      {
        std::string twitter_se_icon = se_icon;
        miscutil::replace_in_string(twitter_se_icon,"icon","search_engine_twitter");
        miscutil::replace_in_string(twitter_se_icon,"setitle","Twitter");
        miscutil::replace_in_string(twitter_se_icon,"seeng","twitter");
        miscutil::replace_in_string(twitter_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += twitter_se_icon;
      }
    if (sp->_engine.has_feed("dailymotion"))
      {
        std::string yt_se_icon = se_icon;
        miscutil::replace_in_string(yt_se_icon,"icon","search_engine_dailymotion");
        miscutil::replace_in_string(yt_se_icon,"setitle","Dailymotion");
        miscutil::replace_in_string(yt_se_icon,"seeng","Dailymotion");
        miscutil::replace_in_string(yt_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += yt_se_icon;
      }
    if (sp->_engine.has_feed("youtube"))
      {
        std::string yt_se_icon = se_icon;
        miscutil::replace_in_string(yt_se_icon,"icon","search_engine_youtube");
        miscutil::replace_in_string(yt_se_icon,"setitle","Youtube");
        miscutil::replace_in_string(yt_se_icon,"seeng","youtube");
        miscutil::replace_in_string(yt_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += yt_se_icon;
      }
    if (sp->_engine.has_feed("dokuwiki"))
      {
        std::string dk_se_icon = se_icon;
        miscutil::replace_in_string(dk_se_icon,"icon","search_engine_dokuwiki");
        miscutil::replace_in_string(dk_se_icon,"setitle","Dokuwiki");
        miscutil::replace_in_string(dk_se_icon,"seeng","dokuwiki");
        miscutil::replace_in_string(dk_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += dk_se_icon;
      }
    if (sp->_engine.has_feed("mediawiki"))
      {
        std::string md_se_icon = se_icon;
        miscutil::replace_in_string(md_se_icon,"icon","search_engine_mediawiki");
        miscutil::replace_in_string(md_se_icon,"setitle","Mediawiki");
        miscutil::replace_in_string(md_se_icon,"seeng","mediawiki");
        miscutil::replace_in_string(md_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += md_se_icon;
      }
    if (sp->_engine.has_feed("opensearch_rss") || sp->_engine.has_feed("opensearch_atom"))
      {
        std::string md_se_icon = se_icon;
        miscutil::replace_in_string(md_se_icon,"icon","search_engine_opensearch");
        miscutil::replace_in_string(md_se_icon,"setitle","Opensearch");
        miscutil::replace_in_string(md_se_icon,"seeng","opensearch");
        miscutil::replace_in_string(md_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += md_se_icon;
      }
    if (sp->_engine.has_feed("seeks"))
      {
        std::string sk_se_icon = se_icon;
        miscutil::replace_in_string(sk_se_icon,"icon","search_engine_seeks");
        miscutil::replace_in_string(sk_se_icon,"setitle","Seeks");
        miscutil::replace_in_string(sk_se_icon,"seeng","seeks");
        miscutil::replace_in_string(sk_se_icon,"@query@",sp->_qc->_url_enc_query);
        html_content += sk_se_icon;
      }

    if (sp->_doc_type == TWEET)
      if (sp->_meta_rank > 1)
        html_content += " (" + miscutil::to_string(sp->_meta_rank) + ")";
    html_content += "</h3>";

    if (!sp->_summary.empty())
      {
        html_content += "<div>";
        std::string summary = sp->_summary;
        search_snippet::highlight_query(words,summary);
        if (websearch::_wconfig->_extended_highlight)
          static_renderer::highlight_discr(sp,summary,base_url_str,words);
        html_content += summary;
      }
    else html_content += "<div>";

    const char *cite_enc = NULL;
    if (sp->_doc_type != VIDEO_THUMB)
      {
        if (!sp->_cite.empty())
          {
            cite_enc = encode::html_encode(sp->_cite.c_str());
          }
        else
          {
            cite_enc = encode::html_encode(sp->_url.c_str());
          }
      }
    else
      {
        cite_enc = encode::html_encode(sp->_date.c_str());
      }
    if (!sp->_summary.empty())
      html_content += "<br>";
    html_content += "<a class=\"search_cite\" href=\"" + sp->_url + "\">";
    html_content += "<cite>";
    html_content += cite_enc;
    free_const(cite_enc);
    html_content += "</cite></a>";

    if (!sp->_cached.empty() && sp->_doc_type != TWEET && sp->_doc_type != VIDEO_THUMB)
      {
        html_content += "\n";
        char *enc_cached = encode::html_encode(sp->_cached.c_str());
        miscutil::chomp(enc_cached);
        html_content += "<a class=\"search_cache\" href=\"";
        html_content += enc_cached;
        html_content += "\">Cached</a>";
        free_const(enc_cached);
      }
    else if (sp->_doc_type == TWEET)
      {
        char *date_enc = encode::html_encode(sp->_date.c_str());
        html_content += "<date> (";
        html_content += date_enc;
        free_const(date_enc);
        html_content += ") </date>\n";
      }
    if (sp->_doc_type != TWEET && sp->_doc_type != VIDEO_THUMB)
      {
        if (sp->_archive.empty())
          {
            sp->set_archive_link();
          }
        html_content += "<a class=\"search_cache\" href=\"";
        html_content += sp->_archive;
        html_content += "\">Archive</a>";
      }

    if (sp->_doc_type != VIDEO_THUMB)
      {
        std::string sim_link;
        const char *engines = miscutil::lookup(parameters,"engines");
        if (!sp->_sim_back)
          {
            sim_link = "/search";
            if (sp->_doc_type == IMAGE)
              sim_link += "_img";
            sim_link += "?q=" + sp->_qc->_url_enc_query + "/" + miscutil::to_string(sp->_id)
                        + "?page=1&amp;expansion=" + miscutil::to_string(sp->_qc->_page_expansion)
                        + "&amp;lang=" + sp->_qc->_auto_lang
                        + "&amp;ui=stat&amp;action=similarity";
            if (engines)
              sim_link += "&amp;engines=" + std::string(engines);
            sp->set_similarity_link(parameters);
            html_content += "<a class=\"search_cache\" href=\"";
          }
        else
          {
            sim_link = "/search";
            if (sp->_doc_type == IMAGE)
              sim_link += "_img";
            sim_link += "?q=" + sp->_qc->_url_enc_query
                        + "?page=1&amp;expansion=" + miscutil::to_string(sp->_qc->_page_expansion)
                        + "&amp;lang=" + sp->_qc->_auto_lang
                        + "&amp;ui=stat&amp;action=expand";
            if (engines)
              sim_link += "&amp;engines=" + std::string(engines);
            sp->set_back_similarity_link(parameters);
            html_content += "<a class=\"search_similarity\" href=\"";
          }
        html_content += base_url_str + sim_link;
        if (!sp->_sim_back)
          html_content += "\">Similar</a>";
        else html_content += "\">Back</a>";
      }

    if (sp->_cached_content)
      {
        html_content += "<a class=\"search_cache\" href=\"";
        html_content += base_url_str + "/search_cache?url="
                        + sp->_url + "&amp;q=" + sp->_qc->_query;
        html_content += " \">Quick link</a>";
      }

    // snippet type rendering
    const char *engines = miscutil::lookup(parameters,"engines");
    if (sp->_doc_type != REJECTED)
      {
        html_content += "<a class=\"search_cache\" href=\"";
        html_content += base_url_str + "/cluster/types/" + sp->_qc->_url_enc_query
                        + "?expansion=xxexp&amp;ui=stat&amp;engines=";
        if (engines)
          html_content += std::string(engines);
        html_content += " \"> ";
        switch (sp->_doc_type)
          {
          case UNKNOWN:
            html_content += "";
            break;
          case WEBPAGE:
            html_content += "Webpage";
            break;
          case FORUM:
            html_content += "Forum";
            break;
          case FILE_DOC:
            html_content += "Document file";
            break;
          case SOFTWARE:
            html_content += "Software";
            break;
          case IMAGE:
            html_content += "Image";
            break;
          case VIDEO:
            html_content += "Video";
            break;
          case VIDEO_THUMB:
            html_content += "Video";
            break;
          case AUDIO:
            html_content += "Audio";
            break;
          case CODE:
            html_content += "Code";
            break;
          case NEWS:
            html_content += "News";
            break;
          case TWEET:
            html_content += "Tweet";
            break;
          case WIKI:
            html_content += "Wiki";
            break;
          case POST:
            html_content += "Post";
            break;
          case BUG:
            html_content += "Bug";
            break;
          case ISSUE:
            html_content += "Issue";
            break;
          case REVISION:
            html_content += "Revision";
            break;
          case COMMENT:
            html_content += "Comment";
            break;
          case REJECTED:
            break;
          }
        html_content += "</a>";
      }

#if defined(PROTOBUF) && defined(TC)
    // snippet thumb down rendering
    if (sp->_personalized)
      {
        html_content += "<a class=\"search_tbd\" title=\"reject personalized result\" href=\"" + base_url_str + "/tbd?q="
                        + sp->_qc->_url_enc_query + "&amp;url=" + url_enc + "&amp;action=expand&amp;expansion=xxexp&amp;ui=stat&amp;engines=";
        if (engines)
          html_content += std::string(engines);
        html_content += "&lang=" + sp->_qc->_auto_lang;
        html_content += "\">&nbsp;</a>";
        if (sp->_hits > 0 && sp->_npeers > 0)
          html_content += "<br><div class=\"snippet_info\">" + miscutil::to_string(sp->_hits)
                          + " recommendation(s) by " + miscutil::to_string(sp->_npeers) + " peer(s).</div>";
      }
#endif

    html_content += "</div></li>\n";
    return html_content;
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
        std::string suggestion_str = "Related queries:";

        int k = 0;
        std::multimap<double,std::string,std::less<double> >::const_iterator mit
        = qc->_suggestions.begin();
        while(mit!=qc->_suggestions.end())
          {
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
            if (k > websearch::_wconfig->_num_reco_queries)
              break;
          }
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
        if (k > websearch::_wconfig->_num_reco_queries)
          break;
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
                                        bool &not_end)
  {
    const char *base_url = miscutil::lookup(exports,"base-url");
    std::string base_url_str = "";
    if (base_url)
      base_url_str = std::string(base_url);

    std::vector<std::string> words;
    miscutil::tokenize(query_clean,words," "); // tokenize query before highlighting keywords.

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
            if (snippets.at(i)->_doc_type == REJECTED)
              continue;
            if (!snippets.at(i)->is_se_enabled(parameters))
              continue;
            if (!safesearch_off && !snippets.at(i)->_safe)
              continue;
            if (only_tweets && snippets.at(i)->_doc_type != TWEET)
              only_tweets = false;

            if (!similarity || snippets.at(i)->_seeks_ir > 0)
              {
                if (count >= snistart)
                  snippets_str += static_renderer::render_snippet(snippets.at(i),words,base_url_str,
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
      cluster *clusters,
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

    std::vector<std::string> words;
    miscutil::tokenize(query_clean,words," "); // tokenize query before highlighting keywords.

    // lookup activated engines.
    feeds se_enabled;
    query_context::fillup_engines(parameters,se_enabled);

    // check for empty cluster, and determine which rendering to use.
    short k = 0;
    for (short c=0; c<K; c++)
      {
        short cc = 0;
        if (!clusters[c]._cpoints.empty())
          {
            hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit
            = clusters[c]._cpoints.begin();
            while (hit!=clusters[c]._cpoints.end())
              {
                search_snippet *sp = qc->get_cached_snippet((*hit).first);
                if (sp->_doc_type == REJECTED)
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
    for (short c=0; c<K; c++)
      {
        if (clusters[c]._cpoints.empty())
          {
            continue;
          }

        std::vector<search_snippet*> snippets;
        snippets.reserve(clusters[c]._cpoints.size());
        hash_map<uint32_t,hash_map<uint32_t,float,id_hash_uint>*,id_hash_uint>::const_iterator hit
        = clusters[c]._cpoints.begin();
        while (hit!=clusters[c]._cpoints.end())
          {
            search_snippet *sp = qc->get_cached_snippet((*hit).first);
            if (sp->_doc_type == REJECTED)
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

        std::stable_sort(snippets.begin(),snippets.end(),search_snippet::max_seeks_ir);

        if (!snippets.empty())
          {
            std::string cluster_str;
            if (!clusterize)
              cluster_str = static_renderer::render_cluster_label(clusters[c]);
            else cluster_str = static_renderer::render_cluster_label_query_link(url_encoded_query,
                                 clusters[c],exports);
            size_t nsps = snippets.size();
            for (size_t i=0; i<nsps; i++)
              cluster_str += static_renderer::render_snippet(snippets.at(i),words,base_url,parameters);
            cluster_str += "</ol><div class=\"clear\"></div>";

            std::string cl = rplcnt;
            if (k>1)
              cl += miscutil::to_string(l++);
            miscutil::add_map_entry(exports,cl.c_str(),1,cluster_str.c_str(),1);
          }
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
    else miscutil::add_map_entry(exports,rplcnt.c_str(),1,"",1);

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

  std::string static_renderer::render_cluster_label(const cluster &cl)
  {
    const char *clabel_encoded = encode::html_encode(cl._label.c_str());
    std::string slabel = "(" + miscutil::to_string(cl._cpoints.size()) + ")";
    const char *slabel_encoded = encode::html_encode(slabel.c_str());
    std::string html_label = "<h2>" + std::string(clabel_encoded)
                             + " <font size=\"2\">" + std::string(slabel_encoded) + "</font></h2><br><ol>";
    free_const(clabel_encoded);
    free_const(slabel_encoded);
    return html_label;
  }

  std::string static_renderer::render_cluster_label_query_link(const std::string &url_encoded_query,
      const cluster &cl,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *exports,
      const std::string &cgi_base)
  {
    const char *base_url = miscutil::lookup(exports,"base-url");
    std::string base_url_str = "";
    if (base_url)
      base_url_str = std::string(base_url);
    char *clabel_url_enc = encode::url_encode(cl._label.c_str());
    char *clabel_html_enc = encode::html_encode(cl._label.c_str());
    std::string clabel_url_enc_str = std::string(clabel_url_enc);
    free(clabel_url_enc);
    std::string clabel_html_enc_str = std::string(clabel_html_enc);
    free(clabel_html_enc);

    std::string slabel = "(" + miscutil::to_string(cl._cpoints.size()) + ")";
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
    // the injected header, if it exists is Seeks-Remote-Location
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
      const std::vector<std::pair<std::string,std::string> > *param_exports)
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
    static_renderer::render_snippets(html_encoded_query,current_page,snippets,parameters,exports,not_end);

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

  sp_err static_renderer::render_clustered_result_page_static(cluster *clusters,
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
