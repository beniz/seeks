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

#include "search_snippet.h"
#include "websearch.h" // for configuration.
#include "mem_utils.h"
#include "miscutil.h"
#include "encode.h"
#include "loaders.h"
#include "urlmatch.h"
#include "plugin_manager.h" // for _plugin_repository.
#include "seeks_proxy.h" // for _datadir.

#ifndef FEATURE_EXTENDED_HOST_PATTERNS
#include "proxy_dts.h" // for http_request.
#endif

#include "mrf.h"

#include <iostream>

using sp::miscutil;
using sp::encode;
using sp::loaders;
using sp::urlmatch;
using sp::plugin_manager;
using sp::seeks_proxy;
using sp::http_request;
using lsh::mrf;

namespace seeks_plugins
{
   // loaded tagging patterns.
   std::vector<url_spec*> search_snippet::_pdf_pos_patterns = std::vector<url_spec*>();
   std::vector<url_spec*> search_snippet::_file_doc_pos_patterns = std::vector<url_spec*>();
   std::vector<url_spec*> search_snippet::_audio_pos_patterns = std::vector<url_spec*>();
   std::vector<url_spec*> search_snippet::_video_pos_patterns = std::vector<url_spec*>();
   std::vector<url_spec*> search_snippet::_forum_pos_patterns = std::vector<url_spec*>();
   std::vector<url_spec*> search_snippet::_reject_pos_patterns = std::vector<url_spec*>();
   
   search_snippet::search_snippet()
     :_qc(NULL),_new(true),_id(0),_sim_back(false),_rank(0),_seeks_ir(0.0),_seeks_rank(0),_doc_type(WEBPAGE),
      _cached_content(NULL),_features(NULL),_features_tfidf(NULL),_bag_of_words(NULL),_safe(true)
       {
       }
   
   search_snippet::search_snippet(const short &rank)
     :_qc(NULL),_new(true),_id(0),_sim_back(false),_rank(rank),_seeks_ir(0.0),_seeks_rank(0),_doc_type(WEBPAGE),
      _cached_content(NULL),_features(NULL),_features_tfidf(NULL),_bag_of_words(NULL),_safe(true)
       {
       }
      
   search_snippet::~search_snippet()
     {
	if (_cached_content)
	  free_const(_cached_content);
	if (_features)
	  delete _features;
	if (_features_tfidf)
	  delete _features_tfidf;
	if (_bag_of_words)
	  delete _bag_of_words;
     }

   void search_snippet::highlight_query(std::vector<std::string> &words,
					std::string &str)
     {
	if (words.empty())
	  return;
	
	// sort words by size.
	std::sort(words.begin(),words.end(),std::greater<std::string>());
	
	// surround every of those words appearing within the
	// argument string with <b> </b> for html
	// bold format. TODO: green ?
	for (size_t i=0;i<words.size();i++)
	  {
	     if (words.at(i).length() > 2)
	       {
		  std::string bold_str = "<b>" + words.at(i) + "</b>";
		  miscutil::ci_replace_in_string(str,words.at(i),bold_str);
	       }
	  }
     }
      
   void search_snippet::highlight_discr(std::string &str, const std::string &base_url_str,
					const std::vector<std::string> &query_words)
     {
	static int max_highlights = 3; // ad-hoc default.
	
	if (!_features_tfidf)
	  return;
	
	std::vector<std::string> words;
	words.reserve(max_highlights);
	std::map<float,uint32_t,std::greater<float> > f_tfidf;
	
	// sort features in decreasing tf-idf order.
	hash_map<uint32_t,float,id_hash_uint>::const_iterator fit
	  = _features_tfidf->begin();
	while(fit!=_features_tfidf->end())
	  {
	     f_tfidf.insert(std::pair<float,uint32_t>((*fit).second,(*fit).first));
	     ++fit;
	  }
	
	size_t nqw = query_words.size();
	int i = 0;
	std::map<float,uint32_t,std::greater<float> >::const_iterator mit = f_tfidf.begin();
	while(mit!=f_tfidf.end())
	  {
	     hash_map<uint32_t,std::string,id_hash_uint>::const_iterator bit;
	     if ((bit=_bag_of_words->find((*mit).second))!=_bag_of_words->end())
	       {
		  bool add = true;
		  for (size_t j=0;j<nqw;j++)
		    if (query_words.at(j) == (*bit).second)
		      add = false;
		  if (add)
		    words.push_back((*bit).second);
		  i++;
		  if (i>=max_highlights)
		    break;
	       }
	     ++mit;
	  }
	
	if (words.empty())
	  return;
	
	// sort words by size.
	std::sort(words.begin(),words.end(),std::greater<std::string>());
	
	// highlighting.
	for (size_t i=0;i<words.size();i++)
	  {
	     if (words.at(i).length() > 2)
	       {
		  char *wenc = encode::url_encode(words.at(i).c_str());
		  std::string rword = " " + words.at(i) + " ";
		  std::string bold_str = "<span class=\"highlight\"><a href=\"" + base_url_str + "/search?q=" + _qc->_url_enc_query + "+" + std::string(wenc) + "&page=1&expansion=1&action=expand\">" + rword + "</a></span>";
		  free(wenc);
		  miscutil::ci_replace_in_string(str,rword,bold_str);
	       }
	  }
     }
      
