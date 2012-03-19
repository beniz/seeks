#ifndef PRIVOXY_BLOCK_H
#define PRIVOXY_BLOCK_H

#define NO_PERL // we do not use Perl.

#include "plugin.h"
#include "filter_plugin.h"

#include <map>
#include <libxml/HTMLparser.h> // For htmlNodePtr

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

    private:
  };

  class adfilter_element : public filter_plugin
  {
    public:
      adfilter_element(plugin *parent);

      ~adfilter_element() {};

      char* run(client_state *csp, char *str);
    private:
      static std::map<const std::string, std::string> _rulesmap; // Maps of rules, key: url to be matched, value: xpath to unlink

      static const std::string _blocked_patterns_filename; // blocked patterns filename = "blocked-patterns"

      static const std::string _adblock_list_filename;     // adblock list file = "adblock_list"

      static const std::string _blocked_html; // <!-- blocked //-->
      static const std::string _blocked_js; // <!-- blocked //-->

      static void _filter(std::string *ret, std::string xpath);

      static std::string _line_to_xpath(std::string *xpath, std::string line);      // Convert an adblock list file line to an xpath
  };

} /* end of namespace. */

#endif
