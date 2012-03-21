/*
 * FIXME
 * - extra caracters returned on some pages
 * TODO
 * - Periodic download of adblock lists
 */

#include "adblock_parser.h"
#include "adfilter_element.h"
#include "adblocker_element.h"
#include "adfilter.h"

#include "miscutil.h"
#include "encode.h"
#include "cgi.h"
#include "parsers.h"
#include "seeks_proxy.h"
#include "errlog.h"

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
    _configuration = NULL;

    // Create a new parser and parse the adblock rules
    _adbparser = new adblock_parser(std::string(_name + "/adblock_list"));
    errlog::log_error(LOG_LEVEL_INFO, "adfilter: %d rules parsed successfully", _adbparser->parse_file());

    // Empty pattern vector for adblocker
    const std::vector<std::string> _empty_pattern;

    // Responses per file type generation
    this->populate_responses();

    // Create the plugins
    _filter_plugin      = new adfilter_element(this);  // Filter plugin
    _interceptor_plugin = new adblocker_element(_adbparser->_blockedurls, _empty_pattern, this); // Interceptor plugin
  }

  /*
   * get_parser
   * Accessor to the _adbparser attribute
   * --------------------
   * Return value :
   * - adblock_parser* : ptr to the adbblock_parser object
   */
  adblock_parser* adfilter::get_parser()
  {
    return this->_adbparser;
  }

  /*
   * populate_response
   * Populate the "response per CT" map
   * --------------------
   */
  void adfilter::populate_responses()
  {
    const char gif[] = "GIF89a\001\000\001\000\200\000\000\377\377\377\000\000"
                       "\000!\371\004\001\000\000\000\000,\000\000\000\000\001"
                       "\000\001\000\000\002\002D\001\000;";
    // Text responses
    this->_responses.insert(std::pair<std::string, std::string>("text/html" , "<!-- Blocked by seeks proxy //-->"));
    this->_responses.insert(std::pair<std::string, std::string>("text/css"  , "/* Blocked by seeks proxy */"));
    this->_responses.insert(std::pair<std::string, std::string>("text/js"   , "// Blocked by seeks proxy"));
    // Image responses
    this->_responses.insert(std::pair<std::string, std::string>("image/gif" , std::string(gif)));
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
    if(path.find(".js") != std::string::npos)
    {
      // Javascript file
      rsp->_body = strdup((*(this->_responses.find("text/js"))).second.c_str());
      miscutil::enlist_unique_header(&rsp->_headers, "Content-Type", "text/js");
    }
    else if(path.find(".css") != std::string::npos)
    {
      // Stylesheet
      rsp->_body = strdup((*(this->_responses.find("text/css"))).second.c_str());
      miscutil::enlist_unique_header(&rsp->_headers, "Content-Type", "text/css");
    }
    else if(path.find(".png") != std::string::npos or
            path.find(".jpg") != std::string::npos or
            path.find(".jpeg") != std::string::npos or
            path.find(".gif") != std::string::npos or
            path.find(".svg") != std::string::npos or
            path.find(".bmp") != std::string::npos or
            path.find(".tif") != std::string::npos)
    {
      // Image
      rsp->_body = strdup((*(this->_responses.find("image/gif"))).second.c_str());
      miscutil::enlist_unique_header(&rsp->_headers, "Content-Type", "image/gif");
    }
    else
    {
      // Unknown type, let's assume that's an HTML response
      rsp->_body = strdup((*(this->_responses.find("text/html"))).second.c_str());
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
