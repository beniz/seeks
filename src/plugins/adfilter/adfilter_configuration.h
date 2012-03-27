#ifndef ADFILTER_CONFIGURATION_H
#define ADFILTER_CONFIGURATION_H

#include "configuration_spec.h"

#include <list>

using namespace sp;
using sp::configuration_spec;

class adfilter_configuration : public configuration_spec
{
  public:
    adfilter_configuration(const std::string &filename);
    ~adfilter_configuration();

    // class methods.
    sp_err load_config();

    sp_err parse_config_line(char *cmd, char* arg, char *tmp, char* buf);

    int check_file_changed();

    // virtual
    void set_default_config();
    void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg, char *buf, const unsigned long &linenum);
    void finalize_configuration();

    // options
    //   adblock rules
    //std::list<std::string> _adblock_lists; // List of adblock rules URL
  private:
    int _update_frequency;                 // List update frequecy in seconds
    //   features activation
    bool _use_filter;                      // If true, use page filtering
    bool _use_blocker;                     // If true, use URL blocking
};
#endif
