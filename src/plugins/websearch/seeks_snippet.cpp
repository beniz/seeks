/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazera@seeks-project.info>
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

#include "seeks_snippet.h"
#include "mem_utils.h"
#include "miscutil.h"
#include "encode.h"
#include "loaders.h"
#include "urlmatch.h"
#include "plugin_manager.h" // for _plugin_repository.
#include "seeks_proxy.h" // for _datadir.
#include "json_renderer.h"

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
#include "img_search_snippet.h"
#endif

#if defined(PROTOBUF) && defined(TC)
#include "query_capture_configuration.h"
#endif

#include "static_renderer.h"

using sp::miscutil;
using sp::encode;
using sp::loaders;
using sp::urlmatch;
using sp::plugin_manager;
using sp::seeks_proxy;

namespace seeks_plugins
{

  // loaded tagging patterns.
  std::vector<url_spec*> seeks_snippet::_pdf_pos_patterns = std::vector<url_spec*>();
  std::vector<url_spec*> seeks_snippet::_file_doc_pos_patterns = std::vector<url_spec*>();
  std::vector<url_spec*> seeks_snippet::_audio_pos_patterns = std::vector<url_spec*>();
  std::vector<url_spec*> seeks_snippet::_video_pos_patterns = std::vector<url_spec*>();
  std::vector<url_spec*> seeks_snippet::_forum_pos_patterns = std::vector<url_spec*>();
  std::vector<url_spec*> seeks_snippet::_reject_pos_patterns = std::vector<url_spec*>();

  seeks_snippet::seeks_snippet()
    :search_snippet()
  {
    _doc_type = seeks_doc_type::WEBPAGE;
  }

  seeks_snippet::seeks_snippet(const double &rank)
    :search_snippet(rank)
  {
    _doc_type = seeks_doc_type::WEBPAGE;
  }

  seeks_snippet::seeks_snippet(const seeks_snippet *s)
    :search_snippet(s),_cite(s->_cite),_cached(s->_cached),_file_format(s->_file_format),
     _date(s->_date),_archive(s->_archive),_forum_thread_info(s->_forum_thread_info)
  {
    _doc_type = seeks_doc_type::WEBPAGE;
  }

  seeks_snippet::~seeks_snippet()
  {
  }