   std::ostream& search_snippet::print(std::ostream &output)
     {
	output << "-----------------------------------\n";
	output << "- seeks rank: " << _seeks_rank << std::endl;
	output << "- rank: " << _rank << std::endl;
	output << "- title: " << _title << std::endl;
	output << "- url: " << _url << std::endl;
	output << "- cite: " << _cite << std::endl;
	output << "- cached: " << _cached << std::endl;
	output << "- summary: " << _summary << std::endl;
	output << "- file format: " << _file_format << std::endl;
	output << "- date: " << _date << std::endl;
	output << "- lang: " << _lang << std::endl;
	if (_doc_type == FORUM)
	  output << "- forum thread info: " << _forum_thread_info << std::endl;
	output << "-----------------------------------\n";
	
	return output;
     }
     
   std::string search_snippet::to_json(const bool &thumbs)
     {
	std::string json_str;
	json_str += "{";
	json_str += "\"id\":" + miscutil::to_string(_id) + ",";
	std::string title = _title;
	miscutil::replace_in_string(title,"\\","");
	miscutil::replace_in_string(title,"\"","\\\"");
	json_str += "\"title\":\"" + title + "\",";
	std::string url = _url;
	miscutil::replace_in_string(url,"\"","\\\"");
	json_str += "\"url\":\"" + url + "\",";
	std::string summary = _summary_noenc;
	miscutil::replace_in_string(title,"\\","");
	miscutil::replace_in_string(summary,"\"","\\\"");
	json_str += "\"summary\":\"" + summary + "\",";
	json_str += "\"seeks_score\":" + miscutil::to_string(_seeks_rank) + ",";
	double rank = _rank / static_cast<double>(_engine.count());
	json_str += "\"rank\":" + miscutil::to_string(rank) + ",";
	json_str += "\"cite\":\"";
	if (!_cite.empty())
	  {
	     std::string cite = _cite;
	     miscutil::replace_in_string(cite,"\"","\\\"");
	     json_str += cite + "\",";
	  }
	else json_str += url + "\",";
	if (!_cached.empty())
	  {
	     std::string cached = _cached;
	     miscutil::replace_in_string(cached,"\"","\\\"");
	     json_str += "\"cached\":\"" + cached + "\","; // XXX: cached might be malformed without preprocessing.
	  }
	if (_archive.empty())
	  set_archive_link();
	std::string archive = _archive;
	miscutil::replace_in_string(archive,"\"","\\\"");
	json_str += "\"archive\":\"" + archive + "\",";
	json_str += "\"engines\":[";
	std::string json_str_eng = "";
	if (_engine.to_ulong()&SE_GOOGLE)
	  json_str_eng += "\"google\"";
	if (_engine.to_ulong()&SE_CUIL)
	  {
	     if (!json_str_eng.empty())
	       json_str_eng += ",";
	     json_str_eng += "\"cuil\"";
	  }
	if (_engine.to_ulong()&SE_BING)
	  {
	     if (!json_str_eng.empty())
	       json_str_eng += ",";
	     json_str_eng += "\"bing\"";
	  }
	if (_engine.to_ulong()&SE_YAHOO)
	  {
	     if (!json_str_eng.empty())
	       json_str_eng += ",";
	     json_str_eng += "\"yahoo\"";
	  }
	if (_engine.to_ulong()&SE_EXALEAD)
	  {
	     if (!json_str_eng.empty())
	       json_str_eng += ",";
	     json_str_eng += "\"exalead\"";
	  }
	json_str += json_str_eng + "]";
	if (thumbs)
	  json_str += ",\"thumb\":\"http://open.thumbshots.org/image.pxf?url=" + url + "\"";
	json_str += "}";
	return json_str;
     }
      
