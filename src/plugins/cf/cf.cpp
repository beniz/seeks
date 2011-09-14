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
#include "json_renderer_private.h"
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
#include "encode.h"
#include "urlmatch.h"
#include "miscutil.h"
#include "charset_conv.h"
#include "errlog.h"

#include <sys/stat.h>
#include <sys/times.h>
#include <iostream>

using namespace json_renderer_private;

namespace seeks_plugins
{

  plugin* cf::_uc_plugin = NULL;

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
  }

  void cf::stop()
  {
  }

  sp_err cf::cgi_peers(client_state *csp,
                       http_response *rsp,
                       const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    std::list<std::string> l;
    hash_map<const char*,peer*,hash<const char*>,eqstr>::const_iterator hit
    = cf_configuration::_config->_pl->_peers.begin();
    while(hit!=cf_configuration::_config->_pl->_peers.end())
      {
        peer *p = (*hit).second;
        l.push_back("\"" + p->_host + ((p->_port == -1) ? "" : (":" + miscutil::to_string(p->_port))) + p->_path + "\"");
        ++hit;
      }
    const std::string json_str = "{\"peers\":" + miscutil::join_string_list(",",l) + "}";
    const std::string body = jsonp(json_str,miscutil::lookup(parameters,"callback"));
    response(rsp,body);
    return SP_ERR_OK;
  }

  sp_err cf::cgi_tbd(client_state *csp,
                     http_response *rsp,
                     const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    // check for query and snippet id.
    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/search/txt/","");
    std::string query = urlmatch::next_elt_from_path(path);
    if (query.empty())
      return SP_ERR_CGI_PARAMS; // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q",1,query.c_str(),1);
    const char *url_str = miscutil::lookup(parameters,"url");
    if (!url_str)
      return SP_ERR_CGI_PARAMS; // 400 error.
    std::string url = url_str;
    if (url.empty())
      return SP_ERR_CGI_PARAMS; // 400 error.
    try
      {
        websearch::preprocess_parameters(parameters,csp); // preprocess the parameters, includes language and query.
      }
    catch(sp_exception &e)
      {
        return e.code();
      }

    miscutil::to_lower(query); // lower case query for filtering operations.
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
      return cgi::cgi_error_bad_param(csp,rsp,"json"); // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q",1,query.c_str(),1); // add query to parameters.

    try
      {
        websearch::preprocess_parameters(parameters,csp);
      }
    catch(sp_exception &e)
      {
        return e.code();
      }

    // ask all peers.
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
    cf::personalize(qc,false,cf::select_p2p_or_local(parameters));
    sp_err err = json_renderer::render_json_suggested_queries(qc,rsp,parameters);
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
        return cf::recommendation_post(csp,rsp,parameters);
      }
    else
      {
        // error.
        errlog::log_error(LOG_LEVEL_ERROR,"wrong HTTP method %s for recommendation call",
                          http_method.c_str());
        return cgi::cgi_error_bad_param(csp,rsp,"json");
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
      return cgi::cgi_error_bad_param(csp,rsp,"json"); // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q",1,query.c_str(),1); // add query to parameters.

    try
      {
        websearch::preprocess_parameters(parameters,csp);
      }
    catch(sp_exception &e)
      {
        return e.code();
      }

    // check on parameters.
    const char *peers = miscutil::lookup(parameters,"peers");
    if (peers && strcasecmp(peers,"local")!=0 && strcasecmp(peers,"p2p")!=0)
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
    cf::personalize(qc,false,cf::select_p2p_or_local(parameters),radius);
    sort_rank::sort_merge_and_rank_snippets(qc,qc->_cached_snippets,parameters); // in case the context is already in memory.
    clock_t end_time = times(&en_cpu);
    double qtime = (end_time-start_time)/websearch::_cl_sec;
    sp_err err = json_renderer::render_json_recommendations(qc,rsp,parameters,qtime,radius);
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
      return cgi::cgi_error_bad_param(csp,rsp,"json"); // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q",1,query.c_str(),1); // add query to parameters.

    try
      {
        websearch::preprocess_parameters(parameters,csp);
      }
    catch(sp_exception &e)
      {
        return e.code();
      }
    query = miscutil::lookup(parameters,"q");

    // check for missing parameters.
    const char *url_str = miscutil::lookup(parameters,"url");
    if (!url_str)
      return cgi::cgi_error_bad_param(csp,rsp,"json"); // 400 error.
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
    const char *lang_str = miscutil::lookup(parameters,"lang");
    if (!lang_str) // should never reach here, lang_str is ensured by preprocess.
      return cgi::cgi_error_bad_param(csp,rsp,"json"); // 400 error.

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
        mutex_unlock(&qc->_qc_mutex);
        sweeper::unregister_sweepable(qc);
        delete qc;
        if (check_title == "404")
          return cgisimple::cgi_error_404(csp,rsp,parameters); // 404. TODO: JSON + message ?
        else return cgi::cgi_error_bad_param(csp,rsp,"json"); // 400 error.
      }
    else if (url_check)
      title = title.empty() ? check_title : title;

    // create snippet.
    search_snippet *sp = new search_snippet(); // XXX: no rank given, temporary snippet.
    sp->set_url(url);
    sp->set_title(title);
    qc->_cached_snippets.push_back(sp);
    qc->add_to_unordered_cache(sp);
    qc->add_to_unordered_cache_title(sp);

    // store query.
    std::string host,path;
    query_capture::process_url(url,host,path);
    int err = SP_ERR_OK;
    try
      {
        query_capture_element::store_queries(qc,url,host,"query-capture",radius);
      }
    catch (sp_exception &e)
      {
        err = e.code();
      }

    // remove query_context as needed (so to not 'flood' the node).
    mutex_unlock(&qc->_qc_mutex);
    if (!has_qc)
      {
        sweeper::unregister_sweepable(qc);
        delete qc;
      }
    qc = websearch::lookup_qc(parameters);
    return err;
  }

  void cf::personalize(query_context *qc,
                       const bool &wait_external_sources,
                       const std::string &peers,
                       const int &radius)
  {
    // check on config file, in case it did change.
    cf_configuration::_config->load_config();
    pthread_rwlock_rdlock(&cf_configuration::_config->_conf_rwlock);

    simple_re sre;
    sre.peers_personalize(qc,wait_external_sources,peers,radius);
    pthread_rwlock_unlock(&cf_configuration::_config->_conf_rwlock);
  }

  void cf::estimate_ranks(const std::string &query,
                          const std::string &lang,
                          const int &radius,
                          std::vector<search_snippet*> &snippets,
                          const std::string &host,
                          const int &port) throw (sp_exception)
  {
    simple_re sre; // estimator.
    sre.estimate_ranks(query,lang,radius,snippets,host,port);
  }

  void cf::thumb_down_url(const std::string &query,
                          const std::string &lang,
                          const std::string &url) throw (sp_exception)
  {
    simple_re sre; // estimator.
    sre.thumb_down_url(query,lang,url);
  }

  void cf::find_bqc_cb(const std::vector<std::string> &qhashes,
                       const int &radius,
                       db_query_record *&dbr)
  {
    hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
    rank_estimator::fetch_user_db_record(qhashes,
                                         seeks_proxy::_user_db,
                                         records);
    std::string query,lang;
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    hash_map<const char*,std::vector<query_data*>,hash<const char*>,eqstr> inv_qdata;
    rank_estimator::extract_queries(query,lang,radius,seeks_proxy::_user_db,
                                    records,qdata,inv_qdata);
    if (!qdata.empty())
      dbr = new db_query_record(qdata); // no copy.
    else dbr = NULL;
    rank_estimator::destroy_records(records);
    rank_estimator::destroy_inv_qdata_key(inv_qdata);
  }

  std::string cf::select_p2p_or_local(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    std::string str = "p2p"; // p2p is default.
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