  std::string seeks_snippet::to_html(std::vector<std::string> &words,
                                     const std::string &base_url_str,
                                     const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    // image snippet cast.
    img_search_snippet *isp = NULL;
    if (_doc_type == seeks_img_doc_type::IMAGE)
      {
        isp = static_cast<img_search_snippet*>(this);
      }
#endif

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

    std::string url = _url;
    char *url_encp = encode::url_encode(url.c_str());
    std::string url_enc = std::string(url_encp);
    free(url_encp);

#if defined(PROTOBUF) && defined(TC)
    if (prs && websearch::_qc_plugin && websearch::_qc_plugin_activated
        && query_capture_configuration::_config)
      {
        std::string redir = "/qc_redir";
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
        if (_doc_type == seeks_img_doc_type::IMAGE)
          redir += "_img";
#endif
        url = base_url_str + redir + "?q=" + _qc->_url_enc_query + "&amp;url=" + url_enc
              + "&amp;lang=" + _qc->_auto_lang;
      }
#endif

    std::string html_content = "<li class=\"search_snippet";
    if (_doc_type == seeks_doc_type::VIDEO_THUMB)
      html_content += " search_snippet_vid";
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    else if (_doc_type == seeks_img_doc_type::IMAGE)
      html_content += " search_snippet_img";
#endif
    html_content += "\">";
    const char *thumbs = miscutil::lookup(parameters,"thumbs");
    bool has_thumbs = websearch::_wconfig->_thumbs;
    if (thumbs && strcasecmp(thumbs,"on") == 0)
      has_thumbs = true;
    if (_doc_type != seeks_doc_type::TWEET
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
        && _doc_type != seeks_img_doc_type::IMAGE
#endif
        && _doc_type != seeks_doc_type::VIDEO_THUMB && has_thumbs)
      {
        html_content += "<a href=\"" + url + "\">";
        html_content += "<img class=\"preview\" src=\"http://open.thumbshots.org/image.pxf?url=";
        html_content += _url;
        html_content += "\" /></a>";
      }
    if (_doc_type == seeks_doc_type::TWEET)
      {
        html_content += "<a href=\"" + _cite + "\">";
        html_content += "<img class=\"tweet_profile\" src=\"" + _cached + "\" ></a>"; // _cached contains the profile's image.
      }
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    if (_doc_type == seeks_doc_type::VIDEO_THUMB)
      {
        html_content += "<a href=\"";
        html_content += url + "\"><img class=\"video_profile\" src=\"";
        html_content += _cached;
        html_content += "\"></a><div>";
      }
#endif
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    else if (_doc_type == seeks_img_doc_type::IMAGE)
      {
        html_content += "<a href=\"";
        html_content += url + "\"><img src=\"";
        html_content += _cached;
        html_content += "\"></a><div>";
      }
#endif

    if (prs && _personalized && !_engine.has_feed("seeks"))
      {
        html_content += "<h3 class=\"personalized_result personalized\" title=\"personalized result\">";
      }
    else html_content += "<h3>";
    html_content += "<a href=\"";
    html_content += url;
    html_content += "\">";

    char* title_enc = encode::html_encode(_title.c_str());
    html_content += std::string(title_enc);
    free(title_enc);
    html_content += "</a>";

    std::string se_icon = "<span class=\"search_engine icon\" title=\"setitle\"><a href=\"" + base_url_str
                          + "/search";
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    if (_doc_type == seeks_img_doc_type::IMAGE)
      se_icon += "_img";
#endif
    se_icon += "?q=@query@?page=1&amp;expansion=1&amp;engines=seeng&amp;lang="
               + _qc->_auto_lang + "&amp;ui=stat\">&nbsp;</a></span>";
    if (_engine.has_feed("google"))
      {
        std::string ggle_se_icon = se_icon;
        miscutil::replace_in_string(ggle_se_icon,"icon","search_engine_google");
        miscutil::replace_in_string(ggle_se_icon,"setitle","Google");
        miscutil::replace_in_string(ggle_se_icon,"seeng","google");
        miscutil::replace_in_string(ggle_se_icon,"@query@",_qc->_url_enc_query);
        html_content += ggle_se_icon;
      }
    if (_engine.has_feed("bing"))
      {
        std::string bing_se_icon = se_icon;
        miscutil::replace_in_string(bing_se_icon,"icon","search_engine_bing");
        miscutil::replace_in_string(bing_se_icon,"setitle","Bing");
        miscutil::replace_in_string(bing_se_icon,"seeng","bing");
        miscutil::replace_in_string(bing_se_icon,"@query@",_qc->_url_enc_query);
        html_content += bing_se_icon;
      }
    if (_engine.has_feed("blekko"))
      {
        std::string blekko_se_icon = se_icon;
        miscutil::replace_in_string(blekko_se_icon,"icon","search_engine_blekko");
        miscutil::replace_in_string(blekko_se_icon,"setitle","blekko!");
        miscutil::replace_in_string(blekko_se_icon,"seeng","blekko");
        miscutil::replace_in_string(blekko_se_icon,"@query@",_qc->_url_enc_query);
        html_content += blekko_se_icon;
      }
    if (_engine.has_feed("yauba"))
      {
        std::string yauba_se_icon = se_icon;
        miscutil::replace_in_string(yauba_se_icon,"icon","search_engine_yauba");
        miscutil::replace_in_string(yauba_se_icon,"setitle","yauba!");
        miscutil::replace_in_string(yauba_se_icon,"seeng","yauba");
        miscutil::replace_in_string(yauba_se_icon,"@query@",_qc->_url_enc_query);
        html_content += yauba_se_icon;
      }
    if (_engine.has_feed("yahoo"))
      {
        std::string yahoo_se_icon = se_icon;
        miscutil::replace_in_string(yahoo_se_icon,"icon","search_engine_yahoo");
        miscutil::replace_in_string(yahoo_se_icon,"setitle","Yahoo!");
        miscutil::replace_in_string(yahoo_se_icon,"seeng","yahoo");
        miscutil::replace_in_string(yahoo_se_icon,"@query@",_qc->_url_enc_query);
        html_content += yahoo_se_icon;
      }
    if (_engine.has_feed("exalead"))
      {
        std::string exalead_se_icon = se_icon;
        miscutil::replace_in_string(exalead_se_icon,"icon","search_engine_exalead");
        miscutil::replace_in_string(exalead_se_icon,"setitle","Exalead");
        miscutil::replace_in_string(exalead_se_icon,"seeng","exalead");
        miscutil::replace_in_string(exalead_se_icon,"@query@",_qc->_url_enc_query);
        html_content += exalead_se_icon;
      }
    if (_engine.has_feed("twitter"))
      {
        std::string twitter_se_icon = se_icon;
        miscutil::replace_in_string(twitter_se_icon,"icon","search_engine_twitter");
        miscutil::replace_in_string(twitter_se_icon,"setitle","Twitter");
        miscutil::replace_in_string(twitter_se_icon,"seeng","twitter");
        miscutil::replace_in_string(twitter_se_icon,"@query@",_qc->_url_enc_query);
        html_content += twitter_se_icon;
      }
    if (_engine.has_feed("dailymotion"))
      {
        std::string yt_se_icon = se_icon;
        miscutil::replace_in_string(yt_se_icon,"icon","search_engine_dailymotion");
        miscutil::replace_in_string(yt_se_icon,"setitle","Dailymotion");
        miscutil::replace_in_string(yt_se_icon,"seeng","Dailymotion");
        miscutil::replace_in_string(yt_se_icon,"@query@",_qc->_url_enc_query);
        html_content += yt_se_icon;
      }
    if (_engine.has_feed("youtube"))
      {
        std::string yt_se_icon = se_icon;
        miscutil::replace_in_string(yt_se_icon,"icon","search_engine_youtube");
        miscutil::replace_in_string(yt_se_icon,"setitle","Youtube");
        miscutil::replace_in_string(yt_se_icon,"seeng","youtube");
        miscutil::replace_in_string(yt_se_icon,"@query@",_qc->_url_enc_query);
        html_content += yt_se_icon;
      }
    if (_engine.has_feed("dokuwiki"))
      {
        std::string dk_se_icon = se_icon;
        miscutil::replace_in_string(dk_se_icon,"icon","search_engine_dokuwiki");
        miscutil::replace_in_string(dk_se_icon,"setitle","Dokuwiki");
        miscutil::replace_in_string(dk_se_icon,"seeng","dokuwiki");
        miscutil::replace_in_string(dk_se_icon,"@query@",_qc->_url_enc_query);
        html_content += dk_se_icon;
      }
    if (_engine.has_feed("mediawiki"))
      {
        std::string md_se_icon = se_icon;
        miscutil::replace_in_string(md_se_icon,"icon","search_engine_mediawiki");
        miscutil::replace_in_string(md_se_icon,"setitle","Mediawiki");
        miscutil::replace_in_string(md_se_icon,"seeng","mediawiki");
        miscutil::replace_in_string(md_se_icon,"@query@",_qc->_url_enc_query);
        html_content += md_se_icon;
      }
    if (_engine.has_feed("opensearch_rss") || _engine.has_feed("opensearch_atom"))
      {
        std::string md_se_icon = se_icon;
        miscutil::replace_in_string(md_se_icon,"icon","search_engine_opensearch");
        miscutil::replace_in_string(md_se_icon,"setitle","Opensearch");
        miscutil::replace_in_string(md_se_icon,"seeng","opensearch");
        miscutil::replace_in_string(md_se_icon,"@query@",_qc->_url_enc_query);
        html_content += md_se_icon;
      }
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    if (isp)
      {
        if (isp->_img_engine.has_feed("bing_img"))
          {
            std::string sk_se_icon = se_icon;
            miscutil::replace_in_string(sk_se_icon,"icon","search_engine_bing");
            miscutil::replace_in_string(sk_se_icon,"setitle","Bing");
            miscutil::replace_in_string(sk_se_icon,"seeng","bing");
            miscutil::replace_in_string(sk_se_icon,"@query@",_qc->_url_enc_query);
            html_content += sk_se_icon;
          }
        if (isp->_img_engine.has_feed("flickr"))
          {
            std::string sk_se_icon = se_icon;
            miscutil::replace_in_string(sk_se_icon,"icon","search_engine_flickr");
            miscutil::replace_in_string(sk_se_icon,"setitle","Flickr");
            miscutil::replace_in_string(sk_se_icon,"seeng","flickr");
            miscutil::replace_in_string(sk_se_icon,"@query@",_qc->_url_enc_query);
            html_content += sk_se_icon;
          }
        if (isp->_img_engine.has_feed("google_img"))
          {
            std::string sk_se_icon = se_icon;
            miscutil::replace_in_string(sk_se_icon,"icon","search_engine_ggle");
            miscutil::replace_in_string(sk_se_icon,"setitle","Google");
            miscutil::replace_in_string(sk_se_icon,"seeng","google");
            miscutil::replace_in_string(sk_se_icon,"@query@",_qc->_url_enc_query);
            html_content += sk_se_icon;
          }
        if (isp->_img_engine.has_feed("wcommons"))
          {
            std::string sk_se_icon = se_icon;
            miscutil::replace_in_string(sk_se_icon,"icon","search_engine_wcommons");
            miscutil::replace_in_string(sk_se_icon,"setitle","WikiCommons");
            miscutil::replace_in_string(sk_se_icon,"seeng","wiki commons");
            miscutil::replace_in_string(sk_se_icon,"@query@",_qc->_url_enc_query);
            html_content += sk_se_icon;
          }
        if (isp->_img_engine.has_feed("yahoo_img"))
          {
            std::string sk_se_icon = se_icon;
            miscutil::replace_in_string(sk_se_icon,"icon","search_engine_yahoo");
            miscutil::replace_in_string(sk_se_icon,"setitle","Yahoo");
            miscutil::replace_in_string(sk_se_icon,"seeng","yahoo");
            miscutil::replace_in_string(sk_se_icon,"@query@",_qc->_url_enc_query);
            html_content += sk_se_icon;
          }
      } // end image sp.
#endif
    if (_engine.has_feed("seeks"))
      {
        std::string sk_se_icon = se_icon;
        miscutil::replace_in_string(sk_se_icon,"icon","search_engine_seeks");
        miscutil::replace_in_string(sk_se_icon,"setitle","Seeks");
        miscutil::replace_in_string(sk_se_icon,"seeng","seeks");
        miscutil::replace_in_string(sk_se_icon,"@query@",_qc->_url_enc_query);
        html_content += sk_se_icon;
      }

    if (_doc_type == seeks_doc_type::TWEET)
      if (_meta_rank > 1)
        html_content += " (" + miscutil::to_string(_meta_rank) + ")";
    html_content += "</h3>";

    if (!_summary.empty())
      {
        html_content += "<div>";
        std::string summary = _summary;
        search_snippet::highlight_query(words,summary);
        if (websearch::_wconfig->_extended_highlight)
          static_renderer::highlight_discr(this,summary,base_url_str,words);
        html_content += summary;
      }
    else html_content += "<div>";

    const char *cite_enc = NULL;
    if (_doc_type != seeks_doc_type::VIDEO_THUMB)
      {
        if (!_cite.empty())
          {
            cite_enc = encode::html_encode(_cite.c_str());
          }
        else
          {
            cite_enc = encode::html_encode(_url.c_str());
          }
      }
    else
      {
        cite_enc = encode::html_encode(_date.c_str());
      }
    if (!_summary.empty())
      html_content += "<br>";
    html_content += "<a class=\"search_cite\" href=\"" + _url + "\">";
    html_content += "<cite>";
    html_content += cite_enc;
    free_const(cite_enc);
    html_content += "</cite></a>";

    if (!_cached.empty() && _doc_type != seeks_doc_type::TWEET && _doc_type != seeks_doc_type::VIDEO_THUMB)
      {
        html_content += "\n";
        char *enc_cached = encode::html_encode(_cached.c_str());
        miscutil::chomp(enc_cached);
        html_content += "<a class=\"search_cache\" href=\"";
        html_content += enc_cached;
        html_content += "\">Cached</a>";
        free_const(enc_cached);
      }
    else if (_doc_type == seeks_doc_type::TWEET)
      {
        char *date_enc = encode::html_encode(_date.c_str());
        html_content += "<date> (";
        html_content += date_enc;
        free_const(date_enc);
        html_content += ") </date>\n";
      }
    if (_doc_type != seeks_doc_type::TWEET && _doc_type != seeks_doc_type::VIDEO_THUMB)
      {
        if (_archive.empty())
          {
            set_archive_link();
          }
        html_content += "<a class=\"search_cache\" href=\"";
        html_content += _archive;
        html_content += "\">Archive</a>";
      }

    if (_doc_type != seeks_doc_type::VIDEO_THUMB)
      {
        std::string sim_link;
        const char *engines = miscutil::lookup(parameters,"engines");
        if (!_sim_back)
          {
            sim_link = "/search";
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
            if (_doc_type == seeks_img_doc_type::IMAGE)
              sim_link += "_img";
#endif
            sim_link += "?q=" + _qc->_url_enc_query + "&amp;id=" + miscutil::to_string(_id)
                        + "&amp;page=1&amp;expansion=" + miscutil::to_string(_qc->_page_expansion)
                        + "&amp;lang=" + _qc->_auto_lang
                        + "&amp;ui=stat&amp;action=similarity";
            if (engines)
              sim_link += "&amp;engines=" + std::string(engines);
            set_similarity_link(parameters);
            html_content += "<a class=\"search_cache\" href=\"";
          }
        else
          {
            sim_link = "/search";
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
            if (_doc_type == seeks_img_doc_type::IMAGE)
              sim_link += "_img";
#endif
            sim_link += "?q=" + _qc->_url_enc_query
                        + "&amp;page=1&amp;expansion=" + miscutil::to_string(_qc->_page_expansion)
                        + "&amp;lang=" + _qc->_auto_lang
                        + "&amp;ui=stat&amp;action=expand";
            if (engines)
              sim_link += "&amp;engines=" + std::string(engines);
            set_back_similarity_link(parameters);
            html_content += "<a class=\"search_similarity\" href=\"";
          }
        html_content += base_url_str + sim_link;
        if (!_sim_back)
          html_content += "\">Similar</a>";
        else html_content += "\">Back</a>";
      }

    if (_cached_content)
      {
        html_content += "<a class=\"search_cache\" href=\"";
        html_content += base_url_str + "/search_cache?url="
                        + _url + "&amp;q=" + _qc->_query;
        html_content += " \">Quick link</a>";
      }

    // snippet type rendering
    const char *engines = miscutil::lookup(parameters,"engines");
    if (_doc_type != seeks_doc_type::REJECTED)
      {
        html_content += "<a class=\"search_cache\" href=\"";
        html_content += base_url_str + "/cluster/types/" + _qc->_url_enc_query
                        + "?expansion=xxexp&amp;ui=stat&amp;engines=";
        if (engines)
          html_content += std::string(engines);
        html_content += " \"> ";
        switch (_doc_type)
          {
          case seeks_doc_type::UNKNOWN:
            html_content += "";
            break;
          case seeks_doc_type::WEBPAGE:
            html_content += "Webpage";
            break;
          case seeks_doc_type::FORUM:
            html_content += "Forum";
            break;
          case seeks_doc_type::FILE_DOC:
            html_content += "Document file";
            break;
          case seeks_doc_type::SOFTWARE:
            html_content += "Software";
            break;
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
          case seeks_img_doc_type::IMAGE:
            html_content += "Image";
            break;
#endif
          case seeks_doc_type::VIDEO:
            html_content += "Video";
            break;
          case seeks_doc_type::VIDEO_THUMB:
            html_content += "Video";
            break;
          case seeks_doc_type::AUDIO:
            html_content += "Audio";
            break;
          case seeks_doc_type::CODE:
            html_content += "Code";
            break;
          case seeks_doc_type::NEWS:
            html_content += "News";
            break;
          case seeks_doc_type::TWEET:
            html_content += "Tweet";
            break;
          case seeks_doc_type::WIKI:
            html_content += "Wiki";
            break;
          case seeks_doc_type::POST:
            html_content += "Post";
            break;
          case seeks_doc_type::BUG:
            html_content += "Bug";
            break;
          case seeks_doc_type::ISSUE:
            html_content += "Issue";
            break;
          case seeks_doc_type::REVISION:
            html_content += "Revision";
            break;
          case seeks_doc_type::COMMENT:
            html_content += "Comment";
            break;
          case seeks_doc_type::REJECTED:
            break;
          }
        html_content += "</a>";
        if (websearch::_readable_plugin_activated)
          {
            html_content += "<a class=\"search_cache\" href=\""
                            + base_url_str + "/readable?url="
                            + url_enc + "&amp;output=html\">Readable</a>";
          }
      }

#if defined(PROTOBUF) && defined(TC)
    // snippet thumb down rendering
    if (_personalized)
      {
        html_content += "<a class=\"search_tbd\" title=\"reject personalized result\" href=\"" + base_url_str + "/tbd?q="
                        + _qc->_url_enc_query + "&amp;url=" + url_enc + "&amp;action=expand&amp;expansion=xxexp&amp;ui=stat&amp;engines=";
        if (engines)
          html_content += std::string(engines);
        html_content += "&amp;lang=" + _qc->_auto_lang;
        html_content += "\">&nbsp;</a>";
        if (_hits > 0 && _npeers > 0)
          html_content += "<br><div class=\"snippet_info\">" + miscutil::to_string(_hits)
                          + " recommendation(s) by " + miscutil::to_string(_npeers) + " peer(s).</div>";
      }
#endif

    html_content += "</div></li>\n";
    return html_content;
  }

