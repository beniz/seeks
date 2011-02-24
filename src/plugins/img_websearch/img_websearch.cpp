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

#include "img_websearch.h"
#include "img_sort_rank.h"
#include "websearch.h" // websearch plugin.
#include "sort_rank.h"
#include "errlog.h"
#include "cgi.h"
#include "cgisimple.h"
#include "miscutil.h"
#include "json_renderer.h"
#include "static_renderer.h"
#include "dynamic_renderer.h"
#include "seeks_proxy.h"

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
    = new cgi_dispatcher("search_img", &img_websearch::cgi_img_websearch_search, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_img_wb_search);
  }

  img_websearch::~img_websearch()
  {
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
    if (!parameters->empty())
      {
        const char *query = miscutil::lookup(parameters,"q"); // grab the query.
        if (!query || strlen(query) == 0)
          {
            // return 400 error.
            return cgi::cgi_error_bad_param(csp,rsp);
          }
        else
          {
            try
              {
                websearch::preprocess_parameters(parameters,csp); // preprocess the parameters (query + language).
              }
            catch (sp_exception &e)
              {
                return e.code();
              }
          }

        // check on requested User Interface.
        const char *ui = miscutil::lookup(parameters,"ui");
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

        return err;
      }
    else return cgi::cgi_error_bad_param(csp,rsp);
  }

