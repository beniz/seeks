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

#include "query_context.h"
#include "websearch.h"
#include "stl_hash.h"
#include "mem_utils.h"
#include "miscutil.h"
#include "mutexes.h"
#include "urlmatch.h"
#include "mrf.h"
#include "errlog.h"
#include "se_handler.h"
#include "iso639.h"
#include "encode.h"
#include "charset_conv.h"

#include <sys/time.h>
#include <algorithm>
#include <iostream>

using sp::sweeper;
using sp::miscutil;
using sp::urlmatch;
using sp::errlog;
using sp::iso639;
using sp::encode;
using lsh::mrf;

namespace seeks_plugins
{
  std::string query_context::_default_alang = "en";
  std::string query_context::_default_alang_reg = "en-US";

  query_context::query_context()
    :sweepable(),_page_expansion(0),_lsh_ham(NULL),_ulsh_ham(NULL),_compute_tfidf_features(true),
     _registered(false),_npeers(0),_lfilter(NULL)
  {
    mutex_init(&_qc_mutex);
  }

  query_context::query_context(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                               const std::list<const char*> &http_headers)
    :sweepable(),_page_expansion(0),_blekko(false),_lsh_ham(NULL),_ulsh_ham(NULL),_compute_tfidf_features(true),
     _registered(false),_npeers(0),_lfilter(NULL)
  {
    mutex_init(&_qc_mutex);

    // reload config if file has changed.
    websearch::_wconfig->load_config();

    // set query.
    const char *q = miscutil::lookup(parameters,"q");
    if (!q)
      {
        // this should not happen.
        q = "";
      }
    _query = q;

    // set timestamp.
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    _creation_time = _last_time_of_use = tv_now.tv_sec;
    grab_useful_headers(http_headers);

    // sets auto_lang & auto_lang_reg.
    const char *alang = miscutil::lookup(parameters,"lang");
    if (!alang)
      {
        // this should not happen.
        alang = query_context::_default_alang.c_str();
      }
    const char *alang_reg = miscutil::lookup(parameters,"lreg");
    if (!alang_reg)
      {
        // this should not happen.
        alang_reg = query_context::_default_alang_reg.c_str();
      }
    _auto_lang = alang;
    _auto_lang_reg = alang_reg;

    // query hashing, with the language included.
    _query_key = query_context::assemble_query(_query,_auto_lang);
    _query_hash = query_context::hash_query_for_context(_query_key);

    // encoded query.
    char *url_enc_query_str = encode::url_encode(_query.c_str());
    _url_enc_query = url_enc_query_str;
    free(url_enc_query_str);

    // lookup requested engines, if any.
    query_context::fillup_engines(parameters,_engines);

    sweeper::register_sweepable(this);
  }

  query_context::~query_context()
  {
    unregister(); // unregister from websearch plugin.

    _unordered_snippets.clear();

    hash_map<const char*,search_snippet*,hash<const char*>,eqstr>::iterator chit;
    hash_map<const char*,search_snippet*,hash<const char*>,eqstr>::iterator hit
    = _unordered_snippets_title.begin();
    while (hit!=_unordered_snippets_title.end())
      {
        chit = hit;
        ++hit;
        const char *k = (*chit).first;
        _unordered_snippets_title.erase(chit);
        free_const(k);
      }

    std::for_each(_cached_snippets.begin(),_cached_snippets.end(),
                  delete_object());

    hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator idhit1, idhit2;
    idhit1 = _recommended_snippets.begin();
    while(idhit1!=_recommended_snippets.end())
      {
        idhit2 = idhit1;
        ++idhit1;
        delete (*idhit2).second;
        _recommended_snippets.erase(idhit2);
      }

    // clears the LSH hashtable.
    if (_ulsh_ham)
      delete _ulsh_ham;
    if (_lsh_ham)
      delete _lsh_ham;

    for (std::list<const char*>::iterator lit=_useful_http_headers.begin();
         lit!=_useful_http_headers.end(); lit++)
      free_const((*lit));

    if (_lfilter)
      delete _lfilter;
  }

