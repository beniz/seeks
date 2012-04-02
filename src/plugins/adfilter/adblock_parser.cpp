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

#include "adblock_parser.h"

#include "plugin.h" // for plugin_manager
#include "errlog.h"
#include "miscutil.h" // for string funcs

#include <pcre.h> // PCRE

#include <iostream>
#include <fstream>
#include <map>
#include <vector>

using namespace sp;

/*
 * Constructor
 * -------------------
 * Parameters
 * - std::string filename : name of the adblock rules file
 */
adblock_parser::adblock_parser(std::string filename)
{
  this->_listfilename = (seeks_proxy::_datadir.empty() ? 
                        plugin_manager::_plugin_repository + filename : 
                        seeks_proxy::_datadir + "/plugins/" + filename);
}

/*
 * parse_file()
 * -------------------
 * Return value :
 * - Number of parsed rules
 */
int adblock_parser::parse_file(bool parse_filters = true, bool parse_blockers = true)
{
  std::ifstream ifs;
  int num_read = 0;

  ifs.open(this->_listfilename.c_str());
  if (ifs.is_open())
  {
    // Clear all knowed rules
    this->_blockedurls.clear();
    this->_filterrules.clear();

    std::string line;
    while (!ifs.eof()) {
      std::string x;
      std::string url;

      getline(ifs, line);
      // Trim ending characters
      line.erase(line.find_last_not_of(" \n\r\t")+1);

      if(!line.empty())
      {
        // Line is not empty
        rule_t ret = adblock_parser::_line_to_rule(&x, &url, line);
  
        // Block the whole URL
        if(ret == ADB_RULE_URL_BLOCK and parse_blockers)
        {
          this->_blockedurls.push_back(url);
        }
  
        // Block elements whatever the url
        else if(ret == ADB_RULE_GENERIC_FILTER and parse_filters)
        {
          // FIXME empty rules with $domain=
          if(!x.empty()) this->_genericrule.append((this->_genericrule.empty() ? "" : " | ") + x);
        }
  
        // Block elements of a specific url
        else if(ret == ADB_RULE_URL_FILTER and parse_filters)
        {
          // url is in fact a list or URLs (url1,url2,...,urln)
          size_t part = 0;
          while(!url.empty() && part != std::string::npos)
          {
            std::string r;
            std::map<std::string, std::string>::iterator it;

            // Add filter for url 0 -> first ","
            part = url.find_first_of(",");

            // If there already is an xpath for the current url, we append this one to it
            if((it = this->_filterrules.find(url.substr(0, part))) != this->_filterrules.end())
            {
              r = (*it).second;
              this->_filterrules.erase(it);
            }

            r.append((r.empty() ? "" : " | ") + x);
            if(!x.empty()) this->_filterrules.insert(std::pair<const std::string, std::string>(url.substr(0, part), r));
            // Analyse from first "," -> end
            url = url.substr(part + 1);
          }
        }
        num_read++;
      }
    }
  } else {
    errlog::log_error(LOG_LEVEL_ERROR, "ADFilter: can't open adblock file '%s':  %E", this->_listfilename.c_str());
    return -1;
  }
  ifs.close();
  return num_read;
}

/*
 * _line_to_rule
 * --------------------
 * Parameters :
 * - std::string *xpath : ptr to the xpath string
 * - std::string *url   : ptr to the url string
 * - std::string line   : line to be parsed
 * Return value :
 * - rule_t : Type of rule (see rule_t declaration)
 */