  std::string seeks_snippet::to_json(const bool &thumbs,
                                     const std::vector<std::string> &query_words)
  {
    std::string json_str = to_json_str(thumbs,query_words); // call in parent class.
    std::list<std::string> json_elts;
    std::string elt = "\"cite\":\"";
    if (!_cite.empty())
      {
        std::string cite = _cite;
        miscutil::replace_in_string(cite,"\"","\\\"");
        miscutil::replace_in_string(cite,"\n","");
        json_elts.push_back(elt + cite + "\"");
      }
    else
      {
        std::string url = _url;
        miscutil::replace_in_string(url,"\"","\\\"");
        miscutil::replace_in_string(url,"\n","");
        json_elts.push_back(elt + url + "\"");
      }
    if (!_cached.empty())
      {
        std::string cached = _cached;
        miscutil::replace_in_string(cached,"\"","\\\"");
        json_elts.push_back("\"cached\":\"" + cached + "\"");
      }
    if (thumbs)
      {
        std::string url = _url;
        miscutil::replace_in_string(url,"\"","\\\"");
        miscutil::replace_in_string(url,"\n","");
        json_elts.push_back("\"thumb\":\"http://open.thumbshots.org/image.pxf?url=" + url + "\"");
      }
    json_elts.push_back("\"type\":\"" + get_doc_type_str() + "\"");
    if (!_date.empty())
      json_elts.push_back("\"date\":\"" + _date + "\"");
    json_str += "," + miscutil::join_string_list(",",json_elts);
    return "{" + json_str + "}";
  }

#ifdef FEATURE_XSLSERIALIZER_PLUGIN
  sp_err seeks_snippet::to_xml(const bool &thumbs,
                               const std::vector<std::string> &query_words,
                               xmlNodePtr parent)
  {
    search_snippet::to_xml(thumbs,query_words,parent);
    xmlSetProp(parent,BAD_CAST "cite", BAD_CAST (_cite.empty()?_url:_cite).c_str());
    if (!_cached.empty())
      {
        xmlSetProp(parent,BAD_CAST "cached",BAD_CAST (_cached).c_str());
      }
    if (thumbs)
      xmlSetProp(parent,BAD_CAST "thumb",BAD_CAST sprintf(NULL,"http://open.thumbshots.org/image.pxf?url=%s",_url.c_str()));

    std::set<std::string> words;
    discr_words(query_words,words);
    if (!words.empty())
      {
        std::set<std::string>::const_iterator sit = words.begin();
        while(sit!=words.end())
          {
            xmlNewTextChild(parent, NULL, BAD_CAST "word", BAD_CAST (*sit).c_str());
            ++sit;
          }
      }
    if (!_date.empty())
      xmlSetProp(parent,BAD_CAST "date", BAD_CAST (_date).c_str());
    return SP_ERR_OK;
  }
#endif

