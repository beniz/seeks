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

#ifndef SE_HANDLER_H
#define SE_HANDLER_H

#include "wb_err.h"
#include "feeds.h"
#include "sp_exception.h"
#include "seeks_proxy.h"

#include <string>
#include <vector>
#include <bitset>
#include <stdint.h>

#include <curl/curl.h>

namespace seeks_plugins
{
  class se_parser;
  class search_snippet;
  class query_context;

  class search_engine
  {
    public:
      search_engine();
      virtual ~search_engine();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc) {};

      /*- variables. -*/
      std::string _description;
      bool _anonymous;  // false by default.
  };

  // arguments to a threaded parser.
  struct ps_thread_arg
  {
    ps_thread_arg()
      :_se_idx(0),_output(NULL),_snippets(NULL),_qr(NULL),_err(SP_ERR_OK)
    {
    };

    ~ps_thread_arg()
    {
      // we do not delete the output, this is handled by the client.
      // we do delete snippets outside the destructor (depends on whether we're using threads).
    };

    feed_parser _se; // feed  parser (ggle, bing, opensearch, ...).
    int _se_idx; // feed url index.
    char *_output; // page content, to be parsed into snippets.
    std::vector<search_snippet*> *_snippets; // websearch result snippets.
    int _offset; // offset to snippets rank (when asking page x, with x > 1).
    query_context *_qr; // pointer to the current query context.
    sp_err _err; // error code.
  };

  class se_ggle : public search_engine
  {
    public:
      se_ggle();
      ~se_ggle();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_bing : public search_engine
  {
    public:
      se_bing();
      ~se_bing();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_yahoo : public search_engine
  {
    public:
      se_yahoo();
      ~se_yahoo();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_exalead : public search_engine
  {
    public:
      se_exalead();
      ~se_exalead();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_twitter : public search_engine
  {
    public:
      se_twitter();
      ~se_twitter();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_youtube : public search_engine
  {
    public:
      se_youtube();
      ~se_youtube();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_blekko : public search_engine
  {
    public:
      se_blekko();
      ~se_blekko();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_yauba : public search_engine
  {
    public:
      se_yauba();
      ~se_yauba();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_dailymotion : public search_engine
  {
    public:
      se_dailymotion();
      ~se_dailymotion();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_doku : public search_engine
  {
    public:
      se_doku();
      ~se_doku();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_mediawiki : public search_engine
  {
    public:
      se_mediawiki();
      ~se_mediawiki();

      virtual void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                               std::string &url, const query_context *qc);
  };

  class se_handler
  {
    public:
      /*-- initialization --*/
      static void init_handlers(const int &num);
      static void cleanup_handlers();

      /*-- querying the search engines. --*/
      static std::string** query_to_ses(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                        int &nresults, const query_context *qc, const feeds &se_enabled) throw (sp_exception);

      static void query_to_se(const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                              const feed_parser &se, std::vector<std::string> &all_urls, const query_context *qc,
                              std::list<const char*> *&lheaders);

      /*-- parsing --*/
      static se_parser* create_se_parser(const feed_parser &se, const size_t &i);

      static void parse_ses_output(std::string **outputs, const int &nresults,
                                   std::vector<search_snippet*> &snippets,
                                   const int &count_offset,
                                   query_context *qr, const feeds &se_enabled);

      static void parse_output(ps_thread_arg &args);

      /*-- variables. --*/
    public:

      /* search engine objects. */
      static se_ggle _ggle;
      static se_bing _bing;
      static se_yahoo _yahoo;
      static se_exalead _exalead;
      static se_twitter _twitter;
      static se_youtube _youtube;
      static se_dailymotion _dailym;
      static se_yauba _yauba;
      static se_blekko _blekko;
      static se_doku _doku;
      static se_mediawiki _mediaw;

      static std::vector<CURL*> _curl_handlers;
      static sp_mutex_t _curl_mutex;
  };

} /* end of namespace. */

#endif
