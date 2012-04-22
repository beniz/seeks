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
#include "parsers.h" // for headers parsing
#include "errlog.h"
#include "miscutil.h" // for string funcs

#include <pcre.h> // PCRE

#include <iostream>
#include <fstream>
#include <map>
#include <vector>

using namespace sp;
using namespace adr;

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
  this->_locallistfilename = this->_listfilename + ".local";
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
  std::ifstream ilfs;
  int num_read = 0;

  ifs.open(this->_listfilename.c_str());
  ilfs.open(this->_locallistfilename.c_str());
  if(ifs.is_open() or ilfs.is_open())
  {
    // Clear all knowed rules
    this->_blockerules.clear();
    this->_filterrules.clear();

    std::string line;
    while(!ifs.eof() or !ilfs.eof()) {
      // Read downloaded rules, then local rules
      if(!ifs.eof()) getline(ifs, line);
      else if(!ilfs.eof()) getline(ilfs, line);

      // Trim ending characters
      line.erase(line.find_last_not_of(" \n\r\t")+1);

      if(!line.empty())
      {
        std::string xpath;
        std::string url;
        adr::rule_t type;
        adr::adblock_rule rule;

        // Line is not empty
        type = adblock_parser::_line_to_rule(line, &rule, &url, &xpath);

        // Block the whole URL
        if(type == ADB_RULE_URL_BLOCK and parse_blockers)
        {
          _blockerules.push_back(rule);
        }
  
        // Block elements whatever the url
        else if(type == ADB_RULE_GENERIC_FILTER and parse_filters)
        {
          // FIXME empty rules with $domain=
          if(!xpath.empty()) this->_genericrule.append((this->_genericrule.empty() ? "" : " | ") + xpath);
        }
  
        // Block elements of a specific url
        else if(type == ADB_RULE_URL_FILTER and parse_filters)
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

            r.append((r.empty() ? "" : " | ") + xpath);
            if(!xpath.empty()) this->_filterrules.insert(std::pair<const std::string, std::string>(url.substr(0, part), r));
            // Analyse from first "," -> end
            url = url.substr(part + 1);
          }
        }

        else if(type == ADB_RULE_ERROR)
        {
          errlog::log_error(LOG_LEVEL_ERROR, "ADFilter: failed to parse : '%s'", line.c_str());
        }
        num_read++;
      }
    }
  } else {
    errlog::log_error(LOG_LEVEL_ERROR, "ADFilter: can't open adblock file '%s':  %E", this->_listfilename.c_str());
    return -1;
  }
  ifs.close();
  ilfs.close();
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
adr::rule_t adblock_parser::_line_to_rule(std::string line, adr::adblock_rule *rule, std::string *url, std::string *xpath)
{
  pcre *re;
  const char *error;
  int erroffset, rc;
  int ovector[18];

  // If the line begins with "||", the complete following URL should be blocked
  if(line.substr(0,2) == "||")
  {
    // full URL blocking RE (||domainpart.domainpart/path$cond1,cond2,..,condn)
    line = line.substr(2);
    std::string cond;
    if(line.find("$") != std::string::npos) cond = line.substr(line.find("$") + 1);;
    line = line.substr(0, line.find("$"));

    const char *rBlock = "^([^\\^\\/]*)[\\^\\/]*(.*)$";

    re = pcre_compile(rBlock, PCRE_CASELESS, &error, &erroffset, NULL);
    rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 15);
    pcre_free(re);
    if(rc > 0)
    {
      std::string domain;
      std::string path;

      // Domain and path computation
      if(ovector[3] > 0) domain = line.substr(ovector[2], ovector[3] - ovector[2]);
      path   = line.substr(ovector[4], ovector[5] - ovector[4]);
      if(!path.empty())
      {
        // RegEx element escaping
        miscutil::replace_in_string(path, ".", "\\.");
        miscutil::replace_in_string(path, "?", "\\?");
        miscutil::replace_in_string(path, "|", "\\|");
        miscutil::replace_in_string(path, "*", ".*");
        rule->url = domain + "/" + path + ".*";
      }
      else
      {
        rule->url = domain;
      }

      // Condition computation
      size_t part = 0;
      while(!cond.empty() && part != std::string::npos)
      {
        std::string c; // Condition
        std::string t; // -> type
        std::string v; // -> value
        struct adr::condition sc; // Condition structure

        // 0 -> first ","
        part = cond.find_first_of(",");
        c = cond.substr(0, part);

        sc.type = ADB_COND_NOT_SUPPORTED;

        if(c.find("=") != std::string::npos)
        {
          t = c.substr(0, c.find("="));
          v = c.substr(c.find("=") + 1);
          if(t == "domain")
          {
            sc.type = ADB_COND_DOMAIN;
            sc.condition = v;
          }
          else if(t == "~domain")
          {
            sc.type = ADB_COND_NOT_DOMAIN;
            sc.condition = v;
          }
        }
        else
        {
          t = c;
          if(t == "stylesheet" or
             t == "script" or
             t == "image" or
             t == "object" or
             t == "xmlhttprequest" or
             t == "object-subrequest" or
             t == "subdocument" or
             t == "document" or
             t == "elemhide" or
             t == "other")
          {
            sc.type = ADB_COND_TYPE;
            sc.condition = t;
          }
          else if(t == "~stylesheet" or
             t == "~script" or
             t == "~image" or
             t == "~object" or
             t == "~xmlhttprequest" or
             t == "~object-subrequest" or
             t == "~subdocument" or
             t == "~document" or
             t == "~elemhide" or
             t == "~other")
          {
            sc.type = ADB_COND_NOT_TYPE;
            sc.condition = t;
          }
          else if(t == "third-party" or t == "~first_party")
          {
            sc.type = ADB_COND_THIRD_PARTY;
          }
          else if(t == "~third-party" or t == "first_party")
          {
            sc.type = ADB_COND_NOT_THIRD_PARTY;
          }
          else if(t == "case")
          {
            sc.type = ADB_COND_CASE;
          }
          else if(t == "~case")
          {
            sc.type = ADB_COND_NOT_CASE;
          }
          else if(t == "collapse")
          {
            sc.type = ADB_COND_COLLAPSE;
          }
          else if(t == "~collapse")
          {
            sc.type = ADB_COND_NOT_COLLAPSE;
          }
          else if(t == "donottrack")
          {
            sc.type = ADB_COND_DO_NOT_TRACK;
          }
        }

        if(sc.type != ADB_COND_NOT_SUPPORTED) rule->conditions.push_back(sc);

        // Next iteration: analyse from first "," -> end
        cond = cond.substr(part + 1);
      }
    }
    else
    {
      // Something is wrong with this rule
      return ADB_RULE_ERROR;
    }

    return ADB_RULE_URL_BLOCK;
  }

  // Ignore positive patterns and comments
  else if(line.substr(0,2) != "@@" and line.substr(0,1) != "!")
  {
    // Matching regexps (see http://forums.wincustomize.com/322441)
    const char *rUrl        = "^([^#\\s]*)##";
    const char *rElement    = "^([#.]?)([a-z0-9\\\\*_-]*)((\\|)([a-z0-9\\\\*_-]*))?";
    const char *rAttr1      = "^\\[([^\\]]*)\\]";
    const char *rAttr2      = "^\\[\\s*([^\\^\\*~=\\s]+)\\s*([~\\^\\*]?=)\\s*\"([^\"]+)\"\\s*\\]";
    const char *rPseudo     = "^:([a-z_-]+)(\\(([n0-9]+)\\))?";
    const char *rCombinator = "^(\\s*[>+\\s])?";
    const char *rComma      = "^\\s*,";

    std::string parts = "//|ELEM|";
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
              if(tmp.empty()) tmp = "*";
              if(parts.rfind("|ELEM|") != std::string::npos) parts.replace(parts.rfind("|ELEM|"), 6, tmp);
            }
            else
            {
              std::string tmp = line.substr(ovector[4], ovector[5] - ovector[4]);
              miscutil::to_lower(tmp);
              if(tmp.empty()) tmp = "*";
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
        } else {
          parts.append("*");
        }

        re = pcre_compile(rAttr2, PCRE_CASELESS, &error, &erroffset, NULL);
        rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 18);
        pcre_free(re);
        // Match attribute selectors : [lvalue=rvalue]
        if(rc > 0)
        {
          std::string lvalue = line.substr(ovector[2], ovector[3] - ovector[2]);
          std::string oper   = line.substr(ovector[4], ovector[5] - ovector[4]);
          std::string rvalue = line.substr(ovector[6], ovector[7] - ovector[6]);
          miscutil::replace_in_string(rvalue, "'", "\\'");
          std::string sep;

          parts.append("[");

          if(oper == "^=")
          {
            // ^= is equivalent to "starts-with"
            parts.append("starts-with(@" + lvalue + ", '" + rvalue + "')");
          }
          else
          {
            // Else check if lvalue contains all rvalue elements
            // If we match classes, matched classes can be in any orders and separated by a space
            // If we match style, matched styles can be in any orders and separated by a semicolon (;)
            if(lvalue == "class") sep = " ";
            if(lvalue == "style") sep = ";";
            size_t part = 0;
            while(!rvalue.empty() && part != std::string::npos)
            {
              // Remove leading spaces
              while(rvalue.substr(0, 1) == " ")
              {
                rvalue = rvalue.substr(1);
              }
  
              // If that's not the first part checked
              if(part != 0) parts.append(" and ");
  
              part = rvalue.find_first_of(sep);
              parts.append("contains(@" + lvalue + ", '" + rvalue.substr(0, part) + "')");
  
              // Check for the next part
              rvalue = rvalue.substr(part + 1);
            }
          }

          parts.append("]");

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

  return ADB_RULE_UNSUPPORTED;
}

