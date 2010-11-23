/**
 * The seeks proxy is part of the SEEKS project
 * It is based on Privoxy (http://www.privoxy.org), developped
 * by the Privoxy team.
 *
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "proxy_dts.h"

#include "mem_utils.h"

#include "interceptor_plugin.h"
#include "action_plugin.h"
#include "filter_plugin.h"

#include "cgi.h" // for cgi_error_memory_response.
#include "miscutil.h"
#include "urlmatch.h"
#include "filters.h"
#include "proxy_configuration.h"

#include <iostream>

namespace sp
{

  /*-- http_response --*/
  http_response::http_response()
      :_status(NULL),_head(NULL),_head_length(0),
      _body(NULL),_content_length(0),_is_static(0),
      _reason(0)
  {}

  http_response::http_response(char *head, char *body)
      :_status(NULL),_head(head),_head_length(strlen(_head)),
      _body(body),_content_length(strlen(_body)),_is_static(0),
      _reason(0)
  {}

  http_response::~http_response()
  {
    if (this != &cgi::_cgi_error_memory_response)
      {
        freez(_status);
        freez(_head);
        freez(_body);

        std::list<const char*>::iterator lit = _headers.begin();
        while (lit!=_headers.end())
          {
            const char *head = (*lit);
            lit = _headers.erase(lit);
            free_const(head);
          }
      }
    _head_length = 0;
  }

  // _reason remains unchanged.
  void http_response::reset()
  {
    if (_status)
      {
        freez(_status);
        _status = NULL;
      }
    if (_head)
      {
        freez(_head);
        _head = NULL;
      }
    if (_body)
      {
        freez(_body);
        _body = NULL;
      }
    _content_length = 0;
    _head_length = 0;
    _is_static = 0;
  }

  http_request::~http_request()
  {
    clear();
  }

  void http_request::clear()
  {
    freez(_cmd);
    freez(_ocmd);
    freez(_gpc);
    freez(_url);
    freez(_ver);

    freez(_host);
    freez(_path);
    freez(_hostport);
//	freez(_host_ip_addr_str); // freed in spsockets ?

#ifndef FEATURE_EXTENDED_HOST_PATTERNS
    freez(_dbuffer);
    _dbuffer = NULL;
    freez(_dvec);
#endif
  }

  // create a url_spec from a string.
  // warning, the content of the buffer is destroyed by this function.
  url_spec::url_spec(const char *buf)
  {
    _spec = strdup(buf);
# ifdef FEATURE_EXTENDED_HOST_PATTERNS
    _host_regex = NULL;
# else
    _dbuffer = NULL;
    _dvec = NULL;
    _dcount = 0;
    _unanchored = 0;
# endif
    _port_list = NULL;
    _preg = NULL;
    _tag_regex = NULL;
  }

  sp_err url_spec::create_url_spec(url_spec *&url, char *buf)
  {
    url = new url_spec(buf);

    /* Is it a tag pattern? */
    if (0 == miscutil::strncmpic("TAG:", url->_spec, 4))
      {
        /* The pattern starts with the first character after "TAG:" */
        const char *tag_pattern = buf + 4;
        return urlmatch::compile_pattern(tag_pattern, NO_ANCHORING, url, &url->_tag_regex);
      }

    /* If it isn't a tag pattern it must be an URL pattern. */
    return urlmatch::compile_url_pattern(url, buf);
  }

  // beware: destroys the object whereas the original
  // function did not...
  url_spec::~url_spec()
  {
    if (_spec)
      freez(_spec);
    _spec = NULL;
# ifdef FEATURE_EXTENDED_HOST_PATTERNS
    if (_host_regex)
      {
        regfree(_host_regex);
        freez(_host_regex);
      }
    _host_regex = NULL;
# else
    if (_dbuffer)
      freez(_dbuffer);
    _dbuffer = NULL;
    if (_dvec)
      freez(_dvec);
    _dvec = NULL;
# endif
    if (_port_list)
      freez(_port_list);
    if (_preg)
      {
        regfree(_preg);
        freez(_preg);
        _preg = NULL;
      }
    if (_tag_regex)
      {
        regfree(_tag_regex);
        freez(_tag_regex);
        _tag_regex = NULL;
      }
  }

  /*- action_spec -*/
  action_spec::action_spec(const action_spec *src)
      : _mask(src->_mask),_add(src->_add)
  {
    for (int i = 0; i < ACTION_STRING_COUNT; i++)
      {
        char * str = src->_string[i];
        if (str)
          {
            str = strdup(str);
            _string[i] = str;
          }
      }

    for (int i = 0; i < ACTION_MULTI_COUNT; i++)
      {
        _multi_remove_all[i] = src->_multi_remove_all[i];
        miscutil::list_duplicate(&_multi_remove[i],&src->_multi_remove[i]);
        miscutil::list_duplicate(&_multi_add[i],&src->_multi_add[i]);
      }
  }
  action_spec::~action_spec()
  {
    for (int i=0; i<ACTION_STRING_COUNT; i++)
      freez(_string[i]);
    for (int i=0; i<ACTION_MULTI_COUNT; i++)
      {
        miscutil::list_remove_all(&_multi_remove[i]);
        miscutil::list_remove_all(&_multi_add[i]);
      }
  }

  /*- current_action_spec -*/
  current_action_spec::~current_action_spec()
  {
    for (int i=0; i<ACTION_STRING_COUNT; i++)
      freez(_string[i]);
    for (int i=0; i<ACTION_MULTI_COUNT; i++)
      miscutil::list_remove_all(&_multi[i]);
  }; // destroys the action inner elements.

  re_filterfile_spec::~re_filterfile_spec()
  {
    freez(_name);
    freez(_description);
    miscutil::list_remove_all(&_patterns);
    pcrs_job::pcrs_free_joblist(_joblist);
  }

  forward_spec::~forward_spec()
  {
    if (_url)
      delete _url;
    freez(_gateway_host);
    freez(_forward_host);
    // beware: _next is left untouched.
  }

  /*- client_state -*/
  client_state::client_state()
      :_config(NULL),_flags(0),
