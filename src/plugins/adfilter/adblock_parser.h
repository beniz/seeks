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

#ifndef ADBLOCK_PARSER_H
#define ADBLOCK_PARSER_H

#include "seeks_proxy.h"

#include <map>
#include <vector>

using namespace sp;

namespace adr
{
  // Enum for rules types
  enum rule_t
  {
    ADB_RULE_URL_BLOCK,      // Block an URL
    ADB_RULE_URL_FILTER,     // Filter a specific URL
    ADB_RULE_GENERIC_FILTER, // Generic filter
    ADB_RULE_UNSUPPORTED,    // Unsupported rule (yet)
    ADB_RULE_ERROR           // No success on parsing the rule
  };
  
  // Enum for conditions types
  enum condition_t
  {
    ADB_COND_TYPE,            // match only specific document type
    ADB_COND_NOT_TYPE,        // match only not specific document type
    ADB_COND_THIRD_PARTY,     // apply only if the referer != domain
    ADB_COND_NOT_THIRD_PARTY, // apply only if the referer == domain
    ADB_COND_DOMAIN,          // apply only if domain is a specific domain
    ADB_COND_NOT_DOMAIN,      // apply only if domain is not a specific domain
    ADB_COND_CASE,            // case sensitive rule
    ADB_COND_NOT_CASE,        // case insensitive rule
    ADB_COND_COLLAPSE,        // hide the element                       XXX default behavior
    ADB_COND_NOT_COLLAPSE,    // replace the element with a blank space XXX not possible
    ADB_COND_DO_NOT_TRACK,    // TODO add the Do-Not-Track header
    ADB_COND_NOT_SUPPORTED    // Unsupported condition
  };
  
  // Rule condition
  struct condition
  {
    condition_t type;
    std::string condition;
  };

  /*
   * ADBlock rule
   */
  struct adblock_rule
  {
    std::string url;
    std::vector<struct condition> conditions;
  };
}
  
/*
 * ADBlock rules parser
 */
class adblock_parser
{
  public:
    adblock_parser(std::string);
    ~adblock_parser() {};
    int parse_file(bool parse_filters, bool parse_blockers);               // Load adblock rules file
    bool is_blocked(client_state *csp);                                    // Is this URL blocked ?
    bool get_xpath(std::string url, std::string *xpath, bool withgeneric); // Get XPath for this URL
  private:
    // Attributes
    std::string                              _listfilename;      // adblock list file = "adblock_list"
    std::string                              _locallistfilename; // adblock list file = "adblock_list"
    std::map<const std::string, std::string> _filterrules;       // Maps of rules, key: url to be matched, value: xpath to unlink
    std::string                              _genericrule;       // Generic XPath for all sites
    std::vector<adr::adblock_rule>           _blockerules;       // Blocker rules
    // Methods
    adr::rule_t _line_to_rule(std::string line, adr::adblock_rule *rule, std::string *url, std::string *xpath); // Convert an adblock list file line to an xpath or a rule
};
#endif
