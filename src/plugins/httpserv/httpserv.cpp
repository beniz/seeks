/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera, <ebenazer@seeks-project.info>
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

#include "httpserv.h"
#include "httpserv_configuration.h"
#include "seeks_proxy.h"
#include "plugin_manager.h"
#include "websearch.h"
#include "websearch_api_compat.h"
#include "cgisimple.h"
#include "sweeper.h"
#include "miscutil.h"
#include "encode.h"
#include "errlog.h"
#include "cgi.h"

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
#include "img_websearch.h"
#endif

#if defined(PROTOBUF) && defined(TC)
#include "query_capture.h"
#include "cf.h"
#include "udb_service.h"
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <iostream>

using namespace sp;

namespace seeks_plugins
{

  httpserv::httpserv(const std::string &address,
                     const u_short &port)
    : plugin(),_address(address),_port(port)
  {
    /* initializing and starting server. */
    _evbase = event_base_new();
    _srv = evhttp_new(_evbase);
    evhttp_bind_socket(_srv, _address.c_str(), _port); // server binding to the address & port.

    errlog::log_error(LOG_LEVEL_INFO,"Seeks HTTP server plugin listening on %s:%u",
                      _address.c_str(),_port);

    init_callbacks();
    event_base_dispatch(_evbase); // XXX: for debug.
  }

  httpserv::httpserv()
    : plugin()
  {
    _name = "httpserv";
    _version_major = "0";
    _version_minor = "1";

    if (seeks_proxy::_datadir.empty())
      _config_filename = plugin_manager::_plugin_repository + "httpserv/httpserv-config";
    else
      _config_filename = seeks_proxy::_datadir + "/plugins/httpserv/httpserv-config";

#ifdef SEEKS_CONFIGDIR
    struct stat stFileInfo;
    if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
      {
        _config_filename = SEEKS_CONFIGDIR "/httpserv-config";
      }
#endif

    if (httpserv_configuration::_hconfig == NULL)
      httpserv_configuration::_hconfig = new httpserv_configuration(_config_filename);
    _configuration = httpserv_configuration::_hconfig;

    /* sets address & port. */
    _address = httpserv_configuration::_hconfig->_host;
    _port = httpserv_configuration::_hconfig->_port;
  }

  httpserv::~httpserv()
  {
  }

  void httpserv::start()
  {
    /* initializing and starting server. */
    _evbase = event_base_new();
    _srv = evhttp_new(_evbase);
    evhttp_bind_socket(_srv, _address.c_str(), _port); // server binding to the address & port.

    errlog::log_error(LOG_LEVEL_INFO,"Seeks HTTP server plugin listening on %s:%u",
                      _address.c_str(),_port);

    init_callbacks();

    int err = pthread_create(&_server_thread,NULL,
                             (void*(*)(void*))&event_base_dispatch,_evbase);
    seeks_proxy::_httpserv_thread = &_server_thread;
    if (seeks_proxy::_run_proxy)
      pthread_detach(_server_thread); // detach if proxy is running, else no (join).
  }

  void httpserv::stop()
  {
    event_base_loopbreak(_evbase);
    evhttp_free(_srv);
    event_base_free(_evbase);
  }

  void httpserv::init_callbacks()
  {
    evhttp_set_cb(_srv,"/search/txt",&httpserv::websearch,NULL);
    evhttp_set_cb(_srv,"/search",&httpserv::websearch,NULL); // compatibility API.
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    evhttp_set_cb(_srv,"/search_img",&httpserv::img_websearch,NULL); // compatibility API.
    evhttp_set_cb(_srv,"/seeks_img_search.css",&httpserv::seeks_img_search_css,NULL);
#endif
    evhttp_set_cb(_srv,"/",&httpserv::websearch_hp,NULL);
    evhttp_set_cb(_srv,"/websearch-hp",&httpserv::websearch_hp,NULL);
    evhttp_set_cb(_srv,"/seeks_hp_search.css",&httpserv::seeks_hp_css,NULL);
    evhttp_set_cb(_srv,"/seeks_search.css",&httpserv::seeks_search_css,NULL);
    evhttp_set_cb(_srv,"/opensearch.xml",&httpserv::opensearch_xml,NULL);
    evhttp_set_cb(_srv,"/info",&httpserv::node_info,NULL);
#if defined(PROTOBUF) && defined(TC)
    evhttp_set_cb(_srv,"/qc_redir",&httpserv::qc_redir,NULL); // compatibility API.
    evhttp_set_cb(_srv,"/tbd",&httpserv::tbd,NULL); // compatibility API.
    evhttp_set_cb(_srv,"/find_dbr",&httpserv::find_dbr,NULL);
    evhttp_set_cb(_srv,"/find_bqc",&httpserv::find_bqc,NULL);
    evhttp_set_cb(_srv,"/peers",&httpserv::peers,NULL);
#endif

    //evhttp_set_gencb(_srv,&httpserv::unknown_path,NULL); /* 404: catches all other resources. */
    evhttp_set_gencb(_srv,&httpserv::api_route,NULL);
  }

