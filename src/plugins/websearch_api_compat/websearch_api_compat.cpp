/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#include "websearch_api_compat.h"
#include "cgi.h"
#include "urlmatch.h"
#include "encode.h"
#include "mrf.h"

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
#include "img_websearch.h"
#endif

using namespace sp;
using lsh::mrf;

namespace seeks_plugins
{

  websearch_api_compat::websearch_api_compat()
    : plugin()
  {
    _name = "websearch-api-compat";
    _version_major = "0";
    _version_minor = "3";

    // cgi_dispatchers.
    cgi_dispatcher *cgid_wb_search
    = new cgi_dispatcher("search", &websearch_api_compat::cgi_search_compat, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_search);

    cgi_dispatcher *cgid_wb_search_cache
    = new cgi_dispatcher("search_cache", &websearch_api_compat::cgi_search_cache_compat, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_search_cache);

    cgi_dispatcher *cgid_wb_qc_redir
    = new cgi_dispatcher("qc_redir",&websearch_api_compat::cgi_qc_redir_compat, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_qc_redir);

    cgi_dispatcher *cgid_wb_tbd
    = new cgi_dispatcher("tbd",&websearch_api_compat::cgi_tbd_compat, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_wb_tbd);

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    cgi_dispatcher *cgid_iwb_search
    = new cgi_dispatcher("search_img",&websearch_api_compat::cgi_img_search_compat, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_iwb_search);

    cgi_dispatcher *cgid_iwb_qc_redir
    = new cgi_dispatcher("qc_redir_img",&websearch_api_compat::cgi_img_qc_redir_compat, NULL, TRUE);
    _cgi_dispatchers.push_back(cgid_iwb_qc_redir);
#endif
  }

  websearch_api_compat::~websearch_api_compat()
  {
  }

