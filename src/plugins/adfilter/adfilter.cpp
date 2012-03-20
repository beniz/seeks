/*
 * FIXME
 * - bug on http://s.s/search/txt/debian no results returned
 * - extra caracters returned on some pages
 * TODO
 * - Periodic download of adblock lists
 * - Generate blocking patterns for blocker
 * - Two element, one plugin ?
 */

#include "adblock_parser.h"
#include "adfilter_element.h"
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

    // Create a new parser
    _adbparser = new adblock_parser(std::string(_name + "/adblock_list"));
    errlog::log_error(LOG_LEVEL_INFO, "adfilter: %d rules parsed successfully", _adbparser->parse_file());
    _filter_plugin = new adfilter_element(this);
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

  /* plugin registration */
  extern "C"
  {
    plugin* maker()
    {
      return new adfilter;
    }
  }

}
