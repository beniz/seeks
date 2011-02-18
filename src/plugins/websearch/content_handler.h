/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
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

#ifndef CONTENT_HANDLER_H
#define CONTENT_HANDLER_H

#include "query_context.h"
#include "search_snippet.h"
#include "mrf.h"

#include <string>
#include <stdint.h>

using lsh::mrf;

namespace seeks_plugins
{
  // arguments to a threaded text content parser.
  struct html_txt_thread_arg
  {
    html_txt_thread_arg()
      :_output(NULL),_qc(NULL)
    {
    };

    ~html_txt_thread_arg()
    {
    };

    std::string _txt_content; // parsed content.
    char *_output; // content, to be parsed.
    query_context *_qc;
  };

  struct feature_thread_arg
  {
    feature_thread_arg(std::string *txt_content,
                       std::vector<uint32_t> *vf)
      :_txt_content(txt_content),_vf(vf)
    {
    };

    ~feature_thread_arg()
    {
    };

    std::string *_txt_content;
    std::vector<uint32_t> *_vf;

    static int _radius;
    static int _step;
    static uint32_t _window_length;
  };

  struct feature_tfidf_thread_arg
  {
    feature_tfidf_thread_arg(std::string *txt_content,
                             hash_map<uint32_t,float,id_hash_uint> *vf)
      :_txt_content(txt_content),_vf(vf),_bow(NULL)
    {
    };

    feature_tfidf_thread_arg(std::string *txt_content,
                             hash_map<uint32_t,float,id_hash_uint> *vf,
                             hash_map<uint32_t,std::string,id_hash_uint> *bow,
                             const std::string &lang)
      :_txt_content(txt_content),_vf(vf),_bow(bow),_lang(lang)
    {
    };

    ~feature_tfidf_thread_arg()
    {
    };

    std::string *_txt_content;
    hash_map<uint32_t,float,id_hash_uint> *_vf;
    hash_map<uint32_t,std::string,id_hash_uint> *_bow;
    std::string _lang;

    static int _radius;
    static int _step;
    static uint32_t _window_length;
  };

  class content_handler
  {
    public:
      static std::string** fetch_snippets_content(const std::vector<std::string> &urls,
          const bool &proxy, const query_context *qc);

      static void fetch_all_snippets_summary_and_features(query_context *qc);

      static void fetch_all_snippets_content_and_features(query_context *qc);

      static void generate_features(feature_thread_arg &args);

      static void generate_features_tfidf(feature_tfidf_thread_arg &args);

      static std::string* parse_snippets_txt_content(const size_t &ncontents,
          std::string **outputs);

      static void parse_output(html_txt_thread_arg &args);

      static void extract_features_from_snippets(query_context *qc,
          const std::vector<std::string*> &txt_contents,
          const std::vector<search_snippet*> &sps);

      static void extract_tfidf_features_from_snippets(query_context *qc,
          const std::vector<std::string*> &txt_contents,
          const std::vector<search_snippet*> &sps);

      static void feature_based_similarity_scoring(query_context *qc,
          const size_t &nsps,
          search_snippet **sps,
          search_snippet *ref_sp) throw (sp_exception);

      static bool has_same_content(query_context *qc,
                                   search_snippet *sp1, search_snippet *sp2,
                                   const double &similarity_threshold);

    private:
  };

}; /* end of namespace. */

#endif