  void httpserv::reply_with_redirect_302(struct evhttp_request *r,
                                         const char *url)
  {
    evhttp_add_header(r->output_headers,"Location",url);
    evhttp_send_reply(r, HTTP_MOVETEMP, "OK", NULL);
  }

  void httpserv::reply_with_error_400(struct evhttp_request *r)
  {
    evhttp_send_reply(r, HTTP_BADREQUEST, "BAD REQUEST", NULL);
  }

  void httpserv::reply_with_error(struct evhttp_request *r,
                                  const int &http_code,
                                  const char *message,
                                  const std::string &error_message)
  {
    /* error output. */
    errlog::log_error(LOG_LEVEL_ERROR,"httpserv error: code %d %s",
                      http_code,error_message.c_str());

    // body.
    struct evbuffer *buffer;
    buffer = evbuffer_new();

    char msg[error_message.length()];
    for (size_t i=0; i<error_message.length(); i++)
      msg[i] = error_message[i];
    evbuffer_add(buffer,msg,sizeof(msg));

    evhttp_send_reply(r, http_code, message, buffer);
    evbuffer_free(buffer);
  }

  void httpserv::reply_with_empty_body(struct evhttp_request *r,
                                       const int &http_code,
                                       const char *message)
  {
    evhttp_send_reply(r, http_code, message, NULL);
  }

  void httpserv::reply_with_body(struct evhttp_request *r,
                                 const int &http_code,
                                 const char *message,
                                 const std::string &content,
                                 const std::string &content_type)
  {
    /* headers. */
    evhttp_add_header(r->output_headers,"Content-Type",content_type.c_str());

    /* body. */
    struct evbuffer *buffer;
    buffer = evbuffer_new();

    char msg[content.length()];
    for (size_t i=0; i<content.length(); i++)
      msg[i] = content[i];
    evbuffer_add(buffer,msg,sizeof(msg));

    evhttp_send_reply(r, http_code, message, buffer);
    evbuffer_free(buffer);
  }

  void httpserv::websearch_hp(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;

    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);

    /* return requested file. */
    sp_err serr = websearch::cgi_websearch_hp(&csp,&rsp,&parameters);
    miscutil::list_remove_all(&csp._headers);
    if (serr != SP_ERR_OK)
      {
        httpserv::reply_with_empty_body(r,404,"ERROR");
        return;
      }

    /* fill up response. */
    std::string content = std::string(rsp._body);
    httpserv::reply_with_body(r,200,"OK",content);

