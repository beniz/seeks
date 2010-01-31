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
 **/

#include "se_handler.h"
#include "miscutil.h"
#include "websearch.h" // for configuration.
#include "curl_mget.h"
#include "encode.h"
#include "errlog.h"
#include "seeks_proxy.h" // for configuration.
#include "proxy_configuration.h"
#include "query_context.h"

#include "se_parser_ggle.h"
#include "se_parser_cuil.h"
#include "se_parser_bing.h"

#include <pthread.h>
#include <algorithm>
#include <iterator>
#include <bitset>
#include <iostream>

using namespace sp;

namespace seeks_plugins
{
   /*- search_engine & derivatives. -*/
   search_engine::search_engine()
     :_description(""),_anonymous(false),_param_translation(NULL)
       {
       }
   
   search_engine::~search_engine()
     {
	// TODO: delete translation map if any.
     }
   
   se_ggle::se_ggle()
     : search_engine()
       {
	  
       }
   
   se_ggle::~se_ggle()
     {
     }
   
   void se_ggle::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
			     std::string &url, const query_context *qc)
     {
	std::string q_ggle = se_handler::_se_strings[0]; // query to ggle.
	const char *query = miscutil::lookup(parameters,"q");
	
	// query.
	int p = 31;
	q_ggle.replace(p,6,se_handler::no_command_query(std::string(query)));
	
	// expansion = result page called...
	const char *expansion = miscutil::lookup(parameters,"expansion");
	int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * websearch::_wconfig->_N : 0;
	std::string pp_str = miscutil::to_string(pp);
	miscutil::replace_in_string(q_ggle,"%start",pp_str);
	
	// number of results.
	int num = websearch::_wconfig->_N; // by default.
	std::string num_str = miscutil::to_string(num);
	miscutil::replace_in_string(q_ggle,"%num",num_str);
	
	// encoding.
	miscutil::replace_in_string(q_ggle,"%encoding","utf-8");
	
	// language.
	if (websearch::_wconfig->_lang == "auto")
	  miscutil::replace_in_string(q_ggle,"%lang",qc->_auto_lang);
	else miscutil::replace_in_string(q_ggle,"%lang",websearch::_wconfig->_lang);
	
	// log the query.
	errlog::log_error(LOG_LEVEL_INFO, "Querying ggle: %s", q_ggle.c_str());
	
	url = q_ggle;
     } 
   
   se_bing::se_bing()
     : search_engine()
       {
       }
   
   se_bing::~se_bing()
     {
     }
      
   void se_bing::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
			     std::string &url, const query_context *qc)
     {
	std::string q_bing = se_handler::_se_strings[2]; // query to bing.
	const char *query = miscutil::lookup(parameters,"q");
	        
	// query.
	int p = 29;
	q_bing.replace(p,6,se_handler::no_command_query(std::string(query)));
	
	// page.
	const char *expansion = miscutil::lookup(parameters,"expansion");
	int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * websearch::_wconfig->_N : 0;
	std::string pp_str = miscutil::to_string(pp);
	miscutil::replace_in_string(q_bing,"%start",pp_str);
	
	// number of results.
	// can't figure out what argument to pass to Bing. Seems only feasible through cookies (losers).
		
	// language.
	// TODO: translation table.
	std::string lang = websearch::_wconfig->_lang;
	if (websearch::_wconfig->_lang == "auto")
	  lang = qc->_auto_lang;
	if (lang == "en")
	  miscutil::replace_in_string(q_bing,"%lang","en-US");
	else if (lang == "fr")
	  miscutil::replace_in_string(q_bing,"%lang","fr-FR");
	
	// log the query.
	errlog::log_error(LOG_LEVEL_INFO, "Querying bing: %s", q_bing.c_str());
	
	url = q_bing;
     }

   se_cuil::se_cuil()
     : search_engine()
       {
       }
   
   se_cuil::~se_cuil()
     {
     }
   
   void se_cuil::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
			     std::string &url, const query_context *qc)
     {
	std::string q_cuil = se_handler::_se_strings[1];
	const char *query = miscutil::lookup(parameters,"q");
	
	// query.
	int p = 29;
	q_cuil.replace(p,6,se_handler::no_command_query(std::string(query)));

	// language detection is done through headers.
	//miscutil::replace_in_string(q_cuil,"%lang",websearch::_wconfig->_lang);

	// expansion + hack for getting Cuil's next pages.
	const char *expansion = miscutil::lookup(parameters,"expansion");
	int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * websearch::_wconfig->_N : 0;
	if (pp > 1)
	  {
	     const char *cuil_npage = miscutil::lookup(parameters,"cuil_npage");
	     if (cuil_npage)
	       {
		  q_cuil.append(std::string(cuil_npage));
	       }
	  }
		
	// log the query.
	errlog::log_error(LOG_LEVEL_INFO, "Querying cuil: %s", q_cuil.c_str());
	url = q_cuil;
     }
   
   /*- se_handler. -*/
   std::string se_handler::_se_strings[NSEs] =
    {
      // ggle: http://www.google.com/search?q=help&ie=utf-8&oe=utf-8&aq=t&rls=org.mozilla:en-US:official&client=firefox-a
      "http://www.google.com/search?q=%query&start=%start&num=%num&hl=%lang&ie=%encoding&oe=%encoding",
      // cuil: www.cuil.com/search?q=markov+chain&lang=en
      "http://www.cuil.com/search?q=%query",
      // bing: www.bing.com/search?q=markov+chain&go=&form=QBLH&filt=all
      "http://www.bing.com/search?q=%query&first=%start&mkt=%lang"
    };

   se_ggle se_handler::_ggle = se_ggle();
   se_cuil se_handler::_cuil = se_cuil();
   se_bing se_handler::_bing = se_bing();
   
   /*-- preprocessing queries. */
   void se_handler::preprocess_parameters(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	// query.
	const char *q = "q";
	const char *query = miscutil::lookup(parameters,q);
	std::string query_str = std::string(query);
	
	// known query.
	const char *query_known = miscutil::lookup(parameters,"qknown");
	
	// if the current query is the same as before, let's apply the current language to it
	// (i.e. the in-query language command, if any).
	if (query_known)
	  {
	     std::string query_known_str = std::string(query_known);
	     if (query_known_str != query_str)
	       {
		  // look for in-query commands.
		  std::string no_command_query = se_handler::no_command_query(query_known_str);
		  if (no_command_query == query_str)
		    query_str = query_known_str; // replace query with query + in-query command.
	       }
	  }
		
	// clean up the 'pluses'.
	miscutil::replace_in_string(query_str," ","+");
	miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),q);
	miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
				q,1,query_str.c_str(),1);
     }
      
   // could be moved elsewhere...
   std::string se_handler::cleanup_query(const std::string &oquery)
     {
	std::string cquery = oquery;
	
	// non interpreted '+' should be deduced.
	size_t end_pos = cquery.size()-1;
	size_t pos = 0;
	while ((pos = cquery.find_last_of('+',end_pos)) != std::string::npos)
	  {
	     // TODO: this is buggy in certain cases, such as "x++" in quotes.
	     if (pos != end_pos)
	       cquery.replace(pos,1," ");
	     while(cquery[--pos] == '+') // deal with multiple pluses.
	       {
	       }
	     end_pos = pos;	     
	  }
	return cquery;
     }
   
   std::string se_handler::no_command_query(const std::string &oquery)
     {
	std::string cquery = oquery;
	// remove any command from the query.
	if (cquery[0] == ':')
	  cquery = cquery.substr(4);
	return cquery;
     }
      
  /*-- queries to the search engines. */  
   std::string** se_handler::query_to_ses(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					  int &nresults, const query_context *qc)
  {
    std::vector<std::string> urls;
    
    // config, enabling of SEs.
    for (int i=0;i<NSEs;i++)
      {
	 if (websearch::_wconfig->_se_enabled[i])
	  {
	    std::string url;
	    se_handler::query_to_se(parameters,(SE)i,url,qc);
	    urls.push_back(url);
	  }
      }
    
    if (urls.empty())
      {
	nresults = 0;
	return NULL; // beware.
      }
    else nresults = urls.size();
    
    // get content.
     curl_mget cmg(urls.size(),websearch::_wconfig->_se_transfer_timeout,0,
		   websearch::_wconfig->_se_connect_timeout,0,qc->_auto_lang,
		   &qc->_useful_http_headers);
     cmg.www_mget(urls,urls.size(),false); // don't go through the proxy, or will loop til death!
    
     std::string **outputs = new std::string*[urls.size()];
     bool have_outputs = false;
     for (size_t i=0;i<urls.size();i++)
       {
	  outputs[i] = NULL;
	  if (cmg._outputs[i])
	    {
	       outputs[i] = cmg._outputs[i];
	       have_outputs = true;
	    }
       }
     
     if (!have_outputs)
       {
	  delete[] outputs;
	  outputs = NULL;
       }
         
    return outputs;
  }
   
  void se_handler::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
			       const SE &se, std::string &url, const query_context *qc)
  {
     switch(se)
      {
      case GOOGLE:
	 _ggle.query_to_se(parameters,url,qc);
	 break;
      case CUIL:
	 _cuil.query_to_se(parameters,url,qc);
	break;
      case BING:
	 _bing.query_to_se(parameters,url,qc);
	break;
      }

  }
   
  /*-- parsing. --*/
  sp_err se_handler::parse_ses_output(std::string **outputs, const int &nresults,
				      std::vector<search_snippet*> &snippets,
				      const int &count_offset,
				      query_context *qr)
  {
     // use multiple threads unless told otherwise.
     int j = 0;
     if (seeks_proxy::_config->_multi_threaded)
       {
	  size_t active_ses = websearch::_wconfig->_se_enabled.count();
	  pthread_t parser_threads[active_ses];
	  ps_thread_arg* parser_args[active_ses];
	  for (size_t i=0;i<active_ses;i++)
	    parser_args[i] = NULL;
	  
	  // threads, one per parser.
	  int k = 0;
	  for (int i=0;i<NSEs;i++)
	    {
	       if (websearch::_wconfig->_se_enabled[i])
		 {
		    if (outputs[j])
		      {
			 ps_thread_arg *args = new ps_thread_arg();
			 args->_se = (SE)i;
			 args->_output = (char*) outputs[j]->c_str();  // XXX: sad cast.
			 args->_snippets = new std::vector<search_snippet*>();
			 args->_offset = count_offset;
			 args->_qr = qr;
			 parser_args[k] = args;
			 
			 pthread_t ps_thread;
			 int err = pthread_create(&ps_thread, NULL,  // default attribute is PTHREAD_CREATE_JOINABLE
						  (void * (*)(void *))se_handler::parse_output, args);
			 parser_threads[k++] = ps_thread;
		      }
		    else parser_threads[k++] = 0;
		    j++;
		 }
	    }
	       
	  // join and merge results.
       	 for (size_t i=0;i<active_ses;i++)
	    {
	       if (parser_threads[i]!=0)
		 pthread_join(parser_threads[i],NULL);
	    }
	  
	  for (size_t i=0;i<active_ses;i++)
	    {
	       if (parser_args[i])
		 {
		    std::copy(parser_args[i]->_snippets->begin(),parser_args[i]->_snippets->end(),
			      std::back_inserter(snippets));
		    parser_args[i]->_snippets->clear();
		    delete parser_args[i]->_snippets;
		    delete parser_args[i];
		 }
	    }
       }
     else
       {
	  for (int i=0;i<NSEs;i++)
	    {
	       if (websearch::_wconfig->_se_enabled[i])
		 {
		    if (outputs[j])
		      {
			 ps_thread_arg args;
			 args._se = (SE)i;
			 args._output = (char*)outputs[j]->c_str(); // XXX: sad cast.
			 args._snippets = &snippets;
			 args._offset = count_offset;
			 args._qr = qr;
			 parse_output(args);
		      }
		    j++;
		 }
	    }
       }
     
    return SP_ERR_OK;
  }

   void se_handler::parse_output(const ps_thread_arg &args)
     {
	se_parser *se = se_handler::create_se_parser(args._se);
	se->parse_output(args._output,args._snippets,args._offset);

	// link the snippets to the query context
	// and post-process them.
	for (size_t i=0;i<args._snippets->size();i++)
	  {
	     args._snippets->at(i)->_qc = args._qr;
	     args._snippets->at(i)->tag();
	  }
	
	// hack for cuil.
	if (args._se == CUIL)
	  {
	     se_parser_cuil *se_p_cuil = static_cast<se_parser_cuil*>(se);
	     hash_map<int,std::string>::const_iterator hit = se_p_cuil->_links_to_pages.begin();
	     hash_map<int,std::string>::const_iterator hit1;
	     while(hit!=se_p_cuil->_links_to_pages.end())
	       {
		  if ((hit1=args._qr->_cuil_pages.find((*hit).first))==args._qr->_cuil_pages.end())
		    args._qr->_cuil_pages.insert(std::pair<int,std::string>((*hit).first,(*hit).second));
		  ++hit;
	       }
	  }
	// hack for getting stuff out of ggle.
	else if (args._se == GOOGLE)
	  {
	     // get more stuff from the parser.
	     se_parser_ggle *se_p_ggle = static_cast<se_parser_ggle*>(se);
	     // suggestions (later on, we expect to do improve this with shared queries).
	     if (!se_p_ggle->_suggestion.empty())
	       args._qr->_suggestions.push_back(se_p_ggle->_suggestion);
	  }
	delete se;
     }
   
  se_parser* se_handler::create_se_parser(const SE &se)
  {
    se_parser *sep = NULL;
    switch(se)
      {
      case GOOGLE:
	sep = new se_parser_ggle();
	break;
      case CUIL:
	 sep = new se_parser_cuil();
	break;
      case BING:
	sep = new se_parser_bing();
	break;
      }
    return sep;
  }


} /* end of namespace. */
