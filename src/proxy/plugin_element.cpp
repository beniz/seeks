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
 
#include "plugin_element.h"
#include "mem_utils.h"
#include "loaders.h"
#include "urlmatch.h"
#include "errlog.h"

#include <iostream>

#include <assert.h>

namespace sp
{
   
  plugin_element::plugin_element(const std::vector<url_spec*> &pos_patterns,
				 const std::vector<url_spec*> &neg_patterns,
				 plugin *parent)
    : _pos_patterns(pos_patterns),_neg_patterns(neg_patterns),_parent(parent),
      _pcrs(false),_perl(false),
      _joblist(NULL), _is_dynamic(false),
      _pattern_filename(NULL),
      _code_filename(NULL)
   {  
   }

  plugin_element::plugin_element(const std::vector<std::string> &pos_patterns,
				 const std::vector<std::string> &neg_patterns,
				 plugin *parent)
    : _parent(parent),
      _pcrs(false), _perl(false),   
      _joblist(NULL), _is_dynamic(false),
      _pattern_filename(NULL),_code_filename(NULL)
  {     
     compile_patterns(pos_patterns,_pos_patterns);
     compile_patterns(neg_patterns,_neg_patterns);
  }

  plugin_element::plugin_element(const char *filename,
				 plugin *parent)
    :_parent(parent),
     _pcrs(false), _perl(false),
     _joblist(NULL), _is_dynamic(false),
     _pattern_filename(strdup(filename)),
     _code_filename(NULL)
  {
     // TODO: error handling !
     load_pattern_file();
  }

   plugin_element::plugin_element(const std::vector<std::string> &pos_patterns,
				  const std::vector<std::string> &neg_patterns,
				  const char *code_filename,
				  const bool &pcrs, const bool &perl,
				  plugin *parent)
     : _parent(parent),
       _pcrs(pcrs),_perl(perl),
       _joblist(NULL), _is_dynamic(false),
       _pattern_filename(NULL),
       _code_filename(strdup(code_filename))
     {
	compile_patterns(pos_patterns,_pos_patterns);
	compile_patterns(neg_patterns,_neg_patterns);
	load_code_file(); // TODO: error handling !
     }
   
   plugin_element::plugin_element(const char *pattern_filename,
				  const char *code_filename,
				  const bool &pcrs, const bool &perl,
				  plugin *parent)
     :_parent(parent),
      _pcrs(pcrs),_perl(perl),
      _joblist(NULL), _is_dynamic(false),
      _pattern_filename(strdup(pattern_filename)),
      _code_filename(strdup(code_filename))
     {
	//TODO error handling !
	load_pattern_file();
	load_code_file();
     }
   
  plugin_element::~plugin_element()
  {
     clear_patterns();
     freez(_pattern_filename);
     freez(_code_filename);
  
     if (_perl)
       {
	  if (_perl_interpreter)
	    {
	       perl_destruct(_perl_interpreter);
	       perl_free(_perl_interpreter);
	    }
	  
       }
     
  }
   
   void plugin_element::clear_patterns()
     {
	std::vector<url_spec*>::iterator vit;
	for (vit=_pos_patterns.begin();vit!=_pos_patterns.end();++vit)
	  {
	     delete (*vit);
	  }
	_pos_patterns.clear();
	
	for (vit=_neg_patterns.begin();vit!=_neg_patterns.end();++vit)
	  {
	     delete (*vit);
	  }
	_neg_patterns.clear();
     }

   int plugin_element::load_pattern_file()
     {
	if (!_pattern_filename)
	  return 0;
	
	clear_patterns();
	
	// open file.
	FILE *fp;
	if ((fp = fopen(_pattern_filename,"r")) == NULL)
	  {
	     
	     errlog::log_error(LOG_LEVEL_ERROR, "can't load pattern file '%s': error opening file: %E",
			       _pattern_filename);
	     return 0;   // we're done, this plugin will very probably be of no use.
	  }
	
	// - read patterns.
	bool positive = true;
	unsigned long linenum = 0;
	char  buf[BUFFER_SIZE];
	while(loaders::read_config_line(buf, sizeof(buf), fp, &linenum) != NULL)
	  {
	     if (buf[0] == '+')
	       {
		  positive = true;
		  continue;
	       }
	     if (buf[0] == '-')
	       {
		  positive = false;
		  continue;
	       }
	     
	     // - compile them.
	     url_spec *usp = NULL;
	     sp_err err = url_spec::create_url_spec(usp,buf);
	     if (err != SP_ERR_OK)
	       {
		  // signal and skip bad pattern.
		  errlog::log_error(LOG_LEVEL_ERROR,
				    "cannot create URL pattern from: %s", buf);
	       }
	     // - store them.
	     else 
	       {
		  if (positive)
		    {
		       _pos_patterns.push_back(usp);
		    }
		  else
		    {
		       _neg_patterns.push_back(usp);
		    }
	       }
	  }
	
	fclose(fp);
	
	return 0;
     }
      
