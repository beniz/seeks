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

#include "search_snippet.h"
#include "mem_utils.h"
#include "miscutil.h"
#include "encode.h"
#include "loaders.h"
#include "urlmatch.h"
#include "plugin_manager.h" // for _plugin_repository.
#include "seeks_proxy.h" // for _datadir.
#include "json_renderer.h"

#ifndef FEATURE_EXTENDED_HOST_PATTERNS
#include "proxy_dts.h" // for http_request.
#endif

#include "mrf.h"
#if defined(PROTOBUF) && defined(TC)
#include "query_capture_configuration.h"
#endif
#include "static_renderer.h"

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
#include "img_search_snippet.h" // for seeks_img_doc_type.
#endif

#ifdef FEATURE_XSLSERIALIZER_PLUGIN
#include "xml_renderer.h"
#endif

#include <ctype.h>
#include <iostream>

using sp::miscutil;
using sp::encode;
using sp::loaders;
using sp::urlmatch;
using sp::plugin_manager;
using sp::seeks_proxy;
using sp::http_request;
using lsh::mrf;

namespace seeks_plugins
{
  search_snippet::search_snippet()
    :_qc(NULL),_new(true),_id(0),_doc_type(doc_type::UNKNOWN),_sim_back(false),_rank(0),_seeks_ir(0.0),_meta_rank(0),_seeks_rank(0),
     _content_date(0),_record_date(0),_cached_content(NULL),
     _features(NULL),_features_tfidf(NULL),_bag_of_words(NULL),_personalized(false),_npeers(0),_hits(0),_radius(0),_safe(true)
  {
  }

  search_snippet::search_snippet(const double &rank)
    :_qc(NULL),_new(true),_id(0),_doc_type(doc_type::UNKNOWN),_sim_back(false),_rank(rank),_seeks_ir(0.0),_meta_rank(0),_seeks_rank(0),
     _content_date(0),_record_date(0),_cached_content(NULL),
     _features(NULL),_features_tfidf(NULL),_bag_of_words(NULL),_personalized(false),_npeers(0),_hits(0),_radius(0),_safe(true)
  {
  }

  search_snippet::search_snippet(const search_snippet *s)
    :_qc(s->_qc),_new(s->_new),_id(s->_id),_title(s->_title),_url(s->_url),
     _summary(s->_summary),_summary_noenc(s->_summary_noenc),_lang(s->_lang),_doc_type(s->_doc_type),
     _sim_back(s->_sim_back),_rank(s->_rank),_meta_rank(s->_meta_rank),
     _seeks_rank(s->_seeks_rank),
     _content_date(s->_content_date),_record_date(s->_record_date),
     _engine(s->_engine),
     _cached_content(NULL),
     _features(NULL),_features_tfidf(NULL),_bag_of_words(NULL),_personalized(s->_personalized),
     _npeers(s->_npeers),_hits(s->_hits),_radius(s->_radius),_safe(s->_safe)
  {
    if (s->_cached_content)
      _cached_content = new std::string(*s->_cached_content);
    if (s->_features)
      _features = new std::vector<uint32_t>(*s->_features);
    if (s->_features_tfidf)
      _features_tfidf = new hash_map<uint32_t,float,id_hash_uint>(*s->_features_tfidf);
    if (s->_bag_of_words)
      _bag_of_words = new hash_map<uint32_t,std::string,id_hash_uint>(*s->_bag_of_words);
  }

  search_snippet::~search_snippet()
  {
    if (_cached_content)
      delete _cached_content;
    if (_features)
      delete _features;
    if (_features_tfidf)
      delete _features_tfidf;
    if (_bag_of_words)
      delete _bag_of_words;
  }