rule_t adblock_parser::_line_to_rule(std::string *xpath, std::string *url, std::string line)
{
  // If the line begins with "||", the complete following URL should be blocked
  if(line.substr(0,2) == "||")
  {
    // full URL blocking
    /* TODO
Restriction aux requêtes third-party/first-party (provenant d'un autre/du même site) : Si l'option third-party est spécifiée,
le filtre n'est appliqué qu'aux requêtes provenant d'une autre origine que la page actuellement affichée.
De manière similaire, ~third-party restreint l'action du filtre aux requêtes provenant de la même origine que la page couramment affichée.
    */
    line = line.substr(0, line.find("$"));

    // Patterns use regexp style for URL, so * is .*
    miscutil::replace_in_string(line, "*", ".*"); 

    (*url) = line.substr(2);
    return ADB_RULE_URL_BLOCK;
  }

  // Ignore positive patterns and comments
  else if(line.substr(0,2) != "@@" and line.substr(0,1) != "!")
  {
    pcre *re;
    const char *error;
    int erroffset, rc;
    int ovector[18];

    // Matching regexps (see http://forums.wincustomize.com/322441)
    // PCRE_CASELESS
    const char *rUrl        = "^([^#\\s]*)##";
    const char *rElement    = "^([#.]?)([a-z0-9\\\\*_-]*)((\\|)([a-z0-9\\\\*_-]*))?";
    const char *rAttr1      = "^\\[([^\\]]*)\\]";
    const char *rAttr2      = "^\\[\\s*([^\\^\\*~=\\s]+)\\s*([~\\^\\*]?=)\\s*\"([^\"]+)\"\\s*\\]";
    const char *rPseudo     = "^:([a-z_-]+)(\\(([n0-9]+)\\))?";
    const char *rCombinator = "^(\\s*[>+\\s])?";
    const char *rComma      = "^\\s*,";

    std::string parts;

    parts = "//|ELEM|";

    std::string lastRule = "";

    // URL detection
    re = pcre_compile(rUrl, PCRE_CASELESS, &error, &erroffset, NULL);
    rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 15);
    pcre_free(re);
    if(rc > 0)
    {
      (*url) = line.substr(ovector[2], ovector[3] - ovector[2]);
      // From now, start parsing after the URL and the two #
      line = line.substr(ovector[1] - ovector[0]);

      while(line.length() > 0 and line != lastRule)
      {
        lastRule = line;

        miscutil::chomp_cpp(line);
        if(line.length() == 0)
        {
          break;
        }

        re = pcre_compile(rElement, PCRE_CASELESS, &error, &erroffset, NULL);
        rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 18);
        pcre_free(re);
        if(rc > 0)
        {
          // If the first character is not a # or .
          if(ovector[3] == 0)
          {
            // Ignoring namespaces
            if(ovector[10] > 0)
            {
              std::string tmp = line.substr(ovector[10], ovector[11] - ovector[10]);
              miscutil::to_lower(tmp);
              //std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower); // Convert element name to lower case
              if(parts.rfind("|ELEM|") != std::string::npos) parts.replace(parts.rfind("|ELEM|"), 6, tmp);
            }
            else
            {
              std::string tmp = line.substr(ovector[4], ovector[5] - ovector[4]);
              miscutil::to_lower(tmp);
              //std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower); // Convert element name to lower case
              if(parts.rfind("|ELEM|") != std::string::npos) parts.replace(parts.rfind("|ELEM|"), 6, tmp);
            }
          }
          else if(line.substr(ovector[2], ovector[3] - ovector[2]) == "#")
          {
            // #, an ID
            if(parts.rfind("|ELEM|") != std::string::npos) parts.replace(parts.rfind("|ELEM|"), 6, "*");
            parts.append("[@id='" + line.substr(ovector[4], ovector[5] - ovector[4]) + "']");
          }
          else if(line.substr(ovector[2], ovector[3] - ovector[2]) == ".")
          {
            // ., a class
            if(parts.rfind("|ELEM|") != std::string::npos) parts.replace(parts.rfind("|ELEM|"), 6, "*");
            parts.append("[contains(@class, '" + line.substr(ovector[4], ovector[5] - ovector[4]) + "')]");
          }

          line = line.substr(ovector[1] - ovector[0]);
        }

        re = pcre_compile(rAttr2, PCRE_CASELESS, &error, &erroffset, NULL);
        rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 18);
        pcre_free(re);
        // Match attribute selectors : [lvalue=rvalue]
        // FIXME if rvalue contains one or more spaces, it should be 1 rule per value
        if(rc > 0)
        {
          std::string lvalue = line.substr(ovector[2], ovector[3] - ovector[2]);
          std::string oper   = line.substr(ovector[4], ovector[5] - ovector[4]);
          std::string rvalue = line.substr(ovector[6], ovector[7] - ovector[6]);
          miscutil::replace_in_string(rvalue, "'", "\\'");
          if(oper == "~=" or oper == "*=")
          {
            parts.append("[contains(@" + lvalue + ", '" + rvalue + "')]");
          }
          else if(oper == "^=")
          {
            parts.append("[starts-with(@" + lvalue + ", '" + rvalue + "')]");
          }
          else
          {
            // FIXME XPath error : Invalid expression //[@id='Meebo']
            parts.append("[@" + lvalue + "='" + rvalue + "']");
          }
          line = line.substr(ovector[1] - ovector[0]);
        }
        else
        {
          // Match attribute selectors : [lvalue]
          re = pcre_compile(rAttr1, PCRE_CASELESS, &error, &erroffset, NULL);
          rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 18);
          pcre_free(re);
          if(rc > 0)
          {
            // FIXME XPath error : Invalid expression //[@id='Meebo']
            parts.append("[@" + line.substr(ovector[2], ovector[3] - ovector[2]) + "]");
            line = line.substr(ovector[1] - ovector[0]);
          }
        }

        // Skip over pseudo-classes and pseudo-elements, which are of no use to us
        re = pcre_compile(rPseudo, PCRE_CASELESS, &error, &erroffset, NULL);
        while((rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 18)) > 0)
        {
          if(line.substr(ovector[2], ovector[3] - ovector[2]) == "nth-child")
          {
            if(ovector[7] > 0 and line.substr(ovector[6], ovector[7] - ovector[6]) == "n")
            {
              if(parts.rfind("|ELEM|") != std::string::npos)
              {
                parts.replace(parts.rfind("|ELEM|"), 6, "descendant::|ELEM|");
              } else {
                parts.append("descendant::*");
              }
            }
          }
          line = line.substr(ovector[1] - ovector[0]);
        }
        pcre_free(re);
        
        // Match combinators
        re = pcre_compile(rCombinator, PCRE_CASELESS, &error, &erroffset, NULL);
        rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 18);
        pcre_free(re);
        if(rc > 0 and ovector[3] - ovector[2] > 0)
        {
          if(line.substr(ovector[2], ovector[3] - ovector[2]).find(">") != std::string::npos)
          {
            parts.append("/");
          }
          else if(line.substr(ovector[2], ovector[3] - ovector[2]).find("+") != std::string::npos)
          {
            parts.append("/following-sibling::");
          }
          else
          {
            parts.append("//");
          }
          parts.append("|ELEM|");
          line = line.substr(ovector[1] - ovector[0]);
        }
        
        re = pcre_compile(rComma, PCRE_CASELESS, &error, &erroffset, NULL);
        rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 18);
        pcre_free(re);
        if(rc > 0)
        {
          parts.append(" | //|ELEM|");
          line = line.substr(ovector[1] - ovector[0]);
        }
      }

      if(parts == "//*")
      {
        return ADB_RULE_ERROR;
      }
      miscutil::chomp_cpp(parts);
      (*xpath) = parts;
    }
    
    if((*url).empty())
    {
      return ADB_RULE_GENERIC_FILTER;
    }
    else
    {
      return ADB_RULE_URL_FILTER;
    }
  }

  return ADB_RULE_ERROR;
}

/*
 * is_blocked
 * --------------------
 * Parameters :
 * - std::string url    : the checked URL
 * Return value :
 * - bool : true if the URL match something in the blocked list
 */
bool adblock_parser::is_blocked(std::string url)
{
  std::vector<std::string>::iterator it;
  for(it = this->_blockedurls.begin(); it != this->_blockedurls.end(); it++)
  {
    if(url.find((*it)) != std::string::npos) return true;
  }
  return false;
}

/*
 * get_xpath
 * --------------------
 * Parameters :
 * - std::string url    : the checked URL
 * - std::string *xpath : the corresponding XPath
 * - bool withgeneric   : if true, append the generic XPath to the url's specific
 * Return value :
 * - bool               : true if something has been found
 */
bool adblock_parser::get_xpath(std::string url, std::string &xpath, bool withgeneric = true)
{
  std::map<const std::string, std::string>::iterator it;
  xpath = "";

  for(it = this->_filterrules.begin(); it != this->_filterrules.end(); it++)
  {
    if(url.find((*it).first) != std::string::npos)
    {
      xpath.append((*it).second + (withgeneric ? " | " +  this->_genericrule : ""));
    }
  }
  return !xpath.empty();
}
