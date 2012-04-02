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

#ifndef ADFILTER_CONFIGURATION_H
#define ADFILTER_CONFIGURATION_H

#include "configuration_spec.h"

#include <vector>

using namespace sp;
using sp::configuration_spec;

class adfilter_configuration : public configuration_spec
{
  public:
    adfilter_configuration(const std::string &filename);
    ~adfilter_configuration();

    // virtual
    virtual void set_default_config();
    virtual void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg, char *buf, const unsigned long &linenum);
    virtual void finalize_configuration();

    // options
    std::vector<std::string> _adblock_lists; // List of adblock rules URL
    int _update_frequency;                 // List update frequecy in seconds
    //   features activation
    bool _use_filter;                      // If true, use page filtering
    bool _use_blocker;                     // If true, use URL blocking
};
#endif
