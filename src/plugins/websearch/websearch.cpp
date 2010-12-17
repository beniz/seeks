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

#include "websearch.h"
#include "cgi.h"
#include "cgisimple.h"
#include "encode.h"
#include "errlog.h"
#include "query_interceptor.h"
#include "proxy_configuration.h"
#include "se_parser.h"
#include "se_handler.h"
#include "static_renderer.h"
#include "dynamic_renderer.h"
#include "json_renderer.h"
#include "sort_rank.h"
#include "content_handler.h"
#include "oskmeans.h"
#include "mrf.h"

#if defined(PROTOBUF) && defined(TC)
#include "query_capture.h" // dependent plugin.
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <iostream>
#include <algorithm>
#include <bitset>
#include <assert.h>

using namespace sp;

namespace seeks_plugins
{
  websearch_configuration* websearch::_wconfig = NULL;
  hash_map<uint32_t,query_context*,id_hash_uint > websearch::_active_qcontexts
  = hash_map<uint32_t,query_context*,id_hash_uint >();
  double websearch::_cl_sec = -1.0; // filled up at startup.

  plugin* websearch::_qc_plugin = NULL;
  bool websearch::_qc_plugin_activated = false;
  plugin* websearch::_cf_plugin = NULL;
  bool websearch::_cf_plugin_activated = false;

  sp_mutex_t websearch::_context_mutex;

  websearch::websearch()
      : plugin()
  {
    _name = "websearch";
    _version_major = "0";
    _version_minor = "2";

    if (seeks_proxy::_datadir.empty())
      _config_filename = plugin_manager::_plugin_repository + "websearch/websearch-config";
    else
      _config_filename = seeks_proxy::_datadir + "/plugins/websearch/websearch-config";

#ifdef SEEKS_CONFIGDIR
    struct stat stFileInfo;
    if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
      {
        _config_filename = SEEKS_CONFIGDIR "/websearch-config";
      }
#endif

    if (websearch::_wconfig == NULL)
      websearch::_wconfig = new websearch_configuration(_config_filename);
    _configuration = websearch::_wconfig;

    // load tagging patterns.
    search_snippet::load_patterns();

    // cgi dispatchers.
    _cgi_dispatchers.reserve(6);

    cgi_dispatcher *cgid_wb_hp
    = new cgi_dispatcher("websearch-hp", &websearch::cgi_websearch_hp, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_hp);

    cgi_dispatcher *cgid_wb_seeks_hp_search_css
    = new cgi_dispatcher("seeks_hp_search.css", &websearch::cgi_websearch_search_hp_css, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_seeks_hp_search_css);

    cgi_dispatcher *cgid_wb_seeks_search_css
    = new cgi_dispatcher("seeks_search.css", &websearch::cgi_websearch_search_css, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_seeks_search_css);

    cgi_dispatcher *cgid_wb_opensearch_xml
    = new cgi_dispatcher("opensearch.xml", &websearch::cgi_websearch_opensearch_xml, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_opensearch_xml);

    cgi_dispatcher *cgid_wb_search
    = new cgi_dispatcher("search", &websearch::cgi_websearch_search, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_search);

    cgi_dispatcher *cgid_wb_search_cache
    = new cgi_dispatcher("search_cache", &websearch::cgi_websearch_search_cache, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_search_cache);
    
    cgi_dispatcher *cgid_wb_node_info
      = new cgi_dispatcher("info", &websearch::cgi_websearch_node_info, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_node_info);

    // interceptor plugins.
    _interceptor_plugin = new query_interceptor(this);

    // initializes the libxml for multithreaded usage.
    se_parser::libxml_init();

    // get clock ticks per sec.
    websearch::_cl_sec = sysconf(_SC_CLK_TCK);

    // init context mutex.
    mutex_init(&websearch::_context_mutex);
  }

  websearch::~websearch()
  {
  }

  void websearch::start()
  {
    // look for dependent plugins.
    _qc_plugin = plugin_manager::get_plugin("query-capture");
    _qc_plugin_activated = seeks_proxy::_config->is_plugin_activated(_name.c_str()); //TODO: hot deactivation.
    _cf_plugin = plugin_manager::get_plugin("cf");
    _cf_plugin_activated = seeks_proxy::_config->is_plugin_activated(_name.c_str());
  }

