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

#include "websearch.h"
#include "seeks_snippet.h"
#include "cgi.h"
#include "cgisimple.h"
#include "encode.h"
#include "urlmatch.h"
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
#include "charset_conv.h"

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
#include <iostream>
#include <algorithm>
#include <bitset>
//#include <assert.h>

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
  plugin* websearch::_xs_plugin = NULL;
  bool websearch::_xs_plugin_activated = false;
  plugin* websearch::_readable_plugin = NULL;
  bool websearch::_readable_plugin_activated = false;

  sp_mutex_t websearch::_context_mutex;

  websearch::websearch()
    : plugin()
  {
    _name = "websearch";
    _version_major = "0";
    _version_minor = "4";

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
    seeks_snippet::load_patterns();

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
    = new cgi_dispatcher("search/txt", &websearch::cgi_websearch_search, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_search);

    cgi_dispatcher *cgid_wb_words
    = new cgi_dispatcher("words", &websearch::cgi_websearch_words, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_words);

    cgi_dispatcher *cgid_wb_recent_queries
    = new cgi_dispatcher("recent/queries", &websearch::cgi_websearch_recent_queries, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_recent_queries);

    cgi_dispatcher *cgid_wb_cluster_types
    = new cgi_dispatcher("cluster/types", &websearch::cgi_websearch_clustered_types, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_cluster_types);

    cgi_dispatcher *cgid_wb_cluster_auto
    = new cgi_dispatcher("cluster/auto", &websearch::cgi_websearch_clusterize, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_cluster_auto);

    cgi_dispatcher *cgid_wb_search_similar
    = new cgi_dispatcher("similar/txt", &websearch::cgi_websearch_similarity, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_search_similar);

    cgi_dispatcher *cgid_wb_search_cache
    = new cgi_dispatcher("cache", &websearch::cgi_websearch_search_cache, NULL, TRUE);
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
    websearch::_wconfig = NULL; // configuration is destroyed in parent class.
    seeks_snippet::destroy_patterns();
  }

  void websearch::start()
  {
    // look for dependent plugins.
    _qc_plugin = plugin_manager::get_plugin("query-capture");
    _qc_plugin_activated = seeks_proxy::_config->is_plugin_activated("query-capture"); //TODO: hot deactivation.
    _cf_plugin = plugin_manager::get_plugin("cf");
    _cf_plugin_activated = seeks_proxy::_config->is_plugin_activated("cf");
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    _xs_plugin = plugin_manager::get_plugin("xsl-serializer");
    _xs_plugin_activated = seeks_proxy::_config->is_plugin_activated("xsl-serializer");
#endif
    _readable_plugin = plugin_manager::get_plugin("readable");
    _readable_plugin_activated= seeks_proxy::_config->is_plugin_activated("readable");
  }

  void websearch::stop()
  {
    se_handler::cleanup_handlers();
  }

  // CGI calls.
  sp_err websearch::cgi_websearch_hp(client_state *csp,
                                     http_response *rsp,
                                     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    // redirection to local file is forbidden by most browsers. Let's read the file instead.
    sp_err err = static_renderer::render_hp(csp,rsp);

    return err;
  }

  sp_err websearch::cgi_websearch_search_hp_css(client_state *csp,
      http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
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

  /*-- preprocessing queries. */
  std::string websearch::no_command_query(const std::string &oquery)
  {
    std::string cquery = oquery;
    // remove any command from the query.
    if (cquery[0] == ':')
      {
        try
          {
            cquery = cquery.substr(4);
          }
        catch (std::exception &e)
          {
            // do nothing.
          }
      }
    return cquery;
  }

  void websearch::preprocess_parameters(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                        client_state *csp,
                                        bool &has_lang) throw (sp_exception)
  {
    // decode query (URL encoded).
    const char *query = miscutil::lookup(parameters,"q");
    char *dec_query = encode::url_decode(query); // XXX: was url_decode_but_not_plus
    std::string query_str = std::string(dec_query);
    free(dec_query);
    query_str = miscutil::chomp_cpp(query_str);

    // query charset check.
    query_str = charset_conv::charset_check_and_conversion(query_str,csp->_headers);
    if (query_str.empty())
      {
        std::string msg = "Bad charset on query " + std::string(query);
        errlog::log_error(LOG_LEVEL_ERROR, msg.c_str());
        throw sp_exception(WB_ERR_QUERY_ENCODING,msg);
      }
    miscutil::to_lower(query_str);

    miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q");

    if (!query_str.empty())
      miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
                              "q",1,query_str.c_str(),1);

    // detect and process query language.
    // - check for in-query language command.
    // - check for language parameter, if not, fill it up.
    std::string qlang,qlang_reg;
    bool has_query_lang = query_context::has_query_lang(query_str,qlang);
    if (has_query_lang)
      {
        // replace query parameter to remove language command
        // and fill up the lang parameter instead.
        query_str = websearch::no_command_query(query_str);
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q");
        miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
                                "q",1,query_str.c_str(),1);
        const char *lang = miscutil::lookup(parameters,"lang");
        if (lang)
          miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"lang");
        miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
                                "lang",1,qlang.c_str(),1);
        qlang_reg = query_context::lang_forced_region(qlang);
        has_lang = true;
      }
    else if (query_context::has_lang(parameters,qlang))
      {
        // language is specified, detect the region.
        qlang_reg = query_context::lang_forced_region(qlang);
        has_lang = true;
      }
    else if (websearch::_wconfig->_lang == "auto") // no language was specified, we study HTTP headers.
      {
        // detection from HTTP headers.
        query_context::detect_query_lang_http(csp->_headers,qlang,qlang_reg);
        miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
                                "lang",1,qlang.c_str(),1);
        has_lang = false;
      }
    else // use local settings.
      {
        qlang = websearch::_wconfig->_lang;
        qlang_reg = query_context::lang_forced_region(qlang);
        miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
                                "lang",1,qlang.c_str(),1);
        has_lang = false;
      }

    // setup the language region.
    const char *lreg = miscutil::lookup(parameters,"lreg");
    if (!lreg)
      miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
                              "lreg",1,qlang_reg.c_str(),1);

    // set action to expand if request is for a dynamic UI with html output (i.e. js UI bootstrap).
    const char *ui = miscutil::lookup(parameters,"ui");
    std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
    /*if (ui_str == "dyn")
      {
        const char *output = miscutil::lookup(parameters,"output");
        if (!output || miscutil::strcmpic(output,"html") == 0)
          {
            if (miscutil::strcmpic(action,"expand") != 0)
              {
                miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"action");
                miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
                                        "action",1,"expand",1);
    			}
    			}
    			}*/

    // set expansion to 1 if missing while action is expand.
    const char *expansion = miscutil::lookup(parameters,"expansion");
    if (!expansion)// && action && miscutil::strcmpic(action,"expand")==0)
      {
        miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
                                "expansion",1,"1",1);
      }
  }

  sp_err websearch::cgi_websearch_search(client_state *csp, http_response *rsp,
                                         const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    // check for HTTP method and route.
    std::string http_method = csp->_http._gpc;
    miscutil::to_lower(http_method);

    // check for query, part of path.
    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/search/txt/","");
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
            query_context *qc = websearch::lookup_qc(parameters);
            if (!qc)
              {
                // no cache, (re)do the websearch first.
                sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
                if (err != SP_ERR_OK)
                  return err;
                qc = websearch::lookup_qc(parameters);
                if (!qc) // should never happen.
                  return SP_ERR_MEMORY; // 500.

                // reset p2p data if needed.
                websearch::reset_p2p_data(parameters,qc);
              }
            uint32_t sid = (uint32_t)strtod(id_str.c_str(),NULL);
            mutex_lock(&qc->_qc_mutex);
            search_snippet *sp = qc->get_cached_snippet(sid);
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

    // setup action.
    /*miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters)
      ,"action",1,"expand",1);*/

    // check on requested User Interface:
    // - 'dyn' for dynamic interface: detach a thread for performing the requested
    //   action, but return the page with embedded JS right away.
    // - 'stat' for static interface: perform the requested action and render the page
    //   before returning it.
    /*const char *ui = miscutil::lookup(parameters,"ui");
    std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
    const char *output = miscutil::lookup(parameters,"output");
    std::string output_str = output ? std::string(output) : "html";
    std::transform(ui_str.begin(),ui_str.end(),ui_str.begin(),tolower);
    std::transform(output_str.begin(),output_str.end(),output_str.begin(),tolower);
    if (ui_str == "dyn" && output_str == "html")
      {
    dynamic_renderer::render_result_page(csp,rsp,parameters);

    // detach thread for operations.
    pthread_t wo_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    wo_thread_arg *wta = new wo_thread_arg(csp,rsp,parameters,false);
    int perr = pthread_create(&wo_thread,&attr,
    		  (void *(*)(void *))&websearch::perform_action_threaded,wta); //TODO: perform websearch instead.
    if (perr != 0)
    {
      errlog::log_error(LOG_LEVEL_ERROR,"Error creating websearch action thread.");
      return WB_ERR_THREAD;
    }

    return SP_ERR_OK;
    }*/

    // perform websearch or other requested action.
    //return websearch::perform_action(csp,rsp,parameters);

    // possibly a new search: check on config file in case it did change.
    websearch::_wconfig->load_config();
    pthread_rwlock_rdlock(&websearch::_wconfig->_conf_rwlock); // lock config file.
    sp_err err = SP_ERR_OK;
    if (id_str.empty())
      err = websearch::perform_websearch(csp,rsp,parameters);
    else err = websearch::fetch_snippet(csp,rsp,parameters);
    pthread_rwlock_unlock(&websearch::_wconfig->_conf_rwlock); // unlock config file.
    return err;
  }

  sp_err websearch::cgi_websearch_words(client_state *csp, http_response *rsp,
                                        const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    // check for query, part of path.
    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/words/","");
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

    websearch::_wconfig->load_config();
    pthread_rwlock_rdlock(&websearch::_wconfig->_conf_rwlock); // lock config file.
    sp_err err = SP_ERR_OK;
    if (id_str.empty())
      err = websearch::words_query(csp,rsp,parameters);
    else err = websearch::words_snippet(csp,rsp,parameters);

    pthread_rwlock_unlock(&websearch::_wconfig->_conf_rwlock);
    return err;
  }

  sp_err websearch::cgi_websearch_search_cache(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/cache/txt/","");
    std::string query = urlmatch::next_elt_from_path(path);
    if (query.empty())
      return SP_ERR_CGI_PARAMS; // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters)
                            ,"q",1,query.c_str(),1); // add query to parameters.

    const char *url = miscutil::lookup(parameters,"url"); // grab the url.
    if (!url)
      return SP_ERR_CGI_PARAMS;

    try
      {
        bool has_lang;
        websearch::preprocess_parameters(parameters,csp,has_lang); // preprocess the parameters, includes language and query.
      }
    catch(sp_exception &e)
      {
        return e.code();
      }

    query_context *qc = websearch::lookup_qc(parameters);

    if (!qc)
      {
        return SP_ERR_NOT_FOUND; // no local resource.
      }

    mutex_lock(&qc->_qc_mutex);
    search_snippet *sp = NULL;
    if ((sp = qc->get_cached_snippet(url))!=NULL
        && (sp->_cached_content!=NULL))
      {
        errlog::log_error(LOG_LEVEL_INFO,"found cached url %s",url);

        rsp->_body = strdup(sp->_cached_content->c_str());
        rsp->_is_static = 1;

        mutex_unlock(&qc->_qc_mutex);

        return SP_ERR_OK;
      }
    else
      {
        return SP_ERR_NOT_FOUND; // no local resource.
        return SP_ERR_OK;
      }
  }

  sp_err websearch::cgi_websearch_recent_queries(client_state *csp,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    const char *nq_str = miscutil::lookup(parameters,"nq");
    websearch::_wconfig->load_config();
    pthread_rwlock_rdlock(&websearch::_wconfig->_conf_rwlock); // lock config file.
    int nq = websearch::_wconfig->_num_recent_queries;
    if (nq_str)
      nq = strtod(nq_str,NULL);
    const char *output_str = miscutil::lookup(parameters,"output");
    sp_err err=SP_ERR_OK;
    if (!output_str || miscutil::strcmpic(output_str,"json")==0)
      {
        csp->_content_type = CT_JSON;
        err = json_renderer::render_cached_queries(rsp,parameters,"",nq);
      }
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    else if (websearch::_xs_plugin && websearch::_xs_plugin_activated && !miscutil::strcmpic(output_str, "xml"))
      err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_cached_queries(csp,rsp,parameters,"",nq);
#endif
    pthread_rwlock_unlock(&websearch::_wconfig->_conf_rwlock);
    return err;
  }

  sp_err websearch::cgi_websearch_neighbors_url(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        // no cache, (re)do the websearch first.
        sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
        if (err != SP_ERR_OK)
          return err;
        query_context *qc = websearch::lookup_qc(parameters);
        if (!qc)
          qc = new query_context(parameters,csp->_headers); // empty context.

        mutex_lock(&qc->_qc_mutex);

        // render result page.
        err = static_renderer::render_neighbors_result_page(csp,rsp,parameters,qc,0); // 0: urls.

        // reset p2p data if needed.
        websearch::reset_p2p_data(parameters,qc);

        if (qc->empty())
          {
            sweeper::unregister_sweepable(qc);
            delete qc;
          }
        mutex_unlock(&qc->_qc_mutex);

        return err;
      }
    else return SP_ERR_CGI_PARAMS;
  }

  sp_err websearch::cgi_websearch_neighbors_title(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        // no cache, (re)do the websearch first.
        sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
        query_context *qc = websearch::lookup_qc(parameters);
        if (err != SP_ERR_OK)
          return err;
        mutex_lock(&qc->_qc_mutex);

        // render result page.
        const char *ui = miscutil::lookup(parameters,"ui");
        std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
        const char *output = miscutil::lookup(parameters,"output");
        std::string output_str = output ? std::string(output) : "html";
        miscutil::to_lower(ui_str);
        miscutil::to_lower(output_str);

        err = SP_ERR_OK;
        if (ui_str == "stat" && output_str == "html")
          err = static_renderer::render_neighbors_result_page(csp,rsp,parameters,qc,1); // 1: titles.
        else if (output_str == "json")
          {
            csp->_content_type = CT_JSON;

            err = json_renderer::render_json_results(qc->_cached_snippets,
                  csp,rsp,parameters,qc,0.0);
          }
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
        else if (websearch::_xs_plugin && websearch::_xs_plugin_activated && !miscutil::strcmpic(output, "xml"))
          err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_results(csp,rsp,parameters,qc,qc->_cached_snippets,0.0);
#endif

        // reset p2p data if needed.
        websearch::reset_p2p_data(parameters,qc);

        mutex_unlock(&qc->_qc_mutex);

        return err;
      }
    else return SP_ERR_CGI_PARAMS;
  }

  sp_err websearch::cgi_websearch_clustered_types(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    // time measure, returned.
    struct tms st_cpu;
    struct tms en_cpu;
    clock_t start_time = times(&st_cpu);

    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/cluster/types/","");
    std::string query = urlmatch::next_elt_from_path(path);
    if (query.empty())
      return SP_ERR_CGI_PARAMS; // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters)
                            ,"q",1,query.c_str(),1); // add query to parameters.
    try
      {
        bool has_lang;
        websearch::preprocess_parameters(parameters,csp,has_lang); // preprocess the parameters, includes language and query.
      }
    catch(sp_exception &e)
      {
        return e.code();
      }

    sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
    if (err != SP_ERR_OK)
      return err;
    query_context *qc = websearch::lookup_qc(parameters);
    if (!qc)
      qc = new query_context(parameters,csp->_headers); // empty context.

    short K = 11;
    mutex_lock(&qc->_qc_mutex);

    // regroup search snippets by types.
    hash_map<int,cluster*> clusters;
    sort_rank::group_by_types(qc,&clusters);

    // time measured before rendering, since we need to write it down.
    clock_t end_time = times(&en_cpu);
    double qtime = (end_time-start_time)/websearch::_cl_sec;
    if (qtime<0)
      qtime = -1.0; // unavailable.

    // rendering.
    const char *output =miscutil::lookup(parameters,"output");
    err = SP_ERR_OK;
    if (!output || miscutil::strcmpic(output,"html")==0)
      err = static_renderer::render_clustered_result_page_static(&clusters,K,
            csp,rsp,parameters,qc);
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    else if (websearch::_xs_plugin && websearch::_xs_plugin_activated && !miscutil::strcmpic(output, "xml"))
      err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_clustered_results(csp,rsp,parameters,qc,&clusters,K,qtime);
