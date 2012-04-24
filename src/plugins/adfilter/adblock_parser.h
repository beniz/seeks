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
#include <iostream>

using namespace sp;

namespace adr
{
  // Enum for rules types
  enum adb_rule_type
  {
    ADB_RULE_URL_BLOCK,      // Block an URL
    ADB_RULE_URL_FILTER,     // Filter a specific URL
    ADB_RULE_GENERIC_FILTER, // Generic filter
    ADB_RULE_UNSUPPORTED,    // Unsupported rule (yet)
    ADB_RULE_ERROR           // No success on parsing the rule
  };
  
  // Enum for conditions types
  enum adb_condition_type
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

  // Enum for filter types
  enum adb_filter_type
  {
    ADB_FILTER_ATTR_EQUALS,   // Check if attribute lvalue is equals to rvalue
    ADB_FILTER_ATTR_CONTAINS, // Check if attribute lvalue contains rvalue
    ADB_FILTER_ATTR_STARTS,   // Check if attribute lvalue starts with rvalue
    ADB_FILTER_ATTR_EXISTS,   // Check if attribute lvalue exists (no rvalue used here)
    ADB_FILTER_ELEMENT_IS     // Check if element name is rvalue (no lvalue used here)
  };
  
  // Rule condition
  struct adb_condition
  {
    adb_condition_type type;
    std::string condition;
  };

  // Rule filter
  struct adb_filter
  {
    adb_filter_type type;
    std::string lvalue;
    std::string rvalue;
  };

  /*
   * ADBlock rule
   */
  struct adb_rule
  {
    adb_rule_type type;
    std::string url;
    std::vector<struct adb_condition> conditions;
    std::vector<struct adb_filter> filters;
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
    int parse_file(bool parse_filters, bool parse_blockers);             // Load adblock rules file
    bool is_blocked(client_state *csp);                                  // Is this URL blocked ?
    std::vector<struct adr::adb_rule> get_rules(std::string url);        // Get rule for this URL
    std::multimap<std::string, struct adr::adb_rule> _filterrules;       // Filter rules per URL
    std::vector<struct adr::adb_rule>                _genericrules;      // Generic filter rules for all sites
    std::vector<struct adr::adb_rule>                _blockerules;       // Blocker rules
  private:
    // Attributes
    std::string                                      _listfilename;      // adblock list file = "adblock_list"
    std::string                                      _locallistfilename; // Local adblock list file = "adblock_list.local"
    // Methods
    void _line_to_rule(std::string line, struct adr::adb_rule *rule);    // Convert an adblock list file line to a rule
};
#endif
