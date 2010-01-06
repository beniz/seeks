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

#include "content_handler.h"
#include "mem_utils.h"
#include "curl_mget.h"
#include "html_txt_parser.h"
#include "websearch.h"

#include <pthread.h>
#include <iostream>

#include <assert.h>

using sp::curl_mget;

namespace seeks_plugins
{
   std::string feature_thread_arg::_delims = mrf::_default_delims;
   int feature_thread_arg::_radius = 4;
   int feature_thread_arg::_step = 1;
   uint32_t feature_thread_arg::_window_length=5;
   
   char** content_handler::fetch_snippets_content(query_context *qc,
						  const std::vector<std::string> &urls)
     {
	// just in case.
	if (urls.empty())
	  return NULL;
	
	// fetch content.
	curl_mget cmg(urls.size(),1,0,3,0); // 1 second connect timeout, 3 seconds transfer timeout.
	cmg.www_mget(urls,urls.size(),true);
	
	char **outputs = (char**)std::malloc(urls.size()*sizeof(char*));
	int k = 0;
	for (size_t i=0;i<urls.size();i++)
	  {
	     outputs[i] = NULL;
	     if (cmg._outputs[i])
	       {
		  outputs[i] = cmg._outputs[i];
		  k++;
	       }
	     else outputs[i] = NULL;
	  }
	if (k == 0)
	  {
	     freez(outputs); // beware.
	     outputs = NULL;
	  }
	return outputs;
     }
   
   std::string* content_handler::parse_snippets_txt_content(const size_t &ncontents,
							    char **outputs)
     {
	std::string *txt_outputs = new std::string[ncontents];
	
	pthread_t parser_threads[ncontents];
	html_txt_thread_arg* parser_args[ncontents];
	
	// threads, one per parser.
	for (size_t i=0;i<ncontents;i++)
	  {
	     if (outputs[i])
	       {
		  html_txt_thread_arg *args = new html_txt_thread_arg();
		  args->_output = outputs[i];
		  parser_args[i] = args;
		  
		  pthread_t ps_thread;
		  int err = pthread_create(&ps_thread,NULL, // default attribute is PTHREAD_CREATE_JOINABLE
					   (void * (*)(void *))content_handler::parse_output, args);
		  parser_threads[i] = ps_thread;
	       }
	     else 
	       {
		  parser_threads[i] = 0;
		  parser_args[i] = NULL;
	       }
	  }
		
	// join threads.
	for (size_t i=0;i<ncontents;i++)
	  {
	     if (parser_threads[i] != 0)
	       pthread_join(parser_threads[i],NULL);
	  }
	
	for (size_t i=0;i<ncontents;i++)
	  {
	     if (parser_threads[i] != 0)
	       {
		  txt_outputs[i] = parser_args[i]->_txt_content;
		  delete parser_args[i];
	       }
	  }
	
	return txt_outputs;
     }
   
   void content_handler::parse_output(html_txt_thread_arg &args)
     {
	html_txt_parser *txt_parser = new html_txt_parser();
	txt_parser->parse_output(args._output,NULL,0);
	args._txt_content = txt_parser->_txt;
	delete txt_parser;
     }
   
   void content_handler::extract_features_from_snippets(query_context *qc,
							std::string *txt_contents,
							const size_t &ncontents,
							search_snippet **sps)
     {
	pthread_t feature_threads[ncontents];
	feature_thread_arg* feature_args[ncontents];
	
	// TODO: limits the number of threads.
	for  (size_t i=0;i<ncontents;i++)
	  {
	     std::vector<uint32_t> *vf = sps[i]->_features;
	     if (!vf)
	       {
		  vf = new std::vector<uint32_t>();
		  feature_thread_arg *args = new feature_thread_arg(&txt_contents[i],vf);
		  feature_args[i] = args;
	
		  pthread_t f_thread;
		  int err = pthread_create(&f_thread,NULL, // default attribute is PTHREAD_CREATE_JOINABLE
					   (void*(*)(void*))content_handler::generate_features,args);
		  feature_threads[i] = f_thread;
	       }
	     else 
	       {
		  feature_threads[i] = 0;
		  feature_args[i] = NULL;
	       }
	  }
	
	// join threads.
	for (size_t i=0;i<ncontents;i++)
	  {
	     if (feature_threads[i] != 0)
	       pthread_join(feature_threads[i],NULL);
	  }
	
	for (size_t i=0;i<ncontents;i++)
	  {
	     if (feature_threads[i] != 0)
	       {
		  sps[i]->_features = feature_args[i]->_vf;
		  //std::cerr << "[Debug]: url: " << sps[i]->_url << " --> " << sps[i]->_features->size() << " features.\n";
		  delete feature_args[i];
	       }
	  }
     }
   
   void content_handler::generate_features(feature_thread_arg &args)
     {
	mrf::tokenize_and_mrf_features(*args._txt_content,feature_thread_arg::_delims,*args._vf,
				       feature_thread_arg::_radius,feature_thread_arg::_step,
				       feature_thread_arg::_window_length);
     }
      
