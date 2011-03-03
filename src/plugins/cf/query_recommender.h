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

#ifndef QUERY_RECOMMENDER_H
#define QUERY_RECOMMENDER_H

#include <string>
#include <map>

#include "stl_hash.h"
#include "query_context.h"
#include "db_query_record.h"
#include "stopwordlist.h"
#include "mrf.h"

using lsh::str_chain;
using lsh::stopwordlist;

namespace seeks_plugins
{

  class query_recommender
  {
    public:
      static bool select_query(const str_chain &strc_query,
                               const std::string &query,
                               stopwordlist *swl);

      static void recommend_queries(const std::string &query,
                                    const std::string &lang,
                                    std::multimap<double,std::string,std::less<double> > &related_queries,
                                    const std::string &host="",
                                    const int &port=-1) throw (sp_exception);

      static void recommend_queries(const std::string &query,
                                    const std::string &lang,
                                    std::multimap<double,std::string,std::less<double> > &related_queries,
                                    hash_map<const char*,query_data*,hash<const char*>,eqstr> *qdata);

      static void merge_recommended_queries(std::multimap<double,std::string,std::less<double> > &related_queries,
                                            hash_map<const char*,double,hash<const char*>,eqstr> &update);
  };

} /* end of namespace. */

#endif
