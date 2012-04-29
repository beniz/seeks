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
  _listfilename = (seeks_proxy::_datadir.empty() ?
                   plugin_manager::_plugin_repository + filename :
                   seeks_proxy::_datadir + "/plugins/" + filename);
  _locallistfilename = _listfilename + ".local";
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

  ifs.open(_listfilename.c_str());
  ilfs.open(_locallistfilename.c_str());
  if(ifs.is_open() or ilfs.is_open())
    {
      // Clear all knowed rules
      blockerules.clear();
      filterrules.clear();

      std::string line;
      while((ifs.good() and !ifs.eof()) or (ilfs.good() and !ilfs.eof()))
        {
          // Read downloaded rules, then local rules
          if(ifs.good() and !ifs.eof()) getline(ifs, line);
          else if(ilfs.good() and !ilfs.eof()) getline(ilfs, line);

          // Trim ending characters
          line.erase(line.find_last_not_of(" \n\r\t")+1);

          if(!line.empty())
            {
              adr::adb_rule rule;

              // Line is not empty
              adblock_parser::line_to_rule(line, &rule);

              // Block the whole URL
              if(rule.type == ADB_RULE_URL_BLOCK and parse_blockers)
                {
                  blockerules.push_back(rule);
                }

              // Block elements whatever the url
              else if(rule.type == ADB_RULE_GENERIC_FILTER and parse_filters)
                {
                  genericrules.push_back(rule);
                }

              // Block elements of a specific url
              else if(rule.type == ADB_RULE_URL_FILTER and parse_filters)
                {
                  // url is in fact a list or URLs (url1,url2,...,urln)
                  size_t part = 0;
                  std::string url = rule.url;
                  while(!url.empty() && part != std::string::npos)
                    {
                      // Add filter for url 0 -> first ","
                      part = url.find_first_of(",");
                      rule.url = url.substr(0, part);
                      filterrules.insert(std::pair<std::string, adr::adb_rule>(rule.url, rule));

                      // Analyse from first "," -> end
                      url = url.substr(part + 1);
                    }
                }

              // Exception rule, this URL must not be filtered or blocked
              else if(rule.type == ADB_RULE_URL_EXCEPTION)
                {
                  exceptionsrules.push_back(rule);
                }

              // No action (comments, etc.)
              else if(rule.type == ADB_RULE_NO_ACTION)
                {
                  num_read--;
                }

              // An error occured (maybe a malformed rule)
              else if(rule.type == ADB_RULE_ERROR)
                {
                  errlog::log_error(LOG_LEVEL_DEBUG, "ADFilter: failed to parse : '%s'", line.c_str());
                  num_read--;
                }

              // Unsupported rule (yet)
              else
                {
                  errlog::log_error(LOG_LEVEL_DEBUG, "ADFilter: unsupported rule : '%s'", line.c_str());
                  num_read--;
                }

              num_read++;
            }
        }
    }
  else
    {
      errlog::log_error(LOG_LEVEL_ERROR, "ADFilter: can't open adblock file '%s':  %E", _listfilename.c_str());
      return -1;
    }
  ifs.close();
  ilfs.close();
  return num_read;
}

/*
 * line_to_rule
 * --------------------
 * Parameters :
 * - std::string line           : line to be parsed
 * - struct adr::adb_rule *rule : The rule structure (see .h)
 */