  void search_snippet::highlight_query(std::vector<std::string> &words,
                                       std::string &str)
  {
    if (words.empty())
      return;

    // sort words by size.
    std::sort(words.begin(),words.end(),std::greater<std::string>());

    // surround every of those words appearing within the
    // argument string with <b> </b> for html
    // bold format. TODO: green ?
    for (size_t i=0; i<words.size(); i++)
      {
        if (words.at(i).length() > 2)
          {
            std::string bold_str = "<b>" + words.at(i) + "</b>";
            miscutil::ci_replace_in_string(str,words.at(i),bold_str);
          }
      }
  }

  void search_snippet::discr_words(const std::vector<std::string> &query_words,
                                   std::set<std::string> &words) const
  {
    static int max_highlights = 3; // ad-hoc default.

    if (!_features_tfidf)
      return;

    std::map<float,uint32_t,std::greater<float> > f_tfidf;

    // sort features in decreasing tf-idf order.
    hash_map<uint32_t,float,id_hash_uint>::const_iterator fit
    = _features_tfidf->begin();
    while (fit!=_features_tfidf->end())
      {
        f_tfidf.insert(std::pair<float,uint32_t>((*fit).second,(*fit).first));
        ++fit;
      }

    size_t nqw = query_words.size();
    int i = 0;
    std::map<float,uint32_t,std::greater<float> >::const_iterator mit = f_tfidf.begin();
    while (mit!=f_tfidf.end())
      {
        hash_map<uint32_t,std::string,id_hash_uint>::const_iterator bit;
        if ((bit=_bag_of_words->find((*mit).second))!=_bag_of_words->end())
          {
            bool add = true;
            for (size_t j=0; j<nqw; j++)
              if (query_words.at(j) == (*bit).second)
                add = false;
            if (add)
              {
                for (size_t k=0; k<(*bit).second.length(); k++) // alphabetical characters only.
                  if (!isalpha((*bit).second[k]))
                    {
                      add = false;
                      break;
                    }
                if (add)
                  words.insert((*bit).second);
                i++;
              }
            if (i>=max_highlights)
              break;
          }
        ++mit;
      }

    if (words.empty())
      return;

    // sort words by size.
    //std::sort(words.begin(),words.end(),std::greater<std::string>());
  }

  std::string search_snippet::to_html(std::vector<std::string> &words,
                                      const std::string &base_url_str,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    // check for personalization flag.
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
        url = base_url_str + redir + "?q=" + _qc->_url_enc_query + "&amp;url=" + url_enc
              + "&amp;lang=" + _qc->_auto_lang;
      }
#endif

    std::string html_content = "<li class=\"search_snippet\">";
    const char *thumbs = miscutil::lookup(parameters,"thumbs");
    bool has_thumbs = websearch::_wconfig->_thumbs;
    if (has_thumbs || (thumbs && strcasecmp(thumbs,"on") == 0))
      {
        html_content += "<a href=\"" + url + "\">";
        html_content += "<img class=\"preview\" src=\"http://open.thumbshots.org/image.pxf?url=";
        html_content += _url;
        html_content += "\" /></a>";
      }
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
    se_icon += "?q=@query@?page=1&amp;expansion=1&amp;engines=seeng&amp;lang="
               + _qc->_auto_lang + "&amp;ui=stat\">&nbsp;</a></span>";
    if (_engine.has_feed("seeks"))
      {
        std::string sk_se_icon = se_icon;
        miscutil::replace_in_string(sk_se_icon,"icon","search_engine_seeks");
        miscutil::replace_in_string(sk_se_icon,"setitle","Seeks");
        miscutil::replace_in_string(sk_se_icon,"seeng","seeks");
        miscutil::replace_in_string(sk_se_icon,"@query@",_qc->_url_enc_query);
        html_content += sk_se_icon;
      }
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
    char *cite_enc =  encode::html_encode(_url.c_str());
    if (!_summary.empty())
      html_content += "<br>";
    html_content += "<a class=\"search_cite\" href=\"" + _url + "\">";
    html_content += "<cite>";
    html_content += cite_enc;
    free(cite_enc);
    html_content += "</cite></a>";

