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

#include "se_handler.h"
#include "miscutil.h"
#include "websearch.h" // for configuration.
#include "curl_mget.h"
#include "encode.h"
#include "errlog.h"
#include "seeks_proxy.h" // for configuration and mutexes.
#include "proxy_configuration.h"
#include "query_context.h"

#include "se_parser_ggle.h"
#include "se_parser_bing.h"
#include "se_parser_yahoo.h"
#include "se_parser_exalead.h"
#include "se_parser_twitter.h"
#include "se_parser_youtube.h"
#include "se_parser_dailymotion.h"
#include "se_parser_yauba.h"
#include "se_parser_blekko.h"
#include "se_parser_doku.h"
#include "se_parser_mediawiki.h"
#include "se_parser_osearch.h"

#include <cctype>
#include <pthread.h>
#include <algorithm>
#include <iterator>
#include <iostream>

using namespace sp;

namespace seeks_plugins
{
  /*- search_engine & derivatives. -*/
  search_engine::search_engine()
    :_description(""),_anonymous(false)
  {
  }

  search_engine::~search_engine()
  {
  }

  se_ggle::se_ggle()
    : search_engine()
  {

  }

  se_ggle::~se_ggle()
  {
  }

  void se_ggle::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                            std::string &url, const query_context *qc)
  {
    std::string q_ggle = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_ggle,"%query",qenc_str);

    // expansion = result page called...
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * websearch::_wconfig->_Nr : 0;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_ggle,"%start",pp_str);

    // number of results.
    int num = websearch::_wconfig->_Nr; // by default.
    std::string num_str = miscutil::to_string(num);
    miscutil::replace_in_string(q_ggle,"%num",num_str);

    // encoding.
    miscutil::replace_in_string(q_ggle,"%encoding","utf-8");

    // language.
    if (websearch::_wconfig->_lang == "auto")
      miscutil::replace_in_string(q_ggle,"%lang",qc->_auto_lang);
    else miscutil::replace_in_string(q_ggle,"%lang",websearch::_wconfig->_lang);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying ggle: %s", q_ggle.c_str());

    url = q_ggle;
  }

  se_bing::se_bing()
    : search_engine()
  {
  }

  se_bing::~se_bing()
  {
  }

  void se_bing::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                            std::string &url, const query_context *qc)
  {
    std::string q_bing = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_bing,"%query",qenc_str);

    // page.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * websearch::_wconfig->_Nr : 0;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_bing,"%start",pp_str);

    // number of results.
    // can't figure out what argument to pass to Bing. Seems only feasible through cookies (losers).

    // language.
    miscutil::replace_in_string(q_bing,"%lang",qc->_auto_lang_reg);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying bing: %s", q_bing.c_str());

    url = q_bing;
  }

  se_yahoo::se_yahoo()
    : search_engine()
  {
  }

  se_yahoo::~se_yahoo()
  {
  }

  void se_yahoo::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                             std::string &url, const query_context *qc)
  {
    std::string q_yahoo = url;
    const char *query = miscutil::lookup(parameters,"q");

    // page.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp =  (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * websearch::_wconfig->_Nr : 0;
    if (pp>1) pp++;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_yahoo,"%start",pp_str);

    // language, in yahoo is obtained by hitting the regional server.
    miscutil::replace_in_string(q_yahoo,"%lang",qc->_auto_lang);

    // query (don't move it, depends on domain name, which is language dependent).
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_yahoo,"%query",qenc_str);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying yahoo: %s", q_yahoo.c_str());

    url = q_yahoo;
  }

  se_exalead::se_exalead()
    :search_engine()
  {
  }

  se_exalead::~se_exalead()
  {
  }

  void se_exalead::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc)
  {
    std::string q_exa = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_exa,"%query",qenc_str);

    // page
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * websearch::_wconfig->_Nr : 0;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_exa,"%start",pp_str);

    // number of results.
    int num = websearch::_wconfig->_Nr;
    std::string num_str = miscutil::to_string(num);
    miscutil::replace_in_string(q_exa,"%num",num_str);

    // language
    if (websearch::_wconfig->_lang == "auto")
      miscutil::replace_in_string(q_exa,"%lang",qc->_auto_lang);
    else miscutil::replace_in_string(q_exa,"%lang",websearch::_wconfig->_lang);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying exalead: %s", q_exa.c_str());

    url = q_exa;
  }

  se_twitter::se_twitter()
    :search_engine()
  {
  }

  se_twitter::~se_twitter()
  {
  }

  void se_twitter::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc)
  {
    std::string q_twit = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_twit,"%query",qenc_str);

    // page.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? atoi(expansion) : 1;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_twit,"%start",pp_str);

    // number of results.
    int num = websearch::_wconfig->_Nr;
    std::string num_str = miscutil::to_string(num);
    miscutil::replace_in_string(q_twit,"%num",num_str);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying twitter: %s", q_twit.c_str());

    url = q_twit;
  }

  se_youtube::se_youtube()
    :search_engine()
  {
  }

  se_youtube::~se_youtube()
  {
  }

  void se_youtube::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc)
  {
    std::string q_yt = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_yt,"%query",qenc_str);

    // page.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * websearch::_wconfig->_Nr + 1: 1;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_yt,"%start",pp_str);

    // number of results.
    int num = websearch::_wconfig->_Nr; // by default.
    std::string num_str = miscutil::to_string(num);
    miscutil::replace_in_string(q_yt,"%num",num_str);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying youtube: %s", q_yt.c_str());

    url = q_yt;
  }

  se_blekko::se_blekko()
    :search_engine()
  {
  }

  se_blekko::~se_blekko()
  {
  }

  void se_blekko::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                              std::string &url, const query_context *qc)
  {
    std::string q_blekko = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_blekko,"%query",qenc_str);

    //page
    /* const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)) : 1;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_blekko,"%start",pp_str); */

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying blekko: %s", q_blekko.c_str());

    url = q_blekko;
  }

  se_yauba::se_yauba()
    :search_engine()
  {
  }

  se_yauba::~se_yauba()
  {
  }

  void se_yauba::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                             std::string &url, const query_context *qc)
  {
    static std::string lang[8][2] = {{"it","it"},{"fr","fr"},{"de","de"},{"hi","in"}, {"pt","br"}, {"br","br"},{"ru","ru"}, {"zh","cn"}};

    std::string q_yau = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_yau,"%query",qenc_str);

    // page
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)) : 1;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_yau,"%start",pp_str);

    // language.
    std::string qlang;
    for (short i=0; i<8; i++)
      {
        if (lang[i][0] == qc->_auto_lang)
          {
            qlang = lang[i][1];
            break;
          }
      }
    if (qlang.empty())
      miscutil::replace_in_string(q_yau,"%lang","www");
    else miscutil::replace_in_string(q_yau,"%lang",qlang);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying yauba: %s", q_yau.c_str());

    url = q_yau;
  }

  se_dailymotion::se_dailymotion()
    :search_engine()
  {
  }

  se_dailymotion::~se_dailymotion()
  {
  }

  void se_dailymotion::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                   std::string &url, const query_context *qc)
  {
    std::string q_dm = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_dm,"%query",qenc_str);

    // page.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? atoi(expansion) : 1;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_dm,"%start",pp_str);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying dailymotion: %s", q_dm.c_str());

    url = q_dm;
  }

  se_doku::se_doku()
    :search_engine()
  {
  }

  se_doku::~se_doku()
  {
  }

  void se_doku::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                            std::string &url, const query_context *qc)
  {
    std::string q_dm = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_dm,"%query",qenc_str);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying doku: %s", q_dm.c_str());

    url = q_dm;
  }

  se_mediawiki::se_mediawiki()
    :search_engine()
  {
  }

  se_mediawiki::~se_mediawiki()
  {
  }

  void se_mediawiki::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                 std::string &url, const query_context *qc)
  {
    std::string q_dm = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_dm,"%query",qenc_str);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying mediawiki: %s", q_dm.c_str());

    url = q_dm;
  }

  se_osearch_rss::se_osearch_rss()
    :search_engine()
  {
  }

  se_osearch_rss::~se_osearch_rss()
  {
  }

  void se_osearch_rss::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                   std::string &url, const query_context *qc)
  {
    std::string q_dm = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_dm,"%query",qenc_str);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying opensearch rss: %s", q_dm.c_str());

    url = q_dm;
  }

  se_osearch_atom::se_osearch_atom()
    :search_engine()
  {
  }

  se_osearch_atom::~se_osearch_atom()
  {
  }

  void se_osearch_atom::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                    std::string &url, const query_context *qc)
  {
    std::string q_dm = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_dm,"%query",qenc_str);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying opensearch atom: %s", q_dm.c_str());

    url = q_dm;
  }

  se_ggle se_handler::_ggle = se_ggle();
  se_bing se_handler::_bing = se_bing();
  se_yahoo se_handler::_yahoo = se_yahoo();
  se_exalead se_handler::_exalead = se_exalead();
  se_twitter se_handler::_twitter = se_twitter();
  se_youtube se_handler::_youtube = se_youtube();
  se_yauba se_handler::_yauba = se_yauba();
  se_blekko se_handler::_blekko = se_blekko();
  se_dailymotion se_handler::_dailym = se_dailymotion();
  se_doku se_handler::_doku = se_doku();
  se_mediawiki se_handler::_mediaw = se_mediawiki();
  se_osearch_rss se_handler::_osearch_rss = se_osearch_rss();
  se_osearch_atom se_handler::_osearch_atom = se_osearch_atom();

  std::vector<CURL*> se_handler::_curl_handlers = std::vector<CURL*>();
  sp_mutex_t se_handler::_curl_mutex;

  /*-- initialization. --*/
  void se_handler::init_handlers(const int &num)
  {
    mutex_init(&_curl_mutex);
    if (!_curl_handlers.empty())
      {
        se_handler::cleanup_handlers();
      }
    _curl_handlers.reserve(num);
    for (int i=0; i<num; i++)
      {
        CURL *curl = curl_easy_init();
        _curl_handlers.push_back(curl);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // do not check on SSL certificate.
        curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, -1); // cache forever.
      }
  }

  void se_handler::cleanup_handlers()
  {
    std::vector<CURL*>::iterator vit = _curl_handlers.begin();
    while (vit!=_curl_handlers.end())
      {
        curl_easy_cleanup((*vit));
        vit = _curl_handlers.erase(vit);
      }
  }

  /*-- queries to the search engines. */
  std::string** se_handler::query_to_ses(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                         int &nresults, const query_context *qc, const feeds &se_enabled) throw (sp_exception)
  {
    std::vector<std::string> urls;
    urls.reserve(se_enabled.size());
    std::vector<std::list<const char*>*> headers;
    headers.reserve(se_enabled.size());
    std::set<feed_parser,feed_parser::lxn>::iterator it
    = se_enabled._feedset.begin();
    while(it!=se_enabled._feedset.end())
      {
        std::vector<std::string> all_urls;
        std::list<const char*> *lheaders = NULL;
        se_handler::query_to_se(parameters,(*it),all_urls,qc,lheaders);
        for (size_t j=0; j<all_urls.size(); j++)
          {
            urls.push_back(all_urls.at(j));
            if (j == 0)
              headers.push_back(lheaders);
            else
              {
                std::list<const char*> *lheadersc = new std::list<const char*>();
                miscutil::list_duplicate(lheadersc,lheaders);
                headers.push_back(lheadersc);
              }
          }
        ++it;
      }

    if (urls.empty())
      {
        nresults = 0;
        throw sp_exception(WB_ERR_NO_ENGINE,"no engine enabled to forward query to");
      }
    else nresults = urls.size();

    if (_curl_handlers.size() != urls.size())
      {
        se_handler::init_handlers(urls.size()); // reinitializes the curl handlers.
      }

    // get content.
    curl_mget cmg(urls.size(),websearch::_wconfig->_se_transfer_timeout,0,
                  websearch::_wconfig->_se_connect_timeout,0);
    std::vector<int> status;
    mutex_lock(&_curl_mutex);
    if (websearch::_wconfig->_background_proxy_addr.empty())
      cmg.www_mget(urls,urls.size(),&headers,
                   "",0,status,&se_handler::_curl_handlers); // don't go through the seeks' proxy, or will loop til death!
    else cmg.www_mget(urls,urls.size(),&headers,
                        websearch::_wconfig->_background_proxy_addr,
                        websearch::_wconfig->_background_proxy_port,
                        status,&se_handler::_curl_handlers);
    mutex_unlock(&_curl_mutex);

    std::string **outputs = new std::string*[urls.size()];
    bool have_outputs = false;
    for (size_t i=0; i<urls.size(); i++)
      {
        outputs[i] = NULL;
        if (cmg._outputs[i])
          {
            outputs[i] = cmg._outputs[i];
            have_outputs = true;
          }

        // delete headers, if any.
        if (headers.at(i))
          {
            miscutil::list_remove_all(headers.at(i));
            delete headers.at(i);
          }
      }

    if (!have_outputs)
      {
        delete[] outputs;
        outputs = NULL;
        delete[] cmg._outputs;
        throw sp_exception(WB_ERR_NO_ENGINE_OUTPUT,"no output from any search engine");
      }

    delete[] cmg._outputs;

    return outputs;
  }

  void se_handler::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               const feed_parser &se, std::vector<std::string> &all_urls, const query_context *qc,
                               std::list<const char*> *&lheaders)
  {
    lheaders = new std::list<const char*>();

    /* pass the user-agent header. */
    std::list<const char*>::const_iterator sit = qc->_useful_http_headers.begin();
    while (sit!=qc->_useful_http_headers.end())
      {
        lheaders->push_back(strdup((*sit)));
        ++sit;
      }

    for (size_t i=0; i<se.size(); i++)
      {
        std::string url = se.get_url(i);
        if (se._name == "google")
          _ggle.query_to_se(parameters,url,qc);
        else if (se._name == "bing")
          _bing.query_to_se(parameters,url,qc);
        else if (se._name == "yahoo")
          _yahoo.query_to_se(parameters,url,qc);
        else if (se._name == "exalead")
          _exalead.query_to_se(parameters,url,qc);
        else if (se._name == "twitter")
          _twitter.query_to_se(parameters,url,qc);
        else if (se._name == "youtube")
          _youtube.query_to_se(parameters,url,qc);
        else if (se._name == "yauba")
          _yauba.query_to_se(parameters,url,qc);
        else if (se._name == "blekko")
          _blekko.query_to_se(parameters,url,qc);
        else if (se._name == "dailymotion")
          _dailym.query_to_se(parameters,url,qc);
        else if (se._name == "dokuwiki")
          _doku.query_to_se(parameters,url,qc);
        else if (se._name == "mediawiki")
          _mediaw.query_to_se(parameters,url,qc);
        else if (se._name == "opensearch_rss")
          _osearch_rss.query_to_se(parameters,url,qc);
        else if (se._name == "opensearch_atom")
          _osearch_atom.query_to_se(parameters,url,qc);
        else if (se._name == "seeks")
          {}
        else if (se._name == "dummy")
          {}
        all_urls.push_back(url);
      }
  }

  /*-- parsing. --*/
  void se_handler::parse_ses_output(std::string **outputs, const int &nresults,
                                    std::vector<search_snippet*> &snippets,
                                    const int &count_offset,
                                    query_context *qr,
                                    const feeds &se_enabled)
  {
    // use multiple threads unless told otherwise.
    int j = 0;
    if (seeks_proxy::_config->_multi_threaded)
      {
        std::vector<pthread_t> parser_threads;
        std::vector<ps_thread_arg*> parser_args;

        // threads, one per parser.
        std::set<feed_parser,feed_parser::lxn>::iterator it
        = se_enabled._feedset.begin();
        while(it!=se_enabled._feedset.end())
          {
            for (size_t f=0; f<(*it).size(); f++)
              {
                if (outputs[j])
                  {
                    ps_thread_arg *args = new ps_thread_arg();
                    args->_se = (*it);
                    args->_se_idx = f;
                    args->_output = (char*) outputs[j]->c_str();  // XXX: sad cast.
                    args->_snippets = new std::vector<search_snippet*>();
                    args->_offset = count_offset;
                    args->_qr = qr;
                    //parser_args.push_back(args);

                    pthread_t ps_thread;
                    int err = pthread_create(&ps_thread, NULL,  // default attribute is PTHREAD_CREATE_JOINABLE
                                             (void * (*)(void *))se_handler::parse_output, args);
                    if (err != 0)
                      {
                        errlog::log_error(LOG_LEVEL_ERROR, "Error creating parser thread.");
                        parser_threads.push_back(0);
                        delete args;
                        parser_args.push_back(NULL);
                        continue;
                      }
                    parser_args.push_back(args);
                    parser_threads.push_back(ps_thread);
                  }
                else parser_threads.push_back(0);
                j++;
              }
            ++it;
          }

        // join and merge results.
        for (size_t i=0; i<parser_threads.size(); i++)
          {
            if (parser_threads.at(i)!=0)
              pthread_join(parser_threads.at(i),NULL);
          }

        for (size_t i=0; i<parser_args.size(); i++)
          {
            if (parser_args.at(i))
              {
                if (parser_args.at(i)->_err == SP_ERR_OK)
                  std::copy(parser_args.at(i)->_snippets->begin(),parser_args.at(i)->_snippets->end(),
                            std::back_inserter(snippets));
                parser_args.at(i)->_snippets->clear();
                delete parser_args.at(i)->_snippets;
                delete parser_args.at(i);
              }
          }
      }
    else
      {
        std::set<feed_parser,feed_parser::lxn>::iterator it
        = se_enabled._feedset.begin();
        while(it!=se_enabled._feedset.end())
          {
            if (outputs[j])
              {
                ps_thread_arg args;
                args._se = (*it);
                args._output = (char*)outputs[j]->c_str(); // XXX: sad cast.
                args._snippets = &snippets;
                args._offset = count_offset;
                args._qr = qr;
                parse_output(args);
              }
            j++;
            ++it;
          }
      }
  }

  void se_handler::parse_output(ps_thread_arg &args)
  {
    se_parser *se = se_handler::create_se_parser(args._se,args._se_idx);
    if (!se)
      {
        args._err = WB_ERR_NO_ENGINE;
        errlog::log_error(LOG_LEVEL_ERROR,"no engine for %s",args._se._name.c_str());
        return;
      }
    try
      {
        if (args._se._name == "youtube" || args._se._name == "dailymotion")
          se->parse_output_xml(args._output,args._snippets,args._offset);
        else se->parse_output(args._output,args._snippets,args._offset);
        errlog::log_error(LOG_LEVEL_DEBUG,"parser %s: %u snippets",
                          args._se._name.c_str(),args._snippets->size());
      }
    catch (sp_exception &e)
      {
        delete se;
        args._err = e.code();
        errlog::log_error(LOG_LEVEL_ERROR,e.what().c_str());
        return;
      }

    // link the snippets to the query context
    // and post-process them.
    for (size_t i=0; i<args._snippets->size(); i++)
      {
        args._snippets->at(i)->_qc = args._qr;
        args._snippets->at(i)->tag();
      }

    // hack for getting stuff out of ggle.
    if (args._se._name == "google")
      {
        // get more stuff from the parser.
        se_parser_ggle *se_p_ggle = static_cast<se_parser_ggle*>(se);

        // XXX: suggestions (max weight is given to engines' suggestions,
        //                   this may change in the future).
        if (!se_p_ggle->_suggestion.empty())
          args._qr->_suggestions.insert(std::pair<double,std::string>(1.0,se_p_ggle->_suggestion));
      }
    delete se;
  }

  se_parser* se_handler::create_se_parser(const feed_parser &se,
                                          const size_t &i)
  {
    se_parser *sep = NULL;
    if (se._name == "google")
      sep = new se_parser_ggle(se.get_url(i));
    else if (se._name == "bing")
      sep = new se_parser_bing(se.get_url(i));
    else if (se._name == "yahoo")
      sep = new se_parser_yahoo(se.get_url(i));
    else if (se._name == "exalead")
      sep = new se_parser_exalead(se.get_url(i));
    else if (se._name == "twitter")
      sep = new se_parser_twitter(se.get_url(i));
    else if (se._name == "youtube")
      sep = new se_parser_youtube(se.get_url(i));
    else if (se._name == "yauba")
      sep = new se_parser_yauba(se.get_url(i));
    else if (se._name == "blekko")
      sep = new se_parser_blekko(se.get_url(i));
    else if (se._name == "dailymotion")
      sep = new se_parser_dailymotion(se.get_url(i));
    else if (se._name == "dokuwiki")
      sep = new se_parser_doku(se.get_url(i));
    else if (se._name == "mediawiki")
      sep = new se_parser_mediawiki(se.get_url(i));
    else if (se._name == "opensearch_rss")
      sep = new se_parser_osearch_rss(se.get_url(i));
    else if (se._name == "opensearch_atom")
      sep = new se_parser_osearch_atom(se.get_url(i));
    else if (se._name == "seeks")
      {}
    else if (se._name == "dummy")
      {}

    return sep;
  }

} /* end of namespace. */