   int plugin_element::load_code_file()
     {
	if (_code_filename)
	  {
	     if (_pcrs)
	       return pcrs_load_code_file();
	     else if (_perl)
	       return perl_load_code_file();
	     return 1;
	  }
	return 0;
     }

   int plugin_element::reload()
     {
	int p = load_pattern_file();
	int c = load_code_file();
	return (p == 0 && c == 0);
     }
      
   void plugin_element::compile_patterns(const std::vector<std::string> &patterns,
					 std::vector<url_spec*> &c_patterns)
     {
	std::vector<std::string>::const_iterator vit;
	for (vit=patterns.begin();vit!=patterns.end();++vit)
	  {
	     url_spec *usp = NULL;
	     sp_err err = url_spec::create_url_spec(usp,(char*)(*vit).c_str()); // beware of casting...
	     if (err != SP_ERR_OK)
	       {
		  // signal and skip bad pattern.
		  errlog::log_error(LOG_LEVEL_ERROR,
				    "cannot create URL pattern from: %s", (*vit).c_str());
	       }
	     else c_patterns.push_back(usp);
	  }
     }
   
  bool plugin_element::match_url(const http_request *http)
  {
    // check whether at least a pattern matches the request.
     /* std::cout << "[Debug]:plugin_element::match_url: pos_patterns size: " 
       << _pos_patterns.size() 
	 << " -- neg_patterns size: " << _neg_patterns.size()
	 << std::endl; */
     
     int count = 0;
     std::vector<url_spec*>::const_iterator vit;
     for(vit=_neg_patterns.begin();vit!=_neg_patterns.end();++vit)
       {
	  if (urlmatch::url_match((*vit),http))
	 {
	    return false;
	 }
       }
     
     count = 0;
     for(vit=_pos_patterns.begin();vit!=_pos_patterns.end();++vit)
       {
	  if (urlmatch::url_match((*vit),http))
	    {
	       return true;
	    }
	  count++;
       }
     return false;
  }   

// two paths: pcrs or full Perl interpreter. */
// 1- pcrs.

void plugin_element::pcrs_load_code(char *buf,
				    pcrs_job *lastjob)
{
   _job_patterns.push_back(buf);
   if (_is_dynamic || pcrs::pcrs_job_is_dynamic(buf))
     {
	_is_dynamic = true;
	if (_joblist)
	  {
	     pcrs_job::pcrs_free_joblist(_joblist);
	     _joblist = NULL;
	  }
	return;
     }
   
   int error;
   pcrs_job *job = NULL;
   if ((job = pcrs::pcrs_compile_command(buf, &error)) == NULL)
     {
	errlog::log_error(LOG_LEVEL_ERROR,
			  "Compiling plugin job \'%s\' failed with error %d.", buf, error);
	return;
     }
   else
     {
	if (_joblist == NULL)
	  {
	     _joblist = job;
	  }
	else if (NULL != lastjob)
	  {
	     lastjob->_next = job;
	  }
	lastjob = job;
	errlog::log_error(LOG_LEVEL_RE_FILTER, "Compiling plugin job \'%s\' succeeded.", buf);
     }
}   
       
int plugin_element::pcrs_load_code_file()
{
   FILE *fp;
   
   if ((fp = fopen(_code_filename, "r")) == NULL)
     {
	errlog::log_error(LOG_LEVEL_FATAL, "can't load plugin file '%s': %E",
			  _code_filename);
	return -1;
     }
   
   _is_dynamic = false;
   
   pcrs_job *lastjob = NULL;
   unsigned long linenum = 0;
   char  buf[BUFFER_SIZE];
   while(loaders::read_config_line(buf, sizeof(buf), fp, &linenum) != NULL)
     {
	pcrs_load_code(buf,lastjob);
     } // end while.
   fclose(fp);
   
   return 0;
}  
   
pcrs_job* plugin_element::compile_dynamic_pcrs_job_list(const client_state *csp)
{     
   pcrs_job *job_list = NULL;
   pcrs_job *dummy = NULL;
   pcrs_job *lastjob = NULL;
   int error = 0;
   
   const pcrs_variable variables[] =
     {
	pcrs_variable("url",csp->_http._url,1),
	pcrs_variable("path",csp->_http._path,1),
	pcrs_variable("host",csp->_http._host,1),
	pcrs_variable("origin",csp->_ip_addr_str,1),
	pcrs_variable(NULL,NULL,1)
     };
                
   assert(!_job_patterns.empty());
   
   std::list<const char*>::const_iterator lit = _job_patterns.begin();
   while(lit!=_job_patterns.end())
     {
	assert((*lit) != NULL);
	const char *str = (*lit);
	++lit;
	
	dummy = pcrs::pcrs_compile_dynamic_command((char*)str, variables, &error);
	if (NULL == dummy)
	  {
	     assert(error < 0);
	     errlog::log_error(LOG_LEVEL_ERROR,
			       "Compiling plugin job \'%s\' failed: %s",
			       str, pcrs::pcrs_strerror(error));
	     continue;
	  }
	else
	  {
	     if (error == PCRS_WARN_TRUNCATION)
	       {
		  errlog::log_error(LOG_LEVEL_ERROR,
				    "At least one of the variables in \'%s\' had to "
				    "be truncated before compilation", str);
	       }
	     if (job_list == NULL)
	       {
		  job_list = dummy;
	       }
	     else
	       {
		  lastjob->_next = dummy;
	       }
	     lastjob = dummy;
	  }
     }
   
   return job_list;
}  