#endif
    else
      {
        csp->_content_type = CT_JSON;
        err = json_renderer::render_clustered_json_results(&clusters,K,
              csp,rsp,parameters,qc,qtime);
      }

    hash_map<int,cluster*>::iterator hit = clusters.begin();
    while(hit!=clusters.end())
      {
        delete (*hit).second;
        ++hit;
      }

    // reset p2p data if needed.
    websearch::reset_p2p_data(parameters,qc);

    if (qc->empty())
      {
        sweeper::unregister_sweepable(qc);
        delete qc;
      }
    mutex_unlock(&qc->_qc_mutex);

    return err;
  }

  sp_err websearch::cgi_websearch_similarity(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    // check for query, part of path.
    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/similar/txt/","");
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

    sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
    if (err != SP_ERR_OK)
      return err;
    query_context *qc = websearch::lookup_qc(parameters);
    if (!qc)
      return SP_ERR_MEMORY;

    const char *id = miscutil::lookup(parameters,"id");
    if (!id)
      return SP_ERR_CGI_PARAMS;

    mutex_lock(&qc->_qc_mutex);
    search_snippet *ref_sp = NULL;

    websearch::_wconfig->load_config();
    pthread_rwlock_rdlock(&websearch::_wconfig->_conf_rwlock); // lock config file.
    try
      {
        sort_rank::score_and_sort_by_similarity(qc,id,parameters,ref_sp,qc->_cached_snippets);
      }
    catch (sp_exception &e)
      {
        // reset p2p data if needed.
        websearch::reset_p2p_data(parameters,qc);

        mutex_unlock(&qc->_qc_mutex);
        pthread_rwlock_unlock(&websearch::_wconfig->_conf_rwlock);
        if (e.code() == WB_ERR_NO_REF_SIM)
          return SP_ERR_NOT_FOUND; // XXX: error is intercepted.
        else return e.code();
      }

    const char *output = miscutil::lookup(parameters,"output");
    err = SP_ERR_OK;
    if (!output || miscutil::strcmpic(output,"html")==0)
      err = static_renderer::render_result_page_static(qc->_cached_snippets,
            csp,rsp,parameters,qc);
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    else if (websearch::_xs_plugin && websearch::_xs_plugin_activated &&  !miscutil::strcmpic(output, "xml"))
      err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_results(csp,rsp,parameters,qc,qc->_cached_snippets,0.0);
#endif
    else
      {
        csp->_content_type = CT_JSON;
        err = json_renderer::render_json_results(qc->_cached_snippets,
              csp,rsp,parameters,qc,0.0);
      }

    // reset p2p data if needed.
    websearch::reset_p2p_data(parameters,qc);

    // reset scores.
    std::vector<search_snippet*>::iterator vit = qc->_cached_snippets.begin();
    while (vit!=qc->_cached_snippets.end())
      {
        (*vit)->_seeks_ir = 0;
        ++vit;
      }

    ref_sp->set_similarity_link(parameters); // reset sim_link.
    mutex_unlock(&qc->_qc_mutex);
    pthread_rwlock_unlock(&websearch::_wconfig->_conf_rwlock);
    return err;
  }

  sp_err websearch::cgi_websearch_clusterize(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    // time measure, returned.
    struct tms st_cpu;
    struct tms en_cpu;
    clock_t start_time = times(&st_cpu);

    std::string path = csp->_http._path;
    miscutil::replace_in_string(path,"/cluster/auto/","");
    std::string query = urlmatch::next_elt_from_path(path);
    if (query.empty())
      return SP_ERR_CGI_PARAMS; // 400 error.
    miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters)
                            ,"q",1,query.c_str(),1); // add query to parameters.
    try
      {
        bool has_lang;
        websearch::preprocess_parameters(parameters,csp,has_lang); // preprocess the parameters, includes language and query.
      }
    catch(sp_exception &e)
      {
        return e.code();
      }

    sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
    if (err != SP_ERR_OK)
      return err;
    query_context *qc = websearch::lookup_qc(parameters);
    if (!qc)
      qc = new query_context(parameters,csp->_headers); // empty context.

    mutex_lock(&qc->_qc_mutex);

    bool content_analysis = websearch::_wconfig->_content_analysis;
    const char *ca = miscutil::lookup(parameters,"content_analysis");
    if (ca && strcasecmp(ca,"on") == 0)
      content_analysis = true;
    if (content_analysis)
      content_handler::fetch_all_snippets_content_and_features(qc);
    else content_handler::fetch_all_snippets_summary_and_features(qc);

    err = SP_ERR_OK;
    if (qc->_cached_snippets.empty())
      {
        const char *output = miscutil::lookup(parameters,"output");
        if (!output || miscutil::strcmpic(output,"html")==0)
          err = static_renderer::render_result_page_static(qc->_cached_snippets,
                csp,rsp,parameters,qc);
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
        else if (websearch::_xs_plugin && websearch::_xs_plugin_activated && !miscutil::strcmpic(output, "xml"))
          err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_results(csp,rsp,parameters,qc,qc->_cached_snippets,0.0);
#endif
        else

          {
            csp->_content_type = CT_JSON;
            err = json_renderer::render_json_results(qc->_cached_snippets,
                  csp,rsp,parameters,qc,
                  0.0);
          }
        mutex_unlock(&qc->_qc_mutex);
      }

    const char *nclust_str = miscutil::lookup(parameters,"clusters");
    int nclust = 2;
    if (nclust_str)
      nclust = atoi(nclust_str);
    oskmeans km(qc,qc->_cached_snippets,nclust); // nclust clusters+ 1 garbage for now...
    km.clusterize();
    km.post_processing();
    hash_map<int,cluster*> clusters;
    for (int k=0; k<km._K; k++)
      clusters.insert(std::pair<int,cluster*>(k,&km._clusters[k]));

    // time measured before rendering, since we need to write it down.
    clock_t end_time = times(&en_cpu);
    double qtime = (end_time-start_time)/websearch::_cl_sec;
    if (qtime<0)
      qtime = -1.0; // unavailable.

    // rendering.
    const char *output = miscutil::lookup(parameters,"output");
    if (!output || miscutil::strcmpic(output,"html")==0)
      err = static_renderer::render_clustered_result_page_static(&clusters,km._K,
            csp,rsp,parameters,qc);
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    else if (websearch::_xs_plugin && websearch::_xs_plugin_activated &&  !miscutil::strcmpic(output, "xml"))
      err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_clustered_results(csp,rsp,parameters,qc,&clusters,km._K,qtime);
