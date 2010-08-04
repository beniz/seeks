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

#include "se_handler_img.h"
#include "websearch.h"
#include "curl_mget.h"
#include "miscutil.h"
#include "encode.h"
#include "errlog.h"

#include "se_parser_bing_img.h"
#include "se_parser_ggle_img.h"
#include "se_parser_flickr.h"

using namespace sp;

namespace seeks_plugins
{
   se_bing_img::se_bing_img()
     :search_engine()
       {
       }
   
   se_bing_img::~se_bing_img()
     {
     }
   
   void se_bing_img::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc)
     {
	std::string q_bing = se_handler_img::_se_strings[BING_IMG]; // query to bing.
	const char *query = miscutil::lookup(parameters,"q");
	
	// query.
	int p = 36;
	char *qenc = encode::url_encode(se_handler::no_command_query(std::string(query)).c_str());
	std::string qenc_str = std::string(qenc);
	free(qenc);
	q_bing.replace(p,6,qenc_str);
	
	// page.
	const char *expansion = miscutil::lookup(parameters,"expansion");
	int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * img_websearch_configuration::_img_wconfig->_N : 0;
	std::string pp_str = miscutil::to_string(pp);
	miscutil::replace_in_string(q_bing,"%start",pp_str);
	
	// language.
	miscutil::replace_in_string(q_bing,"%lang",qc->_auto_lang_reg);
	
	// log the query.
	errlog::log_error(LOG_LEVEL_INFO, "Querying bing: %s", q_bing.c_str());
	                  
	url = q_bing;
     }
   
   se_ggle_img::se_ggle_img()
     :search_engine()
       {
       }
   
   se_ggle_img::~se_ggle_img()
     {
     }
   
   void se_ggle_img::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				 std::string &url, const query_context *qc)
     {
	std::string q_ggle = se_handler_img::_se_strings[GOOGLE_IMG]; // query to ggle.
	const char *query = miscutil::lookup(parameters,"q");
     
	// query.
	int p = 31;
	char *qenc = encode::url_encode(se_handler::no_command_query(std::string(query)).c_str());
	std::string qenc_str = std::string(qenc);
	free(qenc);
	q_ggle.replace(p,6,qenc_str);
	
	// expansion = result page called...
	const char *expansion = miscutil::lookup(parameters,"expansion");
	int pp = (strcmp(expansion,"")!=0) ? (atoi(expansion)-1) * img_websearch_configuration::_img_wconfig->_N : 0;
	std::string pp_str = miscutil::to_string(pp);
	miscutil::replace_in_string(q_ggle,"%start",pp_str);
	
	// number of results.
	int num = img_websearch_configuration::_img_wconfig->_N; // by default.
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

   se_flickr::se_flickr()
     :search_engine()
       {
       }
   
   se_flickr::~se_flickr()
     {
     }
   
   void se_flickr::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
			       std::string &url, const query_context *qc)
     {
	std::string q_fl = se_handler_img::_se_strings[FLICKR]; // query to flickr.
	const char *query = miscutil::lookup(parameters,"q");
	
	// query.
	int p = 32;
	char *qenc = encode::url_encode(se_handler::no_command_query(std::string(query)).c_str());
	std::string qenc_str = std::string(qenc);
	free(qenc);
	q_fl.replace(p,6,qenc_str);
	
	// expansion = page requested.
	const char *expansion = miscutil::lookup(parameters,"expansion");
	std::string pp_str = expansion;
	miscutil::replace_in_string(q_fl,"%start",pp_str);
	
	// XXX: could try to limit the number of results.
	
	// log the query.
	errlog::log_error(LOG_LEVEL_INFO, "Querying flickr: %s", q_fl.c_str());
	
	url = q_fl;
     }
      
   /*- se_handler_img -*/
   std::string se_handler_img::_se_strings[IMG_NSEs] =  // in alphabetical order.
     {
	// bing: www.bing.com/images/search?q=markov+chain&go=&form=QBIR
	"http://www.bing.com/images/search?q=%query&first=%start&mkt=%lang",
	// flickr: www.flickr.com/search/?q=markov+chain&z=e
	"http://www.flickr.com/search/?q=%query&page=%start",
	// ggle: www.google.com/images?q=markov+chain&hl=en&safe=off&prmd=mi&source=lnms&tbs=isch:1&sa=X&oi=mode_link&ct=mode
     	"http://www.google.com/images?q=%query&start=%start&num=%num&hl=%lang&ie=%encoding&oe=%encoding"
     };
   
   se_bing_img se_handler_img::_img_bing = se_bing_img();
   se_ggle_img se_handler_img::_img_ggle = se_ggle_img();
   se_flickr se_handler_img::_img_flickr = se_flickr();
   
   /*-- queries to the image search engines. --*/
   std::string** se_handler_img::query_to_ses(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
					      int &nresults, const query_context *qc, const std::bitset<IMG_NSEs> &se_enabled)
     {
	std::vector<std::string> urls;
	urls.reserve(IMG_NSEs);
	std::vector<std::list<const char*>*> headers;
	headers.reserve(IMG_NSEs);
	
	// enabling of SEs.
	for (int i=0;i<IMG_NSEs;i++)
	  {
	     if (se_enabled[i])
	       {
		  std::string url;
		  std::list<const char*> *lheaders = NULL;
		  se_handler_img::query_to_se(parameters,(IMG_SE)i,url,qc,lheaders);
		  urls.push_back(url);
		  headers.push_back(lheaders);
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
		      websearch::_wconfig->_se_connect_timeout,0);
	if (websearch::_wconfig->_background_proxy_addr.empty())
	  cmg.www_mget(urls,urls.size(),&headers,"",0); // don't go through the seeks' proxy, or will loop til death!
	else cmg.www_mget(urls,urls.size(),&headers,
			  websearch::_wconfig->_background_proxy_addr,
			  websearch::_wconfig->_background_proxy_port);
			
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
	     	     
	     // delete headers, if any.
	     if (headers.at(i))
	       {
		  miscutil::list_remove_all(headers.at(i));
		  delete headers.at(i);
	       }
	  }
	//        
	if (!have_outputs)
	  {
	     delete[] outputs;
	     outputs = NULL;
	  }
	
	return outputs;
     }
   
   void se_handler_img::query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
				    const IMG_SE &se, std::string &url, const query_context *qc,
				    std::list<const char*> *&lheaders)
     {
	lheaders = new std::list<const char*>();
	
	/* pass the user-agent header. */
	std::list<const char*>::const_iterator sit = qc->_useful_http_headers.begin();
	while(sit!=qc->_useful_http_headers.end())
	  {
	     if (se == GOOGLE_IMG && miscutil::strncmpic((*sit),"user-agent:",11) == 0)
	       {
		  // XXX: ggle seems to render queries based on the user agent.
	       }
	     else lheaders->push_back(strdup((*sit)));
	     ++sit;
	  }
	
	switch(se)
	  {
	   case GOOGLE_IMG:
	     _img_ggle.query_to_se(parameters,url,qc);
	     break;
	   case BING_IMG:
	     _img_bing.query_to_se(parameters,url,qc);
	     break;
	   case FLICKR:
	     _img_flickr.query_to_se(parameters,url,qc);
	     break;
	  }
     }

   void se_handler_img::set_engines(std::bitset<IMG_NSEs> &se_enabled, const std::vector<std::string> &ses)
     {
	int msize = std::min((int)ses.size(),IMG_NSEs);
	for (int i=0;i<msize;i++)
	  {
	     std::string se = ses.at(i);
	     
	     /* put engine name into lower cases. */
	     std::transform(se.begin(),se.end(),se.begin(),tolower);
	     
	     if (se == "google")
	       {
		  se_enabled |= std::bitset<IMG_NSEs>(SE_GOOGLE_IMG);
	       }
	     else if (se == "bing")
	       {
		  se_enabled |= std::bitset<IMG_NSEs>(SE_BING_IMG);
	       }
	     else if (se == "flickr")
	       {
		  se_enabled |= std::bitset<IMG_NSEs>(SE_FLICKR);
	       }
	     // XXX: other engines come here.
	  }
     }
      
   sp_err se_handler_img::parse_ses_output(std::string **outputs, const int &nresults,
					   std::vector<search_snippet*> &snippets,
					   const int &count_offset,
					   query_context *qr,
					   const std::bitset<IMG_NSEs> &se_enabled)
     {
	int j = 0;
	size_t active_ses = se_enabled.count();
	pthread_t parser_threads[active_ses];
	ps_thread_arg* parser_args[active_ses];
	for (size_t i=0;i<active_ses;i++)
	  parser_args[i] = NULL;
	
	// threads, one per parser.
	int k = 0;
	for (int i=0;i<IMG_NSEs;i++)
	  {
	     if (se_enabled[i])
	       {
		  if (outputs[j])
		    {
		       ps_thread_arg *args = new ps_thread_arg();
		       args->_se = (IMG_SE)i;
		       args->_output = (char*) outputs[j]->c_str();  // XXX: sad cast.
		       args->_snippets = new std::vector<search_snippet*>();
		       args->_offset = count_offset;
		       args->_qr = qr;
		       parser_args[k] = args;
		       
		       pthread_t ps_thread;
		       int err = pthread_create(&ps_thread, NULL,  // default attribute is PTHREAD_CREATE_JOINABLE
						(void * (*)(void *))se_handler_img::parse_output, args);
		       if (err != 0)
			 {
			    errlog::log_error(LOG_LEVEL_ERROR, "Error creating parser thread.");
			    parser_threads[k++] = 0;
			    delete args;
			    parser_args[k] = NULL;
			    continue;
			 }
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
	return SP_ERR_OK;
     }
   
   void se_handler_img::parse_output(const ps_thread_arg &args)
     {
	se_parser *se = se_handler_img::create_se_parser((IMG_SE)args._se);
	if ((IMG_SE)args._se == BING_IMG)
	  se->parse_output_xml(args._output,args._snippets,args._offset);
	else se->parse_output(args._output,args._snippets,args._offset);
	
	// link the snippets to the query context
	// and post-process them.
	for (size_t i=0;i<args._snippets->size();i++)
	  {
	     args._snippets->at(i)->_qc = args._qr;
	     args._snippets->at(i)->tag();
	  }
	delete se;
     }
   
   se_parser* se_handler_img::create_se_parser(const IMG_SE &se)
     {
	se_parser *sep = NULL;
	switch(se)
	  {
	   case GOOGLE_IMG:
	     sep = new se_parser_ggle_img();
	     break;
	   case BING_IMG:
	     sep = new se_parser_bing_img();
	     break;
	   case FLICKR:
	     sep = new se_parser_flickr();
	     break;
	  }
	return sep;
     }
      
} /* end of namespace. */