  std::ostream& seeks_snippet::print(std::ostream &output)
  {
    output << "-----------------------------------\n";
    output << "- seeks rank: " << _meta_rank << std::endl;
    output << "- rank: " << _rank << std::endl;
    output << "- title: " << _title << std::endl;
    output << "- url: " << _url << std::endl;
    output << "- cite: " << _cite << std::endl;
    output << "- cached: " << _cached << std::endl;
    output << "- summary: " << _summary << std::endl;
    output << "- file format: " << _file_format << std::endl;
    output << "- date: " << _date << std::endl;
    output << "- lang: " << _lang << std::endl;
    if (_doc_type == seeks_doc_type::FORUM)
      output << "- forum thread info: " << _forum_thread_info << std::endl;
    output << "-----------------------------------\n";
    return output;
  }

  void seeks_snippet::set_date(const std::string &date)
  {
    size_t p = date.find("+");
    if (p != std::string::npos)
      {
        _date = date.substr(0,p-1);
      }
    else _date = date;
  }

  void seeks_snippet::set_cite(const std::string &cite)
  {
    char *cite_dec = encode::url_decode_but_not_plus(cite.c_str());
    std::string citer = std::string(cite_dec);
    free(cite_dec);
    static size_t cite_max_size = 60;
    _cite = urlmatch::strip_url(citer);
    if (_cite.length()>cite_max_size)
      {
        try
          {
            _cite.substr(0,cite_max_size-3) + "...";
          }
        catch (std::exception &e)
          {
            // do nothing.
          }
      }
  }

