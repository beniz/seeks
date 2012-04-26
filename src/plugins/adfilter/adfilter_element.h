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

#ifndef ADFILTER_ELEMENT_H
#define ADFILTER_ELEMENT_H

#define NO_PERL // we do not use Perl.

#include "adfilter.h"

#include "plugin.h"
#include "filter_plugin.h"

#include <libxml/HTMLparser.h> // For htmlNodePtr

using namespace sp;
using namespace seeks_plugins;

class adfilter_element : public filter_plugin
{
  public:
    adfilter_element(const std::vector<std::string> &pos_patterns, const std::vector<std::string> &neg_patterns, adfilter *parent);
    ~adfilter_element();
    char* run(client_state *csp, char *str, size_t size);
  private:
    // Attributes
    adfilter* parent;                                    // Parent (sp::plugin)
    static const std::string _blocked_patterns_filename; // blocked patterns filename = "blocked-patterns"
    // Methods
    static void _filter(char **ret, std::vector<struct adr::adb_rule> *rules);                 // Filter page
    static xmlNodePtr _filter_node(xmlNodePtr node, std::vector<struct adr::adb_rule> *rules); // Filter next node
    static bool _filter_node_apply(xmlNodePtr node, std::vector<struct adr::adb_rule> *rules);                                                            // Apply filter
    static void _nullGenericErrorFunc(void *ctxt, const char *msg, ...); // Empty error handler for libXML2
};

#endif
