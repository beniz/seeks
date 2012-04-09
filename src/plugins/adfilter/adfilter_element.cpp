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

#include "adfilter.h"
#include "adfilter_element.h"

#include "miscutil.h"
#include "seeks_proxy.h"
#include "errlog.h"
#include "parsers.h"

#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/xpath.h> 

#include <string>

// Patterns that's trigger the plug'in
const std::string adfilter_element::_blocked_patterns_filename = "adfilter/blocked-patterns";

/*
 * Constructor
 * ----------------
 */
adfilter_element::adfilter_element(const std::vector<std::string> &pos_patterns, const std::vector<std::string> &neg_patterns, adfilter *parent)
  : filter_plugin(pos_patterns, neg_patterns, parent)
{
  errlog::log_error(LOG_LEVEL_INFO, "adfilter: initializing filter plugin");
  this->parent = parent;
}


/*
 * run
 * Main plug'in element routine
 * --------------------
 * Parameters :
 * - client_state *csp : ptr to the client state of the current request
 * - char *str         : page being parsed
 * Return value :
 * - char *str         : page parsed
 */
char* adfilter_element::run(client_state *csp, char *str, size_t size)
{
  std::string ret = strndup(str,size);
  std::string ct = parsers::get_header_value(&csp->_headers, "Content-Type:");

  if(ct.find("text/html") != std::string::npos or ct.find("text/xml") != std::string::npos)
  {
    // It's an XML file (or HTML)
    std::string xpath;
    if(this->parent->get_parser()->get_xpath(std::string(csp->_http._url), xpath, true))
    {
      // There is an XPath for this URL, let's filter it
      this->_filter(&ret, xpath);
    }
  }
  // Finally return the modified (or not) page
  csp->_content_length = (size_t)strlen(ret.c_str());
  return strdup(ret.c_str());
}

/*
 * _filter
 * Remove identified elements by XPath from the given XML tree
 * --------------------
 * Parameters :
 * - std::string *ret  : ptr to the XML tree
 * - std::string xpath : the XPath used to identified filtered elements
 */
void adfilter_element::_filter(std::string *ret, std::string xpath)
{
  // HTML parser context
  htmlParserCtxtPtr htmlCtx = htmlNewParserCtxt();
  if(htmlCtx != NULL)
  {
    // HTML doc parsing from memory (char*)
    htmlDocPtr doc = htmlCtxtReadMemory(htmlCtx, ret->c_str(), ret->length(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
    if(doc != NULL)
    {
      xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
      if(xpathCtx != NULL)
      {
        // If the XML tree has been correctly parsed, let's apply the XPath to it
        xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)(xpath.c_str()), xpathCtx);
        if(xpathObj != NULL)
        {
          // The XPath select at least one element
          xmlNodeSetPtr nodes = xpathObj->nodesetval;
  
          // Unlink all found XML element nodes
          int size, i;
          size = (nodes) ? nodes->nodeNr : 0;
          for(i = 0; i < size; ++i)
          {
            if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE)
            {
              xmlUnlinkNode(nodes->nodeTab[i]);
            }
          }
          // Free some memory
          xmlXPathFreeObject(xpathObj);
        }
        // Free some memory
        xmlXPathFreeContext(xpathCtx);
      }
      // Dump the XML tree into the char* passed as argument to this function
      xmlChar *html;
      htmlDocDumpMemory(xpathCtx->doc, &html, NULL);
      // Casting and converting from xmlChar* to std::string
      *ret =  std::string((char *)html);
      // Free some memory
      xmlFreeDoc(doc);
    }
    // Free some memory
    htmlFreeParserCtxt(htmlCtx);
  }
}