  // CGI calls.
  sp_err websearch::cgi_websearch_hp(client_state *csp,
                                     http_response *rsp,
                                     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    assert(csp);
    assert(rsp);
    assert(parameters);

    // redirection to local file is forbidden by most browsers. Let's read the file instead.
    sp_err err = static_renderer::render_hp(csp,rsp);

    return err;
  }

  sp_err websearch::cgi_websearch_search_hp_css(client_state *csp,
      http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    assert(csp);
    assert(rsp);
    assert(parameters);

    std::string seeks_search_css_str = "websearch/templates/themes/"
                                       + websearch::_wconfig->_ui_theme + "/css/seeks_hp_search.css";
    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
    = static_renderer::websearch_exports(csp);
    csp->_content_type = CT_CSS;
    sp_err err = cgi::template_fill_for_cgi_str(csp,seeks_search_css_str.c_str(),
                 (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
                  : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
                 exports,rsp);

    if (err != SP_ERR_OK)
      {
        errlog::log_error(LOG_LEVEL_ERROR, "Could not load seeks_hp_search.css");
      }

    rsp->_is_static = 1;

    return SP_ERR_OK;
  }

  sp_err websearch::cgi_websearch_search_css(client_state *csp,
      http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    assert(csp);
    assert(rsp);
    assert(parameters);

    std::string seeks_search_css_str = "websearch/templates/themes/"
                                       + websearch::_wconfig->_ui_theme + "/css/seeks_search.css";
    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
    = static_renderer::websearch_exports(csp);
    csp->_content_type = CT_CSS;
    sp_err err = cgi::template_fill_for_cgi_str(csp,seeks_search_css_str.c_str(),
                 (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
                  : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
                 exports,rsp);

    if (err != SP_ERR_OK)
      {
        errlog::log_error(LOG_LEVEL_ERROR, "Could not load seeks_search.css");
      }

    rsp->_is_static = 1;

    return SP_ERR_OK;
  }

  sp_err websearch::cgi_websearch_opensearch_xml(client_state *csp,
      http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    assert(csp);
    assert(rsp);
    assert(parameters);

    std::string seeks_opensearch_xml_str = "websearch/templates/opensearch.xml";
    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports
    = static_renderer::websearch_exports(csp);
    csp->_content_type = CT_XML;
    sp_err err = cgi::template_fill_for_cgi(csp,seeks_opensearch_xml_str.c_str(),
                                            (seeks_proxy::_datadir.empty() ? plugin_manager::_plugin_repository.c_str()
                                             : std::string(seeks_proxy::_datadir + "plugins/").c_str()),
                                            exports,rsp);

    if (err != SP_ERR_OK)
      {
        errlog::log_error(LOG_LEVEL_ERROR, "Could not load opensearch.xml");
      }

    rsp->_is_static = 1;

    return SP_ERR_OK;
  }

  sp_err websearch::cgi_websearch_search(client_state *csp, http_response *rsp,
                                         const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        const char *query = miscutil::lookup(parameters,"q"); // grab the query.
        if (!query || strlen(query) == 0)
          {
            // return 400 error.
            return cgi::cgi_error_bad_param(csp,rsp);
          }
        else se_handler::preprocess_parameters(parameters); // preprocess the query...

        // check on requested User Interface:
        // - 'dyn' for dynamic interface: detach a thread for performing the requested
        //   action, but return the page with embedded JS right now.
        // - 'stat' for static interface: perform the requested action and render the page
        //   before returning it.
        const char *ui = miscutil::lookup(parameters,"ui");
        std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
        const char *output = miscutil::lookup(parameters,"output");
        std::string output_str = output ? std::string(output) : "html";
        std::transform(ui_str.begin(),ui_str.end(),ui_str.begin(),tolower);
        std::transform(output_str.begin(),output_str.end(),output_str.begin(),tolower);
        if (ui_str == "dyn" && output_str == "html")
          {
            sp_err err = dynamic_renderer::render_result_page(csp,rsp,parameters);

            // detach thread for operations.
            pthread_t wo_thread;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            wo_thread_arg *wta = new wo_thread_arg(csp,rsp,parameters,false);
            int perr = pthread_create(&wo_thread,&attr,
                                      (void *(*)(void *))&websearch::perform_action_threaded,wta);
            if (perr != 0)
              {
                errlog::log_error(LOG_LEVEL_ERROR,"Error creating websearch action thread.");
              }

            return err;
          }

        // perform websearch or other requested action.
        return websearch::perform_action(csp,rsp,parameters);
      }
    else
      {
        return cgi::cgi_error_bad_param(csp,rsp);
      }
  }

  sp_err websearch::cgi_websearch_search_cache(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        const char *url = miscutil::lookup(parameters,"url"); // grab the url.
        if (!url)
          return cgi::cgi_error_bad_param(csp,rsp);

        query_context *qc = websearch::lookup_qc(parameters,csp);

        if (!qc)
          {
            // redirect to the url.
            cgi::cgi_redirect(rsp,url);
            return SP_ERR_OK;
          }

        mutex_lock(&qc->_qc_mutex);
        qc->_lock = true;

        search_snippet *sp = NULL;
        if ((sp = qc->get_cached_snippet(url))!=NULL
            && (sp->_cached_content!=NULL))
          {
            errlog::log_error(LOG_LEVEL_INFO,"found cached url %s",url);

            rsp->_body = strdup(sp->_cached_content->c_str());
            rsp->_is_static = 1;

            qc->_lock = false;
            mutex_unlock(&qc->_qc_mutex);

            return SP_ERR_OK;
          }
        else
          {
            // redirect to the url.
            cgi::cgi_redirect(rsp,url);

            qc->_lock = false;
            mutex_unlock(&qc->_qc_mutex);

            return SP_ERR_OK;
          }
      }
    else
      {
        return cgi::cgi_error_bad_param(csp,rsp);
      }
  }

