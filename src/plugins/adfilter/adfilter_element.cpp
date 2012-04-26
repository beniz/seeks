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
#include "adblock_parser.h"

#include "miscutil.h"
#include "seeks_proxy.h"
#include "errlog.h"
#include "parsers.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/HTMLtree.h>
//#include <libxml/xmlsave.h>
#include <libxml/threads.h>

#include <string>
#include <malloc.h>

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
  // libXML2 error handler suppression
  xmlThrDefSetGenericErrorFunc(NULL, adfilter_element::_nullGenericErrorFunc);
  xmlSetGenericErrorFunc(NULL, adfilter_element::_nullGenericErrorFunc);
}

/*
 * Generic Error handler for libXML2
 * -----------------
 * Does nothing
 */
void adfilter_element::_nullGenericErrorFunc(void *ctxt, const char *msg, ...)
{
  // Nothing
}

/*
 * Destructor
 * ----------------
 */
adfilter_element::~adfilter_element()
{
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
  char *ret = strndup(str, size);

  std::string ct = parsers::get_header_value(&csp->_headers, "Content-Type:");
  if(!this->parent->get_parser()->is_exception(csp) and (ct.find("text/html") != std::string::npos or ct.find("text/xml") != std::string::npos))
  {
    // It's an XML file (or HTML)
    // Apply generic and specific rules to the XML tree
    std::vector<struct adr::adb_rule> *rules = new std::vector<struct adr::adb_rule>;
    this->parent->get_parser()->get_rules(csp, rules, true);
    // XML seems to have some problems with concurrent parsers, so let's use mutex
    xmlMutexLock(this->parent->mutexTok);
    this->_filter(&ret, rules);
    xmlMutexUnlock(this->parent->mutexTok);
  }
  // Finally return the modified (or not) page
  csp->_content_length = (size_t)strlen(ret);
  return ret;
}

/*
 * _filter
 * Apply rules sets to the given XML tree
 * --------------------
 * Parameters :
 * - char                              **orig          : ptr to the XML tree as char*
 * - std::vector<struct adr::adb_rule> *specific_rules : the rules used to identified filtered elements for this specific URL
 * - std::vector<struct adr::adb_rule> *generic_rules  : the rules used to identified filtered elements
 */
void adfilter_element::_filter(char **orig, std::vector<struct adr::adb_rule> *rules)
{
  xmlDocPtr doc;
  if((doc = xmlParseMemory(*orig, strlen(*orig)))!= NULL)
  {
    // If the document has been correctlty parsed, let's begin by filter the root node (<html></html>) or not.
    adfilter_element::_filter_node(xmlDocGetRootElement(doc), rules);

    // All the node are filtered or not, time to dump the resulting doc into *ret
    xmlChar *mem;
    int s;
    // FIXME Memory leak here ? Or maybe libxml2 handle memory in a weird way
    htmlDocDumpMemory(doc, &mem, &s);
    *orig = strdup((char *)mem);

    // Let's clear some memory
    xmlFree(mem);
    mem = NULL;
    xmlFreeDoc(doc);
    doc = NULL;
  }
}

/*
 * _filter_node
 * Remove identified elements by XPath from the given node
 * --------------------
 * Parameters :
 * - char                              **ret           : ptr to the XML tree as char*
 * - std::vector<struct adr::adb_rule> *specific_rules : the rules used to identified filtered elements for this specific URL
 * - std::vector<struct adr::adb_rule> *generic_rules  : the rules used to identified filtered elements
 */
xmlNodePtr adfilter_element::_filter_node(xmlNodePtr node, std::vector<struct adr::adb_rule> *rules)
{
  // Recursive function until node is NULL
  if(node != NULL)
  {
    if(node->type == XML_ELEMENT_NODE)
    {
      // If we have an element (not a text, not an attribute)
      if(adfilter_element::_filter_node_apply(node, rules))
      {
        // If this node correspond to a specific or a generic rule, lets remove it from the XML tree
        xmlUnlinkNode(node);
        xmlFreeNode(node);
        node = NULL;
        return NULL;
      }
    }
    // Let's go to the next sibling (horizontal crawling) then child (vertical)
    adfilter_element::_filter_node(xmlNextElementSibling(node), rules);
    adfilter_element::_filter_node(xmlFirstElementChild(node), rules);
  }
  return NULL;
}

/*
 * _filter_node_apply
 * Apply rules set to the given node
 * --------------------
 * Parameters :
 * - char                              **ret  : ptr to the XML tree as char*
 * - std::vector<struct adr::adb_rule> *rules : the rules used to identified filtered elements
 */
bool adfilter_element::_filter_node_apply(xmlNodePtr node, std::vector<struct adr::adb_rule> *rules)
{
  // First iterate all rules
  std::vector<struct adr::adb_rule>::iterator it;
  for(it = rules->begin(); node != NULL and it != rules->end(); it++)
  {
    // For now, let's assume we have to remove the node
    bool unlink = true;
    // Then iterate all filters for the current rule
    std::vector<struct adr::adb_filter>::iterator fit;
    for(fit = it->filters.begin(); fit != it->filters.end(); fit++)
    {
      // Check if the node correspond to all filters for the current rule
      // If just one of them does not correspond, we let the node as is
      if(fit->type == adr::ADB_FILTER_ATTR_EQUALS)
      {
        xmlAttrPtr attr = xmlHasProp(node, (xmlChar *)((fit->lvalue).c_str()));
        if(attr == NULL or xmlStrcasecmp(attr->children->content, (xmlChar *)(fit->rvalue).c_str()) != 0) unlink = false;
      }
      else if(fit->type == adr::ADB_FILTER_ATTR_STARTS)
      {
        xmlAttrPtr attr = xmlHasProp(node, (xmlChar *)((fit->lvalue).c_str()));
        if(attr == NULL or xmlStrcasestr(attr->children->content, (xmlChar *)(fit->rvalue).c_str()) != attr->children->content) unlink = false;
      }
      else if(fit->type == adr::ADB_FILTER_ATTR_CONTAINS)
      {
        xmlAttrPtr attr = xmlHasProp(node, (xmlChar *)((fit->lvalue).c_str()));
        if(attr == NULL or xmlStrcasestr(attr->children->content, (xmlChar *)(fit->rvalue).c_str()) == NULL) unlink = false;
      }
      else if(fit->type == adr::ADB_FILTER_ATTR_EXISTS)
      {
        xmlAttrPtr attr = xmlHasProp(node, (xmlChar *)((fit->lvalue).c_str()));
        if(attr == NULL) unlink = false;
      }
      else if(fit->type == adr::ADB_FILTER_ELEMENT_IS)
      {
        if(node->name == NULL or xmlStrcasecmp((const xmlChar *)(node->name), (xmlChar *)(fit->rvalue).c_str()) != 0) unlink = false;
      }
    }
    // All filters for the current rule are satisfied, no need to check the others rules
    if(unlink) return true;
  }
  // No rules correspond, we do not touch this node
  return false;
}
