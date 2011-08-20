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

#include "query_recommender.h"
#include "rank_estimators.h"
#include "query_capture.h"
#include "miscutil.h"
#include "mem_utils.h"

#include <ctype.h>

#include <vector>
#include <algorithm>
#include <iostream>

using sp::miscutil;

namespace seeks_plugins
{

  bool query_recommender::select_query(const str_chain &strc_query,
                                       const std::string &query,
                                       stopwordlist *swl)
  {
    std::string rquery = query_capture_element::no_command_query(query);
    std::transform(rquery.begin(),rquery.end(),rquery.begin(),tolower);
    str_chain strc_rquery = str_chain(rquery,0,true);
    strc_rquery = strc_rquery.rank_alpha();
    std::string ra_rquery = strc_rquery.print_str();

    // intersect queries.
    str_chain inter = strc_query.intersect(strc_rquery);

    // reject stopwords.
    bool reject = true;
    for (size_t i=0; i<inter.size(); i++)
      if (!swl->has_word(inter.at(i)))
        {
          reject = false;
          break;
        }

    return !reject;
  }

  void query_recommender::recommend_queries(const std::string &query,
      const std::string &lang,
      const uint32_t &expansion,
      std::multimap<double,std::string,std::less<double> > &related_queries,
      peer *pe) throw (sp_exception)
  {
    // fetch queries from user DB.
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    try
      {
        //peer pe(host,port,"",""); //TODO: missing peer and rsc.
        rank_estimator::fetch_query_data(query,lang,expansion,qdata,pe);
      }
    catch(sp_exception &e)
      {
        throw e;
      }

    query_recommender::recommend_queries(query,lang,related_queries,&qdata);

    // destroy query data.
    rank_estimator::destroy_query_data(qdata);
  }

  void query_recommender::recommend_queries(const std::string &query,
      const std::string &lang,
      std::multimap<double,std::string,std::less<double> > &related_queries,
      hash_map<const char*,query_data*,hash<const char*>,eqstr> *qdata)
  {
    // get stop word list.
    stopwordlist *swl = seeks_proxy::_lsh_config->get_wordlist(lang);

    // clean query.
    std::string qquery = query_capture_element::no_command_query(query);
    qquery = miscutil::chomp_cpp(qquery);
    std::transform(qquery.begin(),qquery.end(),qquery.begin(),tolower);

    // rank related queries.
    hash_map<const char*,double,hash<const char*>,eqstr> update;
    hash_map<const char*,double,hash<const char*>,eqstr>::iterator uit;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
    = qdata->begin();
    while(hit!=qdata->end())
      {
        std::string rquery = (*hit).second->_query;
        rquery = query_capture_element::no_command_query(rquery);
        std::transform(rquery.begin(),rquery.end(),rquery.begin(),tolower);

        if (qquery != rquery)
          {
            //std::cerr << "rquery: " << rquery << " -- query: " << qquery << std::endl;
            short radius = (*hit).second->_radius;
            double hits = (*hit).second->_hits;
            double score = 1.0 / simple_re::query_halo_weight(query,rquery,radius,swl) * 1.0 / hits; // max weight is best.
            if ((uit = update.find(rquery.c_str()))!=update.end())
              (*uit).second *= score;
            else update.insert(std::pair<const char*,double>(strdup(rquery.c_str()),score));
          }

        ++hit;
      }

    // merging.
    query_recommender::merge_recommended_queries(related_queries,update);
  }

  void query_recommender::merge_recommended_queries(std::multimap<double,std::string,std::less<double> > &related_queries,
      hash_map<const char*,double,hash<const char*>,eqstr> &update)
  {
    hash_map<const char*,double,hash<const char*>,eqstr>::iterator hit;
    std::multimap<double,std::string,std::less<double> >::iterator mit
    = related_queries.begin();
    while(mit!=related_queries.end())
      {
        std::string rquery = (*mit).second;
        if ((hit = update.find(rquery.c_str()))!=update.end())
          {
            (*hit).second = std::min((*mit).first,(*hit).second);
            std::multimap<double,std::string,std::less<double> >::iterator mit2 = mit;
            ++mit;
            related_queries.erase(mit2);
          }
        else ++mit;
      }
    hit = update.begin();
    hash_map<const char*,double,hash<const char*>,eqstr>::iterator chit;
    while(hit!=update.end())
      {
        related_queries.insert(std::pair<double,std::string>((*hit).second,std::string((*hit).first)));
        chit = hit;
        ++hit;
        free_const((*chit).first);
      }

  }

} /* end of namespace. */
