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

#include "img_query_context.h"
#include "se_handler_img.h"
#include "img_websearch.h"
#include "miscutil.h"
#include "websearch.h"
#include "errlog.h"

using sp::miscutil;
using sp::errlog;

namespace seeks_plugins
{
   
   img_query_context::img_query_context()
     : query_context()
       {
       }
   
   img_query_context::img_query_context(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					const std::list<const char*> &http_headers)
     : query_context(parameters,http_headers)
       {
	  _img_engines = img_websearch::_iwconfig->_img_se_enabled; //TODO: engine selection.
       }
   
   img_query_context::~img_query_context()
     {
	unregister();
     }
   
   void img_query_context::register_qc()
     {
	img_websearch::_active_img_qcontexts.insert(std::pair<uint32_t,query_context*>(_query_hash,this));
     }
   
   void img_query_context::unregister()
     {
	if (!_registered)
	  return;
	hash_map<uint32_t,query_context*,id_hash_uint>::iterator hit;
	if ((hit = img_websearch::_active_img_qcontexts.find(_query_hash))
	    ==img_websearch::_active_img_qcontexts.end())
	  {
	     /**
	      * We should not reach here.
	      */
	     errlog::log_error(LOG_LEVEL_ERROR,"Cannot find image query context when unregistering for query %s",
			       _query.c_str());
	     return;
	  }
	else
	  {
	     img_websearch::_active_img_qcontexts.erase(hit);  // deletion is controlled elsewhere.
	     _registered = false;
	  }
     }
      
   sp_err img_query_context::generate(client_state *csp,
				      http_response *rsp,
				      const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
				      bool &expanded)
     {
	const char *expansion = miscutil::lookup(parameters,"expansion");
	int horizon = atoi(expansion);
	        
	if (horizon > websearch::_wconfig->_max_expansions) // max expansion protection.
	  horizon = websearch::_wconfig->_max_expansions;
	
	// seeks button used as a back button.
	if (_page_expansion > 0 && horizon < (int)_page_expansion)
	  {  
	     // reset expansion parameter.
	     query_context::update_parameters(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters));
	     expanded = false;
	     return SP_ERR_OK;
	  }
	
	// grab requested engines, if any.
	// if the list is not included in that of the context, update existing results and perform requested expansion.
	// if the list is included in that of the context, perform expansion, results will be filtered later on.
	const char *eng = miscutil::lookup(parameters,"engines");
	if (eng)
	  {
	     // test inclusion.
	     std::bitset<IMG_NSEs> beng;
	     img_query_context::fillup_img_engines(parameters,beng);
	     std::bitset<IMG_NSEs> inc = beng;
	     inc &= _img_engines;
	     
	     if (inc.count() == beng.count())
	       {
		  // included, nothing more to be done.
	       }
	     else // test intersection.
	       {
		  std::bitset<IMG_NSEs> bint;
		  for (int b=0;b<IMG_NSEs;b++)
		    {
		       if (beng[b] && !inc[b])
			 bint.set(b);
		    }
		  
		  // catch up expansion with the newly activated engines.
		  expand_img(csp,rsp,parameters,0,_page_expansion,bint);
		  _img_engines |= bint;
	       }
	  }
	
	// perform requested expansion.
	expand_img(csp,rsp,parameters,_page_expansion,horizon,_img_engines);
	expanded = true;
	
	// update horizon.
	_page_expansion = horizon;
	
	return SP_ERR_OK;	
     }

   sp_err img_query_context::expand_img(client_state *csp,
					http_response *rsp,
					const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					const int &page_start, const int &page_end,
					const std::bitset<IMG_NSEs> &se_enabled)
     {
	for (int i=page_start;i<page_end;i++) // catches up with requested horizon.
	  {
	     // resets expansion parameter.
	     miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"expansion");
	     std::string i_str = miscutil::to_string(i+1);
	     miscutil::add_map_entry(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),
				     "expansion",1,i_str.c_str(),1);
	     
	     // query SEs.
	     int nresults = 0;
	     std::string **outputs = se_handler_img::query_to_ses(parameters,nresults,this,se_enabled);
	     
	     // test for failed connection to the SEs comes here.
	     if (!outputs)
	       {
		  return websearch::failed_ses_connect(csp,rsp);
	       }
	     
	     // parse the output and create result search snippets.
	     int rank_offset = (i > 0) ? i * websearch::_wconfig->_N : 0;
	     
	     se_handler_img::parse_ses_output(outputs,nresults,_cached_snippets,rank_offset,this,se_enabled);
	     for (int j=0;j<nresults;j++)
	       if (outputs[j])
		 delete outputs[j];
	     delete[] outputs;
	  }
	return SP_ERR_OK;
     }

   void img_query_context::fillup_img_engines(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					      std::bitset<IMG_NSEs> &engines)
     {
	const char *eng = miscutil::lookup(parameters,"engines");
	if (eng)
	  {
	     std::string engines_str = std::string(eng);
	     std::vector<std::string> vec_engines;
	     miscutil::tokenize(engines_str,vec_engines,",");
	     std::sort(vec_engines.begin(),vec_engines.end(),std::less<std::string>());
	     se_handler_img::set_engines(engines,vec_engines);
	  }
	else engines = img_websearch_configuration::_img_wconfig->_img_se_enabled;
     }
      
} /* end of namespace. */
  
