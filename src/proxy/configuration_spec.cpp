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

#include "configuration_spec.h"
#include "mem_utils.h"
#include "errlog.h"
#include "loaders.h"
#include "miscutil.h"

#include <sys/stat.h>
#include <iostream>

/*
 * Fix a problem with Solaris.  There should be no effect on other
 * platforms.
 * Solaris's isspace() is a macro which uses it's argument directly
 * as an array index.  Therefore we need to make sure that high-bit
 * characters generate +ve values, and ideally we also want to make
 * the argument match the declared parameter type of "int".
 */
#define ijb_isupper(__X) isupper((int)(unsigned char)(__X))
#define ijb_tolower(__X) tolower((int)(unsigned char)(__X))

namespace sp
{
   configuration_spec::configuration_spec(const std::string &filename)
     :_filename(filename),_lastmodified(0)
     {
	_config_args = strdup("");
     }
   
   configuration_spec::~configuration_spec()
     {
	freez(_config_args);
     }
   
   sp_err configuration_spec::load_config()
     {
	int fchanged = check_file_changed();
	if (fchanged == -1)
	  {
	     set_default_config(); // sets default config before reporting the failure.
	     return SP_ERR_FILE;
	  }
	else if (fchanged == 0)
	  {
	     return SP_ERR_OK;
	  }
	else if (fchanged == 1)
	  {
	     errlog::log_error(LOG_LEVEL_INFO,"Reloading configuration file '%s'",_filename.c_str());
	  }
	
	// free html buffer.
	freez(_config_args);
	_config_args = strdup("");
	
	// set to default config, virtual call.
	set_default_config();
	
	FILE *configfp = fopen(_filename.c_str(),"r");
	if (NULL == configfp)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,
			       "can't open configuration file '%s':  %E", _filename.c_str());
	     return SP_ERR_OK;
	  }
	
	char buf[BUFFER_SIZE]; // BUFFER_SIZE in proxy_dts.h.
	unsigned long linenum = 0;
	while(loaders::read_config_line(buf,sizeof(buf),configfp,&linenum) != NULL)
	  {
	     char cmd[BUFFER_SIZE];
	     char arg[BUFFER_SIZE];
	     char tmp[BUFFER_SIZE];
	
	     if (parse_config_line(cmd,arg,tmp,buf) == SP_ERR_PARSE)
	       continue;
	     
	     uint32_t directive_hash = miscutil::hash_string(cmd,strlen(cmd));
	     
	     // handling config cmd. virtual.
	     handle_config_cmd(cmd,directive_hash,arg,buf,linenum);
	  }

	fclose(configfp); // closing configuration file.
	
	// finalize configuration, warning, changes, default settings, etc... virtual.
	finalize_configuration();
	
	// last modified.
	struct stat statbuf[1];
	if (stat(_filename.c_str(),statbuf) < 0)
	  {
	     /* we should never get here. */
	     errlog::log_error(LOG_LEVEL_ERROR,"Couldn't stat config file, probably the file %s doesn't exist",
			       _filename.c_str());
	     _lastmodified = 0;
	  }
	else
	  {
	     _lastmodified = statbuf->st_mtime;
	  }
	
	return SP_ERR_OK; // TODO: finer error handling...
     }
   
   sp_err configuration_spec::parse_config_line(char *cmd, char* arg, char *tmp, char *buf)
     {
	strlcpy(tmp, buf, sizeof(tmp));
	
	/* Copy command (i.e. up to space or tab) into cmd */
	char *p = buf;
	char *q = cmd;
	while (*p && (*p != ' ') && (*p != '\t'))
	  {
	     *q++ = *p++;
	  }
	*q = '\0';
	
	/* Skip over the whitespace in buf */
	while (*p && ((*p == ' ') || (*p == '\t')))
	  {
	     p++;
	  }
	
	 /* Copy the argument into arg */
	strlcpy(arg, p, BUFFER_SIZE);
	
	/* Should never happen, but check this anyway */
	if (*cmd == '\0')
	  {
	     return SP_ERR_PARSE;
	  }
	
	/* Make sure the command field is lower case */
	for (p = cmd; *p; p++)
	  {
	     if (ijb_isupper(*p))
	       {
		  *p = (char)ijb_tolower(*p);
	       }
	  }
	
	/* std::cout << "cmd: " << cmd << " -- arg: " << arg << std::endl;
	 std::cout << "buf: " << buf << std::endl; */
	
	return SP_ERR_OK;
     }
      
   int configuration_spec::check_file_changed()
     {
	struct stat statbuf[1];
	if (stat(_filename.c_str(),statbuf) < 0)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"Couldn't stat config file, probably the file %s doesn't exist",
			       _filename.c_str());
	     return -1;  // beware, or 1 ?
	  }
	else
	  {
	     if (_lastmodified == statbuf->st_mtime)
	       return 0;
	     else return 1;
	  }
     }

   void configuration_spec::html_table_row(char *&args, char *option, char *value,
					   const char *description)
     {
	miscutil::string_append(&args,"<tr><td><code>");
	miscutil::string_append(&args,option);
	miscutil::string_append(&args,"</code></td><td>");
	miscutil::string_append(&args,value);
	miscutil::string_append(&args,"</td><td>");
	miscutil::string_append(&args,description);
	miscutil::string_append(&args,"</td></tr>");
     }
   
   
} /* end of namespace. */
