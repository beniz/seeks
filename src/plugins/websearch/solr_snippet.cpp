/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2012 Emmanuel Benazera <emmanuel.benazera@seeks.pro>
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

#include "solr_snippet.h"
#include "json_renderer.h"
#include "miscutil.h"
#include "encode.h"

#include <iostream>

using sp::miscutil;
using sp::encode;

namespace seeks_plugins
{

  solr_snippet::solr_snippet()
    :search_snippet()
  {
  }

  solr_snippet::solr_snippet(const short &rank)
    :search_snippet(rank)
  {
  }

  solr_snippet::solr_snippet(const solr_snippet *s)
    :search_snippet(s)
  {
    _str_fields = std::unordered_map<std::string,std::string>(s->_str_fields);
    _int_fields = std::unordered_map<std::string,int>(s->_int_fields);
    _float_fields = std::unordered_map<std::string,float>(s->_float_fields);
    _arr_str_fields = std::unordered_map<std::string,std::vector<std::string> >(s->_arr_str_fields); // XXX: beware.
  }

  solr_snippet::~solr_snippet()
  {
  }

  void solr_snippet::add_str_elt(const std::string &name, const std::string &str)
  {
    _str_fields.insert(std::pair<std::string,std::string>(name,str));
  }

  void solr_snippet::add_int_elt(const std::string &name, const int &v)
  {
    _int_fields.insert(std::pair<std::string,int>(name,v));
  }

  void solr_snippet::add_float_elt(const std::string &name, const float &v)
  {
    _float_fields.insert(std::pair<std::string,float>(name,v));
  }

  void solr_snippet::add_arr_str_elt(const std::string &name, const std::string &str)
  {
    std::unordered_map<std::string,std::vector<std::string> >::iterator hit;
    if ((hit=_arr_str_fields.find(name))!=_arr_str_fields.end())
      (*hit).second.push_back(str);
    else
      {
	std::vector<std::string> vec;
	vec.push_back(str);
	_arr_str_fields.insert(std::pair<std::string,std::vector<std::string> >(name,vec));
      }
  }

  void solr_snippet::merge_snippets(const search_snippet *s2)
  {
    //TODO.
  }

  Json::Value solr_snippet::to_json(const bool &thumbs,
				    const std::vector<std::string> &query_words)
  {
    Json::Value jres = search_snippet::to_json(thumbs,query_words); // call in parent class.
    std::list<std::string> json_elts;
    std::unordered_map<std::string,std::string>::const_iterator mstrit
      = _str_fields.begin();
    while(mstrit!=_str_fields.end())
      {
	jres[(*mstrit).first] = (*mstrit).second;
	++mstrit;
      }
    std::unordered_map<std::string,int>::const_iterator mintit
      = _int_fields.begin();
    while(mintit!=_int_fields.end())
      {
	jres[(*mintit).first] = (*mintit).second;
	++mintit;
      }
    std::unordered_map<std::string,float>::const_iterator mflit
      = _float_fields.begin();
    while(mflit!=_float_fields.end())
      {
	jres[(*mflit).first] = (*mflit).second;
	++mflit;
      }
    std::unordered_map<std::string,std::vector<std::string> >::const_iterator ait
      = _arr_str_fields.begin();
    while(ait!=_arr_str_fields.end())
      {
	Json::Value je;
	for (size_t i=0;i<(*ait).second.size();i++)
	  {
	    je.append((*ait).second.at(i));
	  }
	jres[(*ait).first] = je;
	++ait;
      }
    return jres;
  }

  std::ostream& solr_snippet::print(std::ostream &output)
  {
    output << "-----------------------------------\n";
    output << "- seeks rank: " << _meta_rank << std::endl;
    output << "- rank: " << _rank << std::endl;
    output << "- title: " << _title << std::endl;
    output << "- url: " << _url << std::endl;
    
    std::unordered_map<std::string,std::string>::const_iterator mstrit
      = _str_fields.begin();
    while(mstrit!=_str_fields.end())
      {
	output << "- " << (*mstrit).first << ": " << (*mstrit).second << std::endl;
	++mstrit;
      }
    std::unordered_map<std::string,int>::const_iterator mintit
      = _int_fields.begin();
    while(mintit!=_int_fields.end())
      {
	output << "- " << (*mintit).first << ": " << (*mintit).second << std::endl;
	++mintit;
      }
    std::unordered_map<std::string,float>::const_iterator mflit
      = _float_fields.begin();
    while(mflit!=_float_fields.end())
      {
	output << "- " << (*mflit).first << ": " << (*mflit).second << std::endl;
	++mflit;
      }
    std::unordered_map<std::string,std::vector<std::string> >::const_iterator ait
      = _arr_str_fields.begin();
    while(ait!=_arr_str_fields.end())
      {
	output << "- " << (*ait).first << " (" << (*ait).second.size() << "): ";
	for (size_t i=0;i<(*ait).second.size();i++)
	  {
	    output << (*ait).second.at(i) << " / ";
	  }
	output << std::endl;
	++ait;
      }
    return output;
  }

} /* end of namespace. */