  void seeks_snippet::set_cite_no_decode(const std::string &cite)
  {
    static size_t cite_max_size = 60;
    _cite = urlmatch::strip_url(cite);
    if (_cite.length()>cite_max_size)
      {
        try
          {
            _cite.substr(0,cite_max_size-3) + "...";
          }
        catch (std::exception &e)
          {
            // do nothing.
          }
      }
  }

  void seeks_snippet::set_archive_link()
  {
    _archive = "http://web.archive.org/web/*/" + _url;
  }

  void seeks_snippet::tag()
  {
    // detect extension, if any, and if not already tagged.
    if (_doc_type == seeks_doc_type::WEBPAGE) // not already tagged.
      {
        // grab the 3 char long extension, if any.
        std::string file_ext;
        if (_url.size()>4 && _url[_url.size()-4] == '.')
          {
            try
              {
                file_ext = _url.substr(_url.size()-3);
              }
            catch (std::exception &e)
              {
                file_ext = "";
              }
            _file_format = file_ext;
          }

        if (seeks_snippet::match_tag(_url,seeks_snippet::_pdf_pos_patterns))
          _doc_type = seeks_doc_type::FILE_DOC;
        else if (seeks_snippet::match_tag(_url,seeks_snippet::_file_doc_pos_patterns))
          _doc_type = seeks_doc_type::FILE_DOC;
        else if (seeks_snippet::match_tag(_url,seeks_snippet::_audio_pos_patterns))
          _doc_type = seeks_doc_type::AUDIO;
        else if (seeks_snippet::match_tag(_url,seeks_snippet::_video_pos_patterns))
          _doc_type = seeks_doc_type::VIDEO;
        else if (seeks_snippet::match_tag(_url,seeks_snippet::_forum_pos_patterns))
          _doc_type = seeks_doc_type::FORUM;
        else if (seeks_snippet::match_tag(_url,seeks_snippet::_reject_pos_patterns))
          _doc_type = seeks_doc_type::REJECTED;

        /* std::cerr << "[Debug]: tagged snippet: url: " << _url
          << " -- tag: " << (int)_doc_type << std::endl; */
      }

    // detect wikis. XXX: could be put into a pattern file if more complex patterns are needed.
    if (_doc_type == seeks_doc_type::WEBPAGE)
      {
        size_t pos = 0;
        std::string wiki_pattern = "wiki";
        std::string::const_iterator sit = _url.begin();
        if ((pos = miscutil::ci_find(_url,wiki_pattern,sit))!=std::string::npos)
          {
            _doc_type = seeks_doc_type::WIKI;
          }
      }
  }