  std::string query_context::sort_query(const std::string &query)
  {
    std::string clean_query = query;
    std::vector<std::string> tokens;
    mrf::tokenize(clean_query,tokens," ");
    std::sort(tokens.begin(),tokens.end(),std::less<std::string>());
    std::string sorted_query;
    size_t ntokens = tokens.size();
    for (size_t i=0; i<ntokens; i++)
      sorted_query += tokens.at(i);
    return sorted_query;
  }

  uint32_t query_context::hash_query_for_context(const std::string &query_key)
  {
    std::string sorted_query = query_context::sort_query(query_key);
    return mrf::mrf_single_feature(sorted_query);
  }

  void query_context::update_parameters(hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    // reset expansion parameter.
    miscutil::unmap(parameters,"expansion");
    std::string exp_str = miscutil::to_string(_page_expansion);
    miscutil::add_map_entry(parameters,"expansion",1,exp_str.c_str(),1);
  }

  bool query_context::sweep_me()
  {
    // check last_time_of_use + delay against current time.
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    double dt = difftime(tv_now.tv_sec,_last_time_of_use);

    //debug
    /* std::cout << "[Debug]:query_context #" << _query_hash
      << ": sweep_me time difference: " << dt << std::endl; */
    //debug

    if (dt >= websearch::_wconfig->_query_context_delay)
      return true;
    else return false;
  }