    /* run the sweeper, for timed out query contexts. */
    sweeper::sweep();
  }

  void httpserv::seeks_hp_css(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;

    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);

    /* return requested file. */
    sp_err serr = websearch::cgi_websearch_search_hp_css(&csp,&rsp,&parameters);
    miscutil::list_remove_all(&csp._headers);
    if (serr != SP_ERR_OK)
      {
        httpserv::reply_with_empty_body(r,404,"ERROR");
        return;
      }

    /* fill up response. */
    std::string content = std::string(rsp._body);
    httpserv::reply_with_body(r,200,"OK",content,"text/css");
  }

  void httpserv::seeks_search_css(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);

    /* return requested file. */
    sp_err serr = websearch::cgi_websearch_search_css(&csp,&rsp,&parameters);
    miscutil::list_remove_all(&csp._headers);
    if (serr != SP_ERR_OK)
      {
        httpserv::reply_with_empty_body(r,404,"ERROR");
        return;
      }

    /* fill up response. */
    std::string content = std::string(rsp._body);
    httpserv::reply_with_body(r,200,"OK",content,"text/css");
  }

  void httpserv::opensearch_xml(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;

    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);

    /* return requested file. */
    sp_err serr = websearch::cgi_websearch_opensearch_xml(&csp,&rsp,&parameters);
    miscutil::list_remove_all(&csp._headers);
    if (serr != SP_ERR_OK)
      {
        httpserv::reply_with_empty_body(r,404,"ERROR");
        return;
      }

    /* fill up response. */
    std::string content = std::string(rsp._body);
    httpserv::reply_with_body(r,200,"OK",content,"application/opensearchdescription+xml");
  }

  void httpserv::api_route(struct evhttp_request *r, void *arg)
  {
    // route to API resources.
    const char *uri_str = r->uri;
    std::string uri;
    if (uri_str)
      {
        uri = r->uri;
      }
    miscutil::to_lower(uri);
    if (uri.substr(0,12)=="/search/txt/")
      httpserv::websearch(r,arg);
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
    else if (uri.substr(0,12)=="/search/img/")
      httpserv::img_websearch(r,arg);
#endif
    // XXX: all websearch plugin calls use the same callback,
    // but we check here for fast error response, if resource
    // is unknown.
    else if (uri.substr(0,7)=="/words/")
      httpserv::websearch(r,arg);
    else if (uri.substr(0,16)=="/recent/queries/")
      httpserv::websearch(r,arg);
    else if (uri.substr(0,14)=="/cluster/type/")
      httpserv::websearch(r,arg);
    else if (uri.substr(0,14)=="/cluster/auto/")
      httpserv::websearch(r,arg);
    else if (uri.substr(0,13)=="/similar/txt/")
      httpserv::websearch(r,arg);
    else if (uri.substr(0,7)=="/cache/")
      httpserv::websearch(r,arg);
    else if (uri.substr(0,12)=="/suggestion/")
      httpserv::suggestion(r,arg);
    else if (uri.substr(0,16)=="/recommendation/")
      httpserv::recommendation(r,arg);

    // if unknown resource, trigger file service.
    else httpserv::file_service(r,arg);
  }

  void httpserv::node_info(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters = NULL;

    /* parse query. */
    const char *uri_str = r->uri;
    if (uri_str)
      {
        std::string uri = std::string(r->uri);
        parameters = httpserv::parse_query(uri);
      }

    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);

    /* return requested information. */
    sp_err serr = websearch::cgi_websearch_node_info(&csp,&rsp,parameters);
    miscutil::free_map(parameters);
    miscutil::list_remove_all(&csp._headers);
    if (serr != SP_ERR_OK)
      {
        httpserv::reply_with_empty_body(r,500,"ERROR");
        return;
      }

    /* fill up response. */
    std::string content = std::string(rsp._body);
    httpserv::reply_with_body(r,200,"OK",content,"application/json"); // this call is JSON only.
  }

  void httpserv::file_service(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
    = new hash_map<const char*,const char*,hash<const char*>,eqstr>();

    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);

    /* return requested file. */
    std::string uri_str = std::string(r->uri);

    /* XXX: truely, this is a hack, we're routing websearch file service. */
    std::string ct;  // content-type.
    sp_err serr = SP_ERR_OK;
    if (miscutil::strncmpic(uri_str.c_str(),"/plugins",8)==0)
      {
        uri_str = uri_str.substr(9);
        miscutil::add_map_entry(parameters,"file",1,uri_str.c_str(),1);
        serr = cgisimple::cgi_plugin_file_server(&csp,&rsp,parameters);
      }
    else if (miscutil::strncmpic(uri_str.c_str(),"/public",7)==0)
      {
        uri_str = uri_str.substr(8);
        miscutil::add_map_entry(parameters,"file",1,uri_str.c_str(),1);
        serr = cgisimple::cgi_file_server(&csp,&rsp,parameters);
      }
    else if (miscutil::strncmpic(uri_str.c_str(),"/websearch-hp",13)==0) // to catch some errors.
      {
        miscutil::free_map(parameters);
        miscutil::list_remove_all(&csp._headers);
        httpserv::websearch_hp(r,arg);
        return;
      }
    else if (miscutil::strncmpic(uri_str.c_str(),"/robots.txt",11)==0)
      {
        miscutil::add_map_entry(parameters,"file",1,uri_str.c_str(),1); // returns public/robots.txt
        serr = cgisimple::cgi_file_server(&csp,&rsp,parameters);
        ct = "text/plain";
      }

    // XXX: other services can be routed here.
    miscutil::free_map(parameters);
    miscutil::list_remove_all(&csp._headers);
    if (serr != SP_ERR_OK)
      {
        httpserv::reply_with_empty_body(r,404,"ERROR");
        return;
      }

    /* fill up response. */
    if (ct.empty())
      {
        std::list<const char*>::const_iterator lit = rsp._headers.begin();
        while (lit!=rsp._headers.end())
          {
            if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
              {
                ct = std::string((*lit));
                ct = ct.substr(14);
                break;
              }
            ++lit;
          }
      }
    std::string content = std::string(rsp._body,rsp._content_length);
    httpserv::reply_with_body(r,200,"OK",content,ct);
  }

  void httpserv::websearch(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters = NULL;

    /* parse query. */
    const char *uri_str = r->uri;
    std::string uri;
    if (uri_str)
      {
        uri = std::string(r->uri);
        parameters = httpserv::parse_query(uri);
      }
    if (!parameters || !uri_str)
      {
        // send 400 error response.
        if (parameters)
          miscutil::free_map(parameters);
        httpserv::reply_with_error_400(r);
        return;
      }

    /* fill up csp headers. */
    const char *rheader = evhttp_find_header(r->input_headers, "accept-language");
    if (rheader)
      miscutil::enlist_unique_header(&csp._headers,"accept-language",rheader);

    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);
    const char *ua = evhttp_find_header(r->input_headers, "user-agent");
    if (ua)
      miscutil::enlist_unique_header(&csp._headers,"user-agent",ua);

    /* perform websearch. */
    const char *output = miscutil::lookup(parameters,"output");
    if (!output)
      output = "html";
    sp_err serr = SP_ERR_OK;
    csp._http._path = strdup(uri.c_str());
    std::string http_method = httpserv::get_method(r);
    csp._http._gpc = strdup(http_method.c_str());
    if (uri.substr(0,12)=="/search/txt/")
      serr = websearch::cgi_websearch_search(&csp,&rsp,parameters);
    else if (uri.substr(0,7)=="/words/")
      serr = websearch::cgi_websearch_words(&csp,&rsp,parameters);
    else if (uri.substr(0,16)=="/recent/queries/")
      serr = websearch::cgi_websearch_recent_queries(&csp,&rsp,parameters);
    else if (uri.substr(0,14)=="/cluster/type/")
      serr = websearch::cgi_websearch_clustered_types(&csp,&rsp,parameters);
    else if (uri.substr(0,14)=="/cluster/auto/")
      serr = websearch::cgi_websearch_clusterize(&csp,&rsp,parameters);
    else if (uri.substr(0,13)=="/similar/txt/")
      serr = websearch::cgi_websearch_similarity(&csp,&rsp,parameters);
    else if (uri.substr(0,7)=="/search")
      serr = websearch_api_compat::cgi_search_compat(&csp,&rsp,parameters);
    miscutil::free_map(parameters);
    miscutil::list_remove_all(&csp._headers);

    int code = 200;
    std::string status = "OK";
    if (serr != SP_ERR_OK)
      {
        status = "ERROR";
        if (serr == SP_ERR_CGI_PARAMS
            || serr == WB_ERR_NO_ENGINE
            || serr == WB_ERR_QUERY_ENCODING)
          {
            cgi::cgi_error_bad_param(&csp,&rsp,output);
            code = 400;
          }
        else if (serr == SP_ERR_MEMORY)
          {
            http_response *crsp = cgi::cgi_error_memory();
            rsp = *crsp;
            delete crsp;
            code = 500;
          }
        else if (serr == WB_ERR_SE_CONNECT)
          {
            if (rsp._body)
              free(rsp._body);
            rsp._body = strdup("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/stric\
t.dtd\"><html><head><title>408 - Seeks fail connection to background search engines </title></head><body></body></html>");
            code = 408;
          }
        else
          {
            cgi::cgi_error_unknown(&csp,&rsp,serr);
            code = 500;
          }
      }

    /* fill up response. */
    std::string ct = "text/html"; // default content-type.
    std::list<const char*>::const_iterator lit = rsp._headers.begin();
    while (lit!=rsp._headers.end())
      {
        if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
          {
            ct = std::string((*lit));
            ct = ct.substr(14);
            break;
          }
        ++lit;
      }
    std::string content;
    if (rsp._body)
      content = std::string(rsp._body); // XXX: beware of length.

    if (status == "OK")
      httpserv::reply_with_body(r,code,"OK",content,ct);
    else httpserv::reply_with_error(r,code,"ERROR",content);

    /* run the sweeper, for timed out query contexts. */
    sweeper::sweep();
  }

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
  void httpserv::img_websearch(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters = NULL;

    /* parse query. */
    const char *uri_str = r->uri;
    std::string uri;
    if (uri_str)
      {
        uri = std::string(r->uri);
        parameters = httpserv::parse_query(uri);
      }
    if (!parameters || !uri_str)
      {
        // send 400 error response.
        if (parameters)
          miscutil::free_map(parameters);
        httpserv::reply_with_error_400(r);
        return;
      }

    /* fill up csp headers. */
    const char *rheader = evhttp_find_header(r->input_headers, "accept-language");
    if (rheader)
      miscutil::enlist_unique_header(&csp._headers,"accept-language",rheader);

    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);
    const char *ua = evhttp_find_header(r->input_headers, "user-agent");
    if (ua)
      miscutil::enlist_unique_header(&csp._headers,"user-agent",ua);

    /* perform websearch. */
    const char *output = miscutil::lookup(parameters,"output");
    if (!output)
      output = "html";
    sp_err serr = SP_ERR_OK;
    csp._http._path = strdup(uri.c_str());
    csp._http._gpc = strdup(httpserv::get_method(r).c_str());
    if (uri.substr(0,12)=="/search/img/")
      serr = img_websearch::cgi_img_websearch_search(&csp,&rsp,parameters);
    else serr = websearch_api_compat::cgi_img_search_compat(&csp,&rsp,parameters);
    miscutil::free_map(parameters);
    miscutil::list_remove_all(&csp._headers);

    int code = 200;
    std::string status = "OK";
    if (serr != SP_ERR_OK)
      {
        status = "ERROR";
        if (serr == SP_ERR_CGI_PARAMS
            || serr == WB_ERR_NO_ENGINE
            || serr == WB_ERR_QUERY_ENCODING)
          {
            cgi::cgi_error_bad_param(&csp,&rsp,output);
            code = 400;
          }
        else if (serr == SP_ERR_MEMORY)
          {
            http_response *crsp = cgi::cgi_error_memory();
            rsp = *crsp;
            delete crsp;
            code = 500;
          }
        else if (serr == WB_ERR_SE_CONNECT)
          {
            if (rsp._body)
              free(rsp._body);
            rsp._body = strdup("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/stric\
t.dtd\"><html><head><title>408 - Seeks fail connection to background search engines </title></head><body></body></html>");
            code = 408;
          }
        else
          {
            cgi::cgi_error_unknown(&csp,&rsp,serr);
            code = 500;
          }
      }

    /* fill up response. */
    std::string ct = "text/html"; // default content-type.
    std::list<const char*>::const_iterator lit = rsp._headers.begin();
    while (lit!=rsp._headers.end())
      {
        if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
          {
            ct = std::string((*lit));
            ct = ct.substr(14);
            break;
          }
        ++lit;
      }
    std::string content;
    if (rsp._body)
      content = std::string(rsp._body); // XXX: beware of length.

    if (status == "OK")
      httpserv::reply_with_body(r,code,"OK",content,ct);
    else httpserv::reply_with_error(r,code,"ERROR",content);

    /* run the sweeper, for timed out query contexts. */
    sweeper::sweep();
  }

  void httpserv::seeks_img_search_css(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);

    /* return requested file. */
    sp_err serr = img_websearch::cgi_img_websearch_search_css(&csp,&rsp,&parameters);
    miscutil::list_remove_all(&csp._headers);
    if (serr != SP_ERR_OK)
      {
        httpserv::reply_with_empty_body(r,404,"ERROR");
        return;
      }

    /* fill up response. */
    std::string content = std::string(rsp._body);
    httpserv::reply_with_body(r,200,"OK",content,"text/css");
  }
