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
			     std::string &url)
     {
	std::string q_ggle = se_handler::_se_strings[0]; // query to ggle.
	const char *query = miscutil::lookup(parameters,"q");
	
	// query.
	int p = 31;
	q_ggle.replace(p,6,std::string(query));
	
	// expansion = result page called...
	const char *expansion = miscutil::lookup(parameters,"expansion");
	int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * websearch::_wconfig->_N : 0;
	std::string pp_str = miscutil::to_string(pp);
	miscutil::replace_in_string(q_ggle,"%start",pp_str);
	
	// number of results.
	int num = websearch::_wconfig->_N; // by default.
	/* if (pp == 0)
	  num *= se_handler::_results_lookahead_factor; */ 
	std::string num_str = miscutil::to_string(num);
	miscutil::replace_in_string(q_ggle,"%num",num_str);
	
	// encoding.
	miscutil::replace_in_string(q_ggle,"%encoding","utf-8");
	
	// language.
	miscutil::replace_in_string(q_ggle,"%lang",websearch::_wconfig->_lang);
	
	// client version. TODO: grab parameter from http request ?
	miscutil::replace_in_string(q_ggle,"%client","firefox-a"); // beware: may use something else, seeks-a.
	
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
			     std::string &url)
     {
	std::string q_bing = se_handler::_se_strings[2]; // query to bing.
	const char *query = miscutil::lookup(parameters,"q");
	        
	// query.
	int p = 29;
	q_bing.replace(p,6,std::string(query));
	
	// page.
	const char *expansion = miscutil::lookup(parameters,"expansion");
	int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * websearch::_wconfig->_N : 0;
	std::string pp_str = miscutil::to_string(pp);
	miscutil::replace_in_string(q_bing,"%start",pp_str);
	
	// number of results.
	// can't figure out what argument to pass to Bing. Seems only feasible through cookies (losers).
		
	// language.
	// TODO: translation table.
	if (websearch::_wconfig->_lang == "en")
	  miscutil::replace_in_string(q_bing,"%lang","en-US");
	else if (websearch::_wconfig->_lang == "fr")
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
			     std::string &url)
     {
	std::string q_cuil = se_handler::_se_strings[1];
	const char *query = miscutil::lookup(parameters,"q");
	
	// query.
	int p = 29;
	q_cuil.replace(p,6,std::string(query));

	// language.
	miscutil::replace_in_string(q_cuil,"%lang",websearch::_wconfig->_lang);

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
      "http://www.google.com/search?q=%query&start=%start&num=%num&hl=%lang&ie=%encoding&oe=%encoding&client=%client",
      // cuil: www.cuil.com/search?q=markov+chain&lang=en
      "http://www.cuil.com/search?q=%query&lang=%lang",
      // bing: www.bing.com/search?q=markov+chain&go=&form=QBLH&filt=all
      "http://www.bing.com/search?q=%query&first=%start&mkt=%lang"
    };

   se_ggle se_handler::_ggle = se_ggle();
   se_cuil se_handler::_cuil = se_cuil();
   se_bing se_handler::_bing = se_bing();

   long se_handler::_se_connect_timeout = 5; // 5 seconds.
   
   /*-- preprocessing queries. */
   void se_handler::preprocess_parameters(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
     {
	// query.
	const char *q = "q";
	const char *query = miscutil::lookup(parameters,q);
	std::string query_str = std::string(query);
	miscutil::replace_in_string(query_str," ","+");
	miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),q);
	miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
				q,1,query_str.c_str(),1);
     }
      
   // could be moved elsewhere...
   std::string se_handler::cleanup_query(const std::string &oquery)
     {
	// non interpreted '+' should be deduced.
	std::string cquery = oquery;
	size_t end_pos = cquery.size();
	size_t pos = 0;
	while ((pos = cquery.find_last_of('+',end_pos)) != std::string::npos)
	  {
	     cquery.replace(pos,1," ");
	     while(cquery[--pos] == '+') // deal with multiple pluses.
	       {
	       }
	     end_pos = pos;
	  }
	return cquery;
     }
   
  /*-- queries to the search engines. */  
  char** se_handler::query_to_ses(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				  int &nresults)
  {
    std::vector<std::string> urls;
    
    // config, enabling of SEs.
    for (int i=0;i<NSEs;i++)
      {
	 if (websearch::_wconfig->_se_enabled[i])
	  {
	    std::string url;
	    se_handler::query_to_se(parameters,(SE)i,url);
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
    curl_mget cmg(urls.size(),se_handler::_se_connect_timeout);
    cmg.www_mget(urls,urls.size());
    
    char **outputs = new char*[urls.size()];
    bool have_outputs = false;
    for (size_t i=0;i<urls.size();i++)
      {
	if (cmg._outputs[i])
	  {
	    outputs[i] = strdup(cmg._outputs[i]);
	    have_outputs = true;
	  }
      }
    
    if (!have_outputs)
      {
	delete[] outputs;
	outputs = NULL;
      }
    
     /* std::cout << "outputs:\n";
      std::cout << outputs[0] << std::endl; */
    
    return outputs;
  }
   
  void se_handler::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
			       const SE &se, std::string &url)
  {
     switch(se)
      {
      case GOOGLE:
	 _ggle.query_to_se(parameters,url);
	 break;
      case CUIL:
	 _cuil.query_to_se(parameters,url);
	break;
      case BING:
	 _bing.query_to_se(parameters,url);
	break;
      }

  }
   
  /*-- parsing. --*/
  sp_err se_handler::parse_ses_output(char **outputs, const int &nresults,
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
	  
	  // threads, one per parser.
	  int k = 0;
	  for (int i=0;i<NSEs;i++)
	    {
	       if (websearch::_wconfig->_se_enabled[i])
		 {
		    ps_thread_arg *args = new ps_thread_arg();
		    args->_se = (SE)i;
		    args->_output = outputs[j++];
		    args->_snippets = new std::vector<search_snippet*>();
		    args->_offset = count_offset;
		    args->_qr = qr;
		    parser_args[k] = args;
		    
		    pthread_t ps_thread;
		    int err = pthread_create(&ps_thread, NULL,  // default attribute is PTHREAD_CREATE_JOINABLE
					     (void * (*)(void *))se_handler::parse_output, args);
		    parser_threads[k++] = ps_thread;
		 }
	    }
	       
	  // join and merge results.
       	 for (size_t i=0;i<active_ses;i++)
	    {
	       pthread_join(parser_threads[i],NULL);
	    }
	  
	  for (size_t i=0;i<active_ses;i++)
	    {
	       std::copy(parser_args[i]->_snippets->begin(),parser_args[i]->_snippets->end(),
			 std::back_inserter(snippets));
	       delete parser_args[i]->_snippets;
               delete parser_args[i];
	    }
       }
     else
       {
	  for (int i=0;i<NSEs;i++)
	    {
	       if (websearch::_wconfig->_se_enabled[i])
		 {
		    ps_thread_arg args;
		    args._se = (SE)i;
		    args._output = outputs[j++];
		    args._snippets = &snippets;
		    args._offset = count_offset;
		    args._qr = qr;
		    parse_output(args);
		 }
	    }
       }
     
    return SP_ERR_OK;
  }

   void se_handler::parse_output(const ps_thread_arg &args)
     {
	se_parser *se = se_handler::create_se_parser(args._se);
	se->parse_output(args._output,args._snippets,args._offset);

	// link the snippets to the query context.
	for (size_t i=0;i<args._snippets->size();i++)
	  args._snippets->at(i)->_qc = args._qr;
	
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
	// hack
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