  void query_context::update_last_time()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    _last_time_of_use = tv_now.tv_sec;
  }

  void query_context::register_qc()
  {
    if (_registered)
      return;
    websearch::_active_qcontexts.insert(std::pair<uint32_t,query_context*>(_query_hash,this));
    _registered = true;
  }

  void query_context::unregister()
  {
    if (!_registered)
      return;
    hash_map<uint32_t,query_context*,id_hash_uint>::iterator hit;
    if ((hit = websearch::_active_qcontexts.find(_query_hash))==websearch::_active_qcontexts.end())
      {
        /**
         * We should not reach here, unless the destructor is called by a derived class.
         */
        /* errlog::log_error(LOG_LEVEL_ERROR,"Cannot find query context when unregistering for query %s",
         	   _query.c_str()); */
        return;
      }
    else
      {
        websearch::_active_qcontexts.erase(hit);  // deletion is controlled elsewhere.
        _registered = false;
      }
  }

  void query_context::generate(client_state *csp,
                               http_response *rsp,
                               const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                               bool &expanded) throw (sp_exception)
  {
    expanded = false;
    const char *expansion = miscutil::lookup(parameters,"expansion");
    if (!expansion)
      {
        throw sp_exception(SP_ERR_CGI_PARAMS,"no expansion given in call parameters");
      }
    char* endptr;
    int horizon = strtol(expansion, &endptr, 0);
    if (*endptr)
      {
        throw sp_exception(SP_ERR_CGI_PARAMS,std::string("wrong expansion value ") + std::string(expansion));
      }
    if (horizon == 0)
      horizon = 1;

    if (horizon > websearch::_wconfig->_max_expansions) // max expansion protection.
      horizon = websearch::_wconfig->_max_expansions;

    const char *cache_check = miscutil::lookup(parameters,"ccheck");

    // grab requested engines, if any.
    // if the list is not included in that of the context, update existing results and perform requested expansion.
    // if the list is included in that of the context, perform expansion, results will be filtered later on.
    if (!cache_check || strcasecmp(cache_check,"yes") == 0)
      {
        feeds beng;
        const char *eng = miscutil::lookup(parameters,"engines");
        if (eng)
          {
            query_context::fillup_engines(parameters,beng);
          }
        else beng = feeds(websearch::_wconfig->_se_default);

        // test inclusion.
        feeds inc = _engines.inter(beng);

        //TODO: unit test the whole engine selection.
        if (!beng.equal(inc))
          {
            // union of beng and fdiff.
            feeds fdiff = _engines.diff(beng);
            feeds fint = _engines.diff(fdiff);

            // catch up expansion with the newly activated engines.
            if (fint.size() > 1 || !fint.has_feed("seeks"))
              {
                try
                  {
                    expand(csp,rsp,parameters,0,_page_expansion,fint);
                  }
                catch (sp_exception &e)
                  {
                    expanded = false;
                    throw e;
                  }
              }
            expanded = true;

            // union engines & fint.
            _engines = _engines.sunion(fint);
          }

        // whether we need to move forward.
        if (_page_expansion > 0 && horizon <= (int)_page_expansion)
          {
            // reset expansion parameter.
            query_context::update_parameters(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters));
            return;
          }
      }

    // perform requested expansion.
    if (_engines.size() > 1 || !_engines.has_feed("seeks"))
      {
        try
          {
            if (!cache_check)
              expand(csp,rsp,parameters,_page_expansion,horizon,_engines);
            else if (strcasecmp(cache_check,"no") == 0)
              expand(csp,rsp,parameters,0,horizon,_engines);
          }
        catch (sp_exception &e)
          {
            expanded = false;
            throw e;
          }
      }
    expanded = true;

    // update horizon.
    _page_expansion = horizon;
  }

  void query_context::expand(client_state *csp,
                             http_response *rsp,
                             const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                             const int &page_start, const int &page_end,
                             const feeds &se_enabled) throw (sp_exception)
  {
    for (int i=page_start; i<page_end; i++) // catches up with requested horizon.
      {
        // resets expansion parameter.
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"expansion");
        std::string i_str = miscutil::to_string(i+1);
        miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
                                "expansion",1,i_str.c_str(),1);

        // query SEs.
        int nresults = 0;
        std::string **outputs = NULL;
        try
          {
            outputs = se_handler::query_to_ses(parameters,nresults,this,se_enabled);
          }
        catch (sp_exception &e)
          {
            throw e; // no engine found or connection error.
          }

        feed_parser fp = se_enabled.find_feed("blekko");
        if (!fp._name.empty())
          _blekko = true; // call once.

        // parse the output and create result search snippets.
        int rank_offset = (i > 0) ? i * websearch::_wconfig->_Nr : 0;

        se_handler::parse_ses_output(outputs,nresults,_cached_snippets,rank_offset,this,se_enabled);
        for (int j=0; j<nresults; j++)
          if (outputs[j])
            delete outputs[j];
        delete[] outputs;
      }
  }

  void query_context::add_to_unordered_cache(search_snippet *sr)
  {
    hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit;
    if ((hit=_unordered_snippets.find(sr->_id))!=_unordered_snippets.end())
      {
        // do nothing.
      }
    else _unordered_snippets.insert(std::pair<uint32_t,search_snippet*>(sr->_id,sr));
  }

  void query_context::remove_from_unordered_cache(const uint32_t &id)
  {
    hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit;
    if ((hit=_unordered_snippets.find(id))!=_unordered_snippets.end())
      {
        _unordered_snippets.erase(hit);
      }
  }

  void query_context::update_unordered_cache()
  {
    size_t cs_size = _cached_snippets.size();
    for (size_t i=0; i<cs_size; i++)
      {
        hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit;
        if ((hit=_unordered_snippets.find(_cached_snippets[i]->_id))!=_unordered_snippets.end())
          {
            // for now, do nothing. TODO: may merge snippets here.
          }
        else
          _unordered_snippets.insert(std::pair<uint32_t,search_snippet*>(_cached_snippets[i]->_id,
                                     _cached_snippets[i]));
      }
  }

  search_snippet* query_context::get_cached_snippet(const std::string &url) const
  {
    std::string url_lc(url);
    std::transform(url.begin(),url.end(),url_lc.begin(),tolower);
    std::string surl = urlmatch::strip_url(url_lc);
    uint32_t id = mrf::mrf_single_feature(surl);
    return get_cached_snippet(id);
  }

  search_snippet* query_context::get_cached_snippet(const uint32_t &id) const
  {
    hash_map<uint32_t,search_snippet*,id_hash_uint>::const_iterator hit;
    if ((hit = _unordered_snippets.find(id))==_unordered_snippets.end())
      return NULL;
    else return (*hit).second;
  }

  void query_context::add_to_unordered_cache_title(search_snippet *sr)
  {
    std::string lctitle = sr->_title;
    std::transform(lctitle.begin(),lctitle.end(),lctitle.begin(),tolower);
    hash_map<const char*,search_snippet*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=_unordered_snippets_title.find(lctitle.c_str()))!=_unordered_snippets_title.end())
      {
        // do nothing.
      }
    else _unordered_snippets_title.insert(std::pair<const char*,search_snippet*>(strdup(lctitle.c_str()),sr));
  }

  search_snippet* query_context::get_cached_snippet_title(const char *lctitle)
  {
    hash_map<const char*,search_snippet*,hash<const char*>,eqstr>::iterator hit;
    if ((hit = _unordered_snippets_title.find(lctitle))==_unordered_snippets_title.end())
      return NULL;
    else return (*hit).second;
  }

  bool query_context::has_lang(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                               std::string &qlang)
  {
    const char *alang = miscutil::lookup(parameters,"lang");
    if (alang)
      {
        qlang = alang;
        return true;
      }
    else return false;
  }

  bool query_context::has_query_lang(const std::string &query,
                                     std::string &qlang)
  {
    if (query.empty() || query[0] != ':') // XXX: could chomp the query.
      {
        qlang = "";
        return false;
      }
    try
      {
        qlang = query.substr(1,2); // : + 2 characters for the language.
      }
    catch (std::exception &e)
      {
        qlang = "";
      }

    // check whether the language is known ! -> XXX: language table...
    if (iso639::has_code(qlang.c_str()))
      {
        return true;
      }
    else
      {
        errlog::log_error(LOG_LEVEL_INFO,"in query command test: language code not found: %s",qlang.c_str());
        qlang = "";
        return false;
      }
  }

  // static.
  void query_context::detect_query_lang_http(const std::list<const char*> &http_headers,
      std::string &lang, std::string &lang_reg)
  {
    std::list<const char*>::const_iterator sit = http_headers.begin();
    while (sit!=http_headers.end())
      {
        if (miscutil::strncmpic((*sit),"accept-language:",16) == 0)
          {
            // detect language.
            std::string lang_head = (*sit);
            size_t pos = lang_head.find_first_of(" ");
            if (pos != std::string::npos && pos+6<=lang_head.length() && lang_head[pos+3] == '-')
              {
                try
                  {
                    lang = lang_head.substr(pos+1,2);
                    lang_reg = lang_head.substr(pos+1,5);
                  }
                catch (std::exception &e)
                  {
                    lang = query_context::_default_alang;
                    lang_reg = query_context::_default_alang_reg; // default.
                  }
                errlog::log_error(LOG_LEVEL_DEBUG,"Query language detection: %s",lang_reg.c_str());
                return;
              }
            else if (pos != std::string::npos && pos+3<=lang_head.length())
              {
                try
                  {
                    lang = lang_head.substr(pos+1,2);
                  }
                catch (std::exception &e)
                  {
                    lang = query_context::_default_alang; // default.
                  }
                lang_reg = query_context::lang_forced_region(lang);
                errlog::log_error(LOG_LEVEL_DEBUG,"Forced query language region at detection: %s",lang_reg.c_str());
                return;
              }
          }
        ++sit;
      }
    lang_reg = query_context::_default_alang_reg; // beware, returning hardcoded default (since config value is most likely "auto").
    lang = query_context::_default_alang;
  }

  std::string query_context::detect_base_url_http(client_state *csp)
  {
    std::list<const char*> headers = csp->_headers;

    // first we try to get base_url from a custom header
    std::string base_url;
    std::list<const char*>::const_iterator sit = headers.begin();
    while (sit!=headers.end())
      {
        if (miscutil::strncmpic((*sit),"Seeks-Remote-Location:",22) == 0)
          {
            base_url = (*sit);
            size_t pos = base_url.find_first_of(" ");
            try
              {
                base_url = base_url.substr(pos+1);
              }
            catch (std::exception &e)
              {
                base_url = "";
                break;
              }
            break;
          }
        ++sit;
      }
    if (base_url.empty())
      {
        // if no custom header, we build base_url from the generic Host header
        std::list<const char*>::const_iterator sit = headers.begin();
        while (sit!=headers.end())
          {
            if (miscutil::strncmpic((*sit),"Host:",5) == 0)
              {
                base_url = (*sit);
                size_t pos = base_url.find_first_of(" ");
                try
                  {
                    base_url = base_url.substr(pos+1);
                  }
                catch (std::exception &e)
                  {
                    base_url = "";
                    return base_url;
                  }
                break;
              }
            ++sit;
          }
        base_url = csp->_http._ssl ? "https://" : "http://" + base_url;
      }
    return base_url;
  }

  std::string query_context::assemble_query(const std::string &query,
      const std::string &lang)
  {
    if (!lang.empty())
      {
        return ":" + lang + " " + query;
      }
    else return query;
  }

  void query_context::grab_useful_headers(const std::list<const char*> &http_headers)
  {
    std::list<const char*>::const_iterator sit = http_headers.begin();
    while (sit!=http_headers.end())
      {
        // user-agent
        if (miscutil::strncmpic((*sit),"user-agent:",11) == 0)
          {
            const char *ua = strdup((*sit));
            _useful_http_headers.push_back(ua);
          }
        else if (miscutil::strncmpic((*sit),"accept-charset:",15) == 0)
          {
            const char *ac = strdup((*sit));
            /* std::string ac_str = "accept-charset: utf-8";
            const char *ac = strdup(ac_str.c_str()); */
            _useful_http_headers.push_back(ac);
          }
        else if (miscutil::strncmpic((*sit),"accept:",7) == 0)
          {
            const char *aa = strdup((*sit));
            _useful_http_headers.push_back(aa);
          }
        // XXX: other useful headers should be detected and stored here.
        ++sit;
      }
  }

  std::string query_context::lang_forced_region(const std::string &auto_lang)
  {
    // XXX: in-query language commands force the query language to the search engine.
    // As such, we have to decide which region we attach to every of the most common
    // language forced queries.
    // Evidently, this is not a robust nor fast solution. Full support of locales etc... should
    // appear in the future. As for now, this is a simple scheme for a simple need.
    // Unsupported languages default to american english, that's how the world is right
    // now...
    std::string region_lang = query_context::_default_alang_reg; // default.
    if (auto_lang == "en")
      {
      }
    else if (auto_lang == "fr")
      region_lang = "fr-FR";
    else if (auto_lang == "de")
      region_lang = "de-DE";
    else if (auto_lang == "it")
      region_lang = "it-IT";
    else if (auto_lang == "es")
      region_lang = "es-ES";
    else if (auto_lang == "pt")
      region_lang = "es-PT"; // so long for Brazil (BR)...
    else if (auto_lang == "nl")
      region_lang = "nl-NL";
    else if (auto_lang == "ja")
      region_lang = "ja-JP";
    else if (auto_lang == "no")
      region_lang = "no-NO";
    else if (auto_lang == "pl")
      region_lang = "pl-PL";
    else if (auto_lang == "ru")
      region_lang = "ru-RU";
    else if (auto_lang == "ro")
      region_lang = "ro-RO";
    else if (auto_lang == "sh")
      region_lang = "sh-RS"; // Serbia.
    else if (auto_lang == "sl")
      region_lang = "sl-SL";
    else if (auto_lang == "sk")
      region_lang = "sk-SK";
    else if (auto_lang == "sv")
      region_lang = "sv-SE";
    else if (auto_lang == "th")
      region_lang = "th-TH";
    else if (auto_lang == "uk")
      region_lang = "uk-UA";
    else if (auto_lang == "zh")
      region_lang = "zh-CN";
    else if (auto_lang == "ko")
      region_lang = "ko-KR";
    else if (auto_lang == "ar")
      region_lang = "ar-EG"; // Egypt, with _NO_ reasons. In most cases, the search engines will decide based on the 'ar' code.
    else if (auto_lang == "be")
      region_lang = "be-BY";
    else if (auto_lang == "bg")
      region_lang = "bg-BG";
    else if (auto_lang == "bs")
      region_lang = "bs-BA";
    else if (auto_lang == "cs")
      region_lang = "cs-CZ";
    else if (auto_lang == "fi")
      region_lang = "fi-FI";
    else if (auto_lang == "he")
      region_lang = "he-IL";
    else if (auto_lang == "hi")
      region_lang = "hi-IN";
    else if (auto_lang == "hr")
      region_lang = "hr-HR";
    return region_lang;
  }

  std::string query_context::generate_lang_http_header() const
  {
    return "accept-language: " + _auto_lang + "," + _auto_lang_reg + ";q=0.5";
  }

  void query_context::in_query_command_forced_region(std::string &auto_lang,
      std::string &region_lang)
  {
    region_lang = query_context::lang_forced_region(auto_lang);
    if (region_lang == query_context::_default_alang_reg) // in case we are on the default language.
      auto_lang = query_context::_default_alang;
  }

  void query_context::fillup_engines(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                     feeds &engines)
  {
    const char *eng = miscutil::lookup(parameters,"engines");
    if (eng)
      {
        std::string engines_str = std::string(eng);
        std::vector<std::string> vec_engines;
        miscutil::tokenize(engines_str,vec_engines,",");
        for (size_t i=0; i<vec_engines.size(); i++)
          {
            std::string engine = vec_engines.at(i);
            std::vector<std::string> vec_names;
            miscutil::tokenize(engine,vec_names,":");
            if (vec_names.size()==1)
              engines.add_feed(engine,websearch::_wconfig);
            else engines.add_feed(vec_names,
                                    websearch::_wconfig);
          }
      }
    else engines = feeds(websearch::_wconfig->_se_default);
  }

  void query_context::reset_snippets_personalization_flags()
  {
    std::vector<search_snippet*>::iterator vit = _cached_snippets.begin();
    while (vit!=_cached_snippets.end())
      {
        //std::cerr << "reviewing URL: " << (*vit)->_url << std::endl;
        if ((*vit)->_personalized)
          {
            (*vit)->_personalized = false;
            if ((*vit)->_engine.count() == 1
                && (*vit)->_engine.has_feed("seeks"))
              {
                remove_from_unordered_cache((*vit)->_id);
                delete (*vit);
                vit = _cached_snippets.erase(vit);
                continue;
              }
            else if ((*vit)->_engine.has_feed("seeks"))
              (*vit)->_engine.remove_feed("seeks");
            (*vit)->_meta_rank = (*vit)->_engine.size(); //TODO: wrong, every feed_parser may refer to several urls.
            (*vit)->_seeks_rank = 0;
            (*vit)->bing_yahoo_us_merge();
            (*vit)->_npeers = 0;
            (*vit)->_hits = 0;
          }
        else (*vit)->_seeks_rank = 0; // reset.
        ++vit;
      }
  }

  bool query_context::update_recommended_urls()
  {
    bool cache_changed = false;
    hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit, hit2, cit;
    hit = _recommended_snippets.begin();
    while(hit!=_recommended_snippets.end())
      {
        cit = _unordered_snippets.find((*hit).first);
        if (cit != _unordered_snippets.end())
          {
            hit2 = hit;
            ++hit;
            delete (*hit2).second;
            _recommended_snippets.erase(hit2);
          }
        else if (!(*hit).second->_title.empty())
          {
            (*hit).second->_qc = this;
            (*hit).second->_personalized = true;
            (*hit).second->_engine.add_feed("seeks","s.s");
            (*hit).second->_meta_rank++;
            _cached_snippets.push_back((*hit).second);
            //add_to_unordered_cache((*hit).second);
            cache_changed = true;
            hit2 = hit;
            ++hit;
            _recommended_snippets.erase(hit2);
          }
        else ++hit;
      }
    return cache_changed;
  }

} /* end of namespace. */