/*
 * TODO is_blocked
 * --------------------
 * Parameters :
 * - std::string url    : the checked URL
 * Return value :
 * - bool : true if the URL match something in the blocked list
 */
bool adblock_parser::is_blocked(client_state *csp)
{
  std::vector<adr::adblock_rule>::iterator it;
  bool ret = false;
  char *r = parsers::get_header_value(&csp->_headers, "Referer:");

  std::string host  = csp->_http._host;
  std::string lhost = csp->_http._host;
  std::string lpath = csp->_http._path;
  miscutil::to_lower(lhost);
  miscutil::to_lower(lpath);

  for(it = this->_blockerules.begin(); it != this->_blockerules.end(); it++)
  {
    std::string url   = (*it).url.c_str();
    std::string lurl  = (*it).url.c_str();
    miscutil::to_lower(lurl);

    // domain.tld or .domain.tld is found in the url
    if(lhost.find(lurl) == 0 or lhost.find("." + lurl) != std::string::npos)
    {
      ret = true;
      std::vector<adr::condition>::iterator cit;
      for(cit = (*it).conditions.begin(); cit != (*it).conditions.end(); cit++)
      {
        if((*cit).type == ADB_COND_CASE and host.find(url) != 0 and host.find("." + url) == std::string::npos)
        {
          // Case sensitive match
          ret = false;
        }
        else if((*cit).type == ADB_COND_THIRD_PARTY and (NULL == r or strstr(r, host.c_str()) != NULL))
        {
          // If the url is in the referer, then it's not a third party request
          ret = false;
        }
        else if((*cit).type == ADB_COND_NOT_THIRD_PARTY and (NULL == r or strstr(r, host.c_str()) == NULL))
        {
          // If the url is not in the referer, then it's a third party request
          ret = false;
        }
        else if((*cit).type == ADB_COND_DOMAIN and (NULL == r or strstr(r, (*cit).condition.c_str()) == NULL))
        {
          // If the referer does not correspond to the domain, then it's a match
          ret = false;
        }
        else if((*cit).type == ADB_COND_NOT_DOMAIN and (NULL == r or strstr(r, (*cit).condition.c_str()) != NULL))
        {
          // If the referer correspond to the domain, then it's a match
          ret = false;
        }
        else if((*cit).type == ADB_COND_TYPE and (
          ((*cit).condition == "script"     and lpath.find(".js") == std::string::npos) or
          ((*cit).condition == "image"      and
            lpath.find(".jpg") == std::string::npos and
            lpath.find(".png") == std::string::npos and
            lpath.find(".gif") == std::string::npos and
            lpath.find(".bmp") == std::string::npos and
            lpath.find(".svg") == std::string::npos and
            lpath.find(".tif") == std::string::npos
          ) or
          ((*cit).condition == "stylesheet" and lpath.find(".css") == std::string::npos) or
          ((*cit).condition == "object"     and
            lpath.find(".java") == std::string::npos and
            lpath.find(".swf") == std::string::npos
          )
        ))
        {
          // If the file extension does not match the filtered type
          // FIXME very ineficient
          ret = false;
        }
      }
    }
  }
  return ret;
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
bool adblock_parser::get_xpath(std::string url, std::string *xpath, bool withgeneric = true)
{
  std::map<const std::string, std::string>::iterator it;
  *xpath = "";

  for(it = this->_filterrules.begin(); it != this->_filterrules.end(); it++)
  {
    if(url.find((*it).first) != std::string::npos)
    {
      xpath->append((*it).second + (withgeneric ? " | " +  this->_genericrule : ""));
    }
  }
  return !xpath->empty();
}
