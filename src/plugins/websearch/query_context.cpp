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

#include "query_context.h"
#include "websearch.h"
#include "stl_hash.h"
#include "mem_utils.h"
#include "miscutil.h"
#include "urlmatch.h"
#include "mrf.h"
#include "errlog.h"
#include "se_handler.h"
#include "iso639.h"
#include "seeks_proxy.h" // for mutexes

#include <sys/time.h>
#include <iostream>

using sp::sweeper;
using sp::miscutil;
using sp::urlmatch;
using sp::seeks_proxy;
using sp::errlog;
using sp::iso639;
using lsh::mrf;

namespace seeks_plugins
{
   std::string query_context::_query_delims = ""; // delimiters for tokenizing and hashing queries. "" because preprocessed and concatened.
   
   query_context::query_context()
     :sweepable(),_page_expansion(0),_lsh_ham(NULL),_ulsh_ham(NULL),_lock(false),_compute_tfidf_features(true)
       {
	  seeks_proxy::mutex_init(&_qc_mutex);
       }
      
   query_context::query_context(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
				const std::list<const char*> &http_headers)
     :sweepable(),_page_expansion(0),_lsh_ham(NULL),_ulsh_ham(NULL),_lock(false),_compute_tfidf_features(true)
       {
	  seeks_proxy::mutex_init(&_qc_mutex);
	  
	  // reload config if file has changed.
	  websearch::_wconfig->load_config();
	  
	  _query_hash = query_context::hash_query_for_context(parameters,_query); // hashing may contain a language command.
	  struct timeval tv_now;
	  gettimeofday(&tv_now, NULL);
	  _creation_time = _last_time_of_use = tv_now.tv_sec;
	  
	  grab_useful_headers(http_headers);
	  
	  // sets auto_lang & auto_lang_reg.
	  if (detect_query_lang(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters)))
	    {
	       query_context::in_query_command_forced_region(_auto_lang,_auto_lang_reg);
	    }
	  else if (websearch::_wconfig->_lang == "auto")
	    {
	       _auto_lang_reg = query_context::detect_query_lang_http(http_headers);
	       _auto_lang = _auto_lang_reg.substr(0,2);
	    }
	  else 
	    {
	       _auto_lang = websearch::_wconfig->_lang; // fall back is default search language.
	       _auto_lang_reg = query_context::lang_forced_region(websearch::_wconfig->_lang);
	    }
	  
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

	for (std::list<const char*>::iterator lit=_useful_http_headers.begin();
	     lit!=_useful_http_headers.end();lit++)
	  free_const((*lit));
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
	return mrf::mrf_single_feature(sorted_query,query_context::_query_delims);
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
     
