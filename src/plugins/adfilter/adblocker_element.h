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

#ifndef ADBLOCKER_ELEMENT_H
#define ADBLOCKER_ELEMENT_H

#define NO_PERL // we do not use Perl.

#include "adfilter.h"

#include "plugin.h"
#include "interceptor_plugin.h"
#include "cgi.h"

using namespace sp;
using namespace seeks_plugins;

class adblocker_element : public interceptor_plugin
{
  public:
    adblocker_element(const std::vector<std::string> &pos_patterns, const std::vector<std::string> &neg_patterns, adfilter *parent);
    ~adblocker_element() {};
    virtual void start() {};
    virtual void stop() {};
    http_response* plugin_response(client_state *csp);   // Main routine
  private:
    // Attributes
    adfilter* parent;                                    // Parent (sp::plugin)
    // Methods
    http_response* _block(client_state *csp);            // Block a request
};

#endif