#endif

#if defined(PROTOBUF) && defined(TC)
  void httpserv::qc_redir(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters = NULL;

    /* parse query. */
    const char *uri_str = r->uri;
    if (uri_str)
      {
        std::string uri = std::string(r->uri);
        parameters = httpserv::parse_query(uri);
      }
    if (!parameters || !uri_str)
      {
        // send 400 error response.
        if (parameters)
          miscutil::free_map(parameters);
        httpserv::reply_with_error_400(r);
        return;
      }

    // fill up csp headers.
    const char *referer = evhttp_find_header(r->input_headers, "referer");
    if (referer)
      miscutil::enlist_unique_header(&csp._headers,"referer",referer);

    // call for capture callback.
    //char *urlp = NULL;
    //sp_err err = query_capture::qc_redir(&csp,&rsp,parameters,urlp);
    sp_err err = websearch_api_compat::cgi_qc_redir_compat(&csp,&rsp,parameters);
    miscutil::list_remove_all(&csp._headers);

    const char *urlp = miscutil::lookup(parameters,"url");
    char *urlp_dec = encode::url_decode(urlp);

    // error catching.
    if (err != SP_ERR_OK)
      {
        /* fill up response. */
        std::string ct = "text/html"; // default content-type.
        std::string content;
        if (rsp._body)
          content = std::string(rsp._body); // XXX: beware of length.
        int code = 500;
        if (err == SP_ERR_CGI_PARAMS)
          code = 400;
        else if (err == SP_ERR_PARSE)
          code = 403;
        httpserv::reply_with_error(r,code,"ERROR",content);
        miscutil::free_map(parameters);
        free(urlp_dec);

        /* run the sweeper, for timed out query contexts. */
        sweeper::sweep();
        return;
      }

    httpserv::reply_with_redirect_302(r,urlp_dec);
    miscutil::free_map(parameters);
    free(urlp_dec);

    /* run the sweeper, for timed out elements. */
    sweeper::sweep();
  }

  void httpserv::tbd(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters = NULL;

    /* parse query. */
    const char *uri_str = r->uri;
    if (uri_str)
      {
        std::string uri = std::string(r->uri);
        parameters = httpserv::parse_query(uri);
      }
    if (!parameters || !uri_str)
      {
        // send 400 error response.
        if (parameters)
          miscutil::free_map(parameters);
        httpserv::reply_with_error_400(r);
        return;
      }

    // fill up csp headers.
    const char *referer = evhttp_find_header(r->input_headers, "referer");
    if (referer)
      miscutil::enlist_unique_header(&csp._headers,"referer",referer);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",referer);
    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);

    // call for capture callback.
    const char *output = miscutil::lookup(parameters,"output");
    if (!output)
      output = "html";
    //sp_err serr = cf::cgi_tbd(&csp,&rsp,parameters);
    sp_err serr = websearch_api_compat::cgi_tbd_compat(&csp,&rsp,parameters);
    miscutil::free_map(parameters);
    miscutil::list_remove_all(&csp._headers);

    int code = 200;
    std::string status = "OK";
    if (serr != SP_ERR_OK)
      {
        status = "ERROR";
        if (serr == SP_ERR_CGI_PARAMS)
          {
            cgi::cgi_error_bad_param(&csp,&rsp,output);
            code = 400;
          }
        else if (serr == SP_ERR_MEMORY)
          {
            http_response *crsp = cgi::cgi_error_memory();
            rsp = *crsp;
            delete crsp;
            code = 500;
          }
        else
          {
            cgi::cgi_error_unknown(&csp,&rsp,serr);
            code = 500;
          }
      }

    /* fill up response. */
    std::string ct = "text/html"; // default content-type.
    std::list<const char*>::const_iterator lit = rsp._headers.begin();
    while (lit!=rsp._headers.end())
      {
        if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
          {
            ct = std::string((*lit));
            ct = ct.substr(14);
            break;
          }
        ++lit;
      }
    std::string content;
    if (rsp._body)
      content = std::string(rsp._body); // XXX: beware of length.

    if (status == "OK")
      httpserv::reply_with_body(r,code,"OK",content,ct);
    else httpserv::reply_with_error(r,code,"ERROR",content);

    /* run the sweeper, for timed out query contexts. */
    sweeper::sweep();
  }

  void httpserv::find_dbr(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters = NULL;

    /* parse query. */
    const char *uri_str = r->uri;
    if (uri_str)
      {
        std::string uri = std::string(r->uri);
        parameters = httpserv::parse_query(uri);
      }
    if (!parameters || !uri_str)
      {
        // send 400 error response.
        if (parameters)
          miscutil::free_map(parameters);
        httpserv::reply_with_error_400(r);
        return;
      }

    // fill up csp headers.
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);
    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);

    // call to find_dbr callback.
    sp_err serr = udb_service::cgi_find_dbr(&csp,&rsp,parameters);
    miscutil::free_map(parameters);
    miscutil::list_remove_all(&csp._headers);

    int code = 200;
    std::string status = "OK";
    std::string err_msg;
    if (serr != SP_ERR_OK && serr != DB_ERR_NO_REC)
      {
        status = "ERROR";
        if (serr == SP_ERR_CGI_PARAMS)
          {
            cgi::cgi_error_bad_param(&csp,&rsp,"html");
            err_msg = "Bad Parameter";
            code = 400;
          }
        else if (serr == SP_ERR_MEMORY)
          {
            http_response *crsp = cgi::cgi_error_memory();
            rsp = *crsp;
            delete crsp;
            err_msg = "Memory Error";
            code = 500;
          }
        else
          {
            cgi::cgi_error_unknown(&csp,&rsp,serr);
            code = 500;
          }
      }

    /* fill up response. */
    std::string ct = "text/html"; // default content-type.
    std::list<const char*>::const_iterator lit = rsp._headers.begin();
    while (lit!=rsp._headers.end())
      {
        if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
          {
            ct = std::string((*lit));
            ct = ct.substr(14);
            break;
          }
        ++lit;
      }
    std::string content;
    if (rsp._body && serr != DB_ERR_NO_REC)
      content = std::string(rsp._body,rsp._content_length); // XXX: beware of length.

    if (status == "OK")
      httpserv::reply_with_body(r,code,"OK",content,ct);
    else httpserv::reply_with_error(r,code,err_msg.c_str(),content);

    /* run the sweeper, for timed out query contexts. */
    sweeper::sweep();
  }

  void httpserv::find_bqc(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters = NULL;

    /* check that we're getting a proper POST request. */
#ifdef HAVE_LEVENT1
    if (r->type != EVHTTP_REQ_POST)
#else
    if (evhttp_request_get_command(r) != EVHTTP_REQ_POST)
#endif
      {
        httpserv::reply_with_error_400(r); //TODO: proper error type.
        return;
      }

    /* parse query. */
    const char *uri_str = r->uri;
    if (uri_str)
      {
        std::string uri = std::string(r->uri);
        parameters = httpserv::parse_query(uri);
      }
    if (!parameters || !uri_str)
      {
        // send 400 error response.
        if (parameters)
          miscutil::free_map(parameters);
        httpserv::reply_with_error_400(r);
        return;
      }

    /* grab POST content. */
#ifdef HAVE_LEVENT1
    evbuffer *input_buffer = r->input_buffer;
#else
    evbuffer *input_buffer = evhttp_request_get_input_buffer(r);
#endif
    if (!input_buffer)
      {
        httpserv::reply_with_error_400(r); //TODO: proper error type.
        return;
      }

#if !defined(HAVE_LEVENT1)
    std::string post_content;
    while (evbuffer_get_length(input_buffer))
      {
        int n;
        char cbuf[128];
        n = evbuffer_remove(input_buffer, cbuf, sizeof(input_buffer)-1);
        post_content += std::string(cbuf,n);
      }
#else
    std::string post_content = std::string((char*)input_buffer->buffer,
                                           input_buffer->off / sizeof(u_char));
#endif

    if (post_content.empty())
      {
        httpserv::reply_with_error_400(r); //TODO: proper error type.
        return;
      }
    size_t post_content_size = post_content.length() * sizeof(char);
    csp._iob._cur = new char[post_content_size+1];
    strlcpy(csp._iob._cur,post_content.c_str(),post_content_size+1);
    csp._iob._size = post_content_size+1;

    // fill up csp headers.
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);
    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);

    // call to find_bqc callback.
    sp_err serr = udb_service::cgi_find_bqc(&csp,&rsp,parameters);
    miscutil::free_map(parameters);
    miscutil::list_remove_all(&csp._headers);
    delete[] csp._iob._cur;
    csp._iob._cur = NULL;
    csp._iob._size = 0;

    int code = 200;
    std::string status = "OK";
    std::string err_msg;
    if (serr != SP_ERR_OK && serr != DB_ERR_NO_REC)
      {
        status = "ERROR";
        if (serr == SP_ERR_CGI_PARAMS)
          {
            cgi::cgi_error_bad_param(&csp,&rsp,"html");
            err_msg = "Bad Parameter";
            code = 400;
          }
        else if (serr == SP_ERR_MEMORY)
          {
            http_response *crsp = cgi::cgi_error_memory();
            rsp = *crsp;
            delete crsp;
            err_msg = "Memory Error";
            code = 500;
          }
        else
          {
            cgi::cgi_error_unknown(&csp,&rsp,serr);
            code = 500;
          }
      }

    /* fill up response. */
    std::string ct = "text/html"; // default content-type.
    std::list<const char*>::const_iterator lit = rsp._headers.begin();
    while (lit!=rsp._headers.end())
      {
        if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
          {
            ct = std::string((*lit));
            ct = ct.substr(14);
            break;
          }
        ++lit;
      }
    std::string content;
    if (rsp._body && serr != DB_ERR_NO_REC)
      content = std::string(rsp._body,rsp._content_length); // XXX: beware of length.

    if (status == "OK")
      httpserv::reply_with_body(r,code,"OK",content,ct);
    else httpserv::reply_with_error(r,code,err_msg.c_str(),content);

    /* run the sweeper, for timed out query contexts. */
    sweeper::sweep();
  }

