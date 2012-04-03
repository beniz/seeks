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

// Enum for rules types
enum rule_t {
  ADB_RULE_URL_BLOCK,      // Block an URL
  ADB_RULE_URL_FILTER,     // Filter a specific URL
  ADB_RULE_GENERIC_FILTER, // Generic filter
  ADB_RULE_UNSUPPORTED,    // Unsupported rule (yet)
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
