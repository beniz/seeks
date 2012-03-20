#ifndef ADFILTER_ELEMENT_H
#define ADFILTER_ELEMENT_H

#define NO_PERL // we do not use Perl.

#include "adfilter.h"

#include "plugin.h"
#include "filter_plugin.h"

#include <libxml/HTMLparser.h> // For htmlNodePtr

using namespace sp;
using namespace seeks_plugins;

class adfilter_element : public filter_plugin
{
  public:
    adfilter_element(adfilter *parent);
    ~adfilter_element() {};
    char* run(client_state *csp, char *str);
  private:
    // Attributes
    adfilter* parent;                                    // Parent (sp::plugin)
    static const std::string _blocked_patterns_filename; // blocked patterns filename = "blocked-patterns"
    static const std::string _blocked_html;              // <!-- blocked //-->
    static const std::string _blocked_js;                // // blocked
    // Methods
    static void _filter(std::string *ret, std::string xpath);
};

#endif