  // static.
  sp_err seeks_snippet::load_patterns()
  {
    static std::string pdf_patterns_filename
    = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/pdf"
      : seeks_proxy::_datadir + "/plugins/websearch/patterns/pdf";
    static std::string file_doc_patterns_filename
    = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/file_doc"
      : seeks_proxy::_datadir + "/plugins/websearch/patterns/file_doc";
    static std::string audio_patterns_filename
    = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/audio"
      : seeks_proxy::_datadir + "/plugins/websearch/patterns/audio";
    static std::string video_patterns_filename
    = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/video"
      : seeks_proxy::_datadir + "/plugins/websearch/patterns/video";
    static std::string forum_patterns_filename
    = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/forum"
      : seeks_proxy::_datadir + "/plugins/websearch/patterns/forum";
    static std::string reject_patterns_filename
    = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/reject"
      : seeks_proxy::_datadir + "/plugins/websearch/patterns/reject";

    std::vector<url_spec*> fake_neg_patterns; // XXX: maybe to be supported in the future, if needed.

    sp_err err;
    err = loaders::load_pattern_file(pdf_patterns_filename.c_str(),seeks_snippet::_pdf_pos_patterns,
                                     fake_neg_patterns);
    if (err == SP_ERR_OK)
      err = loaders::load_pattern_file(file_doc_patterns_filename.c_str(),seeks_snippet::_file_doc_pos_patterns,
                                       fake_neg_patterns);
    if (err == SP_ERR_OK)
      err = loaders::load_pattern_file(audio_patterns_filename.c_str(),seeks_snippet::_audio_pos_patterns,
                                       fake_neg_patterns);
    if (err == SP_ERR_OK)
      err = loaders::load_pattern_file(video_patterns_filename.c_str(),seeks_snippet::_video_pos_patterns,
                                       fake_neg_patterns);
    if (err == SP_ERR_OK)
      err = loaders::load_pattern_file(forum_patterns_filename.c_str(),seeks_snippet::_forum_pos_patterns,
                                       fake_neg_patterns);
    if (err == SP_ERR_OK)
      err = loaders::load_pattern_file(reject_patterns_filename.c_str(),seeks_snippet::_reject_pos_patterns,
                                       fake_neg_patterns);
    return err;
  }