   std::string search_snippet::to_html()
     {
	std::vector<std::string> words;
	std::string base_url_str;
	return to_html_with_highlight(words,base_url_str);
     }

   std::string search_snippet::to_html_with_highlight(std::vector<std::string> &words,
						      const std::string &base_url_str)
     {
	std::string se_icon = "<span class=\"search_engine icon\" title=\"setitle\"><a href=\"" + base_url_str + "/search?q=" + _qc->_url_enc_query + "&page=1&expansion=1&action=expand&engines=seeng\">&nbsp;</a></span>";
	std::string html_content = "<li class=\"search_snippet\"";
/*	if ( websearch::_wconfig->_thumbs )	
		html_content += " onmouseover=\"snippet_focus(this, 'on');\" onmouseout=\"snippet_focus(this, 'off');\""; */
	html_content += ">";
	if ( websearch::_wconfig->_thumbs ){
		html_content += "<img class=\"preview\" src=\"http://open.thumbshots.org/image.pxf?url=";
		html_content += _url;
		html_content += "\" />";
	}
	html_content += "<h3><a href=\"";
	html_content += _url;
	html_content += "\">";
	
	const char *title_enc = encode::html_encode(_title.c_str());
	html_content += title_enc;
	free_const(title_enc);
	html_content += "</a>";
	
	if (_engine.to_ulong()&SE_GOOGLE)
	  {
	     std::string ggle_se_icon = se_icon;
	     miscutil::replace_in_string(ggle_se_icon,"icon","search_engine_google");
	     miscutil::replace_in_string(ggle_se_icon,"setitle","Google");
	     miscutil::replace_in_string(ggle_se_icon,"seeng","google");
	     html_content += ggle_se_icon;
	  }
	if (_engine.to_ulong()&SE_CUIL)
	  {
	     std::string cuil_se_icon = se_icon;
	     miscutil::replace_in_string(cuil_se_icon,"icon","search_engine_cuil");
	     miscutil::replace_in_string(cuil_se_icon,"setitle","Cuil");
	     miscutil::replace_in_string(cuil_se_icon,"seeng","cuil");
	     html_content += cuil_se_icon;
	  }
	if (_engine.to_ulong()&SE_BING)
	  {
	     std::string bing_se_icon = se_icon;
	     miscutil::replace_in_string(bing_se_icon,"icon","search_engine_bing");
	     miscutil::replace_in_string(bing_se_icon,"setitle","Bing");
	     miscutil::replace_in_string(bing_se_icon,"seeng","bing");
	     html_content += bing_se_icon;
	  }
	if (_engine.to_ulong()&SE_YAHOO)
	  {
	     std::string yahoo_se_icon = se_icon;
	     miscutil::replace_in_string(yahoo_se_icon,"icon","search_engine_yahoo");
	     miscutil::replace_in_string(yahoo_se_icon,"setitle","Yahoo!");
	     miscutil::replace_in_string(yahoo_se_icon,"seeng","yahoo");
	     html_content += yahoo_se_icon;
	  }
	if (_engine.to_ulong()&SE_EXALEAD)
	  {
	     std::string exalead_se_icon = se_icon;
	     miscutil::replace_in_string(exalead_se_icon,"icon","search_engine_exalead");
	     miscutil::replace_in_string(exalead_se_icon,"setitle","Exalead");
	     miscutil::replace_in_string(exalead_se_icon,"seeng","exalead");
	     html_content += exalead_se_icon;
	  }
			
	html_content += "</h3>";
		
	if (!_summary.empty())
	  {
	     html_content += "<div>";
	     std::string summary = _summary;
	     search_snippet::highlight_query(words,summary);
	     if (websearch::_wconfig->_extended_highlight)
	       highlight_discr(summary,base_url_str,words);
	     html_content += summary;
	  }
	else html_content += "<div>";
	
	const char *cite_enc = NULL;
	if (!_cite.empty())
	  {
	     cite_enc = encode::html_encode(_cite.c_str());
	  }
	else 
	  {
	     cite_enc = encode::html_encode(_url.c_str());
	  }
	if (!_summary.empty())
	  html_content += "<br>";
	html_content += "<cite>";
	html_content += cite_enc;
	free_const(cite_enc);
	html_content += "</cite>\n";
	
	if (!_cached.empty())
	  {
	     char *enc_cached = encode::html_encode(_cached.c_str());
	     miscutil::chomp(enc_cached);
	     html_content += "<a class=\"search_cache\" href=\"";
	     html_content += enc_cached;
	     html_content += "\">Cached</a>";
	     free_const(enc_cached);
	  }
	if (_archive.empty())
	  {
	     set_archive_link();  
	  }
	html_content += "<a class=\"search_cache\" href=\"";
	html_content += _archive;
	html_content += "\">Archive</a>";
	
	if (!_sim_back)
	  {
	     set_similarity_link();
	     html_content += "<a class=\"search_cache\" href=\"";
	  }
	else
	  {
	     set_back_similarity_link();
	     html_content += "<a class=\"search_similarity\" href=\"";
	  }
	html_content += base_url_str + _sim_link;
	if (!_sim_back)
	  html_content += "\">Similar</a>";
	else html_content += "\">Back</a>";
	
	if (_cached_content)
	  {
	     html_content += "<a class=\"search_cache\" href=\"";
	     html_content += base_url_str + "/search_cache?url="
	                  + _url + "&amp;q=" + _qc->_query;
	     html_content += " \">Quick link</a>";
	  }
	
	html_content += "</div></li>\n";
		
	/* std::cout << "html_content:\n";
	 std::cout << html_content << std::endl; */
		
	return html_content;
     }