#endif
    else
      {
        csp->_content_type = CT_JSON;
        err = json_renderer::render_clustered_json_results(&clusters,km._K,csp,rsp,parameters,qc,qtime);
      }

    // reset p2p data if needed.
    websearch::reset_p2p_data(parameters,qc);

    // reset scores.
    std::vector<search_snippet*>::iterator vit = qc->_cached_snippets.begin();
    while (vit!=qc->_cached_snippets.end())
      {
        (*vit)->_seeks_ir = 0;
        ++vit;
      }

    mutex_unlock(&qc->_qc_mutex);

    return err;
  }

  sp_err websearch::cgi_websearch_node_info(client_state *csp, http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    const char *output = miscutil::lookup(parameters,"output");
    sp_err err = SP_ERR_OK;
    if (!output || strcmpic(output,"json") == 0)
      {
        err = json_renderer::render_json_node_options(csp,rsp,parameters);
        csp->_content_type = CT_JSON;
      }
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    else if (websearch::_xs_plugin && websearch::_xs_plugin_activated &&  !miscutil::strcmpic(output, "xml"))
      err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_node_options(csp,rsp,parameters);
#endif
    return err;
  }

  /*- internal functions. -*/
  /*void websearch::perform_action_threaded(wo_thread_arg *args)
  {
    perform_action(args->_csp,args->_rsp,args->_parameters,args->_render);
    delete args;
    }*/

  /*sp_err websearch::perform_action(client_state *csp, http_response *rsp,
                                   const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                   bool render)
  {
    const char *action = miscutil::lookup(parameters,"action");
    if (!action)
      return SP_ERR_CGI_PARAMS;

    // possibly a new search: check on config file in cast it did change.
    websearch::_wconfig->load_config();
    pthread_rwlock_rdlock(&websearch::_wconfig->_conf_rwlock); // lock config file.
  */

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
  /*sp_err err = SP_ERR_OK;
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
  else
    {
      pthread_rwlock_unlock(&websearch::_wconfig->_conf_rwlock);
      return SP_ERR_CGI_PARAMS;
    }

  errlog::log_error(LOG_LEVEL_INFO,"query: %s",cgi::build_url_from_parameters(parameters).c_str());

  pthread_rwlock_unlock(&websearch::_wconfig->_conf_rwlock);
  return err;
  }*/