#endif

#if defined(PROTOBUF) && defined(TC)
  void httpserv::peers(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    //csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters = NULL;

    /* parse query. */
    const char *uri_str = r->uri;
    std::string uri;
    if (uri_str)
      {
        uri = std::string(r->uri);
        parameters = httpserv::parse_query(uri);
      }
    if (!parameters || !uri_str)
      {
        // send 400 error response.
        if (parameters)
          miscutil::free_map(parameters);
        httpserv::reply_with_error_400(r);
        return;
      }

    /* fill up csp headers. */
    const char *rheader = evhttp_find_header(r->input_headers, "accept-language");
    if (rheader)
      miscutil::enlist_unique_header(&csp._headers,"accept-language",rheader);

    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);
    const char *ua = evhttp_find_header(r->input_headers, "user-agent");
    if (ua)
      miscutil::enlist_unique_header(&csp._headers,"user-agent",ua);

    /* ask for peer list */
    const char *output = miscutil::lookup(parameters,"output");
    if (!output)
      output = "json";
    csp._http._path = strdup(uri.c_str());
    std::string http_method = httpserv::get_method(r);
    csp._http._gpc = strdup(http_method.c_str());
    sp_err serr = cf::cgi_peers(&csp,&rsp,parameters);
    miscutil::free_map(parameters);
    miscutil::list_remove_all(&csp._headers);

    int code = 200;
    std::string status = "OK";
    if (serr != SP_ERR_OK)
      {
        status = "ERROR";
        if (serr == SP_ERR_CGI_PARAMS)
          {
            cgi::cgi_error_bad_param(&csp,&rsp,output);
            code = 400;
          }
        else if (serr == SP_ERR_MEMORY)
          {
            http_response *crsp = cgi::cgi_error_memory();
            rsp = *crsp;
            delete crsp;
            code = 500;
          }
        else
          {
            cgi::cgi_error_unknown(&csp,&rsp,serr);
            code = 500;
          }
      }

    /* fill up response. */
    std::string ct = "application/json"; // default content-type.
    std::list<const char*>::const_iterator lit = rsp._headers.begin();
    while (lit!=rsp._headers.end())
      {
        if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
          {
            ct = std::string((*lit));
            ct = ct.substr(14);
            break;
          }
        ++lit;
      }
    std::string content;
    if (rsp._body)
      content = std::string(rsp._body); // XXX: beware of length.

    if (status == "OK")
      httpserv::reply_with_body(r,code,"OK",content,ct);
    else httpserv::reply_with_error(r,code,"ERROR",content);

    /* run the sweeper, for timed out query contexts. */
    sweeper::sweep();
  }

  void httpserv::suggestion(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters = NULL;

    /* parse query. */
    const char *uri_str = r->uri;
    std::string uri;
    if (uri_str)
      {
        uri = std::string(r->uri);
        parameters = httpserv::parse_query(uri);
      }
    if (!parameters || !uri_str)
      {
        // send 400 error response.
        if (parameters)
          miscutil::free_map(parameters);
        httpserv::reply_with_error_400(r);
        return;
      }

    /* fill up csp headers. */
    const char *rheader = evhttp_find_header(r->input_headers, "accept-language");
    if (rheader)
      miscutil::enlist_unique_header(&csp._headers,"accept-language",rheader);

    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);
    const char *ua = evhttp_find_header(r->input_headers, "user-agent");
    if (ua)
      miscutil::enlist_unique_header(&csp._headers,"user-agent",ua);

    /* ask for peer list */
    const char *output = miscutil::lookup(parameters,"output");
    if (!output)
      output = "json";
    csp._http._path = strdup(uri.c_str());
    std::string http_method = httpserv::get_method(r);
    csp._http._gpc = strdup(http_method.c_str());
    sp_err serr = cf::cgi_suggestion(&csp,&rsp,parameters);
    miscutil::free_map(parameters);
    miscutil::list_remove_all(&csp._headers);

    int code = 200;
    std::string status = "OK";
    if (serr != SP_ERR_OK)
      {
        status = "ERROR";
        if (serr == SP_ERR_CGI_PARAMS)
          {
            cgi::cgi_error_bad_param(&csp,&rsp,output);
            code = 400;
          }
        else if (serr == SP_ERR_MEMORY)
          {
            http_response *crsp = cgi::cgi_error_memory();
            rsp = *crsp;
            delete crsp;
            code = 500;
          }
        else
          {
            cgi::cgi_error_unknown(&csp,&rsp,serr);
            code = 500;
          }
      }

    /* fill up response. */
    std::string ct = "application/json"; // default content-type.
    std::list<const char*>::const_iterator lit = rsp._headers.begin();
    while (lit!=rsp._headers.end())
      {
        if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
          {
            ct = std::string((*lit));
            ct = ct.substr(14);
            break;
          }
        ++lit;
      }
    std::string content;
    if (rsp._body)
      content = std::string(rsp._body); // XXX: beware of length.

    if (status == "OK")
      httpserv::reply_with_body(r,code,"OK",content,ct);
    else httpserv::reply_with_error(r,code,"ERROR",content);

    /* run the sweeper, for timed out query contexts. */
    sweeper::sweep();
  }

  void httpserv::recommendation(struct evhttp_request *r, void *arg)
  {
    client_state csp;
    csp._config = seeks_proxy::_config;
    http_response rsp;
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters = NULL;

    /* parse query. */
    const char *uri_str = r->uri;
    std::string uri;
    if (uri_str)
      {
        uri = std::string(r->uri);
        parameters = httpserv::parse_query(uri);
      }
    if (!parameters || !uri_str)
      {
        // send 400 error response.
        if (parameters)
          miscutil::free_map(parameters);
        httpserv::reply_with_error_400(r);
        return;
      }

    /* fill up csp headers. */
    const char *rheader = evhttp_find_header(r->input_headers, "accept-language");
    if (rheader)
      miscutil::enlist_unique_header(&csp._headers,"accept-language",rheader);

    const char *host = evhttp_find_header(r->input_headers, "host");
    if (host)
      miscutil::enlist_unique_header(&csp._headers,"host",host);
    const char *baseurl = evhttp_find_header(r->input_headers, "seeks-remote-location");
    if (baseurl)
      miscutil::enlist_unique_header(&csp._headers,"seeks-remote-location",baseurl);
    const char *ua = evhttp_find_header(r->input_headers, "user-agent");
    if (ua)
      miscutil::enlist_unique_header(&csp._headers,"user-agent",ua);

    /* ask for peer list */
    const char *output = miscutil::lookup(parameters,"output");
    if (!output)
      output = "json";
    csp._http._path = strdup(uri.c_str());
    std::string http_method = httpserv::get_method(r);
    csp._http._gpc = strdup(http_method.c_str());
    sp_err serr = cf::cgi_recommendation(&csp,&rsp,parameters);
    miscutil::free_map(parameters);
    miscutil::list_remove_all(&csp._headers);

    int code = 200;
    std::string status = "OK";
    if (serr != SP_ERR_OK)
      {
        status = "ERROR";
        if (serr == SP_ERR_CGI_PARAMS)
          {
            cgi::cgi_error_bad_param(&csp,&rsp,output);
            code = 400;
          }
        else if (serr == SP_ERR_MEMORY)
          {
            http_response *crsp = cgi::cgi_error_memory();
            rsp = *crsp;
            delete crsp;
            code = 500;
          }
        else
          {
            cgi::cgi_error_unknown(&csp,&rsp,serr);
            code = 500;
          }
      }

    /* fill up response. */
    std::string ct = "application/json"; // default content-type.
    std::list<const char*>::const_iterator lit = rsp._headers.begin();
    while (lit!=rsp._headers.end())
      {
        if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
          {
            ct = std::string((*lit));
            ct = ct.substr(14);
            break;
          }
        ++lit;
      }
    std::string content;
    if (rsp._body)
      content = std::string(rsp._body); // XXX: beware of length.

    if (status == "OK")
      httpserv::reply_with_body(r,code,"OK",content,ct);
    else httpserv::reply_with_error(r,code,"ERROR",content);

    /* run the sweeper, for timed out query contexts. */
    sweeper::sweep();
  }

