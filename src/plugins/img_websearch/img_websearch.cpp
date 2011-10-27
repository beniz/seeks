/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#include "img_websearch.h"
#include "img_sort_rank.h"
#include "websearch.h" // websearch plugin.
#include "sort_rank.h"
#include "errlog.h"
#include "cgi.h"
#include "cgisimple.h"
#include "urlmatch.h"
#include "miscutil.h"
#include "json_renderer.h"
#include "static_renderer.h"
#include "dynamic_renderer.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "plugin_manager.h"

#if defined(PROTOBUF) && defined(TC)
#include "query_capture.h" // dependent plugin.
#include "cf.h"
#endif
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
#include "xsl_serializer.h"
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <assert.h>
#include <ctype.h>

#include <algorithm>

using namespace sp;

namespace seeks_plugins
{
  img_websearch_configuration* img_websearch::_iwconfig = NULL;
  hash_map<uint32_t,query_context*,id_hash_uint> img_websearch::_active_img_qcontexts
  = hash_map<uint32_t,query_context*,id_hash_uint>();

  plugin* img_websearch::_xs_plugin = NULL;
  bool img_websearch::_xs_plugin_activated = false;


  img_websearch::img_websearch()
    : plugin()
  {
    _name = "img_websearch";
    _version_major = "0";
    _version_minor = "1";

    // config filename.
    if (seeks_proxy::_datadir.empty())
      _config_filename = plugin_manager::_plugin_repository + "img_websearch/img-websearch-config";
    else _config_filename = seeks_proxy::_datadir + "/plugins/img_websearch/img-websearch-config";

#ifdef SEEKS_CONFIGDIR
    struct stat stFileInfo;
    if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
      {
        _config_filename = SEEKS_CONFIGDIR "/img-websearch-config";
      }
#endif

    if (img_websearch::_iwconfig == NULL)
      img_websearch::_iwconfig = new img_websearch_configuration(_config_filename);
    _configuration = img_websearch::_iwconfig;

    // cgi dispatchers.
    _cgi_dispatchers.reserve(2);

    cgi_dispatcher *cgid_wb_seeks_img_search_css
    = new cgi_dispatcher("seeks_img_search.css", &img_websearch::cgi_img_websearch_search_css, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_seeks_img_search_css);

    cgi_dispatcher *cgid_img_wb_search
    = new cgi_dispatcher("search/img", &img_websearch::cgi_img_websearch_search, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_img_wb_search);

#ifdef FEATURE_OPENCV2
    cgi_dispatcher *cgid_img_wb_similar
    = new cgi_dispatcher("similar/img", &img_websearch::cgi_img_websearch_similarity, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_img_wb_similar);
#endif
  }

  img_websearch::~img_websearch()
  {
    img_websearch::_iwconfig = NULL;
  }

  void img_websearch::start()
  {
    // look for dependent plugins.
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    _xs_plugin = plugin_manager::get_plugin("xsl-serializer");
    _xs_plugin_activated = seeks_proxy::_config->is_plugin_activated("xsl-serializer");
#endif
  }

  // CGI calls.
  sp_err img_websearch::cgi_img_websearch_search_css(client_state *csp,
      http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    assert(csp);
    assert(rsp);
    assert(parameters);

    std::string seeks_search_css_str = "img_websearch/templates/themes/"
                                       + websearch::_wconfig->_ui_theme + "/css/seeks_img_search.css";
    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
    = static_renderer::websearch_exports(csp);
    csp->_content_type = CT_CSS;
    sp_err err = cgi::template_fill_for_cgi_str(csp,seeks_search_css_str.c_str(),
                 (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
                  : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
                 exports,rsp);
    if (err != SP_ERR_OK)
      {
        errlog::log_error(LOG_LEVEL_ERROR, "Could not load seeks_img_search.css");
      }
    rsp->_is_static = 1;

    return SP_ERR_OK;
  }

  sp_err img_websearch::cgi_img_websearch_search(client_state *csp,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    // check for HTTP method and route.
    std::string http_method = csp->_http._gpc;
    miscutil::to_lower(http_method);

    // check for query, part of path.
    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/search/img/","");
    std::string query = urlmatch::next_elt_from_path(path);
    if (query.empty())
      return SP_ERR_CGI_PARAMS; // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters)
                            ,"q",1,query.c_str(),1); // add query to parameters.

    // check for snippet id, part of path.
    std::string id_str = urlmatch::next_elt_from_path(path);
    if (!id_str.empty())
      miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters)
                              ,"id",1,id_str.c_str(),1); // add snippet id to parameters.

    //std::cerr << "query: " << query << " -- id: " << id_str << std::endl;

    try
      {
        bool has_lang;
        websearch::preprocess_parameters(parameters,csp,has_lang); // preprocess the parameters, includes language and query.
      }
    catch(sp_exception &e)
      {
        return e.code();
      }

