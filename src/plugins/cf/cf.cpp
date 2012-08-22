/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "cf.h"
#include "websearch.h"
#include "sort_rank.h"
#include "json_renderer.h"
#include "cf_configuration.h"
#include "rank_estimators.h"
#include "query_recommender.h"
#include "uri_capture.h"
#include "query_capture.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "plugin_manager.h"
#include "cgi.h"
#include "cgisimple.h"
#include "qprocess.h"
#include "encode.h"
#include "urlmatch.h"
#include "miscutil.h"
#include "charset_conv.h"
#include "errlog.h"
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
#include "xsl_serializer.h"
#endif

#include <sys/stat.h>
#include <sys/times.h>
#include <iostream>

using lsh::qprocess;

namespace seeks_plugins
{

  plugin* cf::_uc_plugin = NULL;
  plugin* cf::_xs_plugin = NULL;
  bool cf::_xs_plugin_activated = false;

  cf::cf()
    :plugin()
  {
    _name = "cf";
    _version_major = "0";
    _version_minor = "1";

    // configuration.
    if (seeks_proxy::_datadir.empty())
      _config_filename = plugin_manager::_plugin_repository + "cf/cf-config";
    else
      _config_filename = seeks_proxy::_datadir + "/plugins/cf/cf-config";

#ifdef SEEKS_CONFIGDIR
    struct stat stFileInfo;
    if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
      {
        _config_filename = SEEKS_CONFIGDIR "/cf-config";
      }
#endif

    if (cf_configuration::_config == NULL)
      cf_configuration::_config = new cf_configuration(_config_filename);
    _configuration = cf_configuration::_config;

    // cgi dispatchers.
    cgi_dispatcher *cgid_peers
    = new cgi_dispatcher("peers",&cf::cgi_peers,NULL,TRUE);
    _cgi_dispatchers.push_back(cgid_peers);

    cgi_dispatcher *cgid_suggestion
    = new cgi_dispatcher("suggestion",&cf::cgi_suggestion,NULL,TRUE);
    _cgi_dispatchers.push_back(cgid_suggestion);

    cgi_dispatcher *cgid_recommendation
    = new cgi_dispatcher("recommendation",&cf::cgi_recommendation,NULL,TRUE);
    _cgi_dispatchers.push_back(cgid_recommendation);
  }

  cf::~cf()
  {
    cf_configuration::_config = NULL; // configuration is deleted in parent class.
  }

  void cf::start()
  {
    //TODO: check on user_db.

    // look for dependent plugins.
    cf::_uc_plugin = plugin_manager::get_plugin("uri-capture");
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    _xs_plugin = plugin_manager::get_plugin("xsl-serializer");
    _xs_plugin_activated = seeks_proxy::_config->is_plugin_activated("xsl-serializer");
#endif

  }

  void cf::stop()
  {
  }

  sp_err cf::cgi_peers(client_state *csp,
                       http_response *rsp,
                       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    hash_map<const char*,peer*,hash<const char*>,eqstr>::const_iterator hit
    = cf_configuration::_config->_pl->_peers.begin();
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    const char *output_str = miscutil::lookup(parameters,"output");
    if (cf::_xs_plugin && cf::_xs_plugin_activated && !miscutil::strcmpic(output_str, "xml"))
      {
	std::list<std::string> l;
        while(hit!=cf_configuration::_config->_pl->_peers.end())
          {
            peer *p = (*hit).second;
            l.push_back(p->_host + ((p->_port == -1) ? "" : (":" + miscutil::to_string(p->_port))) + p->_path);
            ++hit;
          }
        return (static_cast<xsl_serializer*>(cf::_xs_plugin))->render_xsl_peers(csp,rsp,parameters, &l);
      }
    else
      {
#endif
	Json::Value jpeers,jres;
        while(hit!=cf_configuration::_config->_pl->_peers.end())
          {
            peer *p = (*hit).second;
	    jpeers.append(p->_host + ((p->_port == -1) ? "" : (":" + miscutil::to_string(p->_port))) + p->_path);
            ++hit;
          }
	jres["peers"] = jpeers;
	Json::FastWriter writer;
	json_renderer::response(rsp,writer.write(jres));
        return SP_ERR_OK;
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
      }
#endif
  }

