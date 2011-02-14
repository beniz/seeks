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
    :_description(""),_anonymous(false),_param_translation(NULL)
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
    std::string q_ggle = se_handler::_se_strings[GOOGLE]; // query to ggle.
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    int p = 31;
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    q_ggle.replace(p,6,qenc_str);

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
    std::string q_bing = se_handler::_se_strings[BING]; // query to bing.
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    int p = 29;
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    q_bing.replace(p,6,qenc_str);

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
    std::string q_yahoo = se_handler::_se_strings[YAHOO];
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
    std::string q_exa = se_handler::_se_strings[EXALEAD];
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
    std::string q_twit = se_handler::_se_strings[TWITTER];
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

  se_identica::se_identica()
    :search_engine()
  {
  }

  se_identica::~se_identica()
  {
  }

  void se_identica::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                std::string &url, const query_context *qc)
  {
    std::string q_dent = se_handler::_se_strings[IDENTICA];
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    char *qenc = encode::url_encode(std::string(query).c_str());
    std::string qenc_str = std::string(qenc);
    free(qenc);
    miscutil::replace_in_string(q_dent,"%query",qenc_str);

    // page.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? atoi(expansion) : 1;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_dent,"%start",pp_str);

    // number of results.
    int num = websearch::_wconfig->_Nr;
    std::string num_str = miscutil::to_string(num);
    miscutil::replace_in_string(q_dent,"%num",num_str);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying identi.ca: %s", q_dent.c_str());

    url = q_dent;
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
    std::string q_yt = se_handler::_se_strings[YOUTUBE];
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
    std::string q_blekko = se_handler::_se_strings[BLEKKO];
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

    std::string q_yau = se_handler::_se_strings[YAUBA];
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
    std::string q_dm = se_handler::_se_strings[DAILYMOTION];
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

  /*- se_handler. -*/
  std::string se_handler::_se_strings[NSEs] =  // in alphabetical order.
  {
    // bing: www.bing.com/search?q=markov+chain&go=&form=QBLH&filt=all
    "http://www.bing.com/search?q=%query&first=%start&mkt=%lang",
    // http://blekko.com/ws/P+/rss?fp=&p=3,
    // https://blekko.com/ws/query+terms+here+/ps=100
    //"http://blekko.com/ws/%query/rss?fp=&p=%start",
    "http://blekko.com/ws/%query/rss?ps=100",
    // http://www.dailymotion.com/rss/relevance/search/thé+vert/1
    "http://www.dailymotion.com/rss/relevance/search/%query/%start",
    // "http://www.exalead.com/search/web/results/?q=%query+language=%lang&elements_per_page=%num&start_index=%start"
    "http://www.exalead.com/search/web/results/?q=%query+language=%lang&elements_per_page=%num&start_index=%start",
    // ggle: http://www.google.com/search?q=help&ie=utf-8&oe=utf-8&aq=t&rls=org.mozilla:en-US:official&client=firefox-a
    "http://www.google.com/search?q=%query&start=%start&num=%num&hl=%lang&ie=%encoding&oe=%encoding",
    // identica: http://identi.ca/api/search.atom?q=paris&rpp=20&page=1
    "http://identi.ca/api/search.atom?q=%query&page=%start&rpp=%num",
    // seeks.
    "",
    // twitter: http://search.twitter.com/search.atom?q=seeksproject
    "http://search.twitter.com/search.atom?q=%query&page=%start&rpp=%num",
    // yahoo: search.yahoo.com/search?p=markov+chain&vl=lang_fr
    "http://search.yahoo.com/search?n=10&ei=UTF-8&va_vt=any&vo_vt=any&ve_vt=any&vp_vt=any&vd=all&vst=0&vf=all&vm=p&fl=1&vl=lang_%lang&p=%query&vs=",
    // http://fr.yauba.com/?q=chocolat+pouet&target=websites&pg=1&ss=n
    "http://%lang.yauba.com/?q=%query&target=websites&pg=%start&ss=n&con=y",
    // http://gdata.youtube.com/feeds/base/videos?q=sax roll&client=ytapi-youtube-search&alt=rss&v=2
    "http://gdata.youtube.com/feeds/base/videos?q=%query&client=ytapi-youtube-search&alt=rss&v=2&start-index=%start&max-results=%num"
  };

  se_ggle se_handler::_ggle = se_ggle();
  se_bing se_handler::_bing = se_bing();
  se_yahoo se_handler::_yahoo = se_yahoo();
  se_exalead se_handler::_exalead = se_exalead();
  se_twitter se_handler::_twitter = se_twitter();
  se_identica se_handler::_identica = se_identica();
  se_youtube se_handler::_youtube = se_youtube();
  se_yauba se_handler::_yauba = se_yauba();
  se_blekko se_handler::_blekko = se_blekko();
  se_dailymotion se_handler::_dailym = se_dailymotion();

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
                                         int &nresults, const query_context *qc, const std::bitset<NSEs> &se_enabled) throw (sp_exception)
  {
    std::vector<std::string> urls;
    urls.reserve(NSEs);
    std::vector<std::list<const char*>*> headers;
    headers.reserve(NSEs);

    // enabling of SEs.
    for (int i=0; i<NSEs; i++)
      {
        if (se_enabled[i])
          {
            std::string url;
            std::list<const char*> *lheaders = NULL;
            se_handler::query_to_se(parameters,(SE)i,url,qc,lheaders);
            urls.push_back(url);
            headers.push_back(lheaders);
          }
      }

    if (urls.empty())
      {
        nresults = 0;
        //return NULL; // beware.
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
    mutex_lock(&_curl_mutex);
    if (websearch::_wconfig->_background_proxy_addr.empty())
      cmg.www_mget(urls,urls.size(),&headers,
                   "",0,&se_handler::_curl_handlers); // don't go through the seeks' proxy, or will loop til death!
    else cmg.www_mget(urls,urls.size(),&headers,
                        websearch::_wconfig->_background_proxy_addr,
                        websearch::_wconfig->_background_proxy_port,
                        &se_handler::_curl_handlers);
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
        throw sp_exception(WB_ERR_NO_ENGINE_OUTPUT,"no output from any search engine");
      }

    delete[] cmg._outputs;

    return outputs;
  }

  void se_handler::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               const SE &se, std::string &url, const query_context *qc,
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

    switch (se)
      {
      case GOOGLE:
        _ggle.query_to_se(parameters,url,qc);
        break;
      case BING:
        _bing.query_to_se(parameters,url,qc);
        break;
      case YAHOO:
        _yahoo.query_to_se(parameters,url,qc);
        break;
      case EXALEAD:
        _exalead.query_to_se(parameters,url,qc);
        break;
      case TWITTER:
        _twitter.query_to_se(parameters,url,qc);
        break;
      case IDENTICA:
        _identica.query_to_se(parameters,url,qc);
        break;
      case YOUTUBE:
        _youtube.query_to_se(parameters,url,qc);
        break;
      case YAUBA:
        _yauba.query_to_se(parameters,url,qc);
        break;
      case BLEKKO:
        _blekko.query_to_se(parameters,url,qc);
        break;
      case DAILYMOTION:
        _dailym.query_to_se(parameters,url,qc);
        break;
      case SEEKS:
        break;
      }
  }

  void se_handler::set_engines(std::bitset<NSEs> &se_enabled, const std::vector<std::string> &ses)
  {
    int msize = std::min((int)ses.size(),NSEs);
    for (int i=0; i<msize; i++)
      {
        std::string se = ses.at(i);

        /* put engine name into lower cases. */
        std::transform(se.begin(),se.end(),se.begin(),tolower);

        if (se == "google")
          {
            se_enabled |= std::bitset<NSEs>(SE_GOOGLE);
          }
        else if (se == "bing")
          {
            se_enabled |= std::bitset<NSEs>(SE_BING);
          }
        else if (se == "yahoo")
          {
            se_enabled |= std::bitset<NSEs>(SE_YAHOO);
          }
        else if (se == "exalead")
          {
            se_enabled |= std::bitset<NSEs>(SE_EXALEAD);
          }
        else if (se == "twitter")
          {
            se_enabled |= std::bitset<NSEs>(SE_TWITTER);
          }
        else if (se == "identica")
          {
            se_enabled |= std::bitset<NSEs>(SE_IDENTICA);
          }
        else if (se == "youtube")
          {
            se_enabled |= std::bitset<NSEs>(SE_YOUTUBE);
          }
        else if (se == "yauba")
          {
            se_enabled |= std::bitset<NSEs>(SE_YAUBA);
          }
        else if (se == "blekko")
          {
            se_enabled |= std::bitset<NSEs>(SE_BLEKKO);
          }
        else if (se == "dailymotion")
          {
            se_enabled |= std::bitset<NSEs>(SE_DAILYMOTION);
          }
      }
  }

  /*-- parsing. --*/
  void se_handler::parse_ses_output(std::string **outputs, const int &nresults,
                                    std::vector<search_snippet*> &snippets,
                                    const int &count_offset,
                                    query_context *qr,
                                    const std::bitset<NSEs> &se_enabled)
  {
    // use multiple threads unless told otherwise.
    int j = 0;
    if (seeks_proxy::_config->_multi_threaded)
      {
        size_t active_ses = se_enabled.count();
        pthread_t parser_threads[active_ses];
        ps_thread_arg* parser_args[active_ses];
        for (size_t i=0; i<active_ses; i++)
          parser_args[i] = NULL;

        // threads, one per parser.
        int k = 0;
        for (int i=0; i<NSEs; i++)
          {
            if (se_enabled[i])
              {
                if (outputs[j])
                  {
                    ps_thread_arg *args = new ps_thread_arg();
                    args->_se = (SE)i;
                    args->_output = (char*) outputs[j]->c_str();  // XXX: sad cast.
                    args->_snippets = new std::vector<search_snippet*>();
                    args->_offset = count_offset;
                    args->_qr = qr;
                    parser_args[k] = args;

                    pthread_t ps_thread;
                    int err = pthread_create(&ps_thread, NULL,  // default attribute is PTHREAD_CREATE_JOINABLE
                                             (void * (*)(void *))se_handler::parse_output, args);
                    if (err != 0)
                      {
                        errlog::log_error(LOG_LEVEL_ERROR, "Error creating parser thread.");
                        parser_threads[k++] = 0;
                        delete args;
                        parser_args[k] = NULL;
                        continue;
                      }
                    parser_threads[k++] = ps_thread;
                  }
                else parser_threads[k++] = 0;
                j++;
              }
          }

        // join and merge results.
        for (size_t i=0; i<active_ses; i++)
          {
            if (parser_threads[i]!=0)
              pthread_join(parser_threads[i],NULL);
          }

        for (size_t i=0; i<active_ses; i++)
          {
            if (parser_args[i])
              {
                if (parser_args[i]->_err == SP_ERR_OK)
                  std::copy(parser_args[i]->_snippets->begin(),parser_args[i]->_snippets->end(),
                            std::back_inserter(snippets));
                parser_args[i]->_snippets->clear();
                delete parser_args[i]->_snippets;
                delete parser_args[i];
              }
          }
      }
    else
      {
        for (int i=0; i<NSEs; i++)
          {
            if (se_enabled[i])
              {
                if (outputs[j])
                  {
                    ps_thread_arg args;
                    args._se = (SE)i;
                    args._output = (char*)outputs[j]->c_str(); // XXX: sad cast.
                    args._snippets = &snippets;
                    args._offset = count_offset;
                    args._qr = qr;
                    parse_output(args);
                  }
                j++;
              }
          }
      }
  }

  void se_handler::parse_output(ps_thread_arg &args)
  {
    se_parser *se = se_handler::create_se_parser((SE)args._se);

    try
      {
        if ((SE)args._se == YOUTUBE || (SE)args._se == DAILYMOTION)
          se->parse_output_xml(args._output,args._snippets,args._offset);
        else se->parse_output(args._output,args._snippets,args._offset);
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
    if (args._se == GOOGLE)
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

  se_parser* se_handler::create_se_parser(const SE &se)
  {
    se_parser *sep = NULL;
    switch (se)
      {
      case GOOGLE:
        sep = new se_parser_ggle();
        break;
      case BING:
        sep = new se_parser_bing();
        break;
      case YAHOO:
        sep = new se_parser_yahoo();
        break;
      case EXALEAD:
        sep = new se_parser_exalead();
        break;
      case TWITTER:
        sep = new se_parser_twitter();
        break;
      case IDENTICA:
        sep = new se_parser_twitter("identica");
        break;
      case YOUTUBE:
        sep = new se_parser_youtube();
        break;
      case YAUBA:
        sep = new se_parser_yauba();
        break;
      case BLEKKO:
        sep = new se_parser_blekko();
        break;
      case DAILYMOTION:
        sep = new se_parser_dailymotion();
        break;
      case SEEKS:
        break;
      }

    return sep;
  }

} /* end of namespace. */
