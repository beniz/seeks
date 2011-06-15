/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "se_handler_img.h"
#include "img_query_context.h"
#include "websearch.h"
#include "curl_mget.h"
#include "miscutil.h"
#include "encode.h"
#include "errlog.h"

#include "se_parser_bing_img.h"
#include "se_parser_ggle_img.h"
#include "se_parser_flickr.h"
#include "se_parser_wcommons.h"
#include "se_parser_yahoo_img.h"

using namespace sp;

namespace seeks_plugins
{
  std::string se_bing_img::_safe_search_cookie = "SRCHHPGUSR=\"NEWWND=0&ADLT=OFF&NRSLT=10&NRSPH=2&SRCHLANG=\"";
  se_bing_img::se_bing_img()
    :search_engine()
  {
  }

  se_bing_img::~se_bing_img()
  {
  }

  void se_bing_img::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                std::string &url, const query_context *qc)
  {
    std::string q_bing = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    int p = 36;
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    q_bing.replace(p,6,qenc_str);

    // page.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * img_websearch_configuration::_img_wconfig->_Nr : 0;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_bing,"%start",pp_str);

    // language.
    miscutil::replace_in_string(q_bing,"%lang",qc->_auto_lang_reg);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying bing: %s", q_bing.c_str());

    url = q_bing;
  }

  se_ggle_img::se_ggle_img()
    :search_engine()
  {
  }

  se_ggle_img::~se_ggle_img()
  {
  }

  void se_ggle_img::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                std::string &url, const query_context *qc)
  {
    std::string q_ggle = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    int p = 31;
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    q_ggle.replace(p,6,qenc_str);

    // expansion = result page called.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * img_websearch_configuration::_img_wconfig->_Nr : 0;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_ggle,"%start",pp_str);

    // number of results.
    int num = img_websearch_configuration::_img_wconfig->_Nr; // by default.
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

  se_flickr::se_flickr()
    :search_engine()
  {
  }

  se_flickr::~se_flickr()
  {
  }

  void se_flickr::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                              std::string &url, const query_context *qc)
  {
    std::string q_fl = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    int p = 32;
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    q_fl.replace(p,6,qenc_str);

    // expansion = page requested.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    std::string pp_str = expansion;
    miscutil::replace_in_string(q_fl,"%start",pp_str);

    // XXX: could try to limit the number of results.

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying flickr: %s", q_fl.c_str());

    url = q_fl;
  }

  std::string se_yahoo_img::_safe_search_cookie = "sB=vm=p\\&v=1";
  se_yahoo_img::se_yahoo_img()
    :search_engine()
  {
  }

  se_yahoo_img::~se_yahoo_img()
  {
  }

  void se_yahoo_img::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                 std::string &url, const query_context *qc)
  {
    std::string q_ya = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    int p = 56;
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    q_ya.replace(p,6,qenc_str);

    // page.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * img_websearch_configuration::_img_wconfig->_Nr : 0;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_ya,"%start",pp_str);

    // language.
    miscutil::replace_in_string(q_ya,"%lang",qc->_auto_lang);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying yahoo: %s", q_ya.c_str());

    url = q_ya;
  }

  se_wcommons::se_wcommons()
  {
  }

  se_wcommons::~se_wcommons()
  {
  }

  void se_wcommons::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                std::string &url, const query_context *qc)
  {
    std::string q_wcom = url;
    const char *query = miscutil::lookup(parameters,"q");

    // query.
    //int p = 71;
    char *qenc = encode::url_encode(query);
    std::string qenc_str = std::string(qenc);
    free(qenc);
    //q_wcom.replace(p,6,qenc_str);
    miscutil::replace_in_string(q_wcom,"%query",qenc_str);

    // expansion.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * img_websearch_configuration::_img_wconfig->_Nr : 0;
    std::string pp_str = miscutil::to_string(pp);
    miscutil::replace_in_string(q_wcom,"%start",pp_str);

    // number of results.
    int num = img_websearch_configuration::_img_wconfig->_Nr; // by default.
    std::string num_str = miscutil::to_string(num);
    miscutil::replace_in_string(q_wcom,"%num",num_str);

    // log the query.
    errlog::log_error(LOG_LEVEL_DEBUG, "Querying wikimedia commons: %s", q_wcom.c_str());

    url = q_wcom;
  }

  /*- se_handler_img -*/
  /*std::string se_handler_img::_se_strings[IMG_NSEs] =  // in alphabetical order.
  {
    // bing: www.bing.com/images/search?q=markov+chain&go=&form=QBIR
    "http://www.bing.com/images/search?q=%query&first=%start&mkt=%lang",
    // flickr: www.flickr.com/search/?q=markov+chain&z=e
    "http://www.flickr.com/search/?q=%query&page=%start",
    // ggle: www.google.com/images?q=markov+chain&hl=en&safe=off&prmd=mi&source=lnms&tbs=isch:1&sa=X&oi=mode_link&ct=mode
    "http://www.google.com/images?q=%query&gbv=1&start=%start&hl=%lang&ie=%encoding&oe=%encoding",
    // wcommons: commons.wikimedia.org/w/index.php?title=Special:Search&limit=20&offset=20&search=markov+chain
    "http://commons.wikimedia.org/w/index.php?search=%query&limit=%num&offset=%start",
    // images.search.yahoo.com/search/images?ei=UTF-8&p=lapin&js=0&lang=fr&b=21
    "http://images.search.yahoo.com/search/images?ei=UTF-8&p=%query&js=0&vl=lang_%lang&b=%start"
    };*/

  se_bing_img se_handler_img::_img_bing = se_bing_img();
  se_ggle_img se_handler_img::_img_ggle = se_ggle_img();
  se_flickr se_handler_img::_img_flickr = se_flickr();
  se_wcommons se_handler_img::_img_wcommons = se_wcommons();
  se_yahoo_img se_handler_img::_img_yahoo = se_yahoo_img();

  /*-- queries to the image search engines. --*/
  std::string** se_handler_img::query_to_ses(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
      int &nresults, const query_context *qc, const feeds &se_enabled) throw (sp_exception)
  {
    std::vector<std::string> urls;
    urls.reserve(se_enabled.size());
    std::vector<std::list<const char*>*> headers;
    headers.reserve(se_enabled.size());
    std::vector<std::string> cookies;
    cookies.reserve(se_enabled.size());

    // enabling of SEs.
    std::set<feed_parser,feed_parser::lxn>::iterator it
    = se_enabled._feedset.begin();
    while(it!=se_enabled._feedset.end())
      {
        std::vector<std::string> all_urls;
        std::list<const char*> *lheaders = NULL;
        se_handler_img::query_to_se(parameters,(*it),all_urls,qc,lheaders);
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
            if ((*it)._name == "bing_img")  // safe search cookies.
              cookies.push_back(se_bing_img::_safe_search_cookie.c_str());
            else if ((*it)._name == "yahoo_img")
              cookies.push_back(se_yahoo_img::_safe_search_cookie.c_str());
            else cookies.push_back("");
          }
        ++it;
      }

    if (urls.empty())
      {
        nresults = 0;
        throw sp_exception(WB_ERR_NO_ENGINE,"no engine enabled to forward query to");
      }
    else nresults = urls.size();

    // get content.
    std::vector<std::string> *safesearch_cookies = NULL;
    const char *safesearch_p = miscutil::lookup(parameters,"safesearch");
    if ((safesearch_p && strcasecmp(safesearch_p,"off") == 0)
        || (!safesearch_p && !img_websearch_configuration::_img_wconfig->_safe_search))
      safesearch_cookies = &cookies;
    std::vector<int> status;
    curl_mget cmg(urls.size(),websearch::_wconfig->_se_transfer_timeout,0,
                  websearch::_wconfig->_se_connect_timeout,0,status);
    if (websearch::_wconfig->_background_proxy_addr.empty())
      cmg.www_mget(urls,urls.size(),&headers,"",0,status,NULL,safesearch_cookies); // don't go through the seeks' proxy, or will loop til death!
    else cmg.www_mget(urls,urls.size(),&headers,
                        websearch::_wconfig->_background_proxy_addr,
                        websearch::_wconfig->_background_proxy_port,
                        status,NULL,safesearch_cookies);

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

  void se_handler_img::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
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
        if (se._name == "google_img")
          _img_ggle.query_to_se(parameters,url,qc);
        else if (se._name == "bing_img")
          _img_bing.query_to_se(parameters,url,qc);
        else if (se._name == "flickr")
          _img_flickr.query_to_se(parameters,url,qc);
        else if (se._name == "wcommons")
          _img_wcommons.query_to_se(parameters,url,qc);
        else if (se._name == "yahoo_img")
          _img_yahoo.query_to_se(parameters,url,qc);
        all_urls.push_back(url);
      }
  }

  /*void se_handler_img::set_engines(const feeds &se_enabled, const std::vector<std::string> &ses)
  {
    int msize = std::min((int)ses.size(),IMG_NSEs);
    for (int i=0; i<msize; i++)
      {
        std::string se = ses.at(i);

  std::transform(se.begin(),se.end(),se.begin(),tolower);

        if (se == "google")
          {
            se_enabled |= std::bitset<IMG_NSEs>(SE_GOOGLE_IMG);
          }
        else if (se == "bing")
          {
            se_enabled |= std::bitset<IMG_NSEs>(SE_BING_IMG);
          }
        else if (se == "flickr")
          {
            se_enabled |= std::bitset<IMG_NSEs>(SE_FLICKR);
          }
        else if (se == "wcommons")
          {
            se_enabled |= std::bitset<IMG_NSEs>(SE_WCOMMONS);
          }
        else if (se == "yahoo")
          {
            se_enabled |= std::bitset<IMG_NSEs>(SE_YAHOO_IMG);
          }
        // XXX: other engines come here.
      }
      }*/

  sp_err se_handler_img::parse_ses_output(std::string **outputs, const int &nresults,
                                          std::vector<search_snippet*> &snippets,
                                          const int &count_offset,
                                          query_context *qr,
                                          const feeds &se_enabled)
  {
    int j = 0;
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
                parser_args.push_back(args);

                pthread_t ps_thread;
                int err = pthread_create(&ps_thread, NULL,  // default attribute is PTHREAD_CREATE_JOINABLE
                                         (void * (*)(void *))se_handler_img::parse_output, args);
                if (err != 0)
                  {
                    errlog::log_error(LOG_LEVEL_ERROR, "Error creating parser thread.");
                    parser_threads.push_back(0);
                    delete args;
                    parser_args.push_back(NULL);
                    continue;
                  }
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
            std::copy(parser_args.at(i)->_snippets->begin(),parser_args.at(i)->_snippets->end(),
                      std::back_inserter(snippets));
            parser_args.at(i)->_snippets->clear();
            delete parser_args.at(i)->_snippets;
            delete parser_args.at(i);
          }
      }
    return SP_ERR_OK;
  }

  void se_handler_img::parse_output(ps_thread_arg &args)
  {
    se_parser *se = se_handler_img::create_se_parser(args._se,args._se_idx,
                    static_cast<img_query_context*>(args._qr)->_safesearch);

    if (!se)
      {
        args._err = WB_ERR_NO_ENGINE;
        errlog::log_error(LOG_LEVEL_ERROR,"no image engine for %s",args._se._name.c_str());
        return;
      }
    try
      {
        if (args._se._name == "bing_img")
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
    delete se;
  }

  se_parser* se_handler_img::create_se_parser(const feed_parser &se,
      const size_t &i, const bool &safesearch)
  {
    se_parser *sep = NULL;
    if (se._name == "google_img")
      sep = new se_parser_ggle_img(se.get_url(i));
    else if (se._name == "bing_img")
      {
        se_parser_bing_img *sepb = new se_parser_bing_img(se.get_url(i));
        sepb->_safesearch = safesearch;
        sep = sepb;
      }
    else if (se._name == "flickr")
      sep = new se_parser_flickr(se.get_url(i));
    else if (se._name == "wcommons")
      sep = new se_parser_wcommons(se.get_url(i));
    else if (se._name == "yahoo_img")
      {
        se_parser_yahoo_img *sepy = new se_parser_yahoo_img(se.get_url(i));
        sepy->_safesearch = safesearch;
        sep = sepy;
      }
    return sep;
  }

} /* end of namespace. */