  sp_err websearch_api_compat::cgi_search_compat(client_state *csp,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        // check for query.
        const char *query_str = miscutil::lookup(parameters,"q");
        if (!query_str || strlen(query_str) == 0)
          return SP_ERR_CGI_PARAMS;
        char *enc_query = encode::url_encode(query_str);
        std::string query = enc_query;
        free(enc_query);
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q");

        // check for action.
        const char *action = miscutil::lookup(parameters,"action");
        if (!action || strcasecmp(action,"expand")==0
            || strcasecmp(action,"page")==0)
          {
            // route to /search/txt
            free(csp->_http._path);
            std::string path = "/search/txt/" + query;
            csp->_http._path = strdup(path.c_str());
            return websearch::cgi_websearch_search(csp,rsp,parameters);
          }
        else if (strcasecmp(action,"types")==0)
          {
            // route to /cluster/types
            free(csp->_http._path);
            std::string path = "/cluster/types/" + query;
            csp->_http._path = strdup(path.c_str());
            return websearch::cgi_websearch_clustered_types(csp,rsp,parameters);
          }
        else if (strcasecmp(action,"clusterize")==0)
          {
            // route to /cluster/auto
            free(csp->_http._path);
            std::string path = "/cluster/auto/" + query;
            csp->_http._path = strdup(path.c_str());
            return websearch::cgi_websearch_clusterize(csp,rsp,parameters);
          }
        else if (strcasecmp(action,"similarity")==0)
          {
            // route to /similar/txt
            free(csp->_http._path);
            std::string path = "/similar/txt/" + query;
            csp->_http._path = strdup(path.c_str());
            return websearch::cgi_websearch_similarity(csp,rsp,parameters);
          }
        else return SP_ERR_CGI_PARAMS;
      }
    else return SP_ERR_CGI_PARAMS;
  }

  sp_err websearch_api_compat::cgi_search_cache_compat(client_state *csp,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        // check for query.
        const char *query_str = miscutil::lookup(parameters,"q");
        if (!query_str || strlen(query_str) == 0)
          return SP_ERR_CGI_PARAMS;
        std::string query = query_str;
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q");

        // check for url.
        const char *url_str = miscutil::lookup(parameters,"url");
        if (!url_str)
          return SP_ERR_CGI_PARAMS;
        std::string url = url_str;
        std::transform(url.begin(),url.end(),url.begin(),tolower);
        std::string surl = urlmatch::strip_url(url);
        uint32_t id = mrf::mrf_single_feature(surl);
        std::string sid = miscutil::to_string(id);
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"url");

        // route to /cache
        free(csp->_http._path);
        std::string path = "/cache/txt/" + query + "/" + sid;
        csp->_http._path = strdup(path.c_str());
        return websearch::cgi_websearch_search_cache(csp,rsp,parameters);
      }
    else return SP_ERR_CGI_PARAMS;
  }

  sp_err websearch_api_compat::cgi_qc_redir_compat(client_state *csp,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        // check for query.
        const char *query_str = miscutil::lookup(parameters,"q");
        if (!query_str || strlen(query_str) == 0)
          return SP_ERR_CGI_PARAMS;
        std::string query = query_str;
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q");

        // redirection.
        miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>\
                                (parameters),"redirect",1,"1",1);

        // check for url.
        const char *url_str = miscutil::lookup(parameters,"url");
        if (!url_str)
          return SP_ERR_CGI_PARAMS;
        std::string url = url_str;
        std::transform(url.begin(),url.end(),url.begin(),tolower);
        std::string surl = urlmatch::strip_url(url);
        uint32_t id = mrf::mrf_single_feature(surl);
        std::string sid = miscutil::to_string(id);
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"url");

        // route to POST /search/txt
        free(csp->_http._path);
        std::string path = "/search/txt/" + query + "/" + sid;
        csp->_http._path = strdup(path.c_str());
        free(csp->_http._gpc);
        csp->_http._gpc = strdup("post");
        return websearch::cgi_websearch_search(csp,rsp,parameters);
      }
    else return SP_ERR_CGI_PARAMS;
  }

  sp_err websearch_api_compat::cgi_tbd_compat(client_state *csp,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        // check for query.
        const char *query_str = miscutil::lookup(parameters,"q");
        if (!query_str || strlen(query_str) == 0)
          return SP_ERR_CGI_PARAMS;
        std::string query = query_str;
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q");

        // check for url.
        const char *url_str = miscutil::lookup(parameters,"url");
        if (!url_str)
          return SP_ERR_CGI_PARAMS;
        std::string url = url_str;
        std::transform(url.begin(),url.end(),url.begin(),tolower);
        std::string surl = urlmatch::strip_url(url);
        uint32_t id = mrf::mrf_single_feature(surl);
        std::string sid = miscutil::to_string(id);
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"url");

        // route to DELETE /search/txt
        free(csp->_http._path);
        std::string path = "/search/txt/" + query + "/" + sid;
        csp->_http._path = strdup(path.c_str());
        free(csp->_http._gpc);
        csp->_http._gpc = strdup("delete");
        sp_err err = websearch::cgi_websearch_search(csp,rsp,parameters);
        if (err != SP_ERR_OK)
          return err;
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"url");
        miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"action",1,"expand",1);
        miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"output",1,"html",1);

        //return websearch_api_compat::cgi_search_compat(csp,rsp,parameters);
        free(csp->_http._gpc);
        csp->_http._gpc = strdup("get");
        free(csp->_http._path);
        path = "/search/txt/" + query;
        csp->_http._path = strdup(path.c_str());
        return websearch::cgi_websearch_search(csp,rsp,parameters);
      }
    else return SP_ERR_CGI_PARAMS;
  }

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
  sp_err websearch_api_compat::cgi_img_search_compat(client_state *csp,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        // check for query.
        const char *query_str = miscutil::lookup(parameters,"q");
        if (!query_str || strlen(query_str) == 0)
          return SP_ERR_CGI_PARAMS;
        std::string query = query_str;
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q");

        // check for action.
        const char *action = miscutil::lookup(parameters,"action");
        if (!action || strcasecmp(action,"expand")==0
            || strcasecmp(action,"page")==0)
          {
            // route to /search/img
            free(csp->_http._path);
            std::string path = "/search/img/" + query;
            csp->_http._path = strdup(path.c_str());
            return img_websearch::cgi_img_websearch_search(csp,rsp,parameters);
          }
#ifdef FEATURE_OPENCV2
        else if (strcasecmp(action,"similarity")==0)
          {
            // route to /similar/img
            free(csp->_http._path);
            std::string path = "/similar/img/" + query;
            csp->_http._path = strdup(path.c_str());
            return img_websearch::cgi_img_websearch_similarity(csp,rsp,parameters);
          }
#endif
        else return SP_ERR_CGI_PARAMS;
      }
    else return SP_ERR_CGI_PARAMS;
  }

  sp_err websearch_api_compat::cgi_img_qc_redir_compat(client_state *csp,
      http_response *rsp,
      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        // check for query.
        const char *query_str = miscutil::lookup(parameters,"q");
        if (!query_str || strlen(query_str) == 0)
          return SP_ERR_CGI_PARAMS;
        std::string query = query_str;
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"q");

        // redirection.
        miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>\
                                (parameters),"redirect",1,"1",1);

        // check for url.
        const char *url_str = miscutil::lookup(parameters,"url");
        if (!url_str)
          return SP_ERR_CGI_PARAMS;
        std::string url = url_str;
        std::transform(url.begin(),url.end(),url.begin(),tolower);
        std::string surl = urlmatch::strip_url(url);
        uint32_t id = mrf::mrf_single_feature(surl);
        std::string sid = miscutil::to_string(id);
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"url");

        // route to POST /search/txt
        free(csp->_http._path);
        std::string path = "/search/img/" + query + "/" + sid;
        csp->_http._path = strdup(path.c_str());
        free(csp->_http._gpc);
        csp->_http._gpc = strdup("post");
        return img_websearch::cgi_img_websearch_search(csp,rsp,parameters);
      }
    else return SP_ERR_CGI_PARAMS;
  }
#endif

  /* plugin registration */
  extern "C"
  {
    plugin* maker()
    {
      return new websearch_api_compat;
    }
  }

} /* end of namespace. */
