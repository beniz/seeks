#ifndef ADFILTER_H
#define ADFILTER_H

#define NO_PERL // we do not use Perl.

#include "plugin.h"
#include "filter_plugin.h"

#include "adblock_parser.h"
#include "adfilter_configuration.h"
#include "configuration_spec.h"

#include <map>

using namespace sp;

namespace seeks_plugins
{
  class adfilter : public plugin
  {
    public:
      adfilter();
      ~adfilter() {};
      virtual void start() {};
      virtual void stop() {};
      // Accessors/mutators
      adblock_parser* get_parser();
      // Methods
      void blocked_response(http_response *rsp, client_state *csp); // Set response regarding the csp content-type
    private:
      // Attributes
      //adfilter_configuration* _adconfig; // Configuration manager
      configuration_spec* _adconfig; // Configuration manager
      adblock_parser* _adbparser; // ADBlock rules parser
      std::map<std::string, std::string> _responses; // Responses per content-type
      // Methods
      void populate_responses(); // Populate _responses
  };

} /* end of namespace. */

#endif
