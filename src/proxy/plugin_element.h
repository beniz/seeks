/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef PLUGIN_ELEMENT_H
#define PLUGIN_ELEMENT_H

#include "plugin.h"
#include "proxy_dts.h"

#if defined(FEATURE_PTHREAD) || defined(_WIN32)
#include "seeks_proxy.h" // thread mutexes.
#endif

#include "pcrs.h"

#if defined(HAVE_PERL) && !defined(NO_PERL)
#include <EXTERN.h>
#include <perl.h>
#endif

#include <string>
#include <vector>

namespace sp
{
   class plugin_element
     {
      public:
	// constructors for plugins with no or custom internal perl-like code.
	plugin_element(const std::vector<std::string> &pos_patterns,
		       const std::vector<std::string> &neg_patterns,
		       plugin *parent);  // compile patterns from strings.
	
	plugin_element(const char *pattern_filename,
		       plugin *parent); // read and compile patterns from file.
	
	// useless ?
	plugin_element(const std::vector<url_spec*> &pos_patterns,
		       const std::vector<url_spec*> &neg_patterns,
		       plugin *parent);
	
	// constructors with code in an external file.
	plugin_element(const std::vector<std::string> &pos_patterns,
		       const std::vector<std::string> &neg_patterns,
		       const char *code_filename,
		       const bool &pcrs, const bool &perl,
		       plugin *parent);
	
	plugin_element(const char *pattern_filename,
		       const char *code_filename,
		       const bool &pcrs, const bool &perl,
		       plugin *parent);
	
	virtual ~plugin_element();

      protected:
	void clear_patterns();
	
      public:
	void compile_patterns(const std::vector<std::string> &patterns,
			      std::vector<url_spec*> &c_patterns);
	
	sp_err load_pattern_file();
	sp_err load_code_file();
	sp_err reload();
	
	virtual char* print() { return (char*)""; };
	
	bool match_url(const http_request *http);
	
	std::vector<url_spec*> _pos_patterns; // (positive) patterns that activate the plugin.
	std::vector<url_spec*> _neg_patterns; // (negative) patterns that deactivate the plugin.
	
	plugin *_parent; // parent plugin.
	
	/* two paths: pcrs or full Perl interpreter. */ // TODO: actually 3, with C/C++ path.
      protected:
	bool _pcrs;
	bool _perl;
	
	//1- pcrs.
      protected:
	void pcrs_load_code(char *buf, pcrs_job *lastjob);
	int pcrs_load_code_file();
	pcrs_job* compile_dynamic_pcrs_job_list(const client_state *csp); // compilation of dynamic jobs.
      public:
	char* pcrs_plugin_response(client_state *csp, char *old);
      
      protected:
	std::list<const char*> _job_patterns; /**< lines of regexp code. */
	pcrs_job *_joblist; /**< The resulting compiled pcrs_jobs. */
	bool _is_dynamic; // whether at least one of the pcrs jobs is dynamic.
	
#if defined(HAVE_PERL) && !defined(NO_PERL)
      public:
	//2- Perl.
	//TODO.
	PerlInterpreter *_perl_interpreter;  // single interpreter.
	                                     // multiple jobs can be handled through the call of Perl subroutines.
	int perl_load_code_file();
	char** perl_execute_subroutine(const std::string &subr_name, char **str);
#endif
	
	// TODO: not sure we need these variables here, but in the children classes instead.
	char* _pattern_filename; // plugin patterns file.
	char* _code_filename; // plugin code in perl or perl regex syntax.

#if defined(FEATURE_PTHREAD) || defined(_WIN32)
	// thread safe.
	sp_mutex_t _ple_mutex;
#endif
     };
   
} /* end of namespace. */

#endif