    std::string sim_link;
    const char *engines = miscutil::lookup(parameters,"engines");
    if (!_sim_back)
      {
        sim_link = "/search?q=" + _qc->_url_enc_query + "&amp;id=" + miscutil::to_string(_id)
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
    if (websearch::_readable_plugin_activated)
      {
        html_content += "<a class=\"search_cache\" href=\""
                        + base_url_str + "/readable?url="
                        + url_enc + "\">Readable</a>";
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

  std::string search_snippet::to_json(const bool &thumbs,
                                      const std::vector<std::string> &query_words)
  {
    return "{" + to_json_str(thumbs,query_words) + "}";
  }

  std::string search_snippet::to_json_str(const bool &thumbs,
                                          const std::vector<std::string> &query_words)
  {
    std::list<std::string> json_elts;
    json_elts.push_back("\"id\":" + miscutil::to_string(_id));
    std::string title = _title;
    miscutil::replace_in_string(title,"\\","\\\\");
    miscutil::replace_in_string(title,"\"","\\\"");
    json_elts.push_back("\"title\":\"" + title + "\"");
    std::string url = _url;
    miscutil::replace_in_string(url,"\"","\\\"");
    miscutil::replace_in_string(url,"\n","");
    json_elts.push_back("\"url\":\"" + url + "\"");
    std::string summary = _summary;
    miscutil::replace_in_string(summary,"\\","\\\\");
    miscutil::replace_in_string(summary,"\"","\\\"");
    json_elts.push_back("\"summary\":\"" + summary + "\"");
    json_elts.push_back("\"seeks_meta\":" + miscutil::to_string(_meta_rank));
    json_elts.push_back("\"seeks_score\":" + miscutil::to_string(_seeks_rank));
    double rank = 0.0;
    if (_engine.size() > 0)
      rank = _rank / static_cast<double>(_engine.size());
    json_elts.push_back("\"rank\":" + miscutil::to_string(rank));
    std::string json_str = "\"engines\":[";
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    img_search_snippet *isp = NULL;
    if (_doc_type == seeks_img_doc_type::IMAGE)
      isp = static_cast<img_search_snippet*>(this);
    if (isp)
      json_str += json_renderer::render_engines(isp->_img_engine,true);
    else
#endif
      json_str += json_renderer::render_engines(_engine);
    json_str += "]";
    json_elts.push_back(json_str);
    if (thumbs)
      json_elts.push_back("\"thumb\":\"http://open.thumbshots.org/image.pxf?url=" + url + "\"");
    std::set<std::string> words;
    discr_words(query_words,words);
    if (!words.empty())
      {
        json_str = "\"words\":[";
        std::list<std::string> json_words;
        std::set<std::string>::const_iterator sit = words.begin();
        while(sit!=words.end())
          {
            json_words.push_back("\"" + (*sit) + "\"");
            ++sit;
          }
        json_str += miscutil::join_string_list(",",json_words);
        json_str += "]";
        json_elts.push_back(json_str);
      }
    json_str = "\"personalized\":\"";
    if (_personalized)
      json_str += "yes";
    else json_str += "no";
    json_str += "\"";
    json_elts.push_back(json_str);
    if (_npeers > 0)
      json_elts.push_back("\"snpeers\":" + miscutil::to_string(_npeers));
    if (_hits > 0)
      json_elts.push_back("\"hits\":" + miscutil::to_string(_hits));
    if (_content_date != 0)
      json_elts.push_back("\"content_date\":" + miscutil::to_string(_content_date));
    if (_record_date != 0)
      json_elts.push_back("\"record_date\":" + miscutil::to_string(_record_date));
    return miscutil::join_string_list(",",json_elts);
  }

#ifdef FEATURE_XSLSERIALIZER_PLUGIN
  sp_err search_snippet::to_xml(const bool &thumbs,
                                const std::vector<std::string> &query_words,
                                xmlNodePtr parent)
  {
    sp_err err=SP_ERR_OK;
    xmlSetProp(parent,BAD_CAST "id",        BAD_CAST (miscutil::to_string(_id).c_str()));
    xmlSetProp(parent,BAD_CAST "title",     BAD_CAST (_title).c_str());
    xmlSetProp(parent,BAD_CAST "url",       BAD_CAST (_url.c_str()));
    xmlSetProp(parent,BAD_CAST "summary",   BAD_CAST (_summary).c_str());
    xmlSetProp(parent,BAD_CAST "seeks_meta", BAD_CAST (miscutil::to_string(_meta_rank).c_str()));
    xmlSetProp(parent,BAD_CAST "seeks_score",BAD_CAST (miscutil::to_string(_seeks_rank).c_str()));
    double rank = 0.0;
    if (_engine.size() > 0)
      rank = _rank / static_cast<double>(_engine.size());
    xmlSetProp(parent,BAD_CAST "rank", BAD_CAST (miscutil::to_string(rank).c_str()));

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    img_search_snippet *isp = NULL;
    if (_doc_type == seeks_img_doc_type::IMAGE)
      isp = static_cast<img_search_snippet*>(this);
    if (isp)
      err=xml_renderer::render_engines(_engine,true,parent);
    else
#endif
      err=xml_renderer::render_engines(_engine,false,parent);
    xmlSetProp(parent,BAD_CAST "type",BAD_CAST get_doc_type_str().c_str());
    xmlSetProp(parent,BAD_CAST "personalized",BAD_CAST (_personalized?"yes":"no"));
    if (_npeers > 0)
      xmlSetProp(parent,BAD_CAST "snpeers", BAD_CAST miscutil::to_string(_npeers).c_str());
    if (_hits > 0)
      xmlSetProp(parent,BAD_CAST "hits", BAD_CAST miscutil::to_string(_hits).c_str());
    return err;
  }
#endif

  std::ostream& search_snippet::print(std::ostream &output)
  {
    output << "-----------------------------------\n";
    output << "- seeks rank: " << _meta_rank << std::endl;
    output << "- rank: " << _rank << std::endl;
    output << "- title: " << _title << std::endl;
    output << "- url: " << _url << std::endl;
    output << "- summary: " << _summary << std::endl;
    output << "- lang: " << _lang << std::endl;
    output << "-----------------------------------\n";
    return output;
  }

  bool search_snippet::is_se_enabled(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    if (_personalized && _engine.has_feed("seeks"))
      return true;
    feeds se_enabled;
    query_context::fillup_engines(parameters,se_enabled);
    feeds band = _engine.inter(se_enabled);

    /*if (band.empty())
      {
        // check for a wildcard (all feeds for a given parser).
        band = _engine.inter_gen(se_enabled);
    	}*/
    return (band.size() != 0);
  }

  void search_snippet::set_title(const std::string &title)
  {
    _title = encode::html_decode(title);
    miscutil::replace_in_string(_title,"\\","");
    miscutil::replace_in_string(_title,"\t"," ");
    miscutil::replace_in_string(_title,"\n"," ");
    miscutil::replace_in_string(_title,"\r"," ");
  }

  void search_snippet::set_title_no_html_decode(const std::string &title)
  {
    _title = title;
    miscutil::replace_in_string(_title,"\\","");
    miscutil::replace_in_string(_title,"\t"," ");
    miscutil::replace_in_string(_title,"\n"," ");
    miscutil::replace_in_string(_title,"\r"," ");
  }

  void search_snippet::set_url(const std::string &url)
  {
    char *url_str = encode::url_decode_but_not_plus(url.c_str());
    _url = std::string(url_str);
    free(url_str);
    std::string url_lc(_url);
    miscutil::to_lower(url_lc);
    std::string surl = urlmatch::strip_url(url_lc);
    _id = mrf::mrf_single_feature(surl);
  }

  void search_snippet::set_url_no_decode(const std::string &url)
  {
    _url = url;
    std::string url_lc(_url);
    miscutil::to_lower(url_lc);
    std::string surl = urlmatch::strip_url(url_lc);
    _id = mrf::mrf_single_feature(surl);
  }

  void search_snippet::set_summary(const std::string &summary)
  {
    static size_t summary_max_size = 240; // characters.
    _summary_noenc = summary;

    // clear escaped characters for unencoded output.
    miscutil::replace_in_string(_summary_noenc,"\\","");

    // encode html so tags are not interpreted.
    char* str = encode::html_encode(summary.c_str());
    if (strlen(str)<summary_max_size)
      _summary = std::string(str);
    else
      {
        try
          {
            _summary = std::string(str).substr(0,summary_max_size-3) + "...";
          }
        catch (std::exception &e)
          {
            _summary = "";
          }
      }
    free(str);
  }

  void search_snippet::set_lang(const std::string &lang)
  {
    _lang = lang;
  }

  void search_snippet::set_radius(const int &radius)
  {
    _radius = radius;
  }

  void search_snippet::set_similarity_link(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    _sim_back = false;
  }

  void search_snippet::set_back_similarity_link(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    _sim_back = true;
  }

  std::string search_snippet::get_stripped_url() const
  {
    std::string url_lc(_url);
    miscutil::to_lower(url_lc);
    std::string surl = urlmatch::strip_url(url_lc);
    return surl;
  }

  std::string search_snippet::get_doc_type_str() const
  {
    if (_doc_type == doc_type::UNKNOWN)
      return "Unknown";
    else if (_doc_type == doc_type::REJECTED)
      return "Rejected";
    else return "";
  }

  bool search_snippet::match_tag(const std::string &url,
                                 const std::vector<url_spec*> &patterns)
  {
    std::string host;
    std::string path;
    urlmatch::parse_url_host_and_path(url,host,path);

    /* std::cerr << "url: " << url << std::endl;
    std::cerr << "[Debug]: host: " << host << " -- path: " << path
      << " -- pattern size: " << patterns.size() << std::endl; */

#ifndef FEATURE_EXTENDED_HOST_PATTERNS
    http_request http;
    http._host = (char*)host.c_str();
    urlmatch::init_domain_components(&http);
#endif

    size_t psize = patterns.size();
    for (size_t i=0; i<psize; i++)
      {
        url_spec *pattern = patterns.at(i);

        // host matching.
#ifdef FEATURE_EXTENDED_HOST_PATTERNS
        int host_match = host.empty() ? 0 : ((NULL == pattern->_host_regex)
                                             || (0 == regexec(pattern->_host_regex, host.c_str(), 0, NULL, 0)));
#else
        int host_match = urlmatch::host_matches(&http,pattern);
#endif
        if (host_match == 0)
          continue;

        // path matching.
        int path_match = urlmatch::path_matches(path.c_str(),pattern);
        if (path_match)
          {
#ifndef FEATURE_EXTENDED_HOST_PATTERNS
            http._host = NULL;
#endif
            return true;
          }
      }
#ifndef FEATURE_EXTENDED_HOST_PATTERNS
    http._host = NULL;
#endif
    return false;
  }

  void search_snippet::merge_snippets(const search_snippet *s2)
  {
    // search engine rank.
    _rank += s2->_rank;

    // search engine.
    _engine = _engine.sunion(s2->_engine);

    // seeks_rank
    _seeks_rank += s2->_seeks_rank;

    // summary.
    if (_summary.length() < s2->_summary.length())
      _summary = s2->_summary;

    // meta rank.
    _meta_rank = _engine.size();

    // radius.
    _radius = std::min(_radius,s2->_radius);

    // record date.
    _record_date = std::max(_record_date,s2->_record_date);

    // content date.
    _content_date = std::max(_content_date,s2->_content_date);
  }

  void search_snippet::reset_p2p_data()
  {
    if (_engine.has_feed("seeks"))
      _engine.remove_feed("seeks");
    _meta_rank = _engine.size();
    _seeks_rank = 0;
    _npeers = 0;
    _hits = 0;
  }

} /* end of namespace. */