void adblock_parser::line_to_rule(std::string line, struct adr::adb_rule *rule)
{
  pcre *re;
  const char *error;
  int erroffset, rc;
  int ovector[18];

  // Comments
  if(line.substr(0, 1) == "!")
    {
      rule->type = ADB_RULE_NO_ACTION;
    }

  // ||domain.tld should block domain.tld but not *.domain.tld
  // domain.tld should block *.domain.tld
  else if(line.find("##") == std::string::npos)
    {
      //line = line.substr(2);
      std::string cond;
      if(line.find("$") != std::string::npos) cond = line.substr(line.find("$") + 1);;
      line = line.substr(0, line.find("$"));

      // TODO Better separator (^) implementation (Should be two fields : host and path)
      const char *rBlock = "^(@@\\|\\||@@|\\|\\|)?([^\\^\\/]*)[\\^\\/]*(.*)$";

      re = pcre_compile(rBlock, PCRE_CASELESS, &error, &erroffset, NULL);
      rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 15);
      pcre_free(re);
      if(rc > 0)
        {
          std::string domain;
          std::string path;

          if(ovector[3] > 0)
            {
              if(line.substr(ovector[2], ovector[3] - ovector[2]) == "||")
                {
                  // ||domain.tld, domain.tld should be blocked
                  rule->type = ADB_RULE_URL_BLOCK;
                  struct adr::adb_condition sc;
                  sc.type = ADB_COND_START_WITH;
                  rule->conditions.push_back(sc);
                }
              else if(line.substr(ovector[2], ovector[3] - ovector[2]) == "@@||")
                {
                  // @@||domain.tld, domain.tld should NOT be blocked
                  rule->type = ADB_RULE_URL_EXCEPTION;
                  struct adr::adb_condition sc;
                  sc.type = ADB_COND_START_WITH;
                  rule->conditions.push_back(sc);
                }
              else if(line.substr(ovector[2], ovector[3] - ovector[2]) == "@@")
                {
                  // @@domain.tld, *domain.tld* should NOT be blocked
                  rule->type = ADB_RULE_URL_EXCEPTION;
                }
            }
          else
            {
              // domain.tld, *domain.tld* should be blocked
              rule->type = ADB_RULE_URL_BLOCK;
            }

          // Domain and path computation
          if(ovector[5] > 0) domain = line.substr(ovector[4], ovector[5] - ovector[4]);
          path   = line.substr(ovector[6], ovector[7] - ovector[6]);
          if(!path.empty())
            {
              // RegEx element escaping
              // TODO pattern matching
              /*miscutil::replace_in_string(path, ".", "\\.");
              miscutil::replace_in_string(path, "?", "\\?");
              miscutil::replace_in_string(path, "|", "\\|");
              miscutil::replace_in_string(path, "*", ".*");*/
              rule->url = domain + "/" + path; // + ".*";
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
              struct adr::adb_condition sc; // Condition structure

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
                      rule->case_sensitive = true;
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
          rule->type = ADB_RULE_ERROR;
        }
    }

  // Element filter
  else if(line.find("##") != std::string::npos)
    {
      // Matching regexps (see http://forums.wincustomize.com/322441)
      const char *rUrl        = "^~?([^#\\s]*)##";
      const char *rElement    = "^([#.]?)([a-z0-9\\\\*_-]*)((\\|)([a-z0-9\\\\*_-]*))?";
      const char *rAttr1      = "^\\[([^\\]]*)\\]";
      const char *rAttr2      = "^\\[\\s*([^\\^\\*~=\\s]+)\\s*([~\\^\\*]?=)\\s*\"([^\"]+)\"\\s*\\]";
      // TODO descendant and childs
      //const char *rPseudo     = "^:([a-z_-]+)(\\(([n0-9]+)\\))?";
      //const char *rCombinator = "^(\\s*[>+\\s])?";
      //const char *rComma      = "^\\s*,";

      rule->type = ADB_RULE_GENERIC_FILTER;
      std::string lastRule = "";

      // URL detection
      re = pcre_compile(rUrl, PCRE_CASELESS, &error, &erroffset, NULL);
      rc = pcre_exec(re, NULL, line.c_str(), line.length(), 0, 0, ovector, 15);
      pcre_free(re);
      if(rc > 0)
        {
          if(ovector[3] > 0)
            {
              // If there is an URL, it's a specific rule
              rule->url = line.substr(ovector[2], ovector[3] - ovector[2]);
              rule->type = ADB_RULE_URL_FILTER;
            }

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
                          if(!tmp.empty())
                            {
                              struct adb_filter f;
                              f.type = ADB_FILTER_ELEMENT_IS;
                              f.rvalue = tmp;
                              rule->filters.push_back(f);
                            }
                        }
                      else
                        {
                          std::string tmp = line.substr(ovector[4], ovector[5] - ovector[4]);
                          if(!tmp.empty())
                            {
                              struct adb_filter f;
                              f.type = ADB_FILTER_ELEMENT_IS;
                              f.rvalue = tmp;
                              rule->filters.push_back(f);
                            }
                        }
                    }
                  else if(line.substr(ovector[2], ovector[3] - ovector[2]) == "#")
                    {
                      // #, an ID
                      struct adb_filter f;
                      f.type = ADB_FILTER_ATTR_EQUALS;
                      f.lvalue = "id";
                      f.rvalue = line.substr(ovector[4], ovector[5] - ovector[4]);
                      rule->filters.push_back(f);
                    }
                  else if(line.substr(ovector[2], ovector[3] - ovector[2]) == ".")
                    {
                      // ., a class
                      struct adb_filter f;
                      f.type = ADB_FILTER_ATTR_CONTAINS;
                      f.lvalue = "class";
                      f.rvalue = line.substr(ovector[4], ovector[5] - ovector[4]);
                      rule->filters.push_back(f);
                    }

                  line = line.substr(ovector[1] - ovector[0]);
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

                  if(oper == "^=")
                    {
                      // ^= is equivalent to "starts with"
                      struct adb_filter f;
                      f.type = ADB_FILTER_ATTR_STARTS;
                      f.lvalue = lvalue;
                      f.rvalue = rvalue;
                      rule->filters.push_back(f);
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

                          part = rvalue.find_first_of(sep);
                          struct adb_filter f;
                          f.type = ADB_FILTER_ATTR_CONTAINS;
                          f.lvalue = lvalue;
                          f.rvalue = rvalue.substr(0, part);
                          rule->filters.push_back(f);

                          // Check for the next part
                          rvalue = rvalue.substr(part + 1);
                        }
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
                      struct adb_filter f;
                      f.type = ADB_FILTER_ATTR_EXISTS;
                      f.lvalue = line.substr(ovector[2], ovector[3] - ovector[2]);
                      rule->filters.push_back(f);
                      line = line.substr(ovector[1] - ovector[0]);
                    }
                }

              // Skip over pseudo-classes and pseudo-elements, which are of no use to us
              // TODO Descendant and children
              /*re = pcre_compile(rPseudo, PCRE_CASELESS, &error, &erroffset, NULL);
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
              }*/
            }
        }

      if(rule->filters.size() == 0)
        {
          rule->type = ADB_RULE_ERROR;
        }
    }

  // Unsupported rule
  else
    {
      rule->type = ADB_RULE_UNSUPPORTED;
    }
}

