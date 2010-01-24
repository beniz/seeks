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

#include "query_context.h"
#include "websearch.h"
#include "stl_hash.h"
#include "mem_utils.h"
#include "miscutil.h"
#include "mrf.h"
#include "errlog.h"
#include "se_handler.h"

#include <sys/time.h>
#include <iostream>

using sp::sweeper;
using sp::miscutil;
using sp::errlog;
using lsh::mrf;

namespace seeks_plugins
{
   
   query_context::query_context(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
				const std::list<const char*> &http_headers)
     :sweepable(),_page_expansion(0),_lsh_ham(NULL),_ulsh_ham(NULL),_lock(false),_compute_tfidf_features(true)
       {
	  _query_hash = query_context::hash_query_for_context(parameters,_query);
	  struct timeval tv_now;
	  gettimeofday(&tv_now, NULL);
	  _creation_time = _last_time_of_use = tv_now.tv_sec;
	  
	  if (websearch::_wconfig->_lang == "auto")
	    _auto_lang = query_context::detect_query_lang_http(http_headers);
	  else _auto_lang = websearch::_wconfig->_lang;
	  
	  sweeper::register_sweepable(this);
	  register_qc(); // register with websearch plugin.
       }
   
   query_context::~query_context()
     {
	unregister(); // unregister from websearch plugin.
       
	_unordered_snippets.clear();
	_unordered_snippets_title.clear();
	
	search_snippet::delete_snippets(_cached_snippets);
			
	// clears the LSH hashtable.
	// the LSH is cleared automatically as well.
	if (_ulsh_ham)
	  delete _ulsh_ham;
     }
   
   std::string query_context::sort_query(const std::string &query)
     {
	std::string clean_query = se_handler::cleanup_query(query);
	std::vector<std::string> tokens;
	mrf::tokenize(clean_query,tokens," ");
	std::sort(tokens.begin(),tokens.end(),std::less<std::string>());
	std::string sorted_query;
	size_t ntokens = tokens.size();
	for (size_t i=0;i<ntokens;i++)
	  sorted_query += tokens.at(i);
	return sorted_query;
     }
      
   uint32_t query_context::hash_query_for_context(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
						  std::string &query)
     {
	query = std::string(miscutil::lookup(parameters,"q"));
	std::string sorted_query = query_context::sort_query(query);
	return mrf::mrf_single_feature(sorted_query);
     }
      
   bool query_context::sweep_me()
     {
	// don't delete if locked.
	if (_lock)
	  return false;
	
	// check last_time_of_use + delay against current time.
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	double dt = difftime(tv_now.tv_sec,_last_time_of_use);
	
	//debug
	/* std::cout << "[Debug]:query_context #" << _query_hash
	  << ": sweep_me time difference: " << dt << std::endl; */
	//debug
	
	if (dt >= websearch::_wconfig->_query_context_delay)
	  return true;
	else return false;
     }

   void query_context::update_last_time()
     {
	 struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	_last_time_of_use = tv_now.tv_sec;
     }
      
  void query_context::register_qc()
  {
     websearch::_active_qcontexts.insert(std::pair<uint32_t,query_context*>(_query_hash,this));
  }
   
  void query_context::unregister()
  {
    hash_map<uint32_t,query_context*,hash<uint32_t> >::iterator hit;
    if ((hit = websearch::_active_qcontexts.find(_query_hash))==websearch::_active_qcontexts.end())
      {
	/**
	 * We should not reach here.
	 */
	 errlog::log_error(LOG_LEVEL_ERROR,"Cannot find query context when unregistering for query %s",
			   _query.c_str());
	 return;
      }
    else
      {
	websearch::_active_qcontexts.erase(hit);  // deletion is controlled elsewhere.
      }
  }

  sp_err query_context::generate(client_state *csp,
				 http_response *rsp,
				 const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
     const char *expansion = miscutil::lookup(parameters,"expansion");
     int horizon = atoi(expansion);
     for (int i=_page_expansion;i<horizon;i++) // catches up with requested horizon.
       {
	  // resets expansion parameter.
	  miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"expansion");
	  std::string i_str = miscutil::to_string(i+1);
	  miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
				  "expansion",1,i_str.c_str(),1);
	  
