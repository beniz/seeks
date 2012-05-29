/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "rdbl_pl.h"
#include "miscutil.h"
#include "curl_mget.h"
#include "websearch.h"
#include "proxy_configuration.h" // for transfer timeouts.
#include "uri_capture.h" // for parsing HTML page title.
#include "errlog.h"

#include <iostream>

namespace seeks_plugins
{

  rdbl_pl::rdbl_pl()
    :plugin()
  {
    _name = "readable";
    _version_major = "0";
    _version_minor = "1";

    //TODO: configuration.

    // cgi dispatchers.
    _cgi_dispatchers.reserve(1);
    cgi_dispatcher *cgid_readable
    = new cgi_dispatcher("readable",&rdbl_pl::cgi_readable,NULL,TRUE);
    _cgi_dispatchers.push_back(cgid_readable);

    // interceptor.
    _interceptor_plugin = new rdbl_elt(this);
  }

  rdbl_pl::~rdbl_pl()
  {
    //TODO: configuration.
  }

  sp_err rdbl_pl::cgi_readable(client_state *csp,
                               http_response *rsp,
                               const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    const char *url = miscutil::lookup(parameters,"url");
    if (!url)
      return SP_ERR_CGI_PARAMS;

    // options.
    int options = READABLE_OPTIONS_DEFAULT; // text only.
    const char *output = miscutil::lookup(parameters,"output");
    if (output && strcasecmp(output,"html")==0)
      options = PLUGIN_OPTIONS_DEFAULT; // html.

    // fetch content and make it readable.
    std::string content,title;
    sp_err err = rdbl_pl::fetch_url_call_readable(url,content,title,"utf-8",options);
    if (err != SP_ERR_OK)
      return err;

    // fill up response.
    miscutil::enlist(&rsp->_headers, "Content-Type: text/html; charset=utf-8");
    rsp->_body = strdup(content.c_str());
    rsp->_content_length = strlen(rsp->_body);
    rsp->_is_static = 1;
    return SP_ERR_OK;
  }

  sp_err rdbl_pl::fetch_url_call_readable(const std::string &url,
                                          std::string &content,
					  std::string &title,
                                          const std::string &encoding,
                                          const int &options)
  {
    static std::string default_ua = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:2.0.1) Gecko/20100101 Firefox/4.0.1";

    // fetch page.
    int status = 0;
    std::list<const char*> headers;
    miscutil::enlist(&headers,default_ua.c_str()); // XXX: we could use a default user-agent in a dedicated configuration file (e.g. wikipedia requires a user-agent header).
    curl_mget cmg(1,seeks_proxy::_config->_ct_connect_timeout,0,
                  seeks_proxy::_config->_ct_transfer_timeout,0);
    std::string *result = cmg.www_simple(url,&headers,status);
    if (status!=0)
      {
        if (result)
          delete result;
        return SP_ERR_NOT_FOUND; //TODO: more accurate error code.
      }

    // fetch title.
    title = uri_capture::parse_uri_html_title(*result);
    
    // call on readable.
    try
      {
        content = rdbl_pl::call_readable(*result,url,encoding,options);
      }
    catch (sp_exception &e)
      {
        delete result;
        errlog::log_error(LOG_LEVEL_ERROR,e.what().c_str());
        return e.code();
      }
    content = miscutil::chomp_cpp(content);
    delete result;
    return SP_ERR_OK;
  }

  std::string rdbl_pl::call_readable(const std::string &html, const std::string &url,
                                     const std::string &encoding, const int &options) throw (sp_exception)
  {
    char *rdbl_html = readable(html.c_str(),url.c_str(),
                               encoding.empty()?NULL:encoding.c_str(),options);
    if (!rdbl_html)
      throw sp_exception(SP_ERR_MEMORY,"Failed processing URL for readability");
    std::string str = rdbl_html;
    free(rdbl_html);
    return str;
  }

  /*- rdbl_elt -*/
  rdbl_elt::rdbl_elt(plugin *parent)
    :interceptor_plugin(std::vector<std::string>(),std::vector<std::string>(),parent)
  {
  }

  http_response* rdbl_elt::plugin_response(client_state *csp)
  {
    //TODO.
    return NULL;
  }

  /* auto-registration */
  extern "C"
  {
    plugin* maker()
    {
      return new rdbl_pl;
    }
  }

} /* end of namespace. */