#if defined(PROTOBUF) && defined(TC)
    if (http_method == "post" || http_method == "delete")
      {
        // turn snippet id into url.
        if (id_str.empty())
          return SP_ERR_CGI_PARAMS; // 400 error.
        else
          {
            query_context *vqc = websearch::lookup_qc(parameters,_active_img_qcontexts);
            if (!vqc)
              {
                // no cache, (re)do the websearch first.
                sp_err err = img_websearch::perform_img_websearch(csp,rsp,parameters,false);
                if (err != SP_ERR_OK)
                  return err;
                vqc = websearch::lookup_qc(parameters,_active_img_qcontexts);
                if (!vqc) // should never happen.
                  return SP_ERR_MEMORY; // 500.
              }
            query_context *qc = dynamic_cast<img_query_context*>(vqc);
            uint32_t sid = (uint32_t)strtod(id_str.c_str(),NULL);
            mutex_lock(&qc->_qc_mutex);
            search_snippet *sp = vqc->get_cached_snippet(sid);
            mutex_unlock(&qc->_qc_mutex);
            if (!sp)
              {
                return SP_ERR_NOT_FOUND;
              }
            else miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"url",1,sp->_url.c_str(),1);
          }
      }
    if (http_method == "delete")
      {
        // route to snippet rejection callback.
        return cf::cgi_tbd(csp,rsp,parameters);
      }
    if (http_method == "post")
      {
        // route to snippet selection callback.
        return query_capture::cgi_qc_redir(csp,rsp,parameters);
      }
    else
#endif
      // empty method is considered as GET, otherwise error.
      if (!http_method.empty() && http_method != "get" && http_method != "put")
        {
          errlog::log_error(LOG_LEVEL_ERROR,"wrong HTTP method %s",http_method.c_str());
          return SP_ERR_MEMORY; // 500.
        }
    if (http_method.empty())
      {
        http_method = "get";
        if (csp->_http._gpc)
          free(csp->_http._gpc);
        csp->_http._gpc = strdup("get");
      }

    // check on requested User Interface.
    /*const char *ui = miscutil::lookup(parameters,"ui");
    std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
    const char *output = miscutil::lookup(parameters,"output");
    std::string output_str = output ? std::string(output) : "html";
    std::transform(ui_str.begin(),ui_str.end(),ui_str.begin(),tolower);
    std::transform(output_str.begin(),output_str.end(),output_str.begin(),tolower);
    if (ui_str == "dyn" && output_str == "html")
      {
    sp_err err = dynamic_renderer::render_result_page(csp,rsp,parameters);
    return err;
      }

        // perform websearch or other requested action.
        const char *action = miscutil::lookup(parameters,"action");
        if (!action)
          return cgi::cgi_error_bad_param(csp,rsp);

        sp_err err = SP_ERR_OK;
        if (strcmp(action,"expand") == 0 || strcmp(action,"page") == 0)
          err = img_websearch::perform_img_websearch(csp,rsp,parameters);
    #ifdef FEATURE_OPENCV2
        else if (strcmp(action,"similarity") == 0)
          err = img_websearch::cgi_img_websearch_similarity(csp,rsp,parameters);
    #endif

        errlog::log_error(LOG_LEVEL_INFO,"img query: %s",cgi::build_url_from_parameters(parameters).c_str());

        return err;*/

    // possibly a new search: check on config file in case it did change.
    img_websearch_configuration::_img_wconfig->load_config();
    pthread_rwlock_rdlock(&img_websearch_configuration::_img_wconfig->_conf_rwlock); // lock config file.
    sp_err err = SP_ERR_OK;
    if (id_str.empty())
      err = img_websearch::perform_img_websearch(csp,rsp,parameters);
    else err = img_websearch::fetch_snippet(csp,rsp,parameters);
    pthread_rwlock_unlock(&img_websearch_configuration::_img_wconfig->_conf_rwlock); // unlock config file.
    return err;
  }

  sp_err img_websearch::fetch_snippet(client_state *csp, http_response *rsp,
                                      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    query_context *vqc = websearch::lookup_qc(parameters,_active_img_qcontexts);

    if (!vqc)
      {
        // no cache, (re)do the websearch first.
        sp_err err = img_websearch::perform_img_websearch(csp,rsp,parameters,false);
        if (err != SP_ERR_OK)
          return err;
        vqc = websearch::lookup_qc(parameters,_active_img_qcontexts);
        if (!vqc) // should never happen.
          return SP_ERR_MEMORY; // 500.
      }
    query_context *qc = dynamic_cast<img_query_context*>(vqc);
    mutex_lock(&qc->_qc_mutex);

    // fetch snippet.
    const char *id = miscutil::lookup(parameters,"id");
    if (!id)
      {
        mutex_unlock(&qc->_qc_mutex);
        return SP_ERR_CGI_PARAMS;
      }
    uint32_t sid = (uint32_t)strtod(id,NULL);
    search_snippet *sp = qc->get_cached_snippet(sid);
    if (!sp)
      {
        mutex_unlock(&qc->_qc_mutex);
        return SP_ERR_NOT_FOUND;
      }

    // render result page.
    sp_err err = SP_ERR_OK;

#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    const char *output = miscutil::lookup(parameters,"output");
    if (!output || miscutil::strcmpic(output,"json")==0)
      {
#endif

        err = json_renderer::render_json_snippet(sp,rsp,parameters,qc);

#ifdef FEATURE_XSLSERIALIZER_PLUGIN
      }
    else if(img_websearch::_xs_plugin && img_websearch::_xs_plugin_activated &&  !miscutil::strcmpic(output, "xml"))
      {
        err = static_cast<xsl_serializer*>(img_websearch::_xs_plugin)->render_xsl_snippet(csp,rsp,parameters,qc,sp);
      }
#endif

    mutex_unlock(&qc->_qc_mutex);

    return err;
  }

