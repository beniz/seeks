/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#ifndef SEEKS_SNIPPET_H
#define SEEKS_SNIPPET_H

#include "search_snippet.h"

namespace seeks_plugins
{

  class seeks_doc_type : public doc_type
  {
    public:
      static const int WEBPAGE = 2;
      static const int FORUM = 3;
      static const int FILE_DOC = 4;
      static const int SOFTWARE = 5;
      static const int VIDEO = 6;
      static const int VIDEO_THUMB = 7;
      static const int AUDIO = 8;
      static const int CODE = 9;
      static const int NEWS = 10;
      static const int TWEET = 11;
      static const int WIKI = 12;
      static const int POST = 13;
      static const int BUG = 14;
      static const int ISSUE = 15;
      static const int REVISION = 16;
      static const int COMMENT = 17;
  };

  class seeks_snippet : public search_snippet
  {
    public:
      seeks_snippet();
      seeks_snippet(const short &rank);
      seeks_snippet(const seeks_snippet *s);

      virtual ~seeks_snippet();

      // HTML rendering.
      virtual std::string to_html(std::vector<std::string> &words,
                                  const std::string &base_url_str,
                                  const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters);

      // JSON rendering.
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

      void set_date(const std::string &date);

      void set_cite(const std::string &cite);
      void set_cite_no_decode(const std::string &cite);

      // sets a link to the archived url at archive.org (e.g. in case we'no cached link).
      void set_archive_link();

      // tagging.
      virtual void tag();

      // load tagging patterns from files.
      static sp_err load_patterns();

      // destroy loaded patterns.
      static void destroy_patterns();

      // merging of snippets (merge s2 into s2, according to certain rules).
      virtual void merge_snippets(const search_snippet *s2);

      // hack for correcting meta rank after Bing and Yahoo merged their results in
      // the US.
      void bing_yahoo_us_merge();

      // get doc type in a string form.
      virtual std::string get_doc_type_str() const;

    public:
      std::string _cite;
      std::string _cached;
      std::string _file_format;
      std::string _date;
      std::string _archive; // a link to archive.org

      // type-dependent information.
      std::string _forum_thread_info;

      // patterns for snippets tagging (positive patterns for now only).
      static std::vector<url_spec*> _pdf_pos_patterns;
      static std::vector<url_spec*> _file_doc_pos_patterns;
      static std::vector<url_spec*> _audio_pos_patterns;
      static std::vector<url_spec*> _video_pos_patterns;
      static std::vector<url_spec*> _forum_pos_patterns;
      static std::vector<url_spec*> _reject_pos_patterns;
  };

} /* end of namespace. */

#endif