#endif

  void httpserv::unknown_path(struct evhttp_request *r, void *arg)
  {
    httpserv::reply_with_empty_body(r,404,"ERROR");
  }

  hash_map<const char*,const char*,hash<const char*>,eqstr>* httpserv::parse_query(const std::string &uri)
  {
    /* decode uri. */
    char *dec_uri_str = evhttp_decode_uri(uri.c_str());
    std::string dec_uri = std::string(dec_uri_str);
    free(dec_uri_str);

    /* parse query. */
    char *argstring = strdup(uri.c_str());
    hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters =  cgi::parse_cgi_parameters(argstring);
    free(argstring);
    return parameters;
  }

  std::string httpserv::get_method(struct evhttp_request *r)
  {
    std::string http_method = "get";
#ifdef HAVE_LEVENT1
    if (r->type == EVHTTP_REQ_POST)
      http_method = "post";
    // XXX: PUT and DELETE are not supported by libevent-1.x
#else
    if (evhttp_request_get_command(r) == EVHTTP_REQ_POST)
      http_method ="post";
    else if (evhttp_request_get_command(r) == EVHTTP_REQ_PUT)
      http_method = "put";
    else if (evhttp_request_get_command(r) == EVHTTP_REQ_DELETE)
      http_method = "delete";
#endif
    return http_method;
  }

  /* plugin registration */
  extern "C"
  {
    plugin* maker()
    {
      return new httpserv;
    }
  }

} /* end of namespace. */
