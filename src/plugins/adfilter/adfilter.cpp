#include "adfilter.h"

#include "miscutil.h"
#include "encode.h"
#include "cgi.h"
#include "parsers.h"
#include "seeks_proxy.h"
#include "errlog.h"

#include <pcrecpp.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/xpath.h> 

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

namespace seeks_plugins
{
  /*- adfilter. -*/
  adfilter::adfilter()
    : plugin()
  {
    _name = "adfilter";

    _version_major = "0";
    _version_minor = "1";

    _config_filename = "adfilter-config";
    _configuration = NULL;

    _filter_plugin = new adfilter_element(this);
  }

  const std::string adfilter_element::_blocked_patterns_filename = "adfilter/blocked-patterns";
  const std::string adfilter_element::_adblock_list_filename = "adfilter/adblock_list";
  const std::string adfilter_element::_blocked_html = "<!-- blocked by seeks proxy //-->";
  const std::string adfilter_element::_blocked_js = "// blocked by seeks proxy";
  std::map< const std::string, std::string> adfilter_element::_rulesmap;

  adfilter_element::adfilter_element(plugin *parent)
    : filter_plugin((seeks_proxy::_datadir.empty() ? std::string(plugin_manager::_plugin_repository
                           + adfilter_element::_blocked_patterns_filename).c_str()
                           : std::string(seeks_proxy::_datadir + "/plugins/" + adfilter_element::_blocked_patterns_filename).c_str()),
                           parent)
  {
    // Load adblock file
    const char* rules_filename= (seeks_proxy::_datadir.empty() ? 
                                 std::string(plugin_manager::_plugin_repository + adfilter_element::_adblock_list_filename).c_str() : 
                                 std::string(seeks_proxy::_datadir + "/plugins/" + adfilter_element::_adblock_list_filename).c_str());

    std::ifstream rules;
    rules.open(rules_filename);
    if (rules.is_open())
    {
      std::string line;
      while (!rules.eof()) {
        std::string x;
        std::string url;

        getline(rules, line);
        line.erase(line.find_last_not_of(" \n\r\t")+1);

        url = adfilter_element::_line_to_xpath(&x, line);

        if(!url.empty() and !x.empty() and x != "|ERROR|")
        {
          std::map<std::string, std::string>::iterator it;
          std::string l;

          // First search if there is an entry for this url
          it = adfilter_element::_rulesmap.find(url);
          if(it !=  adfilter_element::_rulesmap.end()) {
            // Found !
            l = (*it).second;
            adfilter_element::_rulesmap.erase(it);
          }
          l.append((l.empty() ? "" : " | ") + x);
          // Push xpath to the map
          adfilter_element::_rulesmap.insert(std::pair<const std::string, std::string>(url, l));
        }
      }
    } else {
      errlog::log_error(LOG_LEVEL_ERROR, "Can't open adblock rules file '%s':  %E", rules_filename);
    }
    rules.close();
  }

  std::string adfilter_element::_line_to_xpath(std::string *xpath, std::string line)
  {
    std::string url;

    // If the line begins with "||", the complete following URL should be blocked
    if(line.substr(0,2) == "||")
    {
      // full URL blocking
      (*xpath) = "|BLOCK|";
      url = line.substr(2);
    }

    else if(line.substr(0,2) != "@@" and line.substr(0,1) != "!")
    {
      // Matching regexps (see http://forums.wincustomize.com/322441)
      pcrecpp::RE rUrl("^([^#]*)##", pcrecpp::RE_Options(PCRE_CASELESS));
      pcrecpp::RE rElement("^(([#.]?)([a-z0-9\\*_-]*)((\\|)([a-z0-9\\*_-]*))?)", pcrecpp::RE_Options(PCRE_CASELESS));
      pcrecpp::RE rAttr1("^(\\[([^\\]]*)\\])", pcrecpp::RE_Options(PCRE_CASELESS));
      pcrecpp::RE rAttr2("^(\\[\\s*([^\\^\\*~=\\s]+)\\s*([~\\^\\*]?=)\\s*\"([^\"]+)\"\\s*\\])", pcrecpp::RE_Options(PCRE_CASELESS));
      pcrecpp::RE rPseudo("^(:([a-z_-]+)(\\(([n0-9]+)\\))?)", pcrecpp::RE_Options(PCRE_CASELESS));
      pcrecpp::RE rCombinator("^(\\s*[>+\\s])?", pcrecpp::RE_Options(PCRE_CASELESS));
      pcrecpp::RE rComma("^(\\s*,)", pcrecpp::RE_Options(PCRE_CASELESS));

      std::string parts;

      parts = "//|ELEM|";

      std::string lastRule = "";

      // Matched elements
      std::string all;
      std::string a;
      std::string b;
      std::string c;
      std::string d;
      std::string e;

      if(rUrl.PartialMatch(line, &a))
      {
        if(!a.empty())
        {
          url = a;
        } else {
          url = "|GENERIC|";
        }
        line = line.substr(a.length() + 2);

        while(line.length() > 0 and line != lastRule)
        {
          lastRule = line;
  
          pcrecpp::RE("^\\s*|\\s*$").Replace("", &line);
          if(line.length() == 0)
          {
            break;
          }
  
          if(rElement.PartialMatch(line, &all, &a, &b, &c, &d, &e))
          {
            if(a.empty())
            {
              if(!e.empty())
              {
                std::transform(e.begin(), e.end(), e.begin(), ::tolower);
                if(parts.rfind("|ELEM|") != std::string::npos)
                {
                  parts.replace(parts.rfind("|ELEM|"), 6, e);
                }
              }
              else
              {
                std::transform(b.begin(), b.end(), b.begin(), ::tolower);
                if(parts.rfind("|ELEM|") != std::string::npos)
                {
                  parts.replace(parts.rfind("|ELEM|"), 6, b);
                }
              }
            }
            else if(a == "#")
            {
              parts.append("[@id='" + b + "']");
            }
            else if(a == ".")
            {
              parts.append("[contains(@class, '" + b + "')]");
            }
  
            line = line.substr(all.length());
          }
  
          // Match attribute selectors
          if(rAttr2.PartialMatch(line, &all, &a, &b, &c))
          {
            pcrecpp::RE("'").GlobalReplace("\\'", &c);
            if(b == "~=" or b == "*=")
            {
              parts.append("[contains(@" + a + ", '" + c + "')]");
            }
            else if(b == "^=")
            {
              parts.append("[starts-with(@" + a + ", '" + c + "')]");
            }
            else
            {
              // FIXME XPath error : Invalid expression //[@id='Meebo']
              parts.append("[@" + a + "='" + c + "']");
            }
            line = line.substr(all.length());
          }
          else
          {
            if(rAttr1.PartialMatch(line, &all, &a))
            {
              // FIXME XPath error : Invalid expression //[@id='Meebo']
              parts.append("[@" + a + "]");
              line = line.substr(all.length());
            }
          }
  
          // Skip over pseudo-classes and pseudo-elements, which are of no use to us
          while(rPseudo.PartialMatch(line, &all, &a, &b, &c))
          {
            if(a == "nth-child")
            {
              if(c == "n")
              {
                if(parts.rfind("|ELEM|") != std::string::npos)
                {
                  parts.replace(parts.rfind("|ELEM|"), 6, "descendant::|ELEM|");
                } else {
                  parts.append("descendant::*");
                }
              }
            }
            line = line.substr(all.length());
          }
          
          // Match combinators
          if(rCombinator.PartialMatch(line, &a) and a.length() > 0)
          {
            if(a.find(">") != std::string::npos)
            {
              parts.append("/");
            }
            else if(a.find("+") != std::string::npos)
            {
              parts.append("/following-sibling::");
            }
            else
            {
              parts.append("//");
            }
            parts.append("|ELEM|");
            line = line.substr(a.length());
          }
          
          if(rComma.PartialMatch(line, &a))
          {
            parts.append(" | //|ELEM|");
            line = line.substr(a.length());
          }
        }

        pcrecpp::RE("\\|ELEM\\|").GlobalReplace("*", &parts);

        if(parts == "//*")
        {
          parts = "|ERROR|";
        }
        (*xpath) = parts;
      }
    }

    return url;
  }

