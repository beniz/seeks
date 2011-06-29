/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, <ebenazer@seeks-project.info>
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

#ifndef FEEDS_H
#define FEEDS_H

#include "config.h"
#include <string>
#include <algorithm>
#include <set>
#include <vector>

namespace seeks_plugins
{

  class websearch_configuration;
#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
  class img_websearch_configuration;
#endif

  class feed_url_options
  {
    public:
      feed_url_options(const std::string &url,
                       const std::string &id,
                       const bool &sedefault=false)
        :_url(url),_id(id),_default(sedefault)
      {};

      ~feed_url_options() {};

      std::string _url;
      std::string _id; /**< url feed id. */
      bool _default; /**< whether this url is default. */
  };

  class feed_parser
  {
    public:
      // ranking function.
      struct lxn
      {
        bool operator()(feed_parser f1, feed_parser f2) const
        {
          return std::lexicographical_compare(f1._name.begin(),f1._name.end(),
                                              f2._name.begin(),f2._name.end());
        }
      };

    public:
      feed_parser(const std::string &name,
                  const std::string &url);

      feed_parser(const std::string &name);

      feed_parser(const feed_parser &fp);

      feed_parser(const std::string &name,
                  const std::set<std::string> &urls);

      feed_parser();

      ~feed_parser();

      void add_url(const std::string &url);

      std::string get_url() const;

      std::string get_url(const size_t &i) const;

      size_t size() const;

      feed_parser diff(const feed_parser &fp) const;

      feed_parser diff_nosym(const feed_parser &fp) const;

      feed_parser sunion(const feed_parser &fp) const;

      feed_parser inter(const feed_parser &fp) const;

      feed_parser inter_gen(const feed_parser &fp) const;

      bool empty() const;

      std::string _name;
      std::set<std::string> _urls; /**< urls to which this feed parser applies. */
  };

  class feeds
  {
    public:
      feeds();

      feeds(std::set<feed_parser,feed_parser::lxn> &feedset);

      feeds(const std::string &name);

      feeds(const std::string &name,
            const std::string &url);

      feeds(const feeds &f);

      ~feeds();

      bool add_feed(const feed_parser &f);

      bool add_feed(const std::string &name,
                    const std::string &url);

      bool add_feed(const std::string &name);

      bool add_feed(const std::string &name,
                    websearch_configuration *wconfig);

      bool add_feed(const std::vector<std::string> &vec_name_ids,
                    websearch_configuration *wconfig);

#ifdef FEATURE_IMG_WEBSEARCH_PLUGIN
      bool add_feed_img(const std::string &name,
                        img_websearch_configuration *wconfig);

      bool add_feed_img(const std::vector<std::string> &vec_name_ids,
                        img_websearch_configuration *wconfig);
#endif

      bool remove_feed(const std::string &name);

      feed_parser find_feed(const std::string &name) const;

      bool has_feed(const std::string &name) const;

      size_t size() const;

      size_t count() const;

      bool empty() const;

      // comparison functions.
      feeds diff(const feeds &f) const;

      feeds sunion(const feeds &f) const;

      feeds inter(const feeds &f) const;

      feeds inter_gen(const feeds &f) const;

      bool equal(const feeds &f) const;

      std::set<feed_parser,feed_parser::lxn> _feedset;
  };

} /* end of namespace. */

#endif
