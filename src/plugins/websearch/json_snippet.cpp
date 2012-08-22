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

#include "json_snippet.h"
#include <iostream>

namespace seeks_plugins
{

  json_snippet::json_snippet()
    :search_snippet()
  {
  }

  json_snippet::json_snippet(const short &rank)
    :search_snippet(rank)
  {
  }

  json_snippet::json_snippet(const json_snippet *s)
    :search_snippet(s),_root(s->_root)
  {
  }
  
  json_snippet::~json_snippet()
  {
  }

  void json_snippet::merge_snippets(const search_snippet *s2)
  {
    //TODO.
  }

  std::string json_snippet::to_html(std::vector<std::string> &words,
				    const std::string &base_url_str,
				    const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    return search_snippet::to_html(words,base_url_str,parameters);
  }

  Json::Value json_snippet::to_json(const bool &thumbs,
				    const std::vector<std::string> &query_words)
  {
    Json::Value jres = search_snippet::to_json(thumbs,query_words); // call in parent class.
    _root.append(jres);
    /*Json::StyledWriter writer;
    std::string blob_str = writer.write(_root);
    json_str += ",\"type\":\"" + _type + "\"," + blob_str.substr(1);
    return "{" + json_str;*/
    return _root;
  }
  
  std::ostream& json_snippet::print(std::ostream &output)
  {
    output << "-----------------------------------\n";
    output << "- seeks rank: " << _meta_rank << std::endl;
    output << "- rank: " << _rank << std::endl;
    output << "- title: " << _title << std::endl;
    output << "- url: " << _url << std::endl;
    output << "- type: " << _type << std::endl;
    if (!_tags.empty())
      {
	output << "- tags: ";
	hash_map<const char*,float,hash<const char*>,eqstr>::iterator hit
	  = _tags.begin();
	while(hit!=_tags.end())
	  {
	    output << (*hit).first << "(" << (*hit).second << ") ";
	    ++hit;
	  }
      }
    output << std::endl;
    output << "- JSON blog: " << _root << std::endl;
    return output;
  }

} /* end of namespace. */