#ifdef FEATURE_OPENCV2
  sp_err img_websearch::cgi_img_websearch_similarity(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    std::string tmpl_name = "templates/themes/" + websearch::_wconfig->_ui_theme + "/seeks_result_template.html";

    // check for query, part of path.
    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/similar/img/","");
    std::string query = urlmatch::next_elt_from_path(path);
    if (query.empty())
      return SP_ERR_CGI_PARAMS; // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters)
                            ,"q",1,query.c_str(),1); // add query to parameters.

    // check for snippet id, part of path.
    std::string id_str = urlmatch::next_elt_from_path(path);
    if (!id_str.empty())
      miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters)
                              ,"id",1,id_str.c_str(),1); // add snippet id to parameters.

    try
      {
        bool has_lang;
        websearch::preprocess_parameters(parameters,csp,has_lang); // preprocess the parameters, includes language and query.
      }
    catch(sp_exception &e)
      {
        return e.code();
      }

    query_context *vqc = websearch::lookup_qc(parameters,_active_img_qcontexts);
    if (!vqc)
      {
        // no cache, (re)do the websearch first.
        sp_err err = img_websearch::perform_img_websearch(csp,rsp,parameters,false);
        if (err != SP_ERR_OK)
          return err;
        vqc = dynamic_cast<img_query_context*>(websearch::lookup_qc(parameters,_active_img_qcontexts));
        if (!vqc)
          return SP_ERR_MEMORY;
      }
    img_query_context *qc = dynamic_cast<img_query_context*>(vqc);

    const char *id = miscutil::lookup(parameters,"id");
    if (!id)
      return SP_ERR_CGI_PARAMS;//cgi::cgi_error_bad_param(csp,rsp);

    mutex_lock(&qc->_qc_mutex);
    img_search_snippet *ref_sp = NULL;

    websearch::_wconfig->load_config();
    pthread_rwlock_rdlock(&img_websearch_configuration::_img_wconfig->_conf_rwlock); // lock config file.
    try
      {
        img_sort_rank::score_and_sort_by_similarity(qc,id,ref_sp,qc->_cached_snippets,parameters);
      }
    catch (sp_exception &e)
      {
        mutex_unlock(&qc->_qc_mutex);
        pthread_rwlock_unlock(&img_websearch_configuration::_img_wconfig->_conf_rwlock);
        if (e.code() == WB_ERR_NO_REF_SIM)
          return SP_ERR_NOT_FOUND; // XXX: error is intercepted.
        else return e.code();
      }

    const char *output = miscutil::lookup(parameters,"output");
    sp_err err = SP_ERR_OK;
    if (!output || miscutil::strcmpic(output,"html")==0)
      {
        // sets the number of images per page, if not already set.
        const char *rpp = miscutil::lookup(parameters,"rpp");
        if (!rpp)
          {
            miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"rpp",1,
                                    miscutil::to_string(img_websearch_configuration::_img_wconfig->_Nr).c_str(),1);
          }
        std::vector<std::pair<std::string,std::string> > *param_exports
        = img_websearch::safesearch_exports(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters));
        err = static_renderer::render_result_page_static(qc->_cached_snippets,
              csp,rsp,parameters,qc,
              (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository + tmpl_name
               : std::string(seeks_proxy::_datadir) + "plugins/img_websearch/" + tmpl_name),
              "/search_img?",param_exports);
        if (param_exports)
          delete param_exports;
      }
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    else if(img_websearch::_xs_plugin && img_websearch::_xs_plugin_activated &&  !miscutil::strcmpic(output, "xml"))
      {
        err = static_cast<xsl_serializer*>(img_websearch::_xs_plugin)->render_xsl_results(csp,rsp,parameters,qc,qc->_cached_snippets,0.0,true);
      }
