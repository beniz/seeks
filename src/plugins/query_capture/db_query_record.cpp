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

#include "db_query_record.h"
#include "db_query_record_msg.pb.h"
#include "errlog.h"

#include <algorithm>
#include <iterator>
#include <iostream>

using sp::errlog;

namespace seeks_plugins
{
   /*- vurl_data -*/
   std::ostream& vurl_data::print(std::ostream &output) const
     {
	output << "\t\turl: " << _url << " -- hit: " << _hits << std::endl;
     }
      
   /*- query_data -*/
   query_data::query_data(const std::string &query,
			  const short &radius)
     :_query(query),_radius(radius),_hits(1),_visited_urls(NULL)
       {
       }
   
   query_data::query_data(const std::string &query,
			  const short &radius,
			  const std::string &url)
     :_query(query),_radius(radius),_hits(1)
       {
	_visited_urls = new hash_map<const char*,vurl_data*,hash<const char*>,eqstr>(1);
	vurl_data *vd = new vurl_data(url);
	add_vurl(vd);
     }

   query_data::query_data(const query_data *qd)
     :_query(qd->_query),_radius(qd->_radius),_hits(qd->_hits),_visited_urls(NULL)
     {
	if (qd->_visited_urls)
	  {
	     create_visited_urls();
	     hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::const_iterator hit
	       = qd->_visited_urls->begin();
	     while(hit!=qd->_visited_urls->end())
	       {
		  vurl_data *vd = new vurl_data((*hit).second);
		  add_vurl(vd);
		  ++hit;
	       }
	  }
     }
      
   query_data::~query_data()
     {
	if (_visited_urls)
	  {
	     hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator dhit;
	     hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator hit
	       = _visited_urls->begin();
	     while(hit!=_visited_urls->end())
	       {
		  vurl_data *vd = (*hit).second;
		  dhit = hit;
		  ++hit;
		  _visited_urls->erase(dhit);
		  delete vd;
	       }
	     delete _visited_urls;
	     _visited_urls = NULL;
	  }
     }
     
   void query_data::create_visited_urls()
     {
	if (!_visited_urls)
	  _visited_urls = new hash_map<const char*,vurl_data*,hash<const char*>,eqstr>(25);
	else errlog::log_error(LOG_LEVEL_INFO,"query_data::create_visited_urls failed: already exists");
     }
   
