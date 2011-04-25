/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "img_websearch_configuration.h"
#include "miscutil.h"
#include "errlog.h"

#include <iostream>

using sp::miscutil;
using sp::errlog;

namespace seeks_plugins
{

#define hash_img_se     3083524283ul /* "img-search-engine" */
#define hash_img_ca     3745160171ul /* "img-content-analysis" */
#define hash_img_n      3311685141ul /* "img-per-page" */
#define hash_safesearch 3304310928ul /* "safe-search" */

  img_websearch_configuration *img_websearch_configuration::_img_wconfig = NULL;

  img_websearch_configuration::img_websearch_configuration(const std::string &filename)
    :configuration_spec(filename),_default_engines(false)
  {
    if (img_websearch_configuration::_img_wconfig == NULL)
      img_websearch_configuration::_img_wconfig = this;
    load_config();
  }

  img_websearch_configuration::~img_websearch_configuration()
  {
  }

  void img_websearch_configuration::set_default_config()
  {
    set_default_engines();
    _img_content_analysis = false; // no download of image thumbnails is default.
    _Nr = 30; // default number of images per page.
    _safe_search = true; // default is on.
  }

  void img_websearch_configuration::set_default_engines()
  {
    std::string url = "http://www.google.com/images?q=%query&gbv=1&start=%start&hl=%lang&ie=%encoding&oe=%encoding";
    _img_se_enabled.add_feed("google_img",url);
    feed_url_options fuo(url,"google_img",true);
    _se_options.insert(std::pair<const char*,feed_url_options>(fuo._url.c_str(),fuo));
    url = "http://www.flickr.com/search/?q=%query&page=%start";
    _img_se_enabled.add_feed("flickr",url);
    fuo = feed_url_options(url,"flickr",true);
    _se_options.insert(std::pair<const char*,feed_url_options>(fuo._url.c_str(),fuo));
    url = "http://commons.wikimedia.org/w/index.php?search=%query&limit=%num&offset=%start";
    _img_se_enabled.add_feed("wcommons",url);
    fuo= feed_url_options(url,"wcommons",true);
    _se_options.insert(std::pair<const char*,feed_url_options>(fuo._url.c_str(),fuo));
    url = "http://images.search.yahoo.com/search/images?ei=UTF-8&p=%query&js=0&vl=lang_%lang&b=%start";
    _img_se_enabled.add_feed("yahoo_img",url);
    fuo= feed_url_options(url,"yahoo_img",true);
    _se_options.insert(std::pair<const char*,feed_url_options>(fuo._url.c_str(),fuo));
    _default_engines = true;
  }

  void img_websearch_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg,
      char *buf, const unsigned long &linenum)
  {
    std::vector<std::string> bpvec;
    char tmp[BUFFER_SIZE];
    int vec_count;
    char *vec[20]; // max 10 urls per feed parser.
    int i;
    feed_parser fed;
    feed_parser def_fed;
    bool def = false;
    switch (cmd_hash)
      {
      case hash_img_se:
        strlcpy(tmp,arg,sizeof(tmp));
        vec_count = miscutil::ssplit(tmp," \t",vec,SZ(vec),1,1);
        div_t divresult;
        divresult = div(vec_count-1,3);
        if (divresult.rem > 0)
          {
            errlog::log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for search-engine "
                              "directive in websearch plugin configuration file");
            break;
          }

        if (_default_engines)
          {
            // reset engines.
            _img_se_enabled = feeds();
            _se_options.clear();
            _default_engines = false;
          }

        fed = feed_parser(vec[0]);
        def_fed = feed_parser(vec[0]);

        for (i=1; i<vec_count; i+=3)
          {
            fed.add_url(vec[i]);
            std::string fu_name = vec[i+1];
            def = false;
            if (strcmp(vec[i+2],"default")==0)
              def = true;
            feed_url_options fuo(vec[i],fu_name,def);
            _se_options.insert(std::pair<const char*,feed_url_options>(fuo._url.c_str(),fuo));
            if (def)
              def_fed.add_url(vec[i]);
          }
        if (!def_fed.empty())
          _img_se_default.add_feed(def_fed);

        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Enabled image search engine");
        break;

      case hash_img_ca:
        _img_content_analysis = static_cast<bool>(atoi(arg));
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Enable the downloading of image snippets and comparison operations to detect identical images");
        break;

      case hash_img_n:
        _Nr = atoi(arg);
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Number of images per page");
        break;

      case hash_safesearch:
        _safe_search = static_cast<bool>(atoi(arg));
        configuration_spec::html_table_row(_config_args,cmd,arg,
                                           "Enable the safe search (no pornographic images");
        break;

      default:
        break;
      }
  }

  void img_websearch_configuration::finalize_configuration()
  {
  }

} /* end of namespace. */
