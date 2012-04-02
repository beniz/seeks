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
#include "configuration_spec.h"
#include "adfilter_configuration.h"

#include <vector>

#define hash_adblock_list     1446486468ul /* "adblock-list" */
#define hash_update_frequency 3630788420ul /* "update-frequency" */
#define hash_use_filter       1173899210ul /* "use-filter" */
#define hash_use_blocker      2154520601ul /* "use-blocker" */

/*
 * adfilter_configuration
 * Constructor
 * --------------------
 * Parameters :
 * - &filename : path to the config file
 */
adfilter_configuration::adfilter_configuration(const std::string &filename) : configuration_spec(filename)
{
  // Load config file
  load_config();
}

/*
 * ~adfilter_configuration
 * Destructor
 * --------------------
 */
adfilter_configuration::~adfilter_configuration()
{
}

/*
 * set_default_config
 * Defines the default config values (see configuration_spec.h)
 * --------------------
 */
void adfilter_configuration::set_default_config()
{
  this->_update_frequency = 86400 * 7;
  this->_use_filter = true;
  this->_use_blocker = false;
}

/*
 * handle_config_cmd
 * Virtual function called on each parsed config lines (see configuration_spec.h)
 * --------------------
 */
void adfilter_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg, char *buf, const unsigned long &linenum)
{
  switch(cmd_hash)
  {
    case hash_adblock_list:
      this->_adblock_lists.push_back(std::string(arg));
      configuration_spec::html_table_row(_config_args,cmd, arg, "ADBlock list URL");
      break;
    case hash_update_frequency:
      this->_update_frequency = atoi(arg);
      configuration_spec::html_table_row(_config_args,cmd, arg, "ADBlock list update frequency (in seconds)");
      break;
    case hash_use_filter:
      this->_use_filter = (strcmp(arg, "1") == 0);
      configuration_spec::html_table_row(_config_args,cmd, arg, "Use filter feature ?");
      break;
    case hash_use_blocker:
      this->_use_blocker = (strcmp(arg, "1") == 0);
      configuration_spec::html_table_row(_config_args,cmd, arg, "Use blocker feature ?");
      break;
  }
}

/*
 * finalize_configuration
 * Virtual function called after config file parsing (see configuration_spec.h)
 * --------------------
 */
void adfilter_configuration::finalize_configuration()
{
}
