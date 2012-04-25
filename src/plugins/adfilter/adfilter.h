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

#ifndef ADFILTER_H
#define ADFILTER_H

#define NO_PERL // we do not use Perl.

#include "plugin.h"
#include "filter_plugin.h"

#include "adblock_parser.h"
#include "adfilter_configuration.h"
#include "configuration_spec.h"

#include <map>
#include <libxml/threads.h>

using namespace sp;

namespace seeks_plugins
{
  class adfilter : public plugin
  {
    public:
      adfilter();
      ~adfilter();
      virtual void start() {};
      virtual void stop() {};
      // Accessors/mutators
      adblock_parser* get_parser();
      adfilter_configuration* get_config();
      // Methods
      void blocked_response(http_response *rsp, client_state *csp); // Set response regarding the csp content-type
      // Attributes
      xmlMutexPtr mutexTok;
    private:
      // Attributes
      adfilter_configuration* _adconfig; // Configuration manager
      adblock_parser* _adbparser; // ADBlock rules parser
      std::map<std::string, const char *> _responses; // Responses per content-type
      // Methods
      void populate_responses(); // Populate _responses
  };

} /* end of namespace. */

#endif
