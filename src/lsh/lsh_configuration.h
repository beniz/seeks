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

#ifndef LSH_CONFIGURATION_H
#define LSH_CONFIGURATION_H

#include "configuration_spec.h"
#include "stopwordlist.h"
#include "stl_hash.h"

extern "C"
{
#include <pthread.h>
}

using sp::configuration_spec;

typedef pthread_mutex_t sp_mutex_t;

namespace lsh
{
   class lsh_configuration : public configuration_spec
     {
      public:
	lsh_configuration(const std::string &filename);
	
	~lsh_configuration();
	
	// virtual.
	virtual void set_default_config();
	
	virtual void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
				       char *buf, const unsigned long &linenum);
	
	virtual void finalize_configuration();
	
	// local.
	stopwordlist* get_wordlist(const std::string &lang);
	
	// main options.
	hash_map<const char*,stopwordlist*,hash<const char*>,eqstr> _swlists; /**< list of stop word, indexed by 2-char language indicator. */

	std::string _lsh_delims; /**< default delimiters for tokenization in mrf. */
	
	// mutex for loading stop word list.
	sp_mutex_t _load_swl_mutex;
     
	static lsh_configuration *_config;
     };
      
} /* end of namespace. */

#endif