#endif
    else
      {
        csp->_content_type = CT_JSON;
        err = json_renderer::render_json_results(qc->_cached_snippets,
              csp,rsp,parameters,qc,0.0,true);
      }

    // reset scores.
    std::vector<search_snippet*>::iterator vit = qc->_cached_snippets.begin();
    while (vit!=qc->_cached_snippets.end())
      {
        (*vit)->_seeks_ir = 0;
        ++vit;
      }

    ref_sp->set_similarity_link(parameters); // reset sim_link.
    mutex_unlock(&qc->_qc_mutex);
    pthread_rwlock_unlock(&img_websearch_configuration::_img_wconfig->_conf_rwlock);
    return err;
  }
#endif

  // internal functions.
  sp_err img_websearch::perform_img_websearch(client_state *csp,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
      bool render)
  {
    std::string tmpl_name = "templates/themes/"
                            + websearch::_wconfig->_ui_theme + "/seeks_result_template.html";

    // time measure, returned.
    // XXX: not sure whether it is functional on all *nix platforms.
    struct tms st_cpu;
    struct tms en_cpu;
    clock_t start_time = times(&st_cpu);

    // lookup a cached context for the incoming query.
    mutex_lock(&websearch::_context_mutex);
    query_context *vqc = websearch::lookup_qc(parameters,_active_img_qcontexts);
    bool exists_qc = vqc ? true : false;
    img_query_context *qc = NULL;
    if (vqc)
      qc = dynamic_cast<img_query_context*>(vqc);
    else
      {
        qc = new img_query_context(parameters,csp->_headers);
        qc->register_qc();
      }
    mutex_unlock(&websearch::_context_mutex);

    // check for personalization parameter.
    const char *pers = miscutil::lookup(parameters,"prs");
    if (!pers)
      pers = websearch::_wconfig->_personalization ? "on" : "off";
    bool persf = (strcasecmp(pers,"on")==0);
    pthread_t pers_thread = 0;

    // expansion: we fetch more pages from every search engine.
    sp_err err = SP_ERR_OK;
    bool expanded = false;
    if (exists_qc) // we already had a context for this query.
      {
        expanded = true;
        mutex_lock(&qc->_qc_mutex);
        mutex_lock(&qc->_feeds_ack_mutex);
        try
          {
#if defined(PROTOBUF) && defined(TC)
            if (persf)
              {
                int perr = pthread_create(&pers_thread,NULL,
                                          (void *(*)(void *))&sort_rank::th_personalize,
                                          new pers_arg(qc,parameters));
                if (perr != 0)
                  {
                    errlog::log_error(LOG_LEVEL_ERROR,"Error creating main personalization thread.");
                    mutex_unlock(&qc->_qc_mutex);
                    mutex_unlock(&qc->_feeds_ack_mutex);
                    return WB_ERR_THREAD;
                  }
              }
#endif
            qc->generate(csp,rsp,parameters,expanded);
          }
        catch (sp_exception &e)
          {
            err = e.code();
            switch(err)
              {
              case SP_ERR_CGI_PARAMS:
              case WB_ERR_NO_ENGINE:
                break;
              case WB_ERR_NO_ENGINE_OUTPUT:
                if (!persf)
                  websearch::failed_ses_connect(csp,rsp);
                err = WB_ERR_SE_CONNECT;  //TODO: a 408 code error.
                break;
              default:
                break;
              }
          }

        // do not return if perso + err != no engine
        // instead signal all personalization threads that results may have
        // arrived.
        mutex_unlock(&qc->_feeds_ack_mutex);
        if (persf && err != SP_ERR_CGI_PARAMS)
          {
          }
        else if (err != SP_ERR_OK)
          {
            mutex_unlock(&qc->_qc_mutex);
            return err;
          }

        const char *page = miscutil::lookup(parameters,"page");
        if (!page)
          {
            page = "1";
          }
      }
    else
      {
        // new context, whether we're expanding or not doesn't matter, we need
        // to generate snippets first.
        expanded = true;
        mutex_lock(&qc->_qc_mutex);
        mutex_lock(&qc->_feeds_ack_mutex);
        try
          {
#if defined(PROTOBUF) && defined(TC)
            // personalization in parallel to feeds fetching.
            if (persf)
              {
                int perr = pthread_create(&pers_thread,NULL,
                                          (void *(*)(void *))&sort_rank::th_personalize,
                                          new pers_arg(qc,parameters));
                if (perr != 0)
                  {
                    errlog::log_error(LOG_LEVEL_ERROR,"Error creating main personalization thread.");
                    mutex_unlock(&qc->_qc_mutex);
                    mutex_unlock(&qc->_feeds_ack_mutex);
                    return WB_ERR_THREAD;
                  }
              }
#endif
            qc->generate(csp,rsp,parameters,expanded);
          }
        catch (sp_exception &e)
          {
            err = e.code();
            switch(err)
              {
              case SP_ERR_CGI_PARAMS:
              case WB_ERR_NO_ENGINE:
                break;
              case WB_ERR_NO_ENGINE_OUTPUT:
                if (!persf)
                  websearch::failed_ses_connect(csp,rsp);
                err = WB_ERR_SE_CONNECT;
                break;
              default:
                break;
              }
          }

        // do not return if personalization on.
        // instead signal all personalization threads that results may have
        // arrived.
        mutex_unlock(&qc->_feeds_ack_mutex);
        if (persf && err != SP_ERR_CGI_PARAMS)
          {
          }
        else if (err != SP_ERR_OK)
          {
            mutex_unlock(&qc->_qc_mutex);
            return err;
          }

#if defined(PROTOBUF) && defined(TC)
        // query_capture if plugin is available and activated.
        if (websearch::_qc_plugin && websearch::_qc_plugin_activated)
          {
            std::string http_method = csp->_http._gpc;
            miscutil::to_lower(http_method);
            if (http_method == "put")
              {
                try
                  {
                    static_cast<query_capture*>(websearch::_qc_plugin)->store_queries(qc->_lc_query);
                  }
                catch (sp_exception &e)
                  {
                    errlog::log_error(LOG_LEVEL_ERROR,e.to_string().c_str());
                  }
              }
          }
#endif
      }

    // sort and rank search snippets.
    if (persf && pers_thread)
      {
        while(pthread_tryjoin_np(pers_thread,NULL))
          {
            cond_broadcast(&qc->_feeds_ack_cond);
          }
      }
    sort_rank::sort_merge_and_rank_snippets(qc,qc->_cached_snippets,parameters); // to merge P2P non image results.
    img_sort_rank::sort_rank_and_merge_snippets(qc,qc->_cached_snippets); // to merge image results only.

    // rendering.
    // time measured before rendering, since we need to write it down.
    clock_t end_time = times(&en_cpu);
    double qtime = (end_time-start_time)/websearch::_cl_sec;
    if (qtime<0)
      qtime = -1.0; // unavailable.

    if (render)
      {
        const char *ui = miscutil::lookup(parameters,"ui");
        std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
        const char *output = miscutil::lookup(parameters,"output");
        std::string output_str = output ? std::string(output) : "html";
        std::transform(ui_str.begin(),ui_str.end(),ui_str.begin(),tolower);
        std::transform(output_str.begin(),output_str.end(),output_str.begin(),tolower);

        if (ui_str == "stat" && output_str == "html")
          {
            // sets the number of images per page, if not already set.
            const char *rpp = miscutil::lookup(parameters,"rpp");
            if (!rpp)
              {
                miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"rpp",1,
                                        miscutil::to_string(img_websearch_configuration::_img_wconfig->_Nr).c_str(),1);
              }
            std::vector<std::pair<std::string,std::string> > *param_exports
            = img_websearch::safesearch_exports(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters));
            err = static_renderer::render_result_page_static(qc->_cached_snippets,
                  csp,rsp,parameters,qc,
                  (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository + tmpl_name
                   : std::string(seeks_proxy::_datadir) + "plugins/img_websearch/" + tmpl_name),
                  "/search_img?",param_exports,true);
            if (param_exports)
              delete param_exports;
          }
        else if (ui_str == "dyn" && output_str == "html")
          {
            // XXX: the template is filled up and returned earlier.
            // dynamic UI uses JSON calls to fill up results.
          }
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
        else if (img_websearch::_xs_plugin && img_websearch::_xs_plugin_activated && !miscutil::strcmpic(output, "xml"))
          {
            err = static_cast<xsl_serializer*>(img_websearch::_xs_plugin)->render_xsl_results(csp,rsp,parameters,qc,
                  qc->_cached_snippets,qtime,true);
          }