  sp_err websearch::cgi_websearch_neighbors_url(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        query_context *qc = websearch::lookup_qc(parameters,csp);

        if (!qc)
          {
            // no cache, (re)do the websearch first.
            sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
            if (err != SP_ERR_OK)
              return err;
            qc = websearch::lookup_qc(parameters,csp);
            if (!qc)
              qc = new query_context(parameters,csp->_headers); // empty context.
          }

        mutex_lock(&qc->_qc_mutex);
        qc->_lock = true;

        // render result page.
        sp_err err = static_renderer::render_neighbors_result_page(csp,rsp,parameters,qc,0); // 0: urls.

        qc->_lock = false;
        mutex_unlock(&qc->_qc_mutex);
        if (qc->empty())
          {
            sweeper::unregister_sweepable(qc);
            delete qc;
          }

        return err;
      }
    else return cgi::cgi_error_bad_param(csp,rsp);
  }

  sp_err websearch::cgi_websearch_neighbors_title(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        query_context *qc = websearch::lookup_qc(parameters,csp);

        if (!qc)
          {
            // no cache, (re)do the websearch first.
            sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
            qc = websearch::lookup_qc(parameters,csp);
            if (err != SP_ERR_OK)
              return err;
          }

        mutex_lock(&qc->_qc_mutex);
        qc->_lock = true;

        // render result page.
        const char *ui = miscutil::lookup(parameters,"ui");
        std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
        const char *output = miscutil::lookup(parameters,"output");
        std::string output_str = output ? std::string(output) : "html";
        std::transform(ui_str.begin(),ui_str.end(),ui_str.begin(),tolower);
        std::transform(output_str.begin(),output_str.end(),output_str.begin(),tolower);

        sp_err err = SP_ERR_OK;
        if (ui_str == "stat" && output_str == "html")
          err = static_renderer::render_neighbors_result_page(csp,rsp,parameters,qc,1); // 1: titles.
        else if (output_str == "json")
          {
            csp->_content_type = CT_JSON;
            err = json_renderer::render_json_results(qc->_cached_snippets,
                  csp,rsp,parameters,qc,0.0);
          }

        qc->_lock = false;
        mutex_unlock(&qc->_qc_mutex);

        return err;
      }
    else return cgi::cgi_error_bad_param(csp,rsp);
  }

  sp_err websearch::cgi_websearch_clustered_types(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        // time measure, returned.
        struct tms st_cpu;
        struct tms en_cpu;
        clock_t start_time = times(&st_cpu);

        query_context *qc = websearch::lookup_qc(parameters,csp);
        if (!qc)
          {
            // no cache, (re)do the websearch first.
            sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
            if (err != SP_ERR_OK)
              return err;
            qc = websearch::lookup_qc(parameters,csp);
            if (!qc)
              qc = new query_context(parameters,csp->_headers); // empty context.
          }

        cluster *clusters = NULL;
        short K = 0;

        mutex_lock(&qc->_qc_mutex);
        qc->_lock = true;

        // regroup search snippets by types.
        sort_rank::group_by_types(qc,clusters,K);

        // time measured before rendering, since we need to write it down.
        clock_t end_time = times(&en_cpu);
        double qtime = (end_time-start_time)/websearch::_cl_sec;
        if (qtime<0)
          qtime = -1.0; // unavailable.

        // rendering.
        const char *output =miscutil::lookup(parameters,"output");
        sp_err err = SP_ERR_OK;
        if (!output || miscutil::strcmpic(output,"html")==0)
          err = static_renderer::render_clustered_result_page_static(clusters,K,
                csp,rsp,parameters,qc);
        else
          {
            csp->_content_type = CT_JSON;
            err = json_renderer::render_clustered_json_results(clusters,K,
                  csp,rsp,parameters,qc,qtime);
          }

        if (clusters)
          delete[] clusters;

        qc->_lock = false;
        mutex_unlock(&qc->_qc_mutex);
        if (qc->empty())
          {
            sweeper::unregister_sweepable(qc);
            delete qc;
          }

        return err;
      }
    else return cgi::cgi_error_bad_param(csp,rsp);
  }

  sp_err websearch::cgi_websearch_similarity(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        query_context *qc = websearch::lookup_qc(parameters,csp);

        if (!qc)
          {
            // no cache, (re)do the websearch first.
            sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
            if (err != SP_ERR_OK)
              return err;
            qc = websearch::lookup_qc(parameters,csp);
            if (!qc)
              return SP_ERR_MEMORY;
          }
        const char *id = miscutil::lookup(parameters,"id");
        if (!id)
          return cgi::cgi_error_bad_param(csp,rsp);

        mutex_lock(&qc->_qc_mutex);
        qc->_lock = true;
        search_snippet *ref_sp = NULL;
        sort_rank::score_and_sort_by_similarity(qc,id,parameters,ref_sp,qc->_cached_snippets);

        if (!ref_sp)
          {
            qc->_lock = false;
            mutex_unlock(&qc->_qc_mutex);
            return cgisimple::cgi_error_404(csp,rsp,parameters);
          }

        const char *output = miscutil::lookup(parameters,"output");
        sp_err err = SP_ERR_OK;
        if (!output || miscutil::strcmpic(output,"html")==0)
          err = static_renderer::render_result_page_static(qc->_cached_snippets,
                csp,rsp,parameters,qc);
        else
          {
            csp->_content_type = CT_JSON;
            err = json_renderer::render_json_results(qc->_cached_snippets,
                  csp,rsp,parameters,qc,0.0);
          }

        // reset scores.
        std::vector<search_snippet*>::iterator vit = qc->_cached_snippets.begin();
        while (vit!=qc->_cached_snippets.end())
          {
            (*vit)->_seeks_ir = 0;
            ++vit;
          }

        ref_sp->set_similarity_link(parameters); // reset sim_link.
        qc->_lock = false;
        mutex_unlock(&qc->_qc_mutex);

        return err;
      }
    else return cgi::cgi_error_bad_param(csp,rsp);
  }

  sp_err websearch::cgi_websearch_clusterize(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        query_context *qc = websearch::lookup_qc(parameters,csp);

        // time measure, returned.
        struct tms st_cpu;
        struct tms en_cpu;
        clock_t start_time = times(&st_cpu);

        if (!qc)
          {
            // no cache, (re)do the websearch first.
            sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
            qc = websearch::lookup_qc(parameters,csp);
            if (err != SP_ERR_OK)
              return err;
          }

        mutex_lock(&qc->_qc_mutex);
        qc->_lock = true;

        bool content_analysis = websearch::_wconfig->_content_analysis;
        const char *ca = miscutil::lookup(parameters,"content_analysis");
        if (ca && strcasecmp(ca,"on") == 0)
          content_analysis = true;
        if (content_analysis)
          content_handler::fetch_all_snippets_content_and_features(qc);
        else content_handler::fetch_all_snippets_summary_and_features(qc);

        if (qc->_cached_snippets.empty())
          {
            const char *output = miscutil::lookup(parameters,"output");
            sp_err err = SP_ERR_OK;
            if (!output || miscutil::strcmpic(output,"html")==0)
              err = static_renderer::render_result_page_static(qc->_cached_snippets,
                    csp,rsp,parameters,qc);
            else
              {
                csp->_content_type = CT_JSON;
                err = json_renderer::render_json_results(qc->_cached_snippets,
                      csp,rsp,parameters,qc,
                      0.0);
              }
            qc->_lock = false;
            mutex_unlock(&qc->_qc_mutex);
          }

        const char *nclust_str = miscutil::lookup(parameters,"clusters");
        int nclust = 2;
        if (nclust_str)
          nclust = atoi(nclust_str);
        oskmeans km(qc,qc->_cached_snippets,nclust); // nclust clusters+ 1 garbage for now...
        km.clusterize();
        km.post_processing();

        // time measured before rendering, since we need to write it down.
        clock_t end_time = times(&en_cpu);
        double qtime = (end_time-start_time)/websearch::_cl_sec;
        if (qtime<0)
          qtime = -1.0; // unavailable.

        // rendering.
        const char *output = miscutil::lookup(parameters,"output");
        sp_err err = SP_ERR_OK;
        if (!output || miscutil::strcmpic(output,"html")==0)
          err = static_renderer::render_clustered_result_page_static(km._clusters,km._K,
                csp,rsp,parameters,qc);
        else
          {
            csp->_content_type = CT_JSON;
            err = json_renderer::render_clustered_json_results(km._clusters,km._K,
                  csp,rsp,parameters,qc,qtime);
          }

        // reset scores.
        std::vector<search_snippet*>::iterator vit = qc->_cached_snippets.begin();
        while (vit!=qc->_cached_snippets.end())
          {
            (*vit)->_seeks_ir = 0;
            ++vit;
          }

        qc->_lock = false;
        mutex_unlock(&qc->_qc_mutex);

        return err;
      }
    else return cgi::cgi_error_bad_param(csp,rsp);
  }

  sp_err websearch::cgi_websearch_node_info(client_state *csp, http_response *rsp,
					    const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    const char *output = miscutil::lookup(parameters,"output");
    sp_err err = SP_ERR_OK;
    if (!output || strcmpic(output,"json") == 0)
      err = json_renderer::render_json_node_options(csp,rsp,parameters);
    return err;
  }

  /*- internal functions. -*/
  void websearch::perform_action_threaded(wo_thread_arg *args)
  {
    perform_action(args->_csp,args->_rsp,args->_parameters,args->_render);
    delete args;
  }

  sp_err websearch::perform_action(client_state *csp, http_response *rsp,
                                   const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                   bool render)
  {
    const char *action = miscutil::lookup(parameters,"action");
    if (!action)
      return cgi::cgi_error_bad_param(csp,rsp);

    /**
     * Action can be of type:
     * - "expand": requests an expansion of the search results, expansion horizon is
     *           specified by parameter "expansion".
     * - "page": requests a result page, already in cache.
     * - "similarity": requests a reordering of results, in decreasing order from the
     *                 specified search result, identified by parameter "id".
     * - "clusterize": requests the forming of a cluster of results, the number of specified
     *                 clusters is given by parameter "clusters".
     * - "urls": requests a grouping by url.
     * - "titles": requests a grouping by title.
     */
    sp_err err = SP_ERR_OK;
    if (miscutil::strcmpic(action,"expand") == 0 || miscutil::strcmpic(action,"page") == 0)
      err = websearch::perform_websearch(csp,rsp,parameters);
    else if (miscutil::strcmpic(action,"similarity") == 0)
      err = websearch::cgi_websearch_similarity(csp,rsp,parameters);
    else if (miscutil::strcmpic(action,"clusterize") == 0)
      err = websearch::cgi_websearch_clusterize(csp,rsp,parameters);
    else if (miscutil::strcmpic(action,"urls") == 0)
      err = websearch::cgi_websearch_neighbors_url(csp,rsp,parameters);
    else if (miscutil::strcmpic(action,"titles") == 0)
      err = websearch::cgi_websearch_neighbors_title(csp,rsp,parameters);
    else if (miscutil::strcmpic(action,"types") == 0)
      err = websearch::cgi_websearch_clustered_types(csp,rsp,parameters);
    else return cgi::cgi_error_bad_param(csp,rsp);

    return err;
  }

  sp_err websearch::perform_websearch(client_state *csp, http_response *rsp,
                                      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                      bool render)
  {
    // time measure, returned.
    // XXX: not sure whether it is functional on all *nix platforms.
    struct tms st_cpu;
    struct tms en_cpu;
    clock_t start_time = times(&st_cpu);

    // lookup a cached context for the incoming query.
    // Mutex allows multiple simultaneous calls to catch the same context object.
    mutex_lock(&websearch::_context_mutex);
    query_context *qc = websearch::lookup_qc(parameters,csp);
    bool exists_qc = qc ? true : false;
    if (!qc)
      {
        qc = new query_context(parameters,csp->_headers);
        qc->register_qc();
      }
    mutex_unlock(&websearch::_context_mutex);

    // expansion: we fetch more pages from every search engine.
    bool expanded = false;
    if (exists_qc) // we already had a context for this query.
      {
        // check whether search is expanding or the user is leafing through pages.
        const char *action = miscutil::lookup(parameters,"action");

        websearch::_wconfig->load_config(); // reload config if file has changed.
        if (strcmp(action,"expand") == 0)
          {
            const char *expansion = miscutil::lookup(parameters,"expansion");
            if (!expansion)
              return cgi::cgi_error_bad_param(csp,rsp);

            expanded = true;
            mutex_lock(&qc->_qc_mutex);
            qc->_lock = true;
            qc->generate(csp,rsp,parameters,expanded);
            qc->_lock = false;
            mutex_unlock(&qc->_qc_mutex);
          }
        else if (miscutil::strcmpic(action,"page") == 0)
          {
            const char *page = miscutil::lookup(parameters,"page");
            if (!page)
              return cgi::cgi_error_bad_param(csp,rsp);

            // XXX: update other parameters, as needed, qc vs parameters.
            qc->update_parameters(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters));
          }
      }
    else
      {
        // new context, whether we're expanding or not doesn't matter, we need
        // to generate snippets first.
        expanded = true;
        mutex_lock(&qc->_qc_mutex);
        qc->_lock = true;
        qc->generate(csp,rsp,parameters,expanded);
        qc->_lock = false;
        mutex_unlock(&qc->_qc_mutex);

#if defined(PROTOBUF) && defined(TC)
        // query_capture if plugin is available and activated.
        if (_qc_plugin && _qc_plugin_activated)
          static_cast<query_capture*>(_qc_plugin)->store_queries(qc->_query);
#endif
      }

    // sort and rank search snippets.
    mutex_lock(&qc->_qc_mutex);
    qc->_lock = true;
    sort_rank::sort_merge_and_rank_snippets(qc,qc->_cached_snippets,
                                            parameters);
    const char *pers = miscutil::lookup(parameters,"prs");
    if (!pers)
      pers = websearch::_wconfig->_personalization ? "on" : "off";
    if (strcasecmp(pers,"on") == 0)
      {
#if defined(PROTOBUF) && defined(TC)
        sort_rank::personalized_rank_snippets(qc,qc->_cached_snippets);
	sort_rank::get_related_queries(qc);
	sort_rank::get_recommended_urls(qc);
#endif
      }
    
    if (expanded)
      qc->_compute_tfidf_features = true;

    // XXX: we do not recompute features if nothing has changed.
    if (expanded && websearch::_wconfig->_extended_highlight)
      content_handler::fetch_all_snippets_summary_and_features(qc);

    // time measured before rendering, since we need to write it down.
    clock_t end_time = times(&en_cpu);
    double qtime = (end_time-start_time)/websearch::_cl_sec;
    if (qtime<0)
      qtime = -1.0; // unavailable.

    // render the page (static).
    sp_err err = SP_ERR_OK;
    if (render)
      {
        const char *ui = miscutil::lookup(parameters,"ui");
        std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
        const char *output = miscutil::lookup(parameters,"output");
        std::string output_str = output ? std::string(output) : "html";
        std::transform(ui_str.begin(),ui_str.end(),ui_str.begin(),tolower);
        std::transform(output_str.begin(),output_str.end(),output_str.begin(),tolower);

        if (ui_str == "stat" && output_str == "html")
          err = static_renderer::render_result_page_static(qc->_cached_snippets,
                csp,rsp,parameters,qc);
        else if (ui_str == "dyn" && output_str == "html")
          {
            // XXX: the template is filled up and returned earlier.
	    // dynamic UI uses JSON calls to fill up results.
	  }
        else if (output_str == "json")
          {
	    csp->_content_type = CT_JSON;
            err = json_renderer::render_json_results(qc->_cached_snippets,
                  csp,rsp,parameters,qc,
                  qtime);
          }
      }
    
    if (strcasecmp(pers,"on") == 0)
      {
#if defined(PROTOBUF) && defined(TC)
	qc->reset_snippets_personalization_flags();
#endif
      }

    // unlock or destroy the query context.
    qc->_lock = false;
    mutex_unlock(&qc->_qc_mutex);
    if (qc->empty())
      {
        sweeper::unregister_sweepable(qc);
        delete qc;
      }

    // XXX: catch errors.
    return err;
  }

  query_context* websearch::lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                      client_state *csp)
  {
    return websearch::lookup_qc(parameters,csp,websearch::_active_qcontexts);
  }

  query_context* websearch::lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                      client_state *csp, hash_map<uint32_t,query_context*,id_hash_uint> &active_qcontexts)
  {
    std::string qlang;
    bool has_query_lang = false;
    if (query_context::has_lang(parameters,qlang))
      {
      }
    else if (!(has_query_lang = query_context::has_query_lang(parameters,qlang)))
      {
        if (websearch::_wconfig->_lang == "auto")
          {
            std::string auto_lang_reg = query_context::detect_query_lang_http(csp->_headers);
            try
              {
                qlang = auto_lang_reg.substr(0,2);
              }
            catch (std::exception &e)
              {
                qlang = "";
              }
          }
        else qlang = websearch::_wconfig->_lang; // falling back onto default search language.
      }

    std::string query;
    if (!has_query_lang && !qlang.empty())
      query = ":" + qlang + " ";
    std::string url_enc_query;
    uint32_t query_hash = query_context::hash_query_for_context(parameters,query,url_enc_query);
    hash_map<uint32_t,query_context*,id_hash_uint >::iterator hit;
    if ((hit = active_qcontexts.find(query_hash))!=active_qcontexts.end())
      {
        /**
         * Already have a context for this query, update its flags, and return it.
         */
        (*hit).second->update_last_time();
        return (*hit).second;
      }
    else return NULL;
  }

  /* error handling. */
  sp_err websearch::failed_ses_connect(client_state *csp, http_response *rsp)
  {
    errlog::log_error(LOG_LEVEL_ERROR, "connect to the search engines failed");

    rsp->_reason = RSP_REASON_CONNECT_FAILED;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports = cgi::default_exports(csp,NULL);
    char *path = strdup("");
    sp_err err = SP_ERR_OK;
    if (csp->_http._path)
      err = miscutil::string_append(&path, csp->_http._path);

    if (!err)
      err = miscutil::add_map_entry(exports, "host", 1, encode::html_encode(csp->_http._host), 0);
    if (!err)
      err = miscutil::add_map_entry(exports, "hostport", 1, encode::html_encode(csp->_http._hostport), 0);
    if (!err)
      err = miscutil::add_map_entry(exports, "path", 1, encode::html_encode_and_free_original(path), 0);
    if (!err)
      err = miscutil::add_map_entry(exports, "protocol", 1, csp->_http._ssl ? "https://" : "http://", 1);
    if (!err)
      {
        err = miscutil::add_map_entry(exports, "host-ip", 1, encode::html_encode(csp->_http._host_ip_addr_str), 0);
        if (err)
          {
            /* Some failures, like "404 no such domain", don't have an IP address. */
            err = miscutil::add_map_entry(exports, "host-ip", 1, encode::html_encode(csp->_http._host), 0);
          }
      }

    // not catching error on template filling...
    err = cgi::template_fill_for_cgi_str(csp,"connect-failed",csp->_config->_templdir,
                                         exports,rsp);
    return err;
  }

  /* auto-registration */
#if defined(ON_OPENBSD) || defined(ON_OSX)
  extern "C"
  {
    plugin* maker()
    {
      return new websearch;
    }
  }

#else
  plugin* maker()
  {
    return new websearch;
  }
  class proxy_autor
  {
    public:
      proxy_autor()
      {
        plugin_manager::_factory["websearch-hp"] = maker; // beware: default plugin shell with no name.
      }
  };

  proxy_autor _p; // one instance, instanciated when dl-opening.
#endif

} /* end of namespace. */
