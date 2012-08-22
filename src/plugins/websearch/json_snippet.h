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
 * This class captures a result snippet in JSON format.
 * It uses the structure from the jsoncpp library for storage
 * and access to the JSON fields.
 * For every application, a dedicated parser must fill up the required
 * fiels (id, url, title, ...), the other field can be captured 
 * by the JSON structure.
 */

#ifndef JSON_SNIPPET_H
#define JSON_SNIPPET_H

#include "search_snippet.h"
#include <jsoncpp/json/json.h>

namespace seeks_plugins
{

  class json_snippet : public search_snippet
  {
  public:
    json_snippet();
    json_snippet(const short &rank);
    json_snippet(const json_snippet *s);
    
    virtual ~json_snippet();
    
    virtual void tag() {}; // no automatic tagging.

    // merging of snippets.
    virtual void merge_snippets(const search_snippet *s2);
    
    // HTML rendering.
    virtual std::string to_html(std::vector<std::string> &words,
				const std::string &base_url_str,
				const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);
    
    // JSON rendering.
    virtual Json::Value to_json(const bool &thumbs,
				const std::vector<std::string> &query_words);

    // type in string form.
    virtual std::string get_doc_type_str() const { return _type; };
    
    // printing output.
    virtual std::ostream& print(std::ostream &output);

    // local functions.
    
  public:
    Json::Value _root; /**< root value of the JSON structure. */
    std::string _type; /**< snippet custom type. */
  };

} /* end of namespace. */

#endif