   void content_handler::feature_based_similarity_scoring(query_context *qc,
							  const size_t &nsps,
							  search_snippet **sps,
							  const char *url)
     {
	search_snippet *sp = qc->get_cached_snippet(url);
	if (!sp)
	  {
	     return; // we should never reach here.
	  }
	// reference features.
	std::vector<uint32_t> *ref_features = sp->_features;
	if (!ref_features) // sometimes the content wasn't fetched, and features are not there.
	  return; 
	
	// compute scores.
	for (size_t i=0;i<nsps;i++)
	  {
	     if (sps[i]->_features)
	       {
		  uint32_t common_features = 0;
		  sps[i]->_seeks_ir = mrf::radiance(*ref_features,*sps[i]->_features,common_features);
		  
		  std::cerr << "[Debug]: url: " << sps[i]->_url 
		    << " -- score: " << sps[i]->_seeks_ir 
		    << " -- common features: " << common_features << std::endl;
	       }
	  }
     }   
   
   /* void content_handler::feature_based_scoring(query_context *qc, 
					       search_snippet **sps,
					       const char *str)
     {
	// compute str's features. 
	// TODO: with collaboration enabled this could go / be fetched from the query context.
	std::vector<uint32_t> query_features;
	mrf::mrf_features(qc->_query,query_features,content_handler::_mrf_horizon);
	
	// compute scores.
	hash_map<const char*,std::vector<uint32_t>*,hash<const char*>,eqstr>::const_iterator hit 
	  = features.begin();
	while(hit!=features.end())
	  {
	     // TODO: check whether we do have features... just in case.

	     uint32_t common_features = 0;
	     double score = mrf::radiance(query_features,*(*hit).second,common_features);
	     
	     // update query context's snippets' ranks.
	     // TODO: delete snippets whose feature-based rank is 0 ?
	     qc->update_snippet_seeks_rank((*hit).first,score);
	     
	     ++hit;
	  }
     } */
   
   bool content_handler::has_same_content(query_context *qc, 
					  search_snippet *sp1, search_snippet *sp2,
					  const double &similarity_threshold)
     {
	static std::string token_delims = " \t\n";
	
	// we may already have some content in cache, let's check to not fetch it more than once.
	std::string url1 = sp1->_url;
	std::string url2 = sp2->_url;
	std::vector<std::string> urls;
	urls.reserve(2);
	
	const char *content1 = sp1->_cached_content;
	const char *content2 = sp2->_cached_content;
	
	char **outputs = NULL;
	if (!content1 && !content2)
	  {
	     urls.push_back(url1); urls.push_back(url2);
	     outputs = content_handler::fetch_snippets_content(qc,urls);
	     if (outputs)
	       {
		  sp1->_cached_content = outputs[0];
		  sp2->_cached_content = outputs[1];
	       }
	  }
	else if (!content1)
	  {
	     outputs = (char**)malloc(2*sizeof(char*));
	     urls.push_back(url1);
	     char **output1 = content_handler::fetch_snippets_content(qc,urls);
	     
	     if (output1)
	       {
		  outputs[0] = *output1;
		  free(output1);
		  outputs[1] = (char*)content2;
		  urls.push_back(url2);
		  sp1->_cached_content = outputs[0];
	       }
	  }
	else if (!content2)
	  {
	     outputs = (char**)malloc(2*sizeof(char*));
	     urls.push_back(url2);
	     char **output2 = content_handler::fetch_snippets_content(qc,urls);
	     outputs[0] = (char*)content1;
	     if (output2)
	       {
		  outputs[1] = *output2;
		  free(output2);
		  urls.push_back(url1);
		  sp2->_cached_content = outputs[1];
	       }
	  }
			
	if (outputs == NULL
	    || outputs[0] == NULL || outputs[1] == NULL)  // error handling.
	  {
	     // cleanup.
	     if (outputs)
	       free(outputs);
	     return false;
	  }
		
	// parse content and keep text only.
	std::string *txt_contents = content_handler::parse_snippets_txt_content(2,outputs);
	freez(outputs); // beware.
	outputs = NULL;
	
	if (txt_contents[0].empty() || txt_contents[1].empty())
	  return false;
	
	// quick check for similarity.
	double rad = static_cast<double>(std::min(txt_contents[0].size(),txt_contents[1].size())) 
	  / static_cast<double>(std::max(txt_contents[0].size(),txt_contents[1].size()));
	if (rad < similarity_threshold)
	  {
	     return false;
	  }
	
	search_snippet* sps[2] = {sp1,sp2};
	content_handler::extract_features_from_snippets(qc,txt_contents,2,sps);
	delete[] txt_contents;
	
	// check for similarity.
	uint32_t common_features = 0;
	std::vector<uint32_t> *f1 = sp1->_features;
	std::vector<uint32_t> *f2 = sp2->_features;
	assert(f1!=NULL);assert(f2!=NULL);
	
	rad = mrf::radiance(*f1,*f2,common_features);
	double threshold = (common_features*common_features) 
	  / ((1.0+similarity_threshold)*static_cast<double>(f1->size()+f2->size()-2*common_features)+mrf::_epsilon);
	bool result = false;
	if (rad >= threshold)
	  result = true;
	
	/* std::cerr << "Radiance: " << rad << " -- threshold: " << threshold << " -- result: " << result << std::endl;
	 std::cerr << "[Debug]: comparison result: " << result << std::endl; */
	
	return result;
     }
      
} /* end of namespace. */
