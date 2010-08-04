/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
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

#ifndef SE_HANDLER_IMG_H
#define SE_HANDLER_IMG_H

#include "se_handler.h"
#include "img_websearch_configuration.h"

namespace seeks_plugins
{
   
   enum IMG_SE // in alphabetical order.
     {
	BING_IMG,
	FLICKR,
	GOOGLE_IMG
     };
   
   class se_bing_img : public search_engine
     {
      public:
	se_bing_img();
	~se_bing_img();
	
	virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc);
     };
   
   class se_ggle_img : public search_engine
     {
      public:
	se_ggle_img();
	~se_ggle_img();
	
	virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc);
     };
   
   class se_flickr : public search_engine
     {
      public:
	se_flickr();
	~se_flickr();
	
	virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc);
     };
      
   class se_handler_img
     {
      public:
	/*-- querying the search engines. --*/
	static std::string** query_to_ses(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					  int &nresults, const query_context *qc, const std::bitset<IMG_NSEs> &se_enabled);
	
	static void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				const IMG_SE &se, std::string &url, const query_context *qc,
				std::list<const char*> *&lheaders);
	
	static void set_engines(std::bitset<IMG_NSEs> &se_enabled, const std::vector<std::string> &ses);
	
	/*-- parsing --*/
	static se_parser* create_se_parser(const IMG_SE &se);
	
	static sp_err parse_ses_output(std::string **outputs, const int &nresults,
				       std::vector<search_snippet*> &snippets,
				       const int &count_offset,
				       query_context *qr, const std::bitset<IMG_NSEs> &se_enabled);
	
	static void parse_output(const ps_thread_arg &args);
	
	 /*-- variables. --*/
      public:
	static se_bing_img _img_bing;
	static se_ggle_img _img_ggle;
	static se_flickr _img_flickr;
	static std::string _se_strings[IMG_NSEs];
     };
        
} /* end of namespace. */

#endif
