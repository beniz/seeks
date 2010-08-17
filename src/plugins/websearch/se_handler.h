/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
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

#ifndef SE_HANDLER_H
#define SE_HANDLER_H

#include "proxy_dts.h" // sp_err 
#include "seeks_proxy.h"

#include <string>
#include <vector>
#include <bitset>
#include <stdint.h>

#include <curl/curl.h>

using sp::sp_err;

typedef pthread_mutex_t sp_mutex_t;

namespace seeks_plugins
{
   class se_parser;
   class search_snippet;
   class query_context;
   
#define NSEs 5  // number of supported search engines.
   
#ifndef ENUM_SE
#define ENUM_SE
   enum SE  // in alphabetical order.
     {
	BING,
	CUIL,
	EXALEAD,
	GOOGLE,
	YAHOO
     };
#endif
   
   class search_engine
     {
      public:
	search_engine();
	virtual ~search_engine();
	
	virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc) {};
	
	/*- variables. -*/
	std::string _description;
	bool _anonymous;  // false by default.
	hash_map<const char*,const char*,hash<const char*>,eqstr> *_param_translation;
     };

   // arguments to a threaded parser.
   struct ps_thread_arg
     {
	ps_thread_arg()
	  :_se((SE)0),_output(NULL),_snippets(NULL),_qr(NULL)
	    {
	    };
	   
	~ps_thread_arg()
	  {
	     // we do not delete the output, this is handled by the client.
	     // we do delete snippets outside the destructor (depends on whether we're using threads).
	  }
	
	int _se; // search engine (ggle, bing, ...).
	char *_output; // page content, to be parsed into snippets.
	std::vector<search_snippet*> *_snippets; // websearch result snippets.
	int _offset; // offset to snippets rank (when asking page x, with x > 1).
	query_context *_qr; // pointer to the current query context.
     };
   
   class se_ggle : public search_engine
     {
      public:
	se_ggle();
	~se_ggle();
	
	virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc);
     };
   
   class se_bing : public search_engine
     {
      public:
	se_bing();
	~se_bing();
	
	virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc);
     };

   class se_cuil : public search_engine
     {
      public:
	se_cuil();
	~se_cuil();
	
	virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc);
     };

   class se_yahoo : public search_engine
     {
      public:
	se_yahoo();
	~se_yahoo();
	
	virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc);
     };
      
   class se_exalead : public search_engine
     {
      public:
	se_exalead();
	~se_exalead();
	
	virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc);
     };
   
   class se_handler
     {
      public:
	/*-- initialization --*/
	static void init_handlers(const int &num);
	
	/*-- query preprocessing --*/
	static void preprocess_parameters(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters);
	
	static std::string no_command_query(const std::string &oquery);
	
	/*-- querying the search engines. --*/
	static std::string** query_to_ses(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					  int &nresults, const query_context *qc, const std::bitset<NSEs> &se_enabled);
	
	static void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				const SE &se, std::string &url, const query_context *qc,
				std::list<const char*> *&lheaders);
	
	static void set_engines(std::bitset<NSEs> &se_enabled, const std::vector<std::string> &ses);
	
	/*-- parsing --*/
	static se_parser* create_se_parser(const SE &se);
	
	static sp_err parse_ses_output(std::string **outputs, const int &nresults,
				       std::vector<search_snippet*> &snippets,
				       const int &count_offset,
				       query_context *qr, const std::bitset<NSEs> &se_enabled);

	static void parse_output(const ps_thread_arg &args);
	
	/*-- variables. --*/
      public:
	static std::string _se_strings[NSEs];
	
	/* search engine objects. */
	static se_ggle _ggle;
	static se_cuil _cuil;
	static se_bing _bing;
	static se_yahoo _yahoo;
	static se_exalead _exalead;
	static std::vector<CURL*> _curl_handlers;
	static sp_mutex_t _curl_mutex;
     };
      
} /* end of namespace. */

#endif