	  // hack for Cuil.
	  if (i != 0)
	    {
	       int expand=i+1;
	       hash_map<int,std::string>::const_iterator hit;
	       if ((hit=_cuil_pages.find(expand))!=_cuil_pages.end())
		 miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
					 "cuil_npage",1,(*hit).second.c_str(),1); // beware.
	    }
	  // hack
	  
	  
	  // query SEs.                                                                                                 
	  int nresults = 0;
	  std::string **outputs = se_handler::query_to_ses(parameters,nresults,this);
	  
	  // test for failed connection to the SEs comes here.    
	  if (!outputs)
	    {
	       return websearch::failed_ses_connect(csp,rsp);
	    }
	  
	  // parse the output and create result search snippets.   
	  int rank_offset = (i > 0) ? i * websearch::_wconfig->_N : 0;
	  
	  se_handler::parse_ses_output(outputs,nresults,_cached_snippets,rank_offset,this);
	  for (int j=0;j<nresults;j++)
	    if (outputs[j])
	      delete outputs[j];
	  delete[] outputs;
       }
     
     // update horizon.
     _page_expansion = horizon;
	  
     // error.
     return SP_ERR_OK;
  }

   sp_err query_context::regenerate(client_state *csp,
				    http_response *rsp,
				    const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
     {
	// determines whether we have part of the response in cache.
	const char *current_page = miscutil::lookup(parameters,"expansion");
	size_t requested_page = atoi(current_page);
	  
	// check whether to fetch of new results.
	size_t active_ses = websearch::_wconfig->_se_enabled.count(); // active search engines.
	size_t cached_pages = _cached_snippets.size() / active_ses / websearch::_wconfig->_N;     
	
	if (cached_pages < requested_page)
	  return generate(csp,rsp,parameters);
	else return SP_ERR_OK;
     }

   void query_context::add_to_unordered_cache(search_snippet *sr)
     {
	hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit;
	if ((hit=_unordered_snippets.find(sr->_id))!=_unordered_snippets.end())
	  {
	     // do nothing.
	  }
	else _unordered_snippets.insert(std::pair<uint32_t,search_snippet*>(sr->_id,sr));
     }
      
   void query_context::update_unordered_cache()
     {
	size_t cs_size = _cached_snippets.size();
	for (size_t i=0;i<cs_size;i++)
	  {
	     hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit;
	     if ((hit=_unordered_snippets.find(_cached_snippets[i]->_id))!=_unordered_snippets.end())
	       {
		  // for now, do nothing. TODO: may merge snippets here.
	       }
	     else
	       _unordered_snippets.insert(std::pair<uint32_t,search_snippet*>(_cached_snippets[i]->_id,
									      _cached_snippets[i]));
	  }
     }
   
   void query_context::update_snippet_seeks_rank(const uint32_t &id,
						 const double &rank)
     {
	hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit;
	if ((hit = _unordered_snippets.find(id))==_unordered_snippets.end())
	  {
	     // don't have this url in cache ? let's do nothing.
	  }
	else (*hit).second->_seeks_rank = rank;  // BEWARE: we may want to set up a proper formula here
	                                         //         with proper weighting by the consensus rank...
     }

   search_snippet* query_context::get_cached_snippet(const std::string &url) const
     {
	uint32_t id = mrf::mrf_single_feature(url);
	return get_cached_snippet(id);
     }
      
   search_snippet* query_context::get_cached_snippet(const uint32_t &id) const
     {
	hash_map<uint32_t,search_snippet*,id_hash_uint>::const_iterator hit;
	if ((hit = _unordered_snippets.find(id))==_unordered_snippets.end())
	  return NULL;
	else return (*hit).second;
     }
      
   void query_context::add_to_unordered_cache_title(search_snippet *sr)
     {
	hash_map<const char*,search_snippet*,hash<const char*>,eqstr>::iterator hit;
	if ((hit=_unordered_snippets_title.find(sr->_title.c_str()))!=_unordered_snippets_title.end())
	  {
	     // do nothing.
	  }
	else _unordered_snippets_title.insert(std::pair<const char*,search_snippet*>(sr->_title.c_str(),sr));
     }
			     
   search_snippet* query_context::get_cached_snippet_title(const char *title)
     {
	hash_map<const char*,search_snippet*,hash<const char*>,eqstr>::iterator hit;
	if ((hit = _unordered_snippets_title.find(title))==_unordered_snippets_title.end())
	  return NULL;
	else return (*hit).second;    
     }

   std::string query_context::detect_query_lang_http(const std::list<const char*> &http_headers)
     {
	std::list<const char*>::const_iterator sit = http_headers.begin();
	while(sit!=http_headers.end())
	  {
	     if (strncmp((*sit),"Accept-Language:",16) == 0)
	       {
		  // detect language.
		  std::string lang_head = (*sit);
		  size_t pos = lang_head.find_first_of(" ");
		  std::string lang = lang_head.substr(pos+1,2);
		  
		  errlog::log_error(LOG_LEVEL_INFO,"Query language detection: %s",lang.c_str());
		  return lang;
	       }
	     ++sit;
	  }
	return "en"; // beware, returning hardcoded default (since config value is most likely "auto").
     }
      
} /* end of namespace. */