  char* adfilter_element::run(client_state *csp, char *str)
  {
    std::string ret = strdup(str);
    std::map<std::string, std::string>::iterator it;

    std::cerr << "ADFILTER START RUNNING" << std::endl;

    for(it = adfilter_element::_rulesmap.begin(); it != adfilter_element::_rulesmap.end(); it++)
    {
      std::string murl = (*it).first;
      std::string xpath = (*it).second;
      std::string url = csp->_http._url;
      // If the URL is known or it's generic rules
      if(url.find(murl) != std::string::npos or murl == "|GENERIC|")
      {

        // Whole site is blocked, set the response to an HTML or JS comment depending on the content_type
        if(xpath == "|BLOCK|") {
          if(csp->_content_type & CT_XML)
          {
            ret = strdup(adfilter_element::_blocked_html.c_str());
          }
          else if(csp->_content_type & CT_TEXT)
          {
            ret = strdup(adfilter_element::_blocked_js.c_str());
          }
          std::cerr << "ADFILTER HAS BLOCKED SOMETHING" << std::endl;
          break;
        }

        // Parse HTML files with libxml2
        if(csp->_content_type & CT_XML)
        {
          adfilter_element::_filter(&ret, xpath);
        }
      }
    }

    std::cerr << "ADFILTER STOP RUNNING" << std::endl;

    csp->_content_length = (size_t)strlen(ret.c_str());
    return strdup((ret.c_str()));
  }

  void adfilter_element::_filter(std::string *ret, std::string xpath)
  {
    // text and xml files can be parsed by libxml2
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    xmlChar *html = xmlStrdup((xmlChar*)(ret->c_str()));
    htmlDocPtr doc = htmlParseDoc(html, NULL);

    if(doc != NULL)
    {
      xpathCtx = xmlXPathNewContext(doc);

      if(xpathCtx != NULL)
      {
        xpathObj = xmlXPathEvalExpression((xmlChar*)(xpath.c_str()), xpathCtx);
        if(xpathObj != NULL)
        {
          xmlNodeSetPtr nodes = xpathObj->nodesetval;
          int size;
          int i;
          size = (nodes) ? nodes->nodeNr : 0;

          // Unlink all found XML element nodes
          for(i = 0; i < size; ++i)
          {
            if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE)
            {
              xmlUnlinkNode(nodes->nodeTab[i]);
            }
          }
          xmlXPathFreeObject(xpathObj);
          // XXX Freeing the nodes cause a segfault...maybe they are freed with the Object
          // xmlXPathFreeNodeSet(nodes);
        }
      }
      std::cerr << "ADFILTER HAS FOUND SOMETHING" << std::endl;
      int size;
      htmlDocDumpMemory(xpathCtx->doc, &html, &size);
      xmlXPathFreeContext(xpathCtx);
      xmlFreeDoc(doc);
      *ret =  std::string((char *)html);
      return;
    }
  }

  /* plugin registration */
  extern "C"
  {
    plugin* maker()
    {
      return new adfilter;
    }
  }

} /* end of namespace. */