  sp_err cf::cgi_tbd(client_state *csp,
                     http_response *rsp,
                     const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    // check for query and snippet id.
    const char *query = miscutil::lookup(parameters,"q");
    if (!query)
      return SP_ERR_CGI_PARAMS; // 400 error.
    const char *url_str = miscutil::lookup(parameters,"url");
    if (!url_str)
      return SP_ERR_CGI_PARAMS; // 400 error.
    std::string url = url_str;
    if (url.empty())
      return SP_ERR_CGI_PARAMS; // 400 error.

    sp_err err = cf::tbd(parameters,url,query);
    if (err != SP_ERR_OK && err == SP_ERR_CGI_PARAMS)
      {
        errlog::log_error(LOG_LEVEL_INFO,"bad parameter to tbd callback");
      }
    return err;
  }

  sp_err cf::tbd(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                 const std::string &url, const std::string &query)
  {
    char *dec_urlp = encode::url_decode_but_not_plus(url.c_str());
    std::string durl = std::string(dec_urlp);
    free(dec_urlp);
    const char *langp = miscutil::lookup(parameters,"lang");
    if (!langp)
      {
        // XXX: this should not happen.
        return SP_ERR_CGI_PARAMS;
      }
    std::string lang = std::string(langp);
    try
      {
        cf::thumb_down_url(query,lang,url);
      }
    catch(sp_exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR,e.to_string().c_str());
        return e.code();
      }
    return SP_ERR_OK;
  }

  sp_err cf::cgi_suggestion(client_state *csp,
                            http_response *rsp,
                            const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    // check for query, part of path.
    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/suggestion/","");
    std::string query = urlmatch::next_elt_from_path(path);
    if (query.empty())
      return cgi::cgi_error_bad_param(csp,rsp,parameters,"json"); // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q",1,query.c_str(),1); // add query to parameters.

    try
      {
        bool has_lang;
        websearch::preprocess_parameters(parameters,csp,has_lang);
      }
    catch(sp_exception &e)
      {
        return e.code();
      }

    // check on parameters.
    const char *peers = miscutil::lookup(parameters,"peers");
    if (peers && strcasecmp(peers,"local")!=0 && strcasecmp(peers,"ring")!=0)
      return SP_ERR_CGI_PARAMS;
    int radius = -1;
    const char *radius_str = miscutil::lookup(parameters,"radius");
    if (radius_str)
      {
        char *endptr;
        int tmp = strtol(radius_str,&endptr,0);
        if (*endptr)
          {
            errlog::log_error(LOG_LEVEL_ERROR,"wrong radius parameter");
            return SP_ERR_CGI_PARAMS;
          }
        else radius = tmp;
      }
    bool swf = cf_configuration::_config->_stop_words_filtering;
    const char *swf_str = miscutil::lookup(parameters,"swords");
    if (swf_str)
      {
        if (strcasecmp(swf_str,"yes")==0)
          swf = true;
        else if (strcasecmp(swf_str,"no")==0)
          swf = false;
      }

    // ask all peers if 'ring' is specified.
    // cost is nearly the same to grab both queries and URLs from
    // remote peers, and cache the requested data.
    // for this reason, we call to 'personalize', that fetches both
    // queries and URLs, rank and cache them into memory.
    mutex_lock(&websearch::_context_mutex);
    query_context *qc = websearch::lookup_qc(parameters);
    if (!qc)
      {
        qc = new query_context(parameters,csp->_headers);
        qc->register_qc();
      }
    mutex_unlock(&websearch::_context_mutex);
    mutex_lock(&qc->_qc_mutex);
    cf::personalize(qc,false,cf::select_p2p_or_local(parameters),radius,swf);
    sp_err err=SP_ERR_OK;
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    const char *output_str = miscutil::lookup(parameters,"output");
    if (cf::_xs_plugin && cf::_xs_plugin_activated && !miscutil::strcmpic(output_str, "xml"))
      err = static_cast<xsl_serializer*>(cf::_xs_plugin)->render_xsl_suggested_queries(csp,rsp,parameters,qc);
    else
#endif
      json_renderer::render_json_suggested_queries(qc,rsp,parameters);
    
    qc->reset_p2p_data();
    mutex_unlock(&qc->_qc_mutex);
    return err;
  }

  sp_err cf::cgi_recommendation(client_state *csp,
                                http_response *rsp,
                                const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    // check for HTTP method and route the call.
    std::string http_method = csp->_http._gpc;
    std::transform(http_method.begin(),http_method.end(),http_method.begin(),tolower);

    if (http_method == "get")
      {
        return cf::recommendation_get(csp,rsp,parameters);
      }
    else if (http_method == "post")
      {
        if (cf_configuration::_config->_remote_post)
          return cf::recommendation_post(csp,rsp,parameters);
        else return cgisimple::cgi_error_unauthorized(csp,rsp,parameters);
      }
    else if (http_method == "delete")
      {
        return cf::recommendation_delete(csp,rsp,parameters);
      }
    else
      {
        // error.
        errlog::log_error(LOG_LEVEL_ERROR,"wrong HTTP method %s for recommendation call",
                          http_method.c_str());
        return cgi::cgi_error_bad_param(csp,rsp,parameters,"json");
      }
  }

  sp_err cf::recommendation_get(client_state *csp,
                                http_response *rsp,
                                const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    struct tms st_cpu;
    struct tms en_cpu;
    clock_t start_time = times(&st_cpu);

    // check for query, part of path.
    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/recommendation/","");
    std::string query = urlmatch::next_elt_from_path(path);
    if (query.empty())
      return cgi::cgi_error_bad_param(csp,rsp,parameters,"json"); // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q",1,query.c_str(),1); // add query to parameters.
    bool has_lang;

    try
      {
        websearch::preprocess_parameters(parameters,csp,has_lang);
      }
    catch(sp_exception &e)
      {
        return e.code();
      }

    // check on parameters.
    const char *peers = miscutil::lookup(parameters,"peers");
    if (peers && strcasecmp(peers,"local")!=0 && strcasecmp(peers,"ring")!=0)
      return SP_ERR_CGI_PARAMS;
    int radius = -1;
    const char *radius_str = miscutil::lookup(parameters,"radius");
    if (radius_str)
      {
        char *endptr;
        int tmp = strtol(radius_str,&endptr,0);
        if (*endptr)
          {
            errlog::log_error(LOG_LEVEL_ERROR,"wrong radius parameter");
            return SP_ERR_CGI_PARAMS;
          }
        else radius = tmp;
      }
    bool swf = cf_configuration::_config->_stop_words_filtering;
    const char *swf_str = miscutil::lookup(parameters,"swords");
    if (swf_str)
      {
        if (strcasecmp(swf_str,"yes")==0)
          swf = true;
        else if (strcasecmp(swf_str,"no")==0)
          swf = false;
      }

    // ask all peers.
    mutex_lock(&websearch::_context_mutex);
    query_context *qc = websearch::lookup_qc(parameters);
    if (!qc)
      {
        qc = new query_context(parameters,csp->_headers);
        qc->register_qc();
      }
    mutex_unlock(&websearch::_context_mutex);
    mutex_lock(&qc->_qc_mutex);
    cf::personalize(qc,false,cf::select_p2p_or_local(parameters),radius,swf);
    sort_rank::sort_merge_and_rank_snippets(qc,qc->_cached_snippets,parameters); // in case the context is already in memory.
    clock_t end_time = times(&en_cpu);
    double qtime = (end_time-start_time)/websearch::_cl_sec;
    std::string lang;
    if (has_lang)
      {
        const char *lang_str = miscutil::lookup(parameters,"lang");
        if (lang_str) // the opposite should never happen.
          lang = lang_str;
      }
    sp_err err = SP_ERR_OK;
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    const char *output_str = miscutil::lookup(parameters,"output");
    if (cf::_xs_plugin && cf::_xs_plugin_activated && !miscutil::strcmpic(output_str, "xml"))
      err = static_cast<xsl_serializer*>(cf::_xs_plugin)->render_xsl_recommendations(csp,rsp,parameters,qc,qtime,radius,lang);
    else
#endif
      json_renderer::render_json_recommendations(qc,rsp,parameters,qtime,radius,lang);


    qc->reset_p2p_data();
    mutex_unlock(&qc->_qc_mutex);
    return err;
  }

  sp_err cf::recommendation_post(client_state *csp,
                                 http_response *rsp,
                                 const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    // check for query, part of path.
    std::string ref_path = csp->_http._path;
    miscutil::replace_in_string(ref_path,"/recommendation/","");
    std::string query = urlmatch::next_elt_from_path(ref_path);
    if (query.empty())
      return cgi::cgi_error_bad_param(csp,rsp,parameters,"json"); // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q",1,query.c_str(),1); // add query to parameters.
    bool has_lang;

    try
      {
        websearch::preprocess_parameters(parameters,csp,has_lang);
      }
    catch(sp_exception &e)
      {
        return e.code();
      }
    query = miscutil::lookup(parameters,"q");

    // check for missing parameters.
    const char *url_str = miscutil::lookup(parameters,"url");
    if (!url_str)
      return cgi::cgi_error_bad_param(csp,rsp,parameters,"json"); // 400 error.
    std::string url = url_str;

    // check for optional parameters.
    bool url_check = cf_configuration::_config->_post_url_check;
    const char *url_check_str = miscutil::lookup(parameters,"url-check");
    if (url_check_str) // forces positive check only.
      {
        char *endptr;
        int tmp = strtol(url_check_str,&endptr,0);
        if (!*endptr)
          {
            bool opt_url_check = static_cast<bool>(tmp); // XXX: beware.
            if (!url_check && opt_url_check)
              url_check = true;
          }
      }
    std::string title;
    const char *title_str = miscutil::lookup(parameters,"title");
    if (title_str)
      {
        char *dec_title_str = encode::url_decode(title_str);
        title = dec_title_str;
        free(dec_title_str);
      }
    if (!title.empty())
      title = charset_conv::charset_check_and_conversion(title,csp->_headers); // make title empty if bad charset.
    int radius = cf_configuration::_config->_post_radius;
    const char *radius_str = miscutil::lookup(parameters,"radius");
    if (radius_str)
      {
        char *endptr;
        int tmp = strtol(radius_str,&endptr,0);
        if (!*endptr)
          {
            radius = tmp;
          }
      }

    // create a query_context.
    bool has_qc = true;
    query_context *qc = websearch::lookup_qc(parameters);
    if (!qc)
      {
        has_qc = false;
        qc = new query_context(parameters,csp->_headers); // empty context.
        qc->register_qc();
      }
    mutex_lock(&qc->_qc_mutex);

    // check on URL if needed.
    bool check_success = true;
    std::string check_title;
    if (url_check)
      {
        std::vector<std::string> uris, titles;
        uris.push_back(url);
        std::vector<std::list<const char*>*> headers;
        std::list<const char*> *lheaders = &qc->_useful_http_headers;
        if (!miscutil::list_contains_item(lheaders,"user-agent"))
          miscutil::enlist(lheaders,cf_configuration::_config->_post_url_ua.c_str());
        headers.push_back(lheaders);
        uri_capture::fetch_uri_html_title(uris,titles,5,&headers); // timeout: 5 sec.
        if (titles.empty()
            || (titles.at(0).empty() && title.empty()))
          check_success = false;
        else check_title = titles.at(0);
      }
    if (!check_success && !has_qc)
      {
        sweeper::unregister_sweepable(qc);
        delete qc; // unlocks qc
        if (check_title == "404")
          return cgisimple::cgi_error_404(csp,rsp,parameters); // 404. TODO: JSON + message ?
        else return cgi::cgi_error_bad_param(csp,rsp,parameters,"json"); // 400 error.
      }
    else if (url_check)
      title = title.empty() ? check_title : title;

    // create snippet.
    search_snippet *sp = new search_snippet(); // XXX: no rank given, temporary snippet.
    sp->set_url(url);
    sp->set_title(title);
    sp->_qc = qc;
    if (has_lang)
      {
        const char *lang_str = miscutil::lookup(parameters,"lang");
        sp->set_lang(lang_str);
      }
    qc->add_to_cache(sp);
    if (has_qc)
      sort_rank::sort_merge_and_rank_snippets(qc,qc->_cached_snippets,parameters);
    qc->add_to_unordered_cache(sp);
    qc->add_to_unordered_cache_title(sp);

    // store query.
    std::string host,path;
    query_capture::process_url(url,host,path);
    int err = SP_ERR_OK;
    try
      {
        query_capture_element::store_queries(query,qc,url,host,"query-capture",radius);
      }
    catch (sp_exception &e)
      {
        err = e.code();
      }

    // remove query_context as needed (so to not 'flood' the node).
    if (!has_qc)
      {
        sweeper::unregister_sweepable(qc);
        delete qc;
      }
    mutex_unlock(&qc->_qc_mutex);
    return err;
  }

  sp_err cf::recommendation_delete(client_state *csp,
                                   http_response *rsp,
                                   const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    // check for query, part of path.
    std::string ref_path = csp->_http._path;
    miscutil::replace_in_string(ref_path,"/recommendation/","");
    std::string query = urlmatch::next_elt_from_path(ref_path);
    if (query.empty())
      return cgi::cgi_error_bad_param(csp,rsp,parameters,"json"); // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q",1,query.c_str(),1); // add query to parameters.
    bool has_lang;

    try
      {
        websearch::preprocess_parameters(parameters,csp,has_lang);
      }
    catch(sp_exception &e)
      {
        return e.code();
      }
    query = miscutil::lookup(parameters,"q");

    // check for missing parameters.
    std::string url;
    const char *url_str = miscutil::lookup(parameters,"url");
    if (url_str)
      url = url_str;

    uint32_t hits = 0;
    std::string host;
    query_capture::process_url(url,host);

    if (!url_str) // remove query with all attached urls.
      {
        int radius = cf_configuration::_config->_post_radius;
        const char *radius_str = miscutil::lookup(parameters,"radius");
        if (radius_str)
          {
            char *endptr;
            int tmp = strtol(radius_str,&endptr,0);
            if (!*endptr)
              {
                radius = tmp;
              }
          }
        try
          {
            query_capture_element::remove_queries(query,"query-capture",radius);
          }
        catch(sp_exception &e)
          {
            return e.code();
          }

        // remove query from cache if applicable.
        query_context *qc = websearch::lookup_qc(parameters);
        if (qc)
          {
            sweeper::unregister_sweepable(qc);
            delete qc;
          }
        return SP_ERR_OK;
      }

    hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
    qprocess::generate_query_hashes(query,0,0,features);
    DHTKey key = (*features.begin()).second;
    std::string key_str = key.to_rstring();
    db_record *dbr = seeks_proxy::_user_db->find_dbr(key_str,"query-capture");
    db_query_record *dbqr = static_cast<db_query_record*>(dbr);
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit;
    if ((hit=dbqr->_related_queries.find(query.c_str()))!=dbqr->_related_queries.end())
      {
        query_data *qd = (*hit).second;
        hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::const_iterator vit;
        if (qd->_visited_urls && (vit = qd->_visited_urls->find(url.c_str()))!=qd->_visited_urls->end())
          {
            vurl_data *vd = (*vit).second;
            if (has_lang)
              {
                const char *lang_str = miscutil::lookup(parameters,"lang");
                if (lang_str && vd->_url_lang == std::string(lang_str))
                  {
                    hits = vd->_hits;
                  }
                else
                  {
                    delete dbqr;
                    return DB_ERR_NO_REC;
                  }
              }
            else hits = vd->_hits;
          }
        else
          {
            errlog::log_error(LOG_LEVEL_INFO,"can't find url %s when trying to remove it",
                              url.c_str());
            delete dbqr;
            return DB_ERR_NO_REC;
          }
      }
    else
      {
        errlog::log_error(LOG_LEVEL_INFO,"can't find query %s when trying to remove url %s",
                          query.c_str(),url.c_str());
        delete dbqr;
        return DB_ERR_NO_REC;
      }
    delete dbqr;

    try
      {
        query_capture_element::remove_url(key,query,url,"",hits,0,"query-capture");
      }
    catch(sp_exception &e)
      {
        return SP_ERR_MEMORY; // 500.
      }

    // remove from cache if applicable.
    query_context *qc = websearch::lookup_qc(parameters);
    if (qc)
      {
        mutex_lock(&qc->_qc_mutex);
        search_snippet sp;
        sp.set_url(url);
        qc->remove_from_unordered_cache(sp._id);
        qc->remove_from_cache(&sp);
        mutex_unlock(&qc->_qc_mutex);
      }

    return SP_ERR_OK;
  }

  void cf::personalize(query_context *qc,
                       const bool &wait_external_sources,
                       const std::string &peers,
                       const int &radius,
                       const bool &swf)
  {
    // check on config file, in case it did change.
    cf_configuration::_config->load_config();
    pthread_rwlock_rdlock(&cf_configuration::_config->_conf_rwlock);

    rank_estimator *sre = rank_estimator::create(cf_configuration::_config->_estimator,swf);
    if (!sre)
      {
        pthread_rwlock_unlock(&cf_configuration::_config->_conf_rwlock);
        errlog::log_error(LOG_LEVEL_ERROR,"unknown estimator %s passed to collaborative filter",
                          cf_configuration::_config->_estimator.c_str());
        return;
      }
    sre->peers_personalize(qc,wait_external_sources,peers,radius);
    pthread_rwlock_unlock(&cf_configuration::_config->_conf_rwlock);
    delete sre;
  }

  void cf::estimate_ranks(const std::string &query,
                          const std::string &lang,
                          const int &radius,
                          std::vector<search_snippet*> &snippets,
                          const std::string &host,
                          const int &port) throw (sp_exception)
  {
    rank_estimator *sre = rank_estimator::create(cf_configuration::_config->_estimator,
                          cf_configuration::_config->_stop_words_filtering);
    if (!sre)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"unknown estimator %s passed to collaborative filter",
                          cf_configuration::_config->_estimator.c_str());
        return;
      }
    sre->estimate_ranks(query,lang,radius,snippets,host,port);
    delete sre;
  }

  void cf::thumb_down_url(const std::string &query,
                          const std::string &lang,
                          const std::string &url) throw (sp_exception)
  {
    rank_estimator *sre = rank_estimator::create(cf_configuration::_config->_estimator,
                          cf_configuration::_config->_stop_words_filtering);
    if (!sre)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"unknown estimator %s passed to collaborative filter",
                          cf_configuration::_config->_estimator.c_str());
        return;
      }
    sre->thumb_down_url(query,lang,url);
  }

  void cf::find_bqc_cb(const std::vector<std::string> &qhashes,
                       const int &radius,
                       db_query_record *&dbr)
  {
    rank_estimator re(cf_configuration::_config->_stop_words_filtering);
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
    rank_estimator::fetch_user_db_record(qhashes,
                                         seeks_proxy::_user_db,
                                         records);
    std::string query,lang;
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    hash_map<const char*,std::vector<query_data*>,hash<const char*>,eqstr> inv_qdata;
    re.extract_queries(query,lang,radius,seeks_proxy::_user_db,
                       records,qdata,inv_qdata);
    if (!qdata.empty())
      dbr = new db_query_record("query-capture",qdata); // no copy.
    else dbr = NULL;
    rank_estimator::destroy_records(records);
    rank_estimator::destroy_inv_qdata_key(inv_qdata);
  }

  std::string cf::select_p2p_or_local(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    std::string str = "ring"; // ring is default.
    const char *peers = miscutil::lookup(parameters,"peers");
    if (peers && strcasecmp(peers,"local")==0)
      str = "local";
    return str;
  }

  /* plugin registration. */
  extern "C"
  {
    plugin* maker()
    {
      return new cf;
    }
  }

} /* end of namespace. */