  void seeks_snippet::destroy_patterns()
  {
    std::for_each(_pdf_pos_patterns.begin(),_pdf_pos_patterns.end(),delete_object());
    std::for_each(_file_doc_pos_patterns.begin(),_file_doc_pos_patterns.end(),delete_object());
    std::for_each(_audio_pos_patterns.begin(),_audio_pos_patterns.end(),delete_object());
    std::for_each(_video_pos_patterns.begin(),_video_pos_patterns.end(),delete_object());
    std::for_each(_forum_pos_patterns.begin(),_forum_pos_patterns.end(),delete_object());
    std::for_each(_reject_pos_patterns.begin(),_reject_pos_patterns.end(),delete_object());
  }

  void seeks_snippet::merge_snippets(const search_snippet *s2)
  {
    if (_doc_type != seeks_doc_type::TWEET)
      {
        if (_engine.equal(s2->_engine))
          return;
      }

    search_snippet::merge_snippets(s2);

    const seeks_snippet *st2 = dynamic_cast<const seeks_snippet*>(s2);
    if (!st2)
      return;

    // cached link.
    if (_cached.empty())
      _cached = st2->_cached;

    // cite.
    if (_cite.length() > st2->_cite.length())
      _cite = st2->_cite;

    // snippet type: more specialize type wins.
    // for now, very basic.
    _doc_type = std::max(_doc_type,st2->_doc_type);

    // file format.
    if (_file_format.length() < st2->_file_format.length())  // we could do better here, ok enough for now.
      _file_format = st2->_file_format;

    /*if (s1->_doc_type == seeks_doc_type::TWEET)
      {
    if (s1->_meta_rank <= 0)
          s1->_meta_rank++;
        s1->_meta_rank++; // similarity detects retweets and merges them.
    }*/
    bing_yahoo_us_merge();
  }