#ifdef FEATURE_OPENCV2
  sp_err img_websearch::cgi_img_websearch_similarity(client_state *csp, http_response *rsp,
      const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    std::string tmpl_name = "templates/themes/" + websearch::_wconfig->_ui_theme + "/seeks_result_template.html";

    if (!parameters->empty())
      {
        img_query_context *qc = dynamic_cast<img_query_context*>(websearch::lookup_qc(parameters,_active_img_qcontexts));

        if (!qc)
          {
            // no cache, (re)do the websearch first.
            sp_err err = img_websearch::perform_img_websearch(csp,rsp,parameters,false);
            if (err != SP_ERR_OK)
              return err;
            qc = dynamic_cast<img_query_context*>(websearch::lookup_qc(parameters,_active_img_qcontexts));
            if (!qc)
              return SP_ERR_MEMORY;
          }
        const char *id = miscutil::lookup(parameters,"id");
        if (!id)
          return cgi::cgi_error_bad_param(csp,rsp);

        mutex_lock(&qc->_qc_mutex);
        img_search_snippet *ref_sp = NULL;

        try
          {
            img_sort_rank::score_and_sort_by_similarity(qc,id,ref_sp,qc->_cached_snippets,parameters);
          }
        catch (sp_exception &e)
          {
            mutex_unlock(&qc->_qc_mutex);
            if (e.code() == WB_ERR_NO_REF_SIM)
              return cgisimple::cgi_error_404(csp,rsp,parameters); // XXX: error is intercepted.
            else return e.code();
          }

        const char *ui = miscutil::lookup(parameters,"ui");
        std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
        const char *output = miscutil::lookup(parameters,"output");
        std::string output_str = output ? std::string(output) : "html";
        std::transform(ui_str.begin(),ui_str.end(),ui_str.begin(),tolower);
        std::transform(output_str.begin(),output_str.end(),output_str.begin(),tolower);
        sp_err err = SP_ERR_OK;
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
                  "/search_img?",param_exports);
            if (param_exports)
              delete param_exports;
          }
        else if (ui_str == "dyn" && output_str == "html")
          {
            // XXX: the template is filled up and returned earlier.
            // dynamic UI uses JSON calls to fill up results.
          }
        else if (output_str == "json")
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
        return err;
      }
    else return cgi::cgi_error_bad_param(csp,rsp);
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
    query_context *vqc = websearch::lookup_qc(parameters,_active_img_qcontexts);
    img_query_context *qc = NULL;
    if (vqc)
      qc = dynamic_cast<img_query_context*>(vqc);

    // check whether search is expanding or the user is leafing through pages.
    const char *action = miscutil::lookup(parameters,"action");

    // expansion: we fetch more pages from every search engine.
    bool expanded = false;
    if (qc) // we already had a context for this query.
      {
        if (strcmp(action,"expand") == 0)
          {
            const char *expansion = miscutil::lookup(parameters,"expansion");
            if (!expansion)
              return cgi::cgi_error_bad_param(csp,rsp);
            expanded = true;

            mutex_lock(&qc->_qc_mutex);
            try
              {
                qc->generate(csp,rsp,parameters,expanded);
              }
            catch (sp_exception &e)
              {
                int code = e.code();
                switch(code)
                  {
                  case SP_ERR_CGI_PARAMS:
                  case WB_ERR_NO_ENGINE:
                    mutex_unlock(&qc->_qc_mutex);
                    break;
                  case WB_ERR_NO_ENGINE_OUTPUT:
                    mutex_unlock(&qc->_qc_mutex);
                    websearch::failed_ses_connect(csp,rsp);
                    code = WB_ERR_SE_CONNECT;  //TODO: a 408 code error.
                    break;
                  default:
                    break;
                  }
                return code;
              }
            mutex_unlock(&qc->_qc_mutex);
          }
        else if (strcmp(action,"page") == 0)
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
        const char *expansion = miscutil::lookup(parameters,"expansion");
        if (!expansion)
          return cgi::cgi_error_bad_param(csp,rsp);

        expanded = true;
        qc = new img_query_context(parameters,csp->_headers);
        qc->register_qc();
        mutex_lock(&qc->_qc_mutex);
        try
          {
            qc->generate(csp,rsp,parameters,expanded);
          }
        catch (sp_exception &e)
          {
            int code = e.code();
            switch(code)
              {
              case SP_ERR_CGI_PARAMS:
              case WB_ERR_NO_ENGINE:
                mutex_unlock(&qc->_qc_mutex);
                break;
              case WB_ERR_NO_ENGINE_OUTPUT:
                mutex_unlock(&qc->_qc_mutex);
                websearch::failed_ses_connect(csp,rsp);
                code = WB_ERR_SE_CONNECT;
                break;
              default:
                break;
              }
            return code;
          }
        mutex_unlock(&qc->_qc_mutex);
      }

    // sort and rank search snippets.
    mutex_lock(&qc->_qc_mutex);
    img_sort_rank::sort_rank_and_merge_snippets(qc,qc->_cached_snippets);

    const char *pers = miscutil::lookup(parameters,"prs");
    if (!pers)
      pers = websearch::_wconfig->_personalization ? "on" : "off";
    if (strcasecmp(pers,"on") == 0)
      {
#if defined(PROTOBUF) && defined(TC)
        try
          {
            sort_rank::personalized_rank_snippets(qc,qc->_cached_snippets);
            sort_rank::get_related_queries(qc);
            sort_rank::get_recommended_urls(qc);
          }
        catch (sp_exception &e)
          {
            std::string msg = "Failed personalization of image results: " + e.to_string();
            errlog::log_error(LOG_LEVEL_ERROR,msg.c_str());
          }
#endif
      }

    // rendering.
    // time measured before rendering, since we need to write it down.
    clock_t end_time = times(&en_cpu);
    double qtime = (end_time-start_time)/websearch::_cl_sec;
    if (qtime<0)
      qtime = -1.0; // unavailable.

    if (!render)
      {
        mutex_unlock(&qc->_qc_mutex);
        return SP_ERR_OK;
      }

    const char *ui = miscutil::lookup(parameters,"ui");
    std::string ui_str = ui ? std::string(ui) : (websearch::_wconfig->_dyn_ui ? "dyn" : "stat");
    const char *output = miscutil::lookup(parameters,"output");
    std::string output_str = output ? std::string(output) : "html";
    std::transform(ui_str.begin(),ui_str.end(),ui_str.begin(),tolower);
    std::transform(output_str.begin(),output_str.end(),output_str.begin(),tolower);

    sp_err err = SP_ERR_OK;
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
              "/search_img?",param_exports);
        if (param_exports)
          delete param_exports;
      }
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
              qtime,true);
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
