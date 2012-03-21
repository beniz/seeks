#ifndef ADBLOCKER_ELEMENT_H
#define ADBLOCKER_ELEMENT_H

#define NO_PERL // we do not use Perl.

#include "adfilter.h"

#include "plugin.h"
#include "interceptor_plugin.h"
#include "cgi.h"

using namespace sp;
using namespace seeks_plugins;

class adblocker_element : public interceptor_plugin
{
  public:
    adblocker_element(const std::vector<std::string> &pos_patterns, const std::vector<std::string> &neg_patterns, adfilter *parent);
    ~adblocker_element() {};
    virtual void start() {};
    virtual void stop() {};
    http_response* plugin_response(client_state *csp);   // Main routine
  private:
    // Attributes
    adfilter* parent;                                    // Parent (sp::plugin)
    // Methods
    http_response* _block(client_state *csp);            // Block a request
};

#endif