   bool search_snippet::is_se_enabled(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
     {
	std::bitset<NSEs> se_enabled;
	query_context::fillup_engines(parameters,se_enabled);
	std::bitset<NSEs> band = _engine & se_enabled;
	return (band.count() != 0);
     }
      
   void search_snippet::set_url(const std::string &url)
     {
	char *url_str = encode::url_decode(url.c_str());
	_url = std::string(url_str);
	free(url_str);
	std::string surl = urlmatch::strip_url(_url);
	_id = mrf::mrf_single_feature(surl,"");
     }
   
   void search_snippet::set_url(const char *url)
     {
	char *url_dec = encode::url_decode(url);
	_url = std::string(url_dec);
	free(url_dec);
	std::string surl = urlmatch::strip_url(_url);
	_id = mrf::mrf_single_feature(surl,"");
     }
   
   void search_snippet::set_url_no_decode(const std::string &url)
     {
	_url = url;
	std::string surl = urlmatch::strip_url(_url);
	_id = mrf::mrf_single_feature(surl,"");
     }
      
   void search_snippet::set_cite(const std::string &cite)
     {
	char *cite_dec = encode::url_decode(cite.c_str());
	std::string citer = std::string(cite_dec);
	free(cite_dec);
	static size_t cite_max_size = 60;
	std::string host, path;
	urlmatch::parse_url_host_and_path(citer,host,path);
	_cite = host + path;
	if (_cite.length()>cite_max_size)
	  {
	     try
	       {
		  _cite.substr(0,cite_max_size-3) + "...";
	       }
	     catch(std::exception &e)
	       {
		  // do nothing.
	       }
	  }
     }
   
   void search_snippet::set_cite_no_decode(const std::string &cite)
     {
	static size_t cite_max_size = 60;
	std::string host, path;
	urlmatch::parse_url_host_and_path(cite,host,path);
	_cite = host + path;
	if (_cite.length()>cite_max_size)
	  {
	     try
	       {
		  _cite.substr(0,cite_max_size-3) + "...";
	       }
	     catch(std::exception &e)
	       {
		  // do nothing.
	       }
	  }
     }
   
   void search_snippet::set_summary(const char *summary)
     {
	static size_t summary_max_size = 240; // characters.
	_summary_noenc = std::string(summary);	
	
	// encode html so tags are not interpreted.
	char* str = encode::html_encode(summary);
	if (strlen(str)<summary_max_size)
	  _summary = std::string(str);
	else
	  {
	     try
	       {
		  _summary = std::string(str).substr(0,summary_max_size-3) + "...";
	       }
	     catch(std::exception &e)
	       {
		  _summary = "";
	       }
	  }
	free(str);
     }
      
   void search_snippet::set_summary(const std::string &summary)
     {
	_summary_noenc = summary;
	
	// encode html so tags are not interpreted.
	char* str = encode::html_encode(summary.c_str());
	_summary = std::string(str);
	free(str);
     }
   
   void search_snippet::set_archive_link()
     {
	_archive = "http://web.archive.org/web/*/" + _url;
     }
   	
   void search_snippet::set_similarity_link()
     {
	_sim_link = "/search?q=" + _qc->_url_enc_query 
	  + "&amp;page=1&amp;expansion=" + miscutil::to_string(_qc->_page_expansion) 
	    + "&amp;action=similarity&amp;id=" + miscutil::to_string(_id);
	_sim_back = false;
     }
      
   void search_snippet::set_back_similarity_link()
     {
	_sim_link = "/search?q=" + _qc->_url_enc_query
	  + "&amp;page=1&amp;expansion=" + miscutil::to_string(_qc->_page_expansion) 
	    + "&amp;action=expand";
	_sim_back = true;
     }
      
   void search_snippet::tag()
     {
	// detect extension, if any, and if not already tagged.
	if (_doc_type == WEBPAGE) // not already tagged.
	  {
	     // grab the 3 char long extension, if any.
	     std::string file_ext;
	     if (_url.size()>4 && _url[_url.size()-4] == '.')
	       {
		  try
		    {
		       file_ext = _url.substr(_url.size()-3);
		    }
		  catch(std::exception &e)
		    {
		       file_ext = "";
		    }
		  _file_format = file_ext;
	       }
	     		  
	     if (search_snippet::match_tag(_url,search_snippet::_pdf_pos_patterns))
	       _doc_type = FILE_DOC;
	     else if(search_snippet::match_tag(_url,search_snippet::_file_doc_pos_patterns))
	       _doc_type = FILE_DOC;
	     else if (search_snippet::match_tag(_url,search_snippet::_audio_pos_patterns))
	       _doc_type = AUDIO;
	     else if (search_snippet::match_tag(_url,search_snippet::_video_pos_patterns))
	       _doc_type = VIDEO;
	     else if (search_snippet::match_tag(_url,search_snippet::_forum_pos_patterns))
	       _doc_type = FORUM;
	     else if (search_snippet::match_tag(_url,search_snippet::_reject_pos_patterns))
	       _doc_type = REJECTED;
	     
	     /* std::cerr << "[Debug]: tagged snippet: url: " << _url 
	       << " -- tag: " << (int)_doc_type << std::endl; */
	  }
		
	// detect wikis. XXX: could be put into a pattern file if more complex patterns are needed.
	size_t pos = 0;
	std::string wiki_pattern = "wiki";
	std::string::const_iterator sit = _url.begin();
	if ((pos = miscutil::ci_find(_url,wiki_pattern,sit))!=std::string::npos)
	  {
	     _doc_type = WIKI;
	  }
     }
   
   // static.
   sp_err search_snippet::load_patterns()
     {
	static std::string pdf_patterns_filename 
	  = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/pdf"
	  : seeks_proxy::_datadir + "/plugins/websearch/patterns/pdf";
	static std::string file_doc_patterns_filename 
	  = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/file_doc"
	  : seeks_proxy::_datadir + "/plugins/websearch/patterns/file_doc";
	static std::string audio_patterns_filename 
	  = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/audio"
	  : seeks_proxy::_datadir + "/plugins/websearch/patterns/audio";
	static std::string video_patterns_filename 
	  = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/video"
	  : seeks_proxy::_datadir + "/plugins/websearch/patterns/video";
	static std::string forum_patterns_filename 
	  = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/forum"
	  : seeks_proxy::_datadir + "/plugins/websearch/patterns/forum";
	static std::string reject_patterns_filename
	  = (seeks_proxy::_datadir.empty()) ? plugin_manager::_plugin_repository + "websearch/patterns/reject"
	  : seeks_proxy::_datadir + "/plugins/websearch/patterns/reject";
	
	std::vector<url_spec*> fake_neg_patterns; // XXX: maybe to be supported in the future, if needed.
	
	sp_err err;
	err = loaders::load_pattern_file(pdf_patterns_filename.c_str(),search_snippet::_pdf_pos_patterns,
					 fake_neg_patterns);
	if (err == SP_ERR_OK)
	  err = loaders::load_pattern_file(file_doc_patterns_filename.c_str(),search_snippet::_file_doc_pos_patterns,
					   fake_neg_patterns);
	if (err == SP_ERR_OK)
	  err = loaders::load_pattern_file(audio_patterns_filename.c_str(),search_snippet::_audio_pos_patterns,
					   fake_neg_patterns);
	if (err == SP_ERR_OK)
	  err = loaders::load_pattern_file(video_patterns_filename.c_str(),search_snippet::_video_pos_patterns,
					   fake_neg_patterns);
	if (err == SP_ERR_OK)
	  err = loaders::load_pattern_file(forum_patterns_filename.c_str(),search_snippet::_forum_pos_patterns,
					   fake_neg_patterns);
	if (err == SP_ERR_OK)
	  err = loaders::load_pattern_file(reject_patterns_filename.c_str(),search_snippet::_reject_pos_patterns,
					   fake_neg_patterns);
	return err;
     }
      
   bool search_snippet::match_tag(const std::string &url,
				  const std::vector<url_spec*> &patterns)
     {
	std::string host;
	std::string path;
	urlmatch::parse_url_host_and_path(url,host,path);
	
	/* std::cerr << "url: " << url << std::endl;
	std::cerr << "[Debug]: host: " << host << " -- path: " << path 
	  << " -- pattern size: " << patterns.size() << std::endl; */
	
#ifndef FEATURE_EXTENDED_HOST_PATTERNS
	http_request http;
	http._host = (char*)host.c_str();
	urlmatch::init_domain_components(&http);
#endif
	
	size_t psize = patterns.size();
	for (size_t i=0;i<psize;i++)
	  {
	     url_spec *pattern = patterns.at(i);
	     
	     // host matching.
#ifdef FEATURE_EXTENDED_HOST_PATTERNS
	     int host_match = host.empty() ? 0 : ((NULL == pattern->_host_regex)
						  || (0 == regexec(pattern->_host_regex, host.c_str(), 0, NULL, 0)));
#else
	     int host_match = urlmatch::host_matches(&http,pattern);
#endif
	     if (host_match == 0)
	       continue;
	     	     
	     // path matching.
	     int path_match = urlmatch::path_matches(path.c_str(),pattern);
	     if (path_match)
	       {
#ifndef FEATURE_EXTENDED_HOST_PATTERNS
		  http._host = NULL;
#endif
		  return true;
	       }
	  }
#ifndef FEATURE_EXTENDED_HOST_PATTERNS
	http._host = NULL;
#endif
	return false;
     }
         
   void search_snippet::delete_snippets(std::vector<search_snippet*> &snippets)
     {
	size_t snippets_size = snippets.size();
	for (size_t i=0;i<snippets_size; i++)
	  delete snippets.at(i);
	snippets.clear();
     }

   void search_snippet::merge_snippets(search_snippet *s1,
				       const search_snippet *s2)
     {
	std::bitset<NSEs> setest = s1->_engine;
	setest &= s2->_engine;
	if (setest.count()>0)
	  return;
	
	// seeks_rank is updated after merging.
	// search engine rank.
	s1->_rank += s2->_rank;
	
	// search engine.
	s1->_engine |= s2->_engine;
     
	// cached link.
	if (s1->_cached.empty())
	  s1->_cached = s2->_cached;
	
	// summary.
	if (s1->_summary.length() < s2->_summary.length())
	  s1->_summary = s2->_summary;
     
	// cite.
	if (s1->_cite.length() > s2->_cite.length())
	  s1->_cite = s2->_cite;
	
	// TODO: snippet type.
	
	// file format.
	if (s1->_file_format.length() < s2->_file_format.length())  // we could do better here, ok enough for now.
	  s1->_file_format = s2->_file_format;
     }
   
} /* end of namespace. */