#if defined(PROTOBUF) && defined(TC)
  /*- internal functions. -*/
  void *websearch::perform_websearch_threaded(ws_thread_arg *args)
  {
    sort_rank::th_personalize(args->_arg);
    args->_done = true;
    return NULL;
  }
#endif

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
    query_context *qc = websearch::lookup_qc(parameters);
    bool exists_qc = qc ? true : false;
    if (!qc)
      {
        qc = new query_context(parameters,csp->_headers);
        qc->register_qc();
      }
    mutex_unlock(&websearch::_context_mutex);

    // check for personalization parameter.
    const char *pers = miscutil::lookup(parameters,"prs");
    if (!pers)
      pers = websearch::_wconfig->_personalization ? "on" : "off";
    bool persf = (strcasecmp(pers,"on")==0);
    pthread_t pers_thread = 0;
    ws_thread_arg *pers_thread_arg = NULL;

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
                pers_thread_arg = new ws_thread_arg(new pers_arg(qc,parameters));
                int perr = pthread_create(&pers_thread,NULL,
                                          (void *(*)(void *))&websearch::perform_websearch_threaded,
                                          pers_thread_arg);
                if (perr != 0)
                  {
                    errlog::log_error(LOG_LEVEL_ERROR,"Error creating main personalization thread.");
                    mutex_unlock(&qc->_qc_mutex);
                    mutex_unlock(&qc->_feeds_ack_mutex);
                    delete pers_thread_arg;
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
#if defined(PROTOBUF) && defined(TC)
            if (persf)
              {
                while(!pers_thread_arg->_done)
                  {
                    cond_broadcast(&qc->_feeds_ack_cond);
                  }
                delete pers_thread_arg;
                pthread_join(pers_thread,NULL);
              }
#endif
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
                pers_thread_arg = new ws_thread_arg(new pers_arg(qc,parameters));
                int perr = pthread_create(&pers_thread,NULL,
                                          (void *(*)(void *))&websearch::perform_websearch_threaded,
                                          pers_thread_arg);
                if (perr != 0)
                  {
                    errlog::log_error(LOG_LEVEL_ERROR,"Error creating main personalization thread.");
                    mutex_unlock(&qc->_qc_mutex);
                    mutex_unlock(&qc->_feeds_ack_mutex);
                    delete pers_thread_arg;
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
#if defined(PROTOBUF) && defined(TC)
            if (persf)
              {
                while(!pers_thread_arg->_done)
                  {
                    cond_broadcast(&qc->_feeds_ack_cond);
                  }
                delete pers_thread_arg;
                pthread_join(pers_thread,NULL);
              }
#endif
            return err;
          }

#if defined(PROTOBUF) && defined(TC)
        // query_capture if plugin is available and activated.
        if (websearch::_qc_plugin && websearch::_qc_plugin_activated)
          {
            std::string http_method = csp->_http._gpc;
            miscutil::to_lower(http_method);
            const char *output = miscutil::lookup(parameters,"output");
            if (http_method == "put"
                || !output || strcmpic(output,"html")==0) // when returning the static UI, we enforced storing queries because the GET / PUT control is not available.
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

#if defined(PROTOBUF) && defined(TC)
    // signal personalization thread that we're done with external data sources.
    if (persf && pers_thread && pers_thread_arg)
      {
        while(!pers_thread_arg->_done)
          {
            cond_broadcast(&qc->_feeds_ack_cond);
          }
        delete pers_thread_arg;
        pthread_join(pers_thread,NULL);
      }
#endif

    // sort and merge snippets.
    sort_rank::sort_merge_and_rank_snippets(qc,qc->_cached_snippets,
                                            parameters);

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
    err = SP_ERR_OK;
    if (render)
      {
        const char *ui = miscutil::lookup(parameters,"ui");
        std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
        const char *output = miscutil::lookup(parameters,"output");
        std::string output_str = output ? std::string(output) : "html";
        miscutil::to_lower(ui_str);
        miscutil::to_lower(output_str);

        if (ui_str == "stat" && output_str == "html")
          err = static_renderer::render_result_page_static(qc->_cached_snippets,
                csp,rsp,parameters,qc);
        else if (ui_str == "dyn" && output_str == "html")
          {
            // dynamic UI uses JSON calls to fill up results.
            dynamic_renderer::render_result_page(csp,rsp,parameters);
          }
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
        else if (websearch::_xs_plugin && websearch::_xs_plugin_activated &&  !miscutil::strcmpic(output, "xml"))
          err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_results(csp,rsp,parameters,qc,qc->_cached_snippets,qtime);
#endif
        else if (output_str == "json")
          {
            csp->_content_type = CT_JSON;
            err = json_renderer::render_json_results(qc->_cached_snippets,
                  csp,rsp,parameters,qc,
                  qtime);
          }
      }

    // resets P2P data on snippets.
    // if we're not rendering here, rendering will happen elsewhere,
    // as will the reset of the p2p filter data.
    if (persf && render)
      {
#if defined(PROTOBUF) && defined(TC)
        qc->reset_p2p_data();
#endif
      }

    // unlock or destroy the query context.
    if (qc->empty())
      {
        sweeper::unregister_sweepable(qc);
        delete qc;
      }
    mutex_unlock(&qc->_qc_mutex);

    return err;
  }

  sp_err websearch::fetch_snippet(client_state *csp, http_response *rsp,
                                  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
    if (err != SP_ERR_OK)
      return err;
    query_context *qc = websearch::lookup_qc(parameters);
    if (!qc) // should never happen.
      return SP_ERR_MEMORY; // 500.
    mutex_lock(&qc->_qc_mutex);

    // fetch snippet.
    const char *id = miscutil::lookup(parameters,"id");
    if (!id)
      {
        websearch::reset_p2p_data(parameters,qc);
        mutex_unlock(&qc->_qc_mutex);
        return SP_ERR_CGI_PARAMS;
      }
    uint32_t sid = (uint32_t)strtod(id,NULL);
    search_snippet *sp = qc->get_cached_snippet(sid);
    websearch::reset_p2p_data(parameters,qc);
    if (!sp)
      {
        mutex_unlock(&qc->_qc_mutex);
        return SP_ERR_NOT_FOUND;
      }

    // render result page.
    const char *output = miscutil::lookup(parameters,"output");
    err = SP_ERR_OK;
    if (!output || miscutil::strcmpic(output,"json")==0)
      {
        csp->_content_type = CT_JSON;
        err = json_renderer::render_json_snippet(sp,rsp,parameters,qc);
      }
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    else if(websearch::_xs_plugin && websearch::_xs_plugin_activated &&  !miscutil::strcmpic(output, "xml"))
      err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_snippet(csp,rsp,parameters,qc,sp);
#endif
    mutex_unlock(&qc->_qc_mutex);
    return err;
  }

  sp_err websearch::words_query(client_state *csp, http_response *rsp,
                                const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
    if (err != SP_ERR_OK)
      return err;
    query_context *qc = websearch::lookup_qc(parameters);
    if (!qc)
      return SP_ERR_MEMORY; // 500.
    mutex_lock(&qc->_qc_mutex);

    std::set<std::string> words;
    for (size_t i=0; i<qc->_cached_snippets.size(); i++)
      {
        qc->_cached_snippets.at(i)->discr_words(qc->_query_words,words);
      }
    const char *output = miscutil::lookup(parameters,"output");
    err = SP_ERR_OK;
    if (!output || miscutil::strcmpic(output,"json")==0)
      {
        csp->_content_type = CT_JSON;
        err = json_renderer::render_json_words(words,rsp,parameters);
      }
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    else if(websearch::_xs_plugin && websearch::_xs_plugin_activated &&  !miscutil::strcmpic(output, "xml"))
      err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_words(csp,rsp,parameters,words);
#endif
    websearch::reset_p2p_data(parameters,qc);
    mutex_unlock(&qc->_qc_mutex);
    return err;
  }

  sp_err websearch::words_snippet(client_state *csp, http_response *rsp,
                                  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    sp_err err = websearch::perform_websearch(csp,rsp,parameters,false);
    if (err != SP_ERR_OK)
      return err;
    query_context *qc = websearch::lookup_qc(parameters);
    if (!qc)
      return SP_ERR_MEMORY; // 500.
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

    // get most discriminant words.
    std::set<std::string> words;
    sp->discr_words(qc->_query_words,words);

    // render result page.
    const char *output = miscutil::lookup(parameters,"output");
    err = SP_ERR_OK;
    if (!output || miscutil::strcmpic(output,"json")==0)
      {
        csp->_content_type = CT_JSON;
        err = json_renderer::render_json_words(words,rsp,parameters);
      }
#ifdef FEATURE_XSLSERIALIZER_PLUGIN
    else if(websearch::_xs_plugin && websearch::_xs_plugin_activated &&  !miscutil::strcmpic(output, "xml"))
      err = static_cast<xsl_serializer*>(websearch::_xs_plugin)->render_xsl_words(csp,rsp,parameters,words);
#endif
    websearch::reset_p2p_data(parameters,qc);
    mutex_unlock(&qc->_qc_mutex);
    return err;
  }

  query_context* websearch::lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    return websearch::lookup_qc(parameters,websearch::_active_qcontexts);
  }

  query_context* websearch::lookup_qc(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                      hash_map<uint32_t,query_context*,id_hash_uint> &active_qcontexts)
  {
    // assemble query key to potential query context.
    std::string qlang;
    bool has_lang = query_context::has_lang(parameters,qlang);
    if (!has_lang)
      {
        // should not happen, parameters should have been preprocessed.
        qlang = query_context::_default_alang;
      }
    const char *q_str = miscutil::lookup(parameters,"q");
    if (!q_str)
      {
        // should never happen.
        errlog::log_error(LOG_LEVEL_ERROR,"trying to fetch context with empty query parameter");
        q_str = "";
        return NULL;
      }
    std::string q = q_str;
    miscutil::to_lower(q);
    std::string query_key = query_context::assemble_query(q,qlang);
    uint32_t query_hash = query_context::hash_query_for_context(query_key);
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

  void websearch::reset_p2p_data(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                                 query_context *qc)
  {
    // check for personalization parameter.
    const char *pers = miscutil::lookup(parameters,"prs");
    if (!pers)
      pers = websearch::_wconfig->_personalization ? "on" : "off";
    bool persf = (strcasecmp(pers,"on")==0);

    // resets P2P data on snippets.
    if (persf)
      {
#if defined(PROTOBUF) && defined(TC)
        qc->reset_p2p_data();
#endif
      }
  }

  /* error handling. */
  sp_err websearch::failed_ses_connect(client_state *csp, http_response *rsp)
  {
    errlog::log_error(LOG_LEVEL_ERROR, "connect to the search engines failed");

    rsp->_reason = RSP_REASON_CONNECT_FAILED;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *exports = cgi::default_exports(csp,NULL);
    char *path = NULL;
    sp_err err = SP_ERR_OK;
    if (csp->_http._path)
      {
        path = strdup(csp->_http._path);
      }

    if (!err)
      err = miscutil::add_map_entry(exports, "host", 1, encode::html_encode(csp->_http._host), 0);
    if (!err)
      err = miscutil::add_map_entry(exports, "hostport", 1, encode::html_encode(csp->_http._hostport), 0);
    if (!err)
      err = miscutil::add_map_entry(exports, "path", 1, encode::html_encode(path), 0);
    if (!err)
      err = miscutil::add_map_entry(exports, "protocol", 1, csp->_http._ssl ? "https://" : "http://", 1);
    if (!err)
      {
        err = miscutil::add_map_entry(exports, "host-ip", 1, encode::html_encode(csp->_http._host_ip_addr_str.c_str()), 0);
        if (err)
          {
            /* Some failures, like "404 no such domain", don't have an IP address. */
            err = miscutil::add_map_entry(exports, "host-ip", 1, encode::html_encode(csp->_http._host), 0);
          }
      }

    // not catching error on template filling...
    err = cgi::template_fill_for_cgi_str(csp,"connect-failed",csp->_config->_templdir,
                                         exports,rsp);
    if (path)
      free(path);
    return err;
  }

  /* plugin registration */
  extern "C"
  {
    plugin* maker()
    {
      return new websearch;
    }
  }

} /* end of namespace. */