     // seeks button used as a back button.
     if (_page_expansion > 0 && horizon < (int)_page_expansion)
       {
	  // reset expansion parameter.
	  miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"expansion");
	  std::string exp_str = miscutil::to_string(_page_expansion);
	  miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
				  "expansion",1,exp_str.c_str(),1);
	  return SP_ERR_OK;
       }
          
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
	std::string surl = urlmatch::strip_url(url);
	uint32_t id = mrf::mrf_single_feature(surl,query_context::_query_delims);
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
	std::string lctitle = sr->_title;
	std::transform(lctitle.begin(),lctitle.end(),lctitle.begin(),tolower);
	hash_map<const char*,search_snippet*,hash<const char*>,eqstr>::iterator hit;
	if ((hit=_unordered_snippets_title.find(lctitle.c_str()))!=_unordered_snippets_title.end())
	  {
	     // do nothing.
	  }
	else _unordered_snippets_title.insert(std::pair<const char*,search_snippet*>(strdup(lctitle.c_str()),sr));
     }
			     
   search_snippet* query_context::get_cached_snippet_title(const char *lctitle)
     {
	hash_map<const char*,search_snippet*,hash<const char*>,eqstr>::iterator hit;
	if ((hit = _unordered_snippets_title.find(lctitle))==_unordered_snippets_title.end())
	  return NULL;
	else return (*hit).second;    
     }

   bool query_context::detect_query_lang(hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
     {
	std::string query = std::string(miscutil::lookup(parameters,"q"));
	if (query[0] != ':')
	  return false;
	std::string qlang = query.substr(1,2); // : + 2 characters for the language.
	
	_in_query_command += query.substr(0,3);
	
	// check whether the language is known ! -> XXX: language table...
	if (iso639::has_code(qlang.c_str()))
	  {
	     _auto_lang = qlang;
	     errlog::log_error(LOG_LEVEL_INFO,"In-query language command detection: %s",_auto_lang.c_str());
	     return true;
	  }
	else 
	  {
	     errlog::log_error(LOG_LEVEL_INFO,"language code not found: %s",qlang.c_str());
	     return false;
	  }
     }
      
   // static.
   std::string query_context::detect_query_lang_http(const std::list<const char*> &http_headers)
     {
	std::list<const char*>::const_iterator sit = http_headers.begin();
	while(sit!=http_headers.end())
	  {
	     if (miscutil::strncmpic((*sit),"accept-language:",16) == 0)
	       {
		  // detect language.
		  std::string lang_head = (*sit);
		  size_t pos = lang_head.find_first_of(" ");
		  if (pos != std::string::npos && pos+6<=lang_head.length() && lang_head[pos+3] == '-')
		    {
		       std::string lang_reg = lang_head.substr(pos+1,5);
		       errlog::log_error(LOG_LEVEL_INFO,"Query language detection: %s",lang_reg.c_str());
		       return lang_reg;
		    }
		  else if (pos != std::string::npos && pos+3<=lang_head.length())
		    {
		       std::string lang = lang_head.substr(pos+1,2);
		       std::string lang_reg = query_context::lang_forced_region(lang);
		       errlog::log_error(LOG_LEVEL_INFO,"Forced query language region at detection: %s",lang_reg.c_str());
		       return lang_reg;
		    }
	       }
	     ++sit;
	  }
	return "en-US"; // beware, returning hardcoded default (since config value is most likely "auto").
     }

   void query_context::grab_useful_headers(const std::list<const char*> &http_headers)
     {
	std::list<const char*>::const_iterator sit = http_headers.begin();
	while(sit!=http_headers.end())
	  {
	     // user-agent
	     if (miscutil::strncmpic((*sit),"user-agent:",11) == 0)
	       {
		  const char *ua = strdup((*sit));
		  _useful_http_headers.push_back(ua);
	       }
	     else if (miscutil::strncmpic((*sit),"accept-charset:",15) == 0)
	       {
		  const char *ac = strdup((*sit));
		  /* std::string ac_str = "accept-charset: utf-8";
		  const char *ac = strdup(ac_str.c_str()); */
		  _useful_http_headers.push_back(ac);
	       }
	     else if (miscutil::strncmpic((*sit),"accept:",7) == 0)
	       {
		  const char *aa = strdup((*sit));
		  _useful_http_headers.push_back(aa);
	       }
	     // XXX: other useful headers should be detected and stored here.
	     ++sit;
	  }
     }
   
   std::string query_context::lang_forced_region(const std::string &auto_lang)
     {
	// XXX: in-query language commands force the query language to the search engine.
	// As such, we have to decide which region we attach to every of the most common
	// language forced queries.
	// Evidently, this is not a robust nor fast solution. Full support of locales etc... should
	// appear in the future. As for now, this is a simple scheme for a simple need.
	// Unsupported languages default to american english, that's how the world is right 
	// now...
	std::string region_lang = "en-US"; // default.
	if (auto_lang == "en")
	  {
	  }
	else if (auto_lang == "fr")
	  region_lang = "fr-FR";
	else if (auto_lang == "de")
	  region_lang = "de-DE";
	else if (auto_lang == "it")
	  region_lang = "it-IT";
	else if (auto_lang == "es")
	  region_lang = "es-ES";
	else if (auto_lang == "pt")
	  region_lang = "es-PT"; // so long for Brazil (BR)...
	else if (auto_lang == "nl")
	  region_lang = "nl-NL";
	else if (auto_lang == "ja")
	  region_lang = "ja-JP";
	else if (auto_lang == "no")
	  region_lang = "no-NO";
	else if (auto_lang == "pl")
	  region_lang = "pl-PL";
	else if (auto_lang == "ru")
	  region_lang = "ru-RU";
	else if (auto_lang == "ro")
	  region_lang = "ro-RO";
	else if (auto_lang == "sh")
	  region_lang = "sh-RS"; // Serbia.
	else if (auto_lang == "sl")
	  region_lang = "sl-SL";
	else if (auto_lang == "sk")
	  region_lang = "sk-SK";
	else if (auto_lang == "sv")
	  region_lang = "sv-SE";
	else if (auto_lang == "th")
	  region_lang = "th-TH";
	else if (auto_lang == "uk")
	  region_lang = "uk-UA";
	else if (auto_lang == "zh")
	  region_lang = "zh-CN";
	else if (auto_lang == "ko")
	  region_lang = "ko-KR";
	else if (auto_lang == "ar")
	  region_lang = "ar-EG"; // Egypt, with _NO_ reasons. In most cases, the search engines will decide based on the 'ar' code.
	else if (auto_lang == "be")
	  region_lang = "be-BY";
	else if (auto_lang == "bg")
	  region_lang = "bg-BG";
	else if (auto_lang == "bs")
	  region_lang = "bs-BA";
	else if (auto_lang == "cs")
	  region_lang = "cs-CZ";
	else if (auto_lang == "fi")
	  region_lang = "fi-FI";
	else if (auto_lang == "he")
	  region_lang = "he-IL";
	else if (auto_lang == "hi")
	  region_lang = "hi-IN";
	else if (auto_lang == "hr")
	  region_lang = "hr-HR";
	return region_lang;
     }

   std::string query_context::generate_lang_http_header() const
     {
	return "accept-language: " + _auto_lang + "," + _auto_lang_reg + ";q=0.5";
     }
   
   void query_context::in_query_command_forced_region(std::string &auto_lang,
						      std::string &region_lang)
     {
	region_lang = query_context::lang_forced_region(auto_lang);
	if (region_lang == "en-US") // in case we are on the default language.
	  auto_lang = "en";
     }
	
} /* end of namespace. */
