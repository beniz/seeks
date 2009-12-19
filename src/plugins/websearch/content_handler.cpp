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
#include "curl_mget.h"
#include "html_txt_parser.h"
#include "mrf.h"
#include "websearch.h"

#include <pthread.h>
#include <iostream>

using sp::curl_mget;
using lsh::mrf;

namespace seeks_plugins
{
   int content_handler::_mrf_step = 2;
   
   char** content_handler::fetch_snippets_content(query_context *qc,
						  const size_t &ncontents,
						  const std::vector<std::string> &urls,
						  const bool &have_outputs)
     {
	// fetch content.
	curl_mget cmg(urls.size(),3); // 3 seconds timeout.
	cmg.www_mget(urls,urls.size());  // TODO: what if we fail to connect to some sites ?
	
	char **outputs = new char*[urls.size()];
	int k = 0;
	for (size_t i=0;i<urls.size();i++)
	  {
	     if (cmg._outputs[i])
	       {
		  outputs[i] = strdup(cmg._outputs[i]);
		  k++;
		  
		  // cache output for fast user access.
		  qc->cache_url(urls[i],std::string(outputs[i]));
	       }
	  }
	if (!have_outputs || k == 0)
	  {
	     delete[] outputs;
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
	
	// threads, one per parser. TODO: beware, put a limit and run several passes.
	for (size_t i=0;i<ncontents;i++)
	  {
	     html_txt_thread_arg *args = new html_txt_thread_arg();
	     args->_output = outputs[i];
	     parser_args[i] = args;
	     
	     pthread_t ps_thread;
	     int err = pthread_create(&ps_thread,NULL, // default attribute is PTHREAD_CREATE_JOINABLE
				      (void * (*)(void *))content_handler::parse_output, args);
	     parser_threads[i] = ps_thread;
	  }
	
	// join threads.
	for (size_t i=0;i<ncontents;i++)
	  {
	     pthread_join(parser_threads[i],NULL);
	  }
	
	for (size_t i=0;i<ncontents;i++)
	  {
	     txt_outputs[i] = parser_args[i]->_txt_content;
	     delete parser_args[i];
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
   
   void content_handler::extract_features_from_snippets(query_context *qc, std::vector<std::vector<std::string>*> &tokens,
							const size_t &ncontents,
							const std::vector<std::string> &urls,
							hash_map<const char*,std::vector<uint32_t>*,hash<const char*>,eqstr> &features)
     {
	for  (size_t i=0;i<ncontents;i++)
	  {
	     std::vector<uint32_t> *vf = new std::vector<uint32_t>();
	     mrf::mrf_features(*tokens[i],*vf,1);
	     
	     //std::cerr << "[Debug]: url: " << urls[i] << " --> " << vf->size() << " features.\n";
	     
	     features.insert(std::pair<const char*,std::vector<uint32_t>*>(urls[i].c_str(),vf));
	  }
     }
   
   /* void content_handler::feature_based_scoring(query_context *qc, 
					       const hash_map<const char*,std::vector<uint32_t>*,hash<const char*>,eqstr> &features)
     {
	// compute query's features. 
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
					  const std::string &url1, const std::string &url2,
					  const double &similarity_threshold)
     {
	//std::cerr << "[Debug]: comparing urls: " << url1 << std::endl << url2 <<std::endl;
	
	// we may already have some content in cache, let's check to not fetch it more than once.
	std::vector<std::string> urls;
	std::string urlc;
	if (!qc->is_cached(url1))
	  {
	     urls.push_back(url1);
	     urlc = url2;
	  }
	if (!qc->is_cached(url2))
	  {
	     urls.push_back(url2);
	     urlc = url1;
	  }
	
	char **outputs = NULL;
	if (!urls.empty())
	  outputs = content_handler::fetch_snippets_content(qc,2,urls,true);
	else
	  {
	     outputs = new char*[2];
	     outputs[0] = strdup(qc->has_cached(url1.c_str()).c_str());
	     outputs[1] = strdup(qc->has_cached(url2.c_str()).c_str());
	     urls.push_back(url1); urls.push_back(url2);
	  }
	
	if (outputs == NULL)  // error handling.
	  return false;
	
	if (urls.size() < 2)
	  {
	     char **outputs_tmp = new char*[2];
	     outputs_tmp[0] = outputs[0];
	     outputs_tmp[1] = strdup(qc->has_cached(urlc.c_str()).c_str());
	     outputs = outputs_tmp;
	     urls.push_back(urlc);
	  }
		
	// parse content and keep text only.
	std::string *txt_contents = content_handler::parse_snippets_txt_content(2,outputs);
	delete[] outputs;
	
	// tokenize & extract features from fetched text.
	std::vector<std::string> tokens1;
	mrf::tokenize(txt_contents[0],tokens1,mrf::_default_delims);
	if (tokens1.empty())
	  return false;
	std::vector<std::string> tokens2;
	mrf::tokenize(txt_contents[1],tokens2,mrf::_default_delims);
	if (tokens2.empty())
	  return false;
	
	// quick check for similarity.
	double rad = static_cast<double>(std::min(tokens1.size(),tokens2.size()))
	  / static_cast<double>(std::max(tokens1.size(),tokens2.size()));
	if (rad < similarity_threshold)
	  return false;
	
	delete[] txt_contents;
	std::vector<std::vector<std::string>*> tokens;
	tokens.push_back(&tokens1); tokens.push_back(&tokens2);
	
	hash_map<const char*,std::vector<uint32_t>*,hash<const char*>,eqstr> features;
	content_handler::extract_features_from_snippets(qc,tokens,2,urls,features);
	
	// check for similarity.
	uint32_t common_features = 0;
	std::vector<uint32_t> *f1 = features[url1.c_str()];
	std::vector<uint32_t> *f2 = features[url2.c_str()];
	if (f1->empty() || f2->empty())
	  {
	     content_handler::delete_features(features);
	     return false;
	  }
	
	rad = mrf::radiance(*f1,*f2,common_features);
	double threshold = (common_features*common_features) 
	  / ((1.0-similarity_threshold)*static_cast<double>(f1->size()+f2->size())+mrf::_epsilon);
	bool result = false;
	if (rad >= threshold)
	  result = true;
	
	// destroy feature sets.
	content_handler::delete_features(features);

	//std::cerr << "[Debug]: comparison result: " << result << std::endl;
	
	return result;
     }
      
   void content_handler::delete_features(hash_map<const char*,std::vector<uint32_t>*,hash<const char*>,eqstr> &features)
     {
	hash_map<const char*,std::vector<uint32_t>*,hash<const char*>,eqstr>::iterator hit
	  = features.begin();
	while(hit!=features.end())
	  {
	     delete (*hit).second;
	     ++hit;
	  }
     }
   
   
} /* end of namespace. */
