/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009-2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#ifndef SEARCH_SNIPPET_H
#define SEARCH_SNIPPET_H

#include "proxy_dts.h" // for url_spec.
#include "feeds.h"

#ifdef FEATURE_XSLSERIALIZER_PLUGIN
#include <libxml/parser.h>
#include <libxml/tree.h>
#endif

#include <string>
#include <vector>
#include <bitset>
#include <algorithm>
#include <ostream>

using sp::url_spec;

namespace seeks_plugins
{
  class doc_type
  {
    public:
      static const int UNKNOWN = 0;
      static const int REJECTED = 1; /* user reject */
  };

  class query_context;

  class search_snippet
  {
    public:
      // comparison functions.
      static bool less_url(const search_snippet *s1, const search_snippet *s2)
      {
        return std::lexicographical_compare(s1->_url.begin(),s1->_url.end(),
                                            s2->_url.begin(),s2->_url.end());
      };

      static bool equal_url(const search_snippet *s1, const search_snippet *s2)
      {
        return s1->_url == s2->_url;
      };

      static bool less_meta_rank(const search_snippet *s1, const search_snippet *s2)
      {
        if (s1->_meta_rank == s2->_meta_rank)
          return s1->_rank / static_cast<double>(s1->_engine.size())
                 < s2->_rank / static_cast<double>(s2->_engine.size());
        else
          return s1->_meta_rank < s2->_meta_rank;
      };

      static bool max_meta_rank(const search_snippet *s1, const search_snippet *s2)
      {
        if (s1->_meta_rank == s2->_meta_rank)
          return s1->_rank / static_cast<double>(s1->_engine.size())
                 < s2->_rank / static_cast<double>(s2->_engine.size());  // beware: min rank is still better.
        else
          return s1->_meta_rank > s2->_meta_rank;  // max seeks rank is better.
      };

      static bool max_seeks_ir(const search_snippet *s1, const search_snippet *s2)
      {
        if (s1->_seeks_ir == s2->_seeks_ir)
          return search_snippet::max_meta_rank(s1,s2);
        else return s1->_seeks_ir > s2->_seeks_ir;
      };

      static bool min_seeks_ir(const search_snippet *s1, const search_snippet *s2)
      {
        if (s1->_seeks_ir == s2->_seeks_ir)
          return search_snippet::less_meta_rank(s1,s2); // XXX: beware, may not apply to inherited classes.
        else return s1->_seeks_ir < s2->_seeks_ir;
      };

      static bool max_seeks_rank(const search_snippet *s1, const search_snippet *s2)
      {
        if (s1->_seeks_rank == s2->_seeks_rank)
          return search_snippet::max_meta_rank(s1,s2);
        else return s1->_seeks_rank > s2->_seeks_rank;
      };

      static bool new_date(const search_snippet *s1, const search_snippet *s2)
      {
        if (s1->_content_date == s2->_content_date)
          return max_seeks_rank(s1,s2);
        else return s1->_content_date > s2->_content_date;
      };

      static bool old_date(const search_snippet *s1, const search_snippet *s2)
      {
        if (s1->_content_date == s2->_content_date)
          return max_seeks_rank(s1,s2);
        else return s1->_content_date < s2->_content_date;
      };

      static bool new_activity(const search_snippet *s1, const search_snippet *s2)
      {
        if (s1->_record_date == s2->_record_date)
          return max_seeks_rank(s1,s2);
        else return s1->_record_date > s2->_record_date;
      };

      static bool old_activity(const search_snippet *s1, const search_snippet *s2)
      {
        if (s1->_record_date == s2->_record_date)
          return max_seeks_rank(s1,s2);
        else return s1->_record_date < s2->_record_date;
      };

      // constructors.
    public:
      search_snippet();
      search_snippet(const double &rank);
      search_snippet(const search_snippet *s);

      virtual ~search_snippet();

      void set_title(const std::string &title);
      void set_title_no_html_decode(const std::string &title);

      void set_url(const std::string &url);
      void set_url_no_decode(const std::string &url);

      void set_summary(const std::string &summary);

      void set_lang(const std::string &lang);

      void set_radius(const int &radius);

      // returns stripped, lower case url for storage and comparisons.
      std::string get_stripped_url() const;

      // sets a link to a sorting of snippets wrt. to their similarity to this snippet.
      virtual void set_similarity_link();

      // sets a back link when similarity is engaged.
      virtual void set_back_similarity_link();

      // whether this snippet's engine(s) is(are) enabled.
      // used in result page rendering.
      virtual bool is_se_enabled(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      // static functions.
      // highlights terms within the argument string.
      // TODO: move to renderer ?
      static void highlight_query(std::vector<std::string> &words,
                                  std::string &str);

      // selects most discriminative terms in the snippet's vocabulary.
      void discr_words(const std::vector<std::string> &query_words,
                       std::set<std::string> &words) const;

      // tag snippet, i.e. detect its type if not already done by the parsers.
      virtual void tag() {};

      // match url against tags.
      static bool match_tag(const std::string &url,
                            const std::vector<url_spec*> &patterns);

      // merging of snippets (merge s2 into s2, according to certain rules).
      virtual void merge_snippets(const search_snippet *s2);

      // reset p2p data.
      void reset_p2p_data();

      // get doc type in a string form.
      virtual std::string get_doc_type_str() const;

      // HTML rendering.
      virtual std::string to_html(std::vector<std::string> &words,
                                  const std::string &base_url_str,
                                  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      // JSON rendering.
      std::string to_json_str(const bool &thumbs,
                              const std::vector<std::string> &query_words);
      virtual std::string to_json(const bool &thumbs,
                                  const std::vector<std::string> &query_words);

#ifdef FEATURE_XSLSERIALIZER_PLUGIN
      // XML rendering.
      virtual sp_err to_xml(const bool &thumbs,
                            const std::vector<std::string> &query_words,
                            xmlNodePtr parent);
#endif

      // printing output.
      virtual std::ostream& print(std::ostream &output);

    public:
      query_context *_qc; // query context the snippet belongs to.
      bool _new; // true before snippet is processed.
      uint32_t _id; // snippet id as hashed url.

      std::string _title;
      std::string _url;
      std::string _summary;
      std::string _lang;
      int _doc_type;
      bool _sim_back; // whether the back 'button' to similarity is present.

      double _rank;  // search engine rank.
      double _seeks_ir; // IR score computed locally.
      double _meta_rank; // rank computed locally by the meta-search engine.
      double _seeks_rank; // rank computed locally, mostly for personalized ranking.

      uint32_t _content_date; // content date, since Epoch.
      uint32_t _record_date; // record date, since Epoch.

      feeds _engine;  // engines from which it was created (if not directly published).

      // cache.
      std::string *_cached_content;
      std::vector<uint32_t> *_features; // temporary set of features, used for fast similarity check between snippets.
      hash_map<uint32_t,float,id_hash_uint> *_features_tfidf; // tf-idf feature set for this snippet.
      hash_map<uint32_t,std::string,id_hash_uint> *_bag_of_words;

      // temporary flag used for marking snippets for which the
      // personalization system has found ranking information in local dataset.
      bool _personalized;

      // number of peers influencing this snippet.
      uint32_t _npeers;

      // number of collaborative 'hits' for this snippet.
      uint32_t _hits;

      // radius from the original query.
      int _radius;

      // generic 'safe' tag, mostly for pornographic images.
      // XXX: may be used later as a generic flag for marking content that
      // cannot be considered to be 'safe' for everyone to read/see.
      bool _safe;
  };

} /* end of namespace. */

#endif