  void seeks_snippet::bing_yahoo_us_merge()
  {
    // XXX: hack, on English queries, Bing & Yahoo are the same engine,
    // therefore the rank must be tweaked accordingly in this special case.
    if (_qc->_auto_lang == "en"
        && _engine.has_feed("yahoo")
        && _engine.has_feed("bing"))
      _meta_rank--;
  }

  std::string seeks_snippet::get_doc_type_str() const
  {
    std::string output;
    switch (_doc_type)
      {
      case seeks_doc_type::WEBPAGE:
        output = "webpage";
        break;
      case seeks_doc_type::FORUM:
        output = "forum";
        break;
      case seeks_doc_type::FILE_DOC:
        output = "file";
        break;
      case seeks_doc_type::SOFTWARE:
        output = "software";
        break;
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
      case seeks_img_doc_type::IMAGE:
        output = "image";
        break;
#endif
      case seeks_doc_type::VIDEO:
        output = "video";
        break;
      case seeks_doc_type::VIDEO_THUMB:
        output = "video_thumb";
        break;
      case seeks_doc_type::AUDIO:
        output = "audio";
        break;
      case seeks_doc_type::CODE:
        output = "code";
        break;
      case seeks_doc_type::NEWS:
        output = "news";
        break;
      case seeks_doc_type::TWEET:
        output = "tweet";
        break;
      case seeks_doc_type::WIKI:
        output = "wiki";
        break;
      case seeks_doc_type::UNKNOWN:
      default:
        output = "unknown";
      }
    return output;
  }

} /* end of namespace. */
