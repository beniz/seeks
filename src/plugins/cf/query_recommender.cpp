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
    std::vector<std::string> inter;
    for (size_t i=0;i<strc_query.size();i++)
      {
	for (size_t j=0;j<strc_rquery.size();j++)
	  {
	    if (strc_query.at(i) == strc_rquery.at(j))
	      {
		inter.push_back(strc_rquery.at(j));
	      }
	  }
      }
    
    // reject stopwords.
    bool reject = true;
    for (size_t i=0;i<inter.size();i++)
      if (!swl->has_word(inter.at(i)))
	{
	  reject = false;
	  break;
	}
    
    return !reject;
  }

  void query_recommender::recommend_queries(const std::string &query,
					    const query_context *qc,
					    std::multimap<double,std::string,std::less<double> > &related_queries)
  {
    // fetch records from user db.
    std::vector<db_record*> records;
    rank_estimator::fetch_user_db_record(query,records);
    
    // aggregate related queries.
    hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
    rank_estimator::extract_queries(query,qc,records,qdata);
  
    // clean query.
    std::string qquery = query_capture_element::no_command_query(query);
    qquery = miscutil::chomp_cpp(qquery);
    std::transform(qquery.begin(),qquery.end(),qquery.begin(),tolower);
    
    // rank related queries.
    hash_map<const char*,double,hash<const char*>,eqstr> update;
    hash_map<const char*,double,hash<const char*>,eqstr>::iterator uit;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
      = qdata.begin();
    while(hit!=qdata.end())
      {
	std::string rquery = (*hit).second->_query;
	rquery = query_capture_element::no_command_query(rquery);
	std::transform(rquery.begin(),rquery.end(),rquery.begin(),tolower);
	
	if (qquery != rquery)
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
