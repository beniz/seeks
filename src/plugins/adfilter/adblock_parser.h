#ifndef ADBLOCK_PARSER_H
#define ADBLOCK_PARSER_H

#include "seeks_proxy.h"

#include <map>
#include <vector>

using namespace sp;

// Enum for rules types
enum rule_t {
  ADB_RULE_URL_BLOCK,      // Block an URL
  ADB_RULE_URL_FILTER,     // Filter a specific URL
  ADB_RULE_GENERIC_FILTER, // Generic filter
  ADB_RULE_ERROR           // No success on parsing the rule
};

/*
 * ADBlock rules parser
 */
class adblock_parser
{
  public:
    adblock_parser(std::string);
    ~adblock_parser() {};
    int parse_file(bool parse_filters, bool parse_blockers);               // Load adblock rules file
    bool is_blocked(std::string url);                                      // Is this URL blocked ?
    bool get_xpath(std::string url, std::string &xpath, bool withgeneric); // Get XPath for this URL
    std::vector<std::string>                                _blockedurls;  // List of blocked sites
  private:
    // Attributes
    std::string                              _listfilename; // adblock list file = "adblock_list"
    std::map<const std::string, std::string> _filterrules;  // Maps of rules, key: url to be matched, value: xpath to unlink
    std::string                              _genericrule;  // Generic XPath for all sites
    // Methods
    rule_t _line_to_rule(std::string *xpath, std::string *url, std::string line); // Convert an adblock list file line to an xpath
};

#endif