# ifndef HAVE_RFC2553
      _ip_addr_long(0),
# endif
      _ip_addr_str(NULL),_fwd(NULL),
      _content_type(0),_content_length(0),
# ifdef FEATURE_CONNECTION_KEEP_ALIVE
      _expected_content_length(0),
# endif
      _error_message(NULL),_next(NULL)
  {
  }

  client_state::~client_state()
  {
    freez(_error_message);
    miscutil::list_remove_all(&_headers);
    miscutil::list_remove_all(&_tags);
  }

  char* client_state::execute_content_filter_plugins()
  {
    if (SP_ERR_OK != filters::prepare_for_filtering(this))
      {
        /*
         * failed to de-chunk or decompress.
         */
        return NULL;
      }

    char *oldstr = _iob._cur;

    std::list<filter_plugin*>::const_iterator fit;
    for (fit=_filter_plugins.begin(); fit!=_filter_plugins.end(); ++fit)
      {
        char *newstr = (*fit)->run(this, oldstr); // TODO: virtual call might be inside, for allowing straight pcrs use...
        /* if (oldstr != _iob._cur)
          {
        freez(oldstr);
          } */
        oldstr = newstr;
      }
    return oldstr;
  }

  void client_state::add_plugin(plugin *p)
  {
    _plugins.insert(p);
  }

  void client_state::add_interceptor_plugin(interceptor_plugin *ip)
  {
    _interceptor_plugins.push_back(ip);
  }

  void client_state::add_action_plugin(action_plugin *ap)
  {
    _action_plugins.push_back(ap);
  }

  void client_state::add_filter_plugin(filter_plugin *fp)
  {
    _filter_plugins.push_back(fp);
  }

} /* end of namespace. */
