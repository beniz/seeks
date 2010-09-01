/**     
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "stopwordlist.h"
#include "errlog.h"
#include "seeks_proxy.h" // for basedir.
#include "miscutil.h" // for strndup

#include <iostream>
#include <fstream>
#include <strings.h>

using sp::errlog;
using sp::seeks_proxy;

namespace lsh
{
   stopwordlist::stopwordlist(const std::string &filename)
     :_swlistfile(filename),_loaded(false)
       {
       }
   
   stopwordlist::~stopwordlist()
     {
	hash_map<const char*,bool,hash<const char*>,eqstr>::iterator hit
	  = _swlist.begin();
	while(hit!=_swlist.end())
	  {
	     delete (*hit).first;
	     ++hit;
	  }
     }
   
   int stopwordlist::load_list(const std::string &filename)
     {
	std::string fullfname = (seeks_proxy::_basedir) ? std::string(seeks_proxy::_basedir) + "/lsh/swl/" + filename
	  : seeks_proxy::_datadir + "/lsh/swl/" + filename;
		
	std::ifstream infile;
	infile.open(fullfname.c_str(),std::ifstream::in);
	if (infile.fail())
	  return 1;
	
	while (infile.good())
	  {
	     char word[256];
	     infile.getline(word,256);
	     if (strlen(word) > 0)
	       _swlist.insert(std::pair<const char*,bool>(strndup(word,strlen(word)-1),true));
	  }
	
	errlog::log_error(LOG_LEVEL_INFO,"Loaded stop word list %s, %d words",fullfname.c_str(),
			  _swlist.size());
	
	infile.close();
	
	_loaded = true;
	
	return 0;
     }
   
   bool stopwordlist::has_word(const std::string &w) const
     {
	hash_map<const char*,bool,hash<const char*>,eqstr>::const_iterator hit;
	if ((hit=_swlist.find(w.c_str()))!=_swlist.end())
	  return true;
	else return false;
     }
      
} /* end of namespace. */
