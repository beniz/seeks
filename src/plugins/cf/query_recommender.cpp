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
#include "mrf.h" // for str_chain

#include <vector>
#include <iostream>

using sp::miscutil;
using lsh::str_chain;

namespace seeks_plugins
{
  
  void query_recommender::recommend_queries(const std::string &query,
					    std::multimap<double,std::string,std::less<double> > &related_queries)
  {
    // fetch records from user db.
    std::vector<db_record*> records;
    rank_estimator::fetch_user_db_record(query,records);
    
    // aggregate related queries.
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    rank_estimator::extract_queries(records,qdata);
  
    // clean query.
    std::string qquery = query_capture_element::no_command_query(query);
    qquery = miscutil::chomp_cpp(qquery);
    str_chain strc_query(qquery,0,true);
    strc_query = strc_query.rank_alpha();
    qquery = strc_query.print_str();

    // rank related queries.
    hash_map<const char*,double,hash<const char*>,eqstr> update;
    hash_map<const char*,double,hash<const char*>,eqstr>::iterator uit;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
      = qdata.begin();
    while(hit!=qdata.end())
      {
	std::string rquery = (*hit).second->_query;
	rquery = query_capture_element::no_command_query(rquery);
	strc_query = str_chain(rquery,0,true);
	strc_query = strc_query.rank_alpha();
	rquery = strc_query.print_str();
	
	if (rquery != qquery) //TODO: alphabetical order of words then compare...
	  {
	    //std::cerr << "rquery: " << rquery << " -- query: " << qquery << std::endl;
	    short radius = (*hit).second->_radius;
	    double hits = (*hit).second->_hits;
	    if ((uit = update.find(rquery.c_str()))!=update.end())
	      (*uit).second *= radius/hits;
	    else update.insert(std::pair<const char*,double>(strdup(rquery.c_str()),radius/hits));
	  }
	++hit;
      }

    // merging.
    query_recommender::merge_recommended_queries(related_queries,update);

    // destroy query data.
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator chit;
    hit = qdata.begin();
    while (hit!=qdata.end())
      {
	query_data *qd = (*hit).second;
	chit = hit;
	++hit;
	delete qd;
      }
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
