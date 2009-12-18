/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */
 
#ifndef CONFIGURATION_SPEC_H
#define CONFIGURATION_SPEC_H

#include "proxy_dts.h"

#include <stdint.h>

namespace sp
{
   class configuration_spec
     {
      public:
	configuration_spec(const std::string &filename);
	
	virtual ~configuration_spec();
	
	// class methods.
	sp_err load_config();
	
	sp_err parse_config_line(char *cmd, char* arg, char *tmp, char* buf);
	
	int check_file_changed();

#ifdef unix
	static int init_file_notification();
	
	static int stop_file_notification();

	int watch_file();
	
	void unwatch_file();
#endif
	// virtual functions.
	virtual void set_default_config() {};

	virtual void handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
				       char *buf, const unsigned long &linenum) {};
	
	virtual void finalize_configuration() {};

	// static functions.
	static void html_table_row(char *&args, char *option, char *value, 
				   const char *description);
	
	// variables.
      public:
	std::string _filename; // config filename.
	
	/**
	 * File last-modified time, so we can check if file has been changed.
	 */
	time_t _lastmodified;

#ifdef unix  // linux only, inotify file descriptor.
	static int _fd;  // system wide descriptor.
	int _wd; // local file descriptor.
	bool _has_changed; // whether inotify has detected a change.
#endif
	
	/**
	 * All options from the config file, HTML-formatted.
	 */
	char *_config_args;
	
	/**
	 * Table of changed values, for configuration reload.
	 */
	hash_map<const char*, bool, hash<const char*>, eqstr> _cchanges;
	
      protected:
	
     };
   
} /* end of namespace. */

#endif
