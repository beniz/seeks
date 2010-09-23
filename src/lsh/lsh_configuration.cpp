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
#include "seeks_proxy.h" // for mutexes.
#include "mrf.h"

#include <iostream>

using sp::errlog;
using sp::seeks_proxy;

namespace lsh
{
   #define hash_en_swl         3069493191ul  /* "en-stopword-list" */
   #define hash_fr_swl         3229696102ul  /* "fr-stopword-list" */
   #define hash_de_swl         1184593985ul  /* "de-stopword-list" */
   #define hash_it_swl         1922004664ul  /* "it-stopword-list" */
   #define hash_es_swl          696183464ul  /* "es-stopword-list" */
   #define hash_pt_swl         2436772936ul  /* "pt-stopword-list" */
   #define hash_fi_swl          471128499ul  /* "fi-stopword-list" */
   #define hash_sv_swl         1176262111ul  /* "sv-stopword-list" */
   #define hash_ar_swl         3009450614ul  /* "ar-stopword-list" */
   #define hash_ru_swl         2303303740ul  /* "ru-stopword-list" */
   #define hash_hu_swl         3972852814ul  /* "hu-stopword-list" */
   #define hash_bg_swl         3065121129ul  /* "bg-stopword-list" */
   #define hash_ro_swl         2960570529ul  /* "ro-stopword-list" */
   #define hash_cs_swl         3332125561ul  /* "cs-stopword-list" */
   #define hash_pl_swl         4105258461ul  /* "pl-stopword-list" */
   #define hash_lsh_delims     3228991366ul  /* "lsh-delims" */
   
   lsh_configuration* lsh_configuration::_config = NULL;
   
   lsh_configuration::lsh_configuration(const std::string &filename)
     :configuration_spec(filename)
       {
	  lsh_configuration::_config = this;
	  mutex_init(&_load_swl_mutex);
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
	
	_lsh_delims = mrf::_default_delims;
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
	     
	   case hash_de_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("de"),swl));
	     break;
	     
	   case hash_it_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("it"),swl));
	     break;
	     
	   case hash_es_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("es"),swl));
	     break;
	     
	   case hash_pt_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("pt"),swl));
	     break;
	     
	   case hash_fi_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("fi"),swl));
	     break;
	     
	   case hash_sv_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("sv"),swl));
	     break;
	     
	   case hash_ar_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("ar"),swl));
	     break;
	     
	   case hash_ru_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("ru"),swl));
	     break;
	     
	   case hash_hu_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("hu"),swl));
	     break;
	     
	   case hash_bg_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("bg"),swl));
	     break;
	     
	   case hash_ro_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("ro"),swl));
	     break;
	     
	   case hash_cs_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("cs"),swl));
	     break;
	     
	   case hash_pl_swl:
	     filename = std::string(arg);
	     swl = new stopwordlist(filename);
	     _swlists.insert(std::pair<const char*,stopwordlist*>(strdup("pl"),swl));
	     break;
	     
	   case hash_lsh_delims:
	     //TODO: beware, do not use, broken, use the default string instead.
	     _lsh_delims = std::string(arg);
	     break;
	     
	   default:
	     break;
	  }
     }
   
   void lsh_configuration::finalize_configuration()
     {
     }
   
   stopwordlist* lsh_configuration::get_wordlist(const std::string &lang)
     {
	mutex_lock(&_load_swl_mutex);
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
	     mutex_unlock(&_load_swl_mutex);
	     return (*hit).second;
	  }
	else 
	  {
	     mutex_unlock(&_load_swl_mutex);
	     return NULL;
	  }
     }
         
} /* end of namespace. */
