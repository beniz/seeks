/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2012 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#include "se_parser_bing_api.h"
#include "miscutil.h"

#include <strings.h>
#include <iostream>

using sp::miscutil;

namespace seeks_plugins
{

  se_parser_bing_api::se_parser_bing_api(const std::string &url)
    :se_parser(url),_deep_link_flag(false),_title_flag(false),_url_flag(false),
     _summary_flag(false),_cached_flag(false),_cite_flag(false),_date_flag(false),
     _sn(NULL)
  {
  }

  se_parser_bing_api::~se_parser_bing_api()
  {
  }

  void se_parser_bing_api::start_element(parser_context *pc,
                                         const xmlChar *name,
                                         const xmlChar **attributes)
  {
    const char *tag = (const char*)name;

    if (strcasecmp(tag,"web:WebResult")==0)
      {
        _sn = new seeks_snippet(_count+1);
        _count++;
        _sn->_engine = feeds("bing_api",_url);
        pc->_current_snippet = _sn;
        pc->_snippets->push_back(_sn);
      }
    else if (strcasecmp(tag,"web:DeepLink")==0)
      {
        _deep_link_flag = true;
      }
    else if (strcasecmp(tag,"web:Title")==0)
      {
        _title_flag = true;
      }
    else if (strcasecmp(tag,"web:Url")==0)
      {
        _url_flag = true;
      }
    else if (strcasecmp(tag,"web:CacheUrl")==0)
      {
        _cached_flag = true;
      }
    else if (strcasecmp(tag,"web:Description")==0)
      {
        _summary_flag = true;
      }
    else if (strcasecmp(tag,"web:DisplayUrl")==0)
      {
        _cite_flag = true;
      }
    else if (strcasecmp(tag,"web:DateTime")==0)
      {
        _date_flag = true;
      }
  }

  void se_parser_bing_api::characters(parser_context *pc,
                                      const xmlChar *chars,
                                      int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_bing_api::cdata(parser_context *pc,
                                 const xmlChar *chars,
                                 int length)
  {
    handle_characters(pc, chars, length);
  }

  void se_parser_bing_api::handle_characters(parser_context *pc,
      const xmlChar *chars,
      int length)
  {
    if (_deep_link_flag)
      return;

    std::string schars = std::string((char*)chars,length);
    if (_title_flag)
      _title += schars;
    else if (_summary_flag)
      _summary += schars;
    else if (_url_flag)
      _surl += schars;
    else if (_cached_flag)
      _cached += schars;
    else if (_cite_flag)
      _cite += schars;
    else if (_date_flag)
      _date += schars;
  }

  void se_parser_bing_api::end_element(parser_context *pc,
                                       const xmlChar *name)
  {
    const char *tag = (const char*)name;

    if (strcasecmp(tag,"web:DeepLink")==0)
      {
        _deep_link_flag = false;
      }
    else if (!_deep_link_flag && _title_flag && strcasecmp(tag,"web:Title")==0)
      {
        _sn->set_title(_title);
        _title.clear();
        _title_flag = false;
      }
    else if (!_deep_link_flag && _url_flag && strcasecmp(tag,"web:Url")==0)
      {
        _sn->set_url_no_decode(_surl);
        _surl.clear();
        _url_flag = false;
      }
    else if (!_deep_link_flag && _cached_flag && strcasecmp(tag,"web:CacheUrl")==0)
      {
        _sn->_cached = _cached;
        _cached.clear();
        _cached_flag = false;
      }
    else if (_summary_flag && strcasecmp(tag,"web:Description")==0)
      {
        _sn->set_summary(_summary);
        _summary.clear();
        _summary_flag = false;
      }
    else if (!_deep_link_flag && _cite_flag && strcasecmp(tag,"web:DisplayUrl")==0)
      {
        _sn->set_cite(_cite);
        _cite.clear();
        _cite_flag = false;
      }
    else if (!_deep_link_flag && _date_flag && strcasecmp(tag,"web:DateTime")==0)
      {
        _sn->set_date(_date);
        _date.clear();
        _date_flag = false;
      }
  }

} /* end of namespace. */
