#ifndef ADFILTER_H
#define ADFILTER_H

#define NO_PERL // we do not use Perl.

#include "plugin.h"
#include "filter_plugin.h"

#include "adblock_parser.h"

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
    private:
      // Attributes
      adblock_parser *_adbparser; // ADBlock rules parser
  };

} /* end of namespace. */

#endif
