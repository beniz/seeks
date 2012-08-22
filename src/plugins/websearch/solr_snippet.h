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

/**
 * This class captures results from a Solr instance as Seeks 
 * search results snippets.
 * It uses the structure returned by Solr, in order to dynamically
 * fill up the snippet.
 */

#ifndef SOLR_SNIPPET_H
#define SOLR_SNIPPET_H

#include "search_snippet.h"
#include <unordered_map>

namespace seeks_plugins
{

  class solr_snippet : public search_snippet
  {
  public:
    solr_snippet();
    solr_snippet(const short &rank);
    solr_snippet(const solr_snippet *s);
    
    virtual ~solr_snippet();
    
    virtual void tag() {}; // no automatic tagging.
    
    // merging of snippets.
    virtual void merge_snippets(const search_snippet *s2);

    // HTML rendering.
    virtual std::string to_html(std::vector<std::string> &words,
				const std::string &base_url_str,
				const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters) { return ""; };
    
    // JSON rendering.
    virtual Json::Value to_json(const bool &thumbs,
				const std::vector<std::string> &query_words);
    
    // printing output.
    virtual std::ostream& print(std::ostream &output);

    // local functions.
    void add_str_elt(const std::string &name,const std::string &str);
    void add_int_elt(const std::string &name,const int &v);
    void add_float_elt(const std::string &name,const float &v);
    void add_arr_str_elt(const std::string &name, const std::string &str);
    
  public:
    // fields.
    std::unordered_map<std::string,std::string> _str_fields;
    std::unordered_map<std::string,int> _int_fields;
    std::unordered_map<std::string,float> _float_fields;
    std::unordered_map<std::string,std::vector<std::string> > _arr_str_fields; /**< arrays of strings. */
  
    // types: used to cluster the snippets per type.
    std::vector<std::string> _types;
  };

} /* end of namespace. */

#endif