/*
 * is_blocked
 * --------------------
 * Parameters :
 * - std::string url    : the checked URL
 * Return value :
 * - bool : true if the URL match something in the blocked list
 */
bool adblock_parser::is_blocked(client_state *csp)
{
  return adblock_parser::is_in_list(csp, &(blockerules));
}

/*
 * is_exception
 * --------------------
 * Parameters :
 * - std::string url    : the checked URL
 * Return value :
 * - bool : true if the URL match something in the exceptions list
 */
bool adblock_parser::is_exception(client_state *csp)
{
  return adblock_parser::is_in_list(csp, &(exceptionsrules));
}

/*
 * is_in_list
 * --------------------
 * Parameters :
 * - client_state *csp : The checked connexion
 * Return value :
 * - bool : true if the URL match something in the list passed as parameter
 */
bool adblock_parser::is_in_list(client_state *csp, std::vector<adr::adb_rule> *list)
{
  std::vector<adr::adb_rule>::iterator it;
  bool ret = false;
  char *r = parsers::get_header_value(&csp->_headers, "Referer:");

  std::string host  = csp->_http._host;
  std::string lhost = csp->_http._host;
  std::string lpath = csp->_http._path;
  std::string murl  = host + lpath;
  std::string lmurl = murl;
  miscutil::to_lower(lhost);
  miscutil::to_lower(lpath);
  miscutil::to_lower(lmurl);

  for(it = list->begin(); it != list->end(); it++)
    {
      std::string url   = (*it).url.c_str();
      std::string lurl  = (*it).url.c_str();
      miscutil::to_lower(lurl);

      if((!(*it).case_sensitive and lmurl.find(lurl) != std::string::npos) or ((*it).case_sensitive and murl.find(url) != std::string::npos))
        {
          ret = true;
          std::vector<adr::adb_condition>::iterator cit;
          for(cit = (*it).conditions.begin(); cit != (*it).conditions.end(); cit++)
            {
              if((*cit).type == ADB_COND_START_WITH and (
                    (!(*it).case_sensitive and lmurl.find(lurl) != 0) or ((*it).case_sensitive and murl.find(url) != 0)))
                {
                  // If the url does not start with the current one
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
                  ret = false;
                }
            }
        }
    }
  return ret;
}

/*
 * get_rules
 * Get rules for a specific URL
 * --------------------
 * Parameters :
 * - client_state *csp                        : The actual request
 * - std::vector<struct adr::adb_rule> *rules : A pointer to an empty vector of rules
 * - bool with_generic                        : If true, add generic rules to the specific ones
 */
void adblock_parser::get_rules(client_state *csp, std::vector<struct adr::adb_rule> *rules, bool with_generic)
{
  if(csp != NULL)
    {
      std::string url = std::string(csp->_http._host) + std::string(csp->_http._path);
      std::multimap<std::string, struct adr::adb_rule>::iterator mit;
      std::vector<struct adr::adb_rule>::iterator it;

      // multimap iterator
      for(mit = filterrules.begin(); mit != filterrules.end(); mit++)
        {
          // If the current URL correspond, we add the rule
          if(url.find((*mit).first) != std::string::npos)
            {
              rules->push_back((*mit).second);
            }
        }

      // Generic rules (or not)
      if(with_generic)
        {
          for(it = genericrules.begin(); it != genericrules.end(); it++)
            {
              rules->push_back(*it);
            }
        }
    }
}