#endif
        else if (output_str == "json")
          {
            csp->_content_type = CT_JSON;
            err = json_renderer::render_json_results(qc->_cached_snippets,
                  csp,rsp,parameters,qc,
                  qtime,true);
          }
      } // end render.

    // resets P2P data on snippets.
    if (persf)
      {
#if defined(PROTOBUF) && defined(TC)
        std::vector<search_snippet*>::iterator vit = qc->_cached_snippets.begin();
        while (vit!=qc->_cached_snippets.end())
          {
            if ((*vit)->_engine.has_feed("seeks"))
              (*vit)->_engine.remove_feed("seeks");
            (*vit)->_meta_rank = (*vit)->_engine.size();
            (*vit)->_seeks_rank = 0;
            (*vit)->_npeers = 0;
            (*vit)->_hits = 0;
            ++vit;
          }
#endif
      }

    // unlock or destroy the query context.
    mutex_unlock(&qc->_qc_mutex);
    if (qc->empty())
      {
        sweeper::unregister_sweepable(qc);
        delete qc;
      }

    return err;
  }

  std::vector<std::pair<std::string,std::string> >* img_websearch::safesearch_exports(hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    std::vector<std::pair<std::string,std::string> > *param_exports = NULL;
    const char *safesearch_p = miscutil::lookup(parameters,"safesearch");
    param_exports = new std::vector<std::pair<std::string,std::string> >();
    if (safesearch_p && (strcasecmp(safesearch_p,"on")==0 || strcasecmp(safesearch_p,"off") == 0))
      {
        param_exports->push_back(std::pair<std::string,std::string>("$xxsafe",safesearch_p));
        param_exports->push_back(std::pair<std::string,std::string>("$xxoppsafe",strcasecmp(safesearch_p,"on")==0 ?
                                 "off" : "on"));
      }
    else
      {
        param_exports->push_back(std::pair<std::string,std::string>("$xxsafe",img_websearch_configuration::_img_wconfig->_safe_search ?
                                 "on" : "off"));
        param_exports->push_back(std::pair<std::string,std::string>("$xxoppsafe",!img_websearch_configuration::_img_wconfig->_safe_search ?
                                 "on" : "off"));
        if (!safesearch_p)
          miscutil::add_map_entry(parameters,"safesearch",1,
                                  img_websearch_configuration::_img_wconfig->_safe_search ? "on" : "off",1);
      }
    return param_exports;
  }

  /* plugin registration. */
  extern "C"
  {
    plugin* maker()
    {
      return new img_websearch;
    }
  }

} /* end of namespace. */