   void query_data::merge(const query_data *qd)
     {
	if (qd->_query != _query)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"trying to merge query record data for different queries");
	     return;
	  }
	
	assert(qd->_radius == _radius);
	
	if (!qd->_visited_urls)
	  return;
	
	if (!_visited_urls)
	  create_visited_urls();
	hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator fhit;
	hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::const_iterator hit
	  = qd->_visited_urls->begin();
	while(hit!=qd->_visited_urls->end())
	  {
	     if ((fhit=_visited_urls->find((*hit).first))!=_visited_urls->end())
	       {
		  (*fhit).second->merge((*hit).second);
	       }
	     else
	       {
		  vurl_data *vd = new vurl_data((*hit).second);
		  add_vurl(vd);
	       }
	     ++hit;
	  }
     }
      
   void query_data::add_vurl(vurl_data *vd)
     {
	if (!_visited_urls)
	  return; // safe.
	_visited_urls->insert(std::pair<const char*,vurl_data*>(vd->_url.c_str(),vd));
     }

   vurl_data* query_data::find_vurl(const std::string &url) const
     {
	if (!_visited_urls)
	  return NULL;
	hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator hit;
	if ((hit=_visited_urls->find(url.c_str()))!=_visited_urls->end())
	  return (*hit).second;
	return NULL;
     }
      
   float query_data::vurls_total_hits() const
     {
	if (!_visited_urls)
	  return 0.0;
	float res = 0.0;
	hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator hit
	  = _visited_urls->begin();
	while(hit!=_visited_urls->end())
	  {
	     res += (*hit).second->_hits;
	     ++hit;
	  }
     }
      
   std::ostream& query_data::print(std::ostream &output) const
     {
	output << "\t\tradius: " << _radius << std::endl;
	output << "\t\thits: " << _hits << std::endl;
	if (_visited_urls)
	  {
	     hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::const_iterator hit
	       = _visited_urls->begin();
	     while(hit!=_visited_urls->end())
	       {
		  (*hit).second->print(output);
		  ++hit;
	       }
	  }
	return output;
     }
      
   /*- db_query_record -*/
   db_query_record::db_query_record(const time_t &creation_time,
				    const std::string &plugin_name)
     :db_record(creation_time,plugin_name)
       {
       }
             
   db_query_record::db_query_record(const std::string &plugin_name,
				    const std::string &query,
				    const short &radius)
     :db_record(plugin_name)
       {
	  query_data *qd = new query_data(query,radius);
	  _related_queries.insert(std::pair<const char*,query_data*>(qd->_query.c_str(),qd));
       }

   db_query_record::db_query_record(const std::string &plugin_name,
				    const std::string &query,
				    const short &radius,
				    const std::string &url)
     :db_record(plugin_name)
     {
	query_data *qd = new query_data(query,radius,url);
	_related_queries.insert(std::pair<const char*,query_data*>(qd->_query.c_str(),qd));
     }
   
   db_query_record::db_query_record()
     :db_record()
       {
       }
   
   db_query_record::~db_query_record()
     {
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator dhit;
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
	  = _related_queries.begin();
	while(hit!=_related_queries.end())
	  {
	     query_data *qd = (*hit).second;
	     dhit = hit;
	     ++hit;
	     _related_queries.erase(dhit);
	     delete qd;
	  }
     }
     
   int db_query_record::serialize(std::string &msg) const
     {
	sp::db::record r;
	create_query_record(r);
	if (!r.SerializeToString(&msg))
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"failed serializing db_query_record");
	     return 1; // error.
	  }
	else return 0;
     }

   int db_query_record::deserialize(const std::string &msg)
     {
	sp::db::record r;
	if (!r.ParseFromString(msg))
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"failed deserializing db_query_record");
	     return 1; // error.
	  }
	read_query_record(r);
	return 0;
     }
      
   int db_query_record::merge_with(const db_record &dbr)
     {
	if (dbr._plugin_name != _plugin_name)
	  return -2;
	
	const db_query_record &dqr = static_cast<const db_query_record&>(dbr);
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator fhit;
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
	  = dqr._related_queries.begin();
	while(hit!=dqr._related_queries.end())
	  {
	     if ((fhit = _related_queries.find((*hit).first))!=_related_queries.end())
	       {
		  // merge.
		  (*fhit).second->merge((*hit).second);
	       }
	     else 
	       {
		  // add query.
		  query_data *rd = new query_data((*hit).second);
		  _related_queries.insert(std::pair<const char*,query_data*>(rd->_query.c_str(),rd));
	       }
	     ++hit;
	  }
	return 0;
     }
      
   void db_query_record::create_query_record(sp::db::record &r) const
     {
	create_base_record(r);
	sp::db::related_queries *queries = r.MutableExtension(sp::db::queries);
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
	  = _related_queries.begin();
	while(hit!=_related_queries.end())
	  {
	     query_data *rd = (*hit).second;
	     sp::db::related_query* rq = queries->add_rquery();
	     rq->set_radius(rd->_radius);
	     rq->set_query(rd->_query);
	     rq->set_query_hits((*hit).second->_hits);
	     sp::db::visited_urls *rq_vurls = rq->mutable_vurls();
	     
	     if (rd->_visited_urls)
	       {
		  hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::const_iterator vhit
		    = rd->_visited_urls->begin();
		  while(vhit!=rd->_visited_urls->end())
		    {
		       vurl_data *vd = (*vhit).second;
		       if (vd) // XXX: should not happen.
			 {
			    sp::db::visited_url *rq_vurl = rq_vurls->add_vurl();
			    rq_vurl->set_url(vd->_url);
			    rq_vurl->set_hits(vd->_hits);
			 }
		       else std::cerr << "[Debug]: null vurl_data element in visited_urls...\n";
		       ++vhit;
		    }
	       }
	     ++hit;
	  }
     }
   
   void db_query_record::read_query_record(sp::db::record &r)
     {
	read_base_record(r);
	sp::db::related_queries *rqueries = r.MutableExtension(sp::db::queries);
	int nrq = rqueries->rquery_size();
	for (int i=0;i<nrq;i++)
	  {
	     sp::db::related_query *rq = rqueries->mutable_rquery(i);
	     short radius = rq->radius();
	     std::string query = rq->query();
	     query_data *rd = new query_data(query,radius);
	     rd->_hits = rq->query_hits();
	     sp::db::visited_urls *rq_vurls = rq->mutable_vurls();
	     int nvurls = rq_vurls->vurl_size();
	     if (nvurls > 0)
	       rd->create_visited_urls();
	     for (int j=0;j<nvurls;j++)
	       {
		  sp::db::visited_url *rq_vurl = rq_vurls->mutable_vurl(j);
		  std::string url = rq_vurl->url();
		  short uhits = rq_vurl->hits();
		  vurl_data *vd = new vurl_data(url,uhits);
		  rd->_visited_urls->insert(std::pair<const char*,vurl_data*>(vd->_url.c_str(),vd));
	       }
	     _related_queries.insert(std::pair<const char*,query_data*>(rd->_query.c_str(),rd));
	  }
     }
   
   std::ostream& db_query_record::print(std::ostream &output) const
     {
	//output << "\tquery: " << _query;
/*	if (!_visited_urls.empty())
	  {
	     std::set<std::string>::const_iterator sit = _visited_urls.begin();
	     while(sit!=_visited_urls.end())
	       {
		  output << "\n\turl: " << (*sit);
		  ++sit;
	       }
	  } */
	hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
	  = _related_queries.begin();
	while(hit!=_related_queries.end())
	  {
	     output << "\tquery: " << (*hit).first << std::endl;
	     (*hit).second->print(output);
	     ++hit;
	  }
	output << std::endl;
	return output;
     }
      
} /* end of namespace. */
