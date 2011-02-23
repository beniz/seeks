/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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

#ifndef DB_QUERY_RECORD_H
#define DB_QUERY_RECORD_H

#include "stl_hash.h"
#include "db_record.h"
#include "DHTKey.h"

#include "user_db.h" // for fixing issue 169. Will disappear afterwards.
using sp::user_db;

#include <vector>

using sp::db_record;
using dht::DHTKey;

namespace seeks_plugins
{

  class vurl_data
  {
    public:
      vurl_data(const std::string &url)
        :_url(url),_hits(1)
      {};

      vurl_data(const std::string &url,
                const short &hits)
        :_url(url),_hits(hits)
      {};

      vurl_data(const vurl_data *vd)
        :_url(vd->_url),_hits(vd->_hits)
      {};

      ~vurl_data() {};

      void merge(const vurl_data *vd)
      {
        _hits += vd->_hits;
      };

      std::string _url;
      short _hits;
  };

  class query_data
  {
    public:
      query_data(const std::string &query,
                 const short &radius);

      query_data(const std::string &query,
                 const short &radius,
                 const std::string &url,
                 const short &hits=1,
                 const short &url_hits=1);

      query_data(const query_data *qd);

      ~query_data();

      void create_visited_urls();

      void merge(const query_data *qd);

      void add_vurl(vurl_data *vd);

      vurl_data* find_vurl(const std::string &url) const;

      float vurls_total_hits() const;

      std::string _query;
      short _radius;
      short _hits;
      hash_map<const char*,vurl_data*,hash<const char*>,eqstr> *_visited_urls;
      DHTKey *_record_key; /**< optional record key, not stored on db. */
  };

  class db_query_record : public db_record
  {
    public:
      db_query_record(const time_t &creation_time,
                      const std::string &plugin_name);

      db_query_record(const std::string &plugin_name,
                      const std::string &query,
                      const short &radius);

      db_query_record(const std::string &plugin_name,
                      const std::string &query,
                      const short &radius,
                      const std::string &url,
                      const short &hits=1,
                      const short &url_hits=1);

      db_query_record(const db_query_record &dbr);

      db_query_record();

      virtual ~db_query_record();

      virtual int serialize(std::string &msg) const;

      virtual int deserialize(const std::string &msg);

      virtual db_err merge_with(const db_record &dqr);

      void create_query_record(sp::db::record &r) const;

      void read_query_record(sp::db::record &r);

      int fix_issue_169(user_db &cudb);

      int fix_issue_263();

      int fix_issue_281(uint32_t &fixed_urls);

      int fix_issue_154(uint32_t &fixed_urls, uint32_t &fixed_queries, uint32_t &removed_urls);

    public:
      hash_map<const char*,query_data*,hash<const char*>,eqstr> _related_queries;
  };

} /* end of namespace. */

#endif
