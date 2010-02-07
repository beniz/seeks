/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2009, 2010 Emmanuel Benazera, juban@free.fr
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

#include "lsh_configuration.h"
#include "errlog.h"

#include <iostream>

using sp::errlog;

namespace lsh
{
   #define hash_en_swl         3069493191ul  /* "en-stopword-list" */
   #define hash_fr_swl         3229696102ul  /* "fr-stopword-list" */
   
   lsh_configuration::lsh_configuration(const std::string &filename)
     :configuration_spec(filename)
       {
	  load_config();
       }
   
   lsh_configuration::~lsh_configuration()
     {
     }
   
   void lsh_configuration::set_default_config()
     {
	// empty list of stop words by default.
	hash_map<const char*,stopwordlist*,hash<const char*>,eqstr>::iterator hit
	  = _swlists.begin();
	while(hit!=_swlists.end())
	  {
	     delete (*hit).first;
	     delete (*hit).second;
	     ++hit;
	  }
	_swlists.clear();
     }
      
   void lsh_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
					     char *buf, const unsigned long &linenum)
     {
	stopwordlist *swl = NULL;
	std::string filename;
	
	switch(cmd_hash)
	  {
	   case hash_en_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("en"),swl));
	     break;
	   
	   case hash_fr_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("fr"),swl));
	     break;
	     
	   default:
	     break;
	  }
     }
   
   void lsh_configuration::finalize_configuration()
     {
     }
   
   stopwordlist* lsh_configuration::get_wordlist(const std::string &lang) const
     {
	hash_map<const char*,stopwordlist*,hash<const char*>,eqstr>::const_iterator hit;
	if ((hit=_swlists.find(lang.c_str()))!=_swlists.end())
	  {
	     /* if not already loaded, load the stopwordlist. */
	     if (!(*hit).second->_loaded)
	       {
		  int err = (*hit).second->load_list((*hit).second->_swlistfile.c_str());
		  if (err != 0)
		    errlog::log_error(LOG_LEVEL_ERROR,"Failed loading stopword file %s",
				      (*hit).second->_swlistfile.c_str());
	       }
	     return (*hit).second;
	  }
	else return NULL;
     }
         
} /* end of namespace. */
