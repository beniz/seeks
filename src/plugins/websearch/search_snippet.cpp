/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009-2011 Emmanuel Benazera, juban@free.fr
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
#include "websearch.h" // for configuration.
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
  // loaded tagging patterns.
  std::vector<url_spec*> search_snippet::_pdf_pos_patterns = std::vector<url_spec*>();
  std::vector<url_spec*> search_snippet::_file_doc_pos_patterns = std::vector<url_spec*>();
  std::vector<url_spec*> search_snippet::_audio_pos_patterns = std::vector<url_spec*>();
  std::vector<url_spec*> search_snippet::_video_pos_patterns = std::vector<url_spec*>();
  std::vector<url_spec*> search_snippet::_forum_pos_patterns = std::vector<url_spec*>();
  std::vector<url_spec*> search_snippet::_reject_pos_patterns = std::vector<url_spec*>();

  search_snippet::search_snippet()
    :_qc(NULL),_new(true),_id(0),_sim_back(false),_rank(0),_seeks_ir(0.0),_meta_rank(0),_seeks_rank(0),
     _content_date(0),_record_date(0),_doc_type(WEBPAGE),
     _cached_content(NULL),_features(NULL),_features_tfidf(NULL),_bag_of_words(NULL),_safe(true),_personalized(false),_npeers(0),_hits(0),_radius(0)
  {
  }

  search_snippet::search_snippet(const short &rank)
    :_qc(NULL),_new(true),_id(0),_sim_back(false),_rank(rank),_seeks_ir(0.0),_meta_rank(0),_seeks_rank(0),
     _content_date(0),_record_date(0),_doc_type(WEBPAGE),
     _cached_content(NULL),_features(NULL),_features_tfidf(NULL),_bag_of_words(NULL),_safe(true),_personalized(false),_npeers(0),_hits(0),_radius(0)
  {
  }

  search_snippet::search_snippet(const search_snippet *s)
    :_qc(s->_qc),_new(s->_new),_id(s->_id),_title(s->_title),_url(s->_url),_cite(s->_cite),
     _cached(s->_cached),_summary(s->_summary),_summary_noenc(s->_summary_noenc),
     _file_format(s->_file_format),_date(s->_date),_lang(s->_lang),
     _archive(s->_archive),
     _sim_back(s->_sim_back),_rank(s->_rank),_meta_rank(s->_meta_rank),
     _seeks_rank(s->_seeks_rank),_content_date(s->_content_date),_record_date(s->_record_date),
     _engine(s->_engine),_doc_type(s->_doc_type),
     _forum_thread_info(s->_forum_thread_info),_cached_content(NULL),
     _features(NULL),_features_tfidf(NULL),_bag_of_words(NULL),_safe(s->_safe),_personalized(s->_personalized),
     _npeers(s->_npeers),_hits(s->_hits),_radius(s->_radius)
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

  std::ostream& search_snippet::print(std::ostream &output)
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
    if (_doc_type == FORUM)
      output << "- forum thread info: " << _forum_thread_info << std::endl;
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

  void search_snippet::set_cite(const std::string &cite)
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

  void search_snippet::set_cite_no_decode(const std::string &cite)
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

  void search_snippet::set_date(const std::string &date)
  {
    size_t p = date.find("+");
    if (p != std::string::npos)
      {
        _date = date.substr(0,p-1);
      }
    else _date = date;
  }

  void search_snippet::set_lang(const std::string &lang)
  {
    _lang = lang;
  }

  void search_snippet::set_radius(const int &radius)
  {
    _radius = radius;
  }

  void search_snippet::set_archive_link()
  {
    _archive = "http://web.archive.org/web/*/" + _url;
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

  void search_snippet::tag()
  {
    // detect extension, if any, and if not already tagged.
    if (_doc_type == WEBPAGE) // not already tagged.
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

        if (search_snippet::match_tag(_url,search_snippet::_pdf_pos_patterns))
          _doc_type = FILE_DOC;
        else if (search_snippet::match_tag(_url,search_snippet::_file_doc_pos_patterns))
          _doc_type = FILE_DOC;
        else if (search_snippet::match_tag(_url,search_snippet::_audio_pos_patterns))
          _doc_type = AUDIO;
        else if (search_snippet::match_tag(_url,search_snippet::_video_pos_patterns))
          _doc_type = VIDEO;
        else if (search_snippet::match_tag(_url,search_snippet::_forum_pos_patterns))
          _doc_type = FORUM;
        else if (search_snippet::match_tag(_url,search_snippet::_reject_pos_patterns))
          _doc_type = REJECTED;

        /* std::cerr << "[Debug]: tagged snippet: url: " << _url
          << " -- tag: " << (int)_doc_type << std::endl; */
      }

    // detect wikis. XXX: could be put into a pattern file if more complex patterns are needed.
    if (_doc_type == WEBPAGE)
      {
        size_t pos = 0;
        std::string wiki_pattern = "wiki";
        std::string::const_iterator sit = _url.begin();
        if ((pos = miscutil::ci_find(_url,wiki_pattern,sit))!=std::string::npos)
          {
            _doc_type = WIKI;
          }
      }
  }

  // static.
  sp_err search_snippet::load_patterns()
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
    err = loaders::load_pattern_file(pdf_patterns_filename.c_str(),search_snippet::_pdf_pos_patterns,
                                     fake_neg_patterns);
    if (err == SP_ERR_OK)
      err = loaders::load_pattern_file(file_doc_patterns_filename.c_str(),search_snippet::_file_doc_pos_patterns,
                                       fake_neg_patterns);
    if (err == SP_ERR_OK)
      err = loaders::load_pattern_file(audio_patterns_filename.c_str(),search_snippet::_audio_pos_patterns,
                                       fake_neg_patterns);
    if (err == SP_ERR_OK)
      err = loaders::load_pattern_file(video_patterns_filename.c_str(),search_snippet::_video_pos_patterns,
                                       fake_neg_patterns);
    if (err == SP_ERR_OK)
      err = loaders::load_pattern_file(forum_patterns_filename.c_str(),search_snippet::_forum_pos_patterns,
                                       fake_neg_patterns);
    if (err == SP_ERR_OK)
      err = loaders::load_pattern_file(reject_patterns_filename.c_str(),search_snippet::_reject_pos_patterns,
                                       fake_neg_patterns);
    return err;
  }

  void search_snippet::destroy_patterns()
  {
    std::for_each(_pdf_pos_patterns.begin(),_pdf_pos_patterns.end(),delete_object());
    std::for_each(_file_doc_pos_patterns.begin(),_file_doc_pos_patterns.end(),delete_object());
    std::for_each(_audio_pos_patterns.begin(),_audio_pos_patterns.end(),delete_object());
    std::for_each(_video_pos_patterns.begin(),_video_pos_patterns.end(),delete_object());
    std::for_each(_forum_pos_patterns.begin(),_forum_pos_patterns.end(),delete_object());
    std::for_each(_reject_pos_patterns.begin(),_reject_pos_patterns.end(),delete_object());
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

  void search_snippet::merge_snippets(search_snippet *s1,
                                      const search_snippet *s2)
  {
    if (s1->_doc_type != TWEET)
      {
        if (s1->_engine.equal(s2->_engine))
          return;
      }

    // search engine rank.
    s1->_rank += s2->_rank;

    // search engine.
    s1->_engine = s1->_engine.sunion(s2->_engine);

    // seeks_rank
    s1->_seeks_rank += s2->_seeks_rank;

    // cached link.
    if (s1->_cached.empty())
      s1->_cached = s2->_cached;

    // summary.
    if (s1->_summary.length() < s2->_summary.length())
      s1->_summary = s2->_summary;

    // cite.
    if (s1->_cite.length() > s2->_cite.length())
      s1->_cite = s2->_cite;

    // snippet type: more specialize type wins.
    // for now, very basic.
    s1->_doc_type = std::max(s1->_doc_type,s2->_doc_type);

    // TODO: merge dates.

    // file format.
    if (s1->_file_format.length() < s2->_file_format.length())  // we could do better here, ok enough for now.
      s1->_file_format = s2->_file_format;

    // meta rank.
    if (s1->_doc_type == TWEET)
      {
        if (s1->_meta_rank <= 0)
          s1->_meta_rank++;
        s1->_meta_rank++; // similarity detects retweets and merges them.
      }
    else
      {
        s1->_meta_rank = s1->_engine.size();
        s1->bing_yahoo_us_merge();
      }

    // radius.
    s1->_radius = std::min(s1->_radius,s2->_radius);
  }

  void search_snippet::bing_yahoo_us_merge()
  {
    // XXX: hack, on English queries, Bing & Yahoo are the same engine,
    // therefore the rank must be tweaked accordingly in this special case.
    if (_qc->_auto_lang == "en"
        && _engine.has_feed("yahoo")
        && _engine.has_feed("bing"))
      _meta_rank--;
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

  std::string search_snippet::get_doc_type_str() const
  {
    std::string output;
    switch (_doc_type)
      {
      case WEBPAGE:
        output = "webpage";
        break;
      case FORUM:
        output = "forum";
        break;
      case FILE_DOC:
        output = "file";
        break;
      case SOFTWARE:
        output = "software";
        break;
      case IMAGE:
        output = "image";
        break;
      case VIDEO:
        output = "video";
        break;
      case VIDEO_THUMB:
        output = "video_thumb";
        break;
      case AUDIO:
        output = "audio";
        break;
      case CODE:
        output = "code";
        break;
      case NEWS:
        output = "news";
        break;
      case TWEET:
        output = "tweet";
        break;
      case WIKI:
        output = "wiki";
        break;
      case UNKNOWN:
      default:
        output = "unknown";
      }
    return output;
  }

} /* end of namespace. */
