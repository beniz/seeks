/**
* This file is part of the SEEKS project.
* Copyright (C) 2011 Fabien Dupont <fab+seeks@kafe-in.net>
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

#include "adblock_parser.h"
#include "adblocker_element.h"
#include "adfilter_element.h"
#include "adfilter_configuration.h"
#include "adfilter.h"

#include "proxy_configuration.h"
#include "configuration_spec.h"
#include "miscutil.h"
#include "encode.h"
#include "cgi.h"
#include "parsers.h"
#include "seeks_proxy.h"
#include "errlog.h"

#include <libxml/parser.h>
#include <libxml/threads.h>

using namespace sp;

namespace seeks_plugins
{
  /*
   * Constructor
   * --------------------
   */
  adfilter::adfilter() : plugin()
  {
    _name = "adfilter";
    _version_major = "0";
    _version_minor = "1";

    _config_filename = "adfilter-config";

    // Configuration
    _adconfig = new adfilter_configuration(
      seeks_proxy::_datadir.empty() ?
      plugin_manager::_plugin_repository + "adfilter/adfilter-config" :
      seeks_proxy::_datadir + "plugins/adfilter/adfilter-config"
    );
    _configuration = _adconfig;

    // Empty pattern vector for adblocker
    const std::vector<std::string> empty_pattern;

    // Always match pattern
    std::vector<std::string> always_pattern;
    std::string pat = "*";
    for(int i = 0; i < 8; i++)
      {
        pat.append(".*");
        always_pattern.push_back(pat);
      }

    // Create the plugins
    if(_adconfig->_use_filter)
      {
        _filter_plugin      = new adfilter_element(always_pattern, empty_pattern, this); // Filter plugin, everything but blocked URL
      }
    if(_adconfig->_use_blocker)
      {
        _interceptor_plugin = new adblocker_element(always_pattern, empty_pattern, this); // Interceptor plugin, blocked URL only
      }

    // libXML2 mutex
    this->mutexTok = xmlNewMutex();

    // libXML2 memory pre-allocation
    xmlInitParser();
  }

  /*
   * Destructor
   */
  adfilter::~adfilter()
  {
    // libXML2 mutex
    xmlFreeMutex(this->mutexTok);

    // libXML2 memory clean
    xmlCleanupParser();
  }

  void adfilter::start()
  {
    // Create a new parser and parse the adblock rules
    _adbparser = new adblock_parser(std::string(_name + "/adblock_list"));
    errlog::log_error(LOG_LEVEL_INFO, "adfilter: %d rules parsed successfully", _adbparser->parse_file(_adconfig->_use_filter, _adconfig->_use_blocker));

    // Responses per file type generation
    populate_responses();

    _adbdownl = new adblock_downloader(this, std::string(_name + "/adblock_list"));
    _adbdownl->start_timer();
  }

  void adfilter::stop()
  {
    delete _adbparser;
    _adbparser = NULL;
    _adconfig = NULL;
    delete _adbdownl;
  }

  /*
   * get_parser
   * Accessor to the _adbparser attribute
   * --------------------
   * Return value :
   * - adblock_parser* : ptr to the adblock_parser object
   */
  adblock_parser* adfilter::get_parser()
  {
    return _adbparser;
  }

  /*
   * get_config
   * Accessor to the _adconfig attribute
   * --------------------
   * Return value :
   * - adfilter_configuration* : ptr to the adfilter_configuration object
   */
  adfilter_configuration* adfilter::get_config()
  {
    return _adconfig;
  }

  /*
   * populate_response
   * Populate the "response per CT" map
   * --------------------
   */
  void adfilter::populate_responses()
  {
    // Empty gif for image response
    const char gif[] =
      "\107\111\106\070\071\141\004\000\004\000\200\000\000\310\310"
      "\310\377\377\377\041\376\016\111\040\167\141\163\040\141\040"
      "\142\141\156\156\145\162\000\041\371\004\001\012\000\001\000"
      "\054\000\000\000\000\004\000\004\000\000\002\005\104\174\147"
      "\270\005\000\073";

    // Text responses
    _responses.insert(std::pair<std::string, const char *>("text/html"      , "<!-- Blocked by seeks proxy //-->"));
    _responses.insert(std::pair<std::string, const char *>("text/css"       , "/* Blocked by seeks proxy */"));
    _responses.insert(std::pair<std::string, const char *>("text/javascript", "// Blocked by seeks proxy"));
    // Image responses
    _responses.insert(std::pair<std::string, const char *>("image/gif", strdup(gif)));
  }

  /*
   * blocked_response
   * Set the response body regarding the request content_type
   * --------------------
   * Parameters :
   * - http_response *rsp   : The HTTP response
   * - client_state *csp    : The request made through the proxy
   */
  void adfilter::blocked_response(http_response *rsp, client_state *csp)
  {
    // Request path
    std::string path = csp->_http._path;

    // If the request Content-Type match the key, set the value as body
    // FIXME Make a better file extension detection
    // Maybe use PCRE ? (seems heavy for not so much)
    if(path.find(".js") != std::string::npos)
      {
        // Javascript file
        rsp->_body = strdup((*(_responses.find("text/javascript"))).second);
        miscutil::enlist_unique_header(&rsp->_headers, "Content-Type", "text/javascript");
      }
    else if(path.find(".css") != std::string::npos)
      {
        // Stylesheet
        rsp->_body = strdup((*(_responses.find("text/css"))).second);
        miscutil::enlist_unique_header(&rsp->_headers, "Content-Type", "text/css");
      }
    else if(path.find(".png")  != std::string::npos or
            path.find(".jpg")  != std::string::npos or
            path.find(".jpeg") != std::string::npos or
            path.find(".gif")  != std::string::npos or
            path.find(".svg")  != std::string::npos or
            path.find(".bmp")  != std::string::npos or
            path.find(".tif")  != std::string::npos)
      {
        // Image
        rsp->_body = strdup((*(_responses.find("image/gif"))).second);
        miscutil::enlist_unique_header(&rsp->_headers, "Content-Type", "image/gif");
      }
    else
      {
        // Unknown type, let's assume that's an HTML response
        rsp->_body = strdup((*(_responses.find("text/html"))).second);
        miscutil::enlist_unique_header(&rsp->_headers, "Content-Type", "text/html");
      }
  }

  /* plugin registration */
  extern "C"
  {
    plugin* maker()
    {
      return new adfilter;
    }
  }

}
