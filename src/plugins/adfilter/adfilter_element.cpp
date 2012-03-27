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
  xmlXPathContextPtr xpathCtx;
  xmlXPathObjectPtr xpathObj;
  
  // Parse the string as an XML tree
  xmlChar *html = xmlStrdup((xmlChar*)(ret->c_str()));
  htmlDocPtr doc = htmlParseDoc(html, NULL);

  if(doc != NULL)
  {
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx != NULL)
    {
      // If the XML tree has been correctly parsed, let's apply the XPath to it
      xpathObj = xmlXPathEvalExpression((xmlChar*)(xpath.c_str()), xpathCtx);
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
        // XXX Freeing the nodes cause a segfault...maybe they are freed with the Object
        // xmlXPathFreeNodeSet(nodes);
      }
    }
    
    // Dump the XML tree into the char* passed as argument to this function
    htmlDocDumpMemory(xpathCtx->doc, &html, NULL);
    *ret =  std::string((char *)html);
    // Free some memory
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    return;
  }
}