   char* plugin_element::pcrs_plugin_response(client_state *csp,
					      char *old)
     {
	// compile and execute jobs as needed.
	if (_is_dynamic)
	  _joblist = compile_dynamic_pcrs_job_list(csp);

	if (!_joblist)
	  return NULL;
	
	size_t size = 0, prev_size = 0;
	int job_number = 0;
	int hits = 0;
	char *newstr = NULL;
	pcrs_job *job;
	for (job = _joblist; NULL != job; job = job->_next)
	  {
	     job_number++;
	     size = strlen(old)+1;
	     int job_hits = pcrs::pcrs_execute(job, old, size, &newstr, &size);
	     
	     if (job_hits >= 0)
	       {
		  /*
		   * That went well. Continue filtering
		   * and use the result of this job as
		   * input for the next one.
		   */
		  hits += job_hits;
		  if (old != csp->_iob._cur)
		    {
		       freez(old);
		    }
		  old = newstr;
	       }
	     else
	       {
		  /*
		   * This job caused an unexpected error. Inform the user
		   * and skip the rest of the jobs in this filter. We could
		   * continue with the next job, but usually the jobs
		   * depend on each other or are similar enough to
		   * fail for the same reason.
		   *
		   * At the moment our pcrs expects the error codes of pcre 3.4,
		   * but newer pcre versions can return additional error codes.
		   * As a result pcrs_strerror()'s error message might be
		   * "Unknown error ...", therefore we print the numerical value
		   * as well.
		   */
		  errlog::log_error(LOG_LEVEL_ERROR, "Skipped plugin job %u: %s (%d)",
				    job_number, pcrs::pcrs_strerror(job_hits), job_hits);
		  break;
	       }
	  }
	if (_is_dynamic) pcrs_job::pcrs_free_joblist(_joblist);
	
	errlog::log_error(LOG_LEVEL_RE_FILTER,
			  "filtering %s%s (size %d) with \'%s\' produced %d hits (new size %d).",
			  csp->_http._hostport, csp->_http._path, prev_size, _parent->get_name_cstr(), 
			  hits, size);
	
	/* std::cout << "filtering has procuded #" << hits << "hits!\n";
	 std::cout << "newstr: " << newstr << std::endl; */
	
	/*
	 * If there were no hits, destroy our copy and let
	 * chat() use the original in csp->iob
	 */
	
	if (!hits)
	  {
	     freez(newstr);
	     return(NULL);
	  }
	
	csp->_flags |= CSP_FLAG_MODIFIED;
	csp->_content_length = size;
	IOB_RESET(csp);
	
	return(newstr);
     }

   // 2- Perl
   EXTERN_C void xs_init(pTHX);
   
   int plugin_element::perl_load_code_file()
     {
	char *command_line[2] = { (char*)"", (char*)NULL };
	command_line[1] = _code_filename;
	
	_perl_interpreter = perl_alloc();
	perl_construct(_perl_interpreter);
	
	if (perl_parse(_perl_interpreter, xs_init, 2, command_line, (char**)NULL))
	  return 1; // failed to parse.

	std::cout << "Loaded Perl code file " << command_line << std::endl;
	
	return 0;
     }
   
   char** plugin_element::perl_execute_subroutine(const std::string &subr_name,
						  char **str)
     {
//	seeks_proxy::mutex_lock(&_ple_mutex);
	
	PerlInterpreter *my_perl = _perl_interpreter;
	
	dSP; // Perl macro to get access to the stack (of the interpreter pointed by 'my_perl').
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	
	// only use the first string, array is not supported here.
	XPUSHs(sv_2mortal(newSVpv(str[0],0)));
	
	PUTBACK;
	
	int count = call_pv(subr_name.c_str(), G_ARRAY);
	
	SPAGAIN;

	if (SvTRUE(ERRSV))
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,
			       "An error occurred in the Perl preprocessor: %s",
			       SvPV_nolen(ERRSV));
	     return str; // beware: return the original string.
	  }
		
	char **output = (char**)malloc((count+1)*sizeof(char*));
	output[count] = NULL;
	int i=count;
	while(i>0)
	  output[--i] = savepv(SvPV_nolen(POPs)); // stack comes in reverse order...
	
	FREETMPS;
	LEAVE;

//	seeks_proxy::mutex_unlock(&_ple_mutex);
	
	return output;
     }
   
} /* end of namespace. */
