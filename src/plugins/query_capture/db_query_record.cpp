/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "config.h"
#include "db_query_record.h"
#include "db_query_record_msg.pb.h"
#include "miscutil.h"
#include "charset_conv.h"
#include "errlog.h"

#include "DHTKey.h" // for fixing issue 169.
#include "qprocess.h" // idem.
#include "query_capture_configuration.h" // idem.
using lsh::qprocess;

#include "uri_capture.h"

#include <algorithm>
#include <iterator>
#include <iostream>
#include <sstream>

#include <iconv.h>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/gzip_stream.h>

using sp::errlog;
using sp::miscutil;
using sp::charset_conv;

namespace seeks_plugins
{
  /*- query_data -*/
  query_data::query_data(const std::string &query,
                         const short &radius)
    :_query(query),_radius(radius),_hits(1),_visited_urls(NULL),
     _record_key(NULL)
  {
  }

  query_data::query_data(const std::string &query,
                         const short &radius,
                         const std::string &url,
                         const short &hits,
                         const short &url_hits,
                         const std::string &title,
                         const std::string &summary,
                         const uint32_t &url_date,
                         const uint32_t &rec_date,
                         const std::string &url_lang)
    :_query(query),_radius(radius),_hits(hits),_record_key(NULL)
  {
    _visited_urls = new hash_map<const char*,vurl_data*,hash<const char*>,eqstr>(1);
    vurl_data *vd = new vurl_data(url,url_hits,title,summary,url_date,rec_date,url_lang);
    add_vurl(vd);
  }

  query_data::query_data(const query_data *qd)
    :_query(qd->_query),_radius(qd->_radius),_hits(qd->_hits),_visited_urls(NULL),
     _record_key(NULL)
  {
    if (qd->_visited_urls)
      {
        create_visited_urls();
        hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::const_iterator hit
        = qd->_visited_urls->begin();
        while (hit!=qd->_visited_urls->end())
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
        while (hit!=_visited_urls->end())
          {
            dhit = hit;
            vurl_data *vd = (*dhit).second;
            ++hit;
            _visited_urls->erase(dhit);
            delete vd;
          }
        delete _visited_urls;
        _visited_urls = NULL;
      }
    if (_record_key)
      delete _record_key;
  }

  void query_data::create_visited_urls()
  {
    if (!_visited_urls)
      _visited_urls = new hash_map<const char*,vurl_data*,hash<const char*>,eqstr>(25);
    else errlog::log_error(LOG_LEVEL_INFO,"query_data::create_visited_urls failed: already exists");
  }

  void query_data::merge(const query_data *qd)
  {
    // XXX: query hits are not updated.
    // this would require a special flag so that merging URLs
    // does not impact on query hits.

    if (qd->_query != _query)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"trying to merge query record data for different queries");
        return;
      }

    //assert(qd->_radius == _radius);

    if (!qd->_visited_urls)
      return;

    if (!_visited_urls)
      create_visited_urls();
    hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator fhit;
    hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::const_iterator hit
    = qd->_visited_urls->begin();
    while (hit!=qd->_visited_urls->end())
      {
        if ((fhit=_visited_urls->find((*hit).first))!=_visited_urls->end())
          {
            (*fhit).second->merge((*hit).second);

            // remove URL if all hits have been cancelled.
            if ((*fhit).second->_hits == 0)
              {
                vurl_data *vd = (*fhit).second;
                _visited_urls->erase(fhit);
                delete vd;
              }
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
    while (hit!=_visited_urls->end())
      {
        res += (*hit).second->_hits;
        ++hit;
      }
    return res;
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
                                   const std::string &url,
                                   const short &hits,
                                   const short &url_hits,
                                   const std::string &title,
                                   const std::string &summary,
                                   const uint32_t &url_date,
                                   const uint32_t &rec_date,
                                   const std::string &url_lang)
    :db_record(plugin_name)
  {
    query_data *qd = new query_data(query,radius,url,hits,url_hits,title,summary,url_date,rec_date,url_lang);
    _related_queries.insert(std::pair<const char*,query_data*>(qd->_query.c_str(),qd));
  }

  db_query_record::db_query_record(const hash_map<const char*,query_data*,hash<const char*>,eqstr> &rq)
  {
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
    = rq.begin();
    while(hit!=rq.end())
      {
        _related_queries.insert(std::pair<const char*,query_data*>((*hit).second->_query.c_str(),(*hit).second));
        ++hit;
      }
  }

  db_query_record::db_query_record(const db_query_record &dbr)
  {
    db_query_record::copy_related_queries(dbr._related_queries,_related_queries);
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
    while (hit!=_related_queries.end())
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
        errlog::log_error(LOG_LEVEL_ERROR,"Failed serializing db_query_record");
        return 1; // error.
      }
    else return 0;
  }

  int db_query_record::deserialize(const std::string &msg)
  {
    sp::db::record r;
    if (!r.ParseFromString(msg))
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Failed deserializing db_query_record");
        return 1; // error.
      }
    read_query_record(r);
    return 0;
  }

  int db_query_record::serialize_compressed(std::string &msg) const
  {
    sp::db::record r;
    create_query_record(r);
    std::string tmp;
    if (!r.SerializeToString(&tmp))
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Failed serializing db_query_record to gzip stream");
        return 1; // error.
      }
    ::google::protobuf::io::StringOutputStream zoss(&msg);
    ::google::protobuf::io::GzipOutputStream gzos(&zoss);
    ::google::protobuf::io::CodedOutputStream cos(&gzos);
    cos.WriteString(tmp);
    return 0;
  }

  int db_query_record::deserialize_compressed(const std::string &msg)
  {
    sp::db::record r;
    std::istringstream iss(msg,std::istringstream::out);
    ::google::protobuf::io::IstreamInputStream ziss(&iss);
    ::google::protobuf::io::GzipInputStream gzis(&ziss);
    if (!r.ParseFromZeroCopyStream(&gzis))
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Failed deserializing db_query_record from gzip_stream");
        // try uncompressed deserialization.
        return deserialize(msg);
      }
    read_query_record(r);
    return 0;
  }

  db_err db_query_record::merge_with(const db_record &dbr)
  {
    if (dbr._plugin_name != _plugin_name)
      return DB_ERR_MERGE_PLUGIN;

    const db_query_record &dqr = static_cast<const db_query_record&>(dbr);
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator fhit;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
    = dqr._related_queries.begin();
    while (hit!=dqr._related_queries.end())
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
    return SP_ERR_OK;
  }

  void db_query_record::create_query_record(sp::db::record &r) const
  {
    create_base_record(r);
    sp::db::related_queries *queries = r.MutableExtension(sp::db::queries);
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
    = _related_queries.begin();
    while (hit!=_related_queries.end())
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
            while (vhit!=rd->_visited_urls->end())
              {
                vurl_data *vd = (*vhit).second;
                if (vd) // XXX: should not happen.
                  {
                    sp::db::visited_url *rq_vurl = rq_vurls->add_vurl();
                    rq_vurl->set_url(vd->_url);
                    rq_vurl->set_hits(vd->_hits);
                    if (!vd->_title.empty())
                      {
                        rq_vurl->set_title(vd->_title);
                        rq_vurl->set_summary(vd->_summary);
                        rq_vurl->set_url_date(vd->_url_date);
                        rq_vurl->set_rec_date(vd->_rec_date);
                        rq_vurl->set_url_lang(vd->_url_lang);
                      }
                  }
                else errlog::log_error(LOG_LEVEL_DEBUG,"null vurl_data element in visited_urls when creating db_query_record");
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
    for (int i=0; i<nrq; i++)
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
        for (int j=0; j<nvurls; j++)
          {
            sp::db::visited_url *rq_vurl = rq_vurls->mutable_vurl(j);
            std::string url = rq_vurl->url();
            short uhits = rq_vurl->hits();
            std::string title = rq_vurl->title();
            std::string summary = rq_vurl->summary();
            uint32_t url_date = rq_vurl->url_date();
            uint32_t rec_date = rq_vurl->rec_date();
            std::string lang = rq_vurl->url_lang();
            vurl_data *vd = new vurl_data(url,uhits,title,summary,url_date,rec_date,lang);
            rd->_visited_urls->insert(std::pair<const char*,vurl_data*>(vd->_url.c_str(),vd));
          }
        _related_queries.insert(std::pair<const char*,query_data*>(rd->_query.c_str(),rd));
      }
  }

  void db_query_record::copy_related_queries(const hash_map<const char*,query_data*,hash<const char*>,eqstr> &rq,
      hash_map<const char*,query_data*,hash<const char*>,eqstr> &nrq)
  {
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
    = rq.begin();
    while (hit!=rq.end())
      {
        query_data *rd = (*hit).second;
        query_data *crd = new query_data(rd);
        nrq.insert(std::pair<const char*,query_data*>(crd->_query.c_str(),crd));
        ++hit;
      }
  }

  int db_query_record::fix_issue_169(user_db &cudb)
  {
    // lookup a radius 0 query. If one:
    // - convert the record's key.
    // - generate 0 to max_radius keys for this query.
    // - store the newly formed records in to the new DB.
    // - change their keys and store them into the new DB.
    // if none, skip.

    hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
    = _related_queries.begin();
    while (hit!=_related_queries.end())
      {
        query_data *qd = (*hit).second;
        if (qd->_radius != 0)
          {
            ++hit;
            continue;
          }

        // generate query hashes with proper hashing function.
        hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
        qprocess::generate_query_hashes(qd->_query,0,
                                        query_capture_configuration::_config->_max_radius,
                                        features);

        hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator fhit = features.begin();
        while (fhit!=features.end())
          {
            if ((*hit).first == 0)
              {
                // copy this record and store it with fixed key in new db.
                db_query_record cdqr(*this);
                cdqr._creation_time = _creation_time; // reset creation time to original time.
                std::string key_str = (*fhit).second.to_rstring();
                cudb.add_dbr(key_str,cdqr);
              }
            else
              {
                // store the query fragment with fixed key.
                db_query_record ndqr("query-capture",qd->_query,(*fhit).first);
                ndqr._creation_time = _creation_time; // reset creation time to original time.
                std::string key_str = (*fhit).second.to_rstring();
                cudb.add_dbr(key_str,ndqr);
              }
            ++fhit;
          }
        ++hit;
      }

    return 0;
  }

  int db_query_record::fix_issue_263()
  {
    std::vector<query_data*> to_insert;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
    = _related_queries.begin();
    while (hit!=_related_queries.end())
      {
        query_data *qd = (*hit).second;
        std::string query = qd->_query;
        std::string fixed_query = miscutil::chomp_cpp(query);
        if (fixed_query != qd->_query)
          {
            hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit2 = hit;
            ++hit;
            _related_queries.erase(hit2);
            qd->_query = fixed_query;
            to_insert.push_back(qd);
          }
        else ++hit;
      }
    size_t tis = to_insert.size();
    if (tis > 0)
      {
        for (size_t i=0; i<tis; i++)
          _related_queries.insert(std::pair<const char*,query_data*>(to_insert.at(i)->_query.c_str(),
                                  to_insert.at(i)));
        return 1;
      }
    else return 0;
  }

  int db_query_record::fix_issue_281(uint32_t &fixed_urls)
  {
    int fixed_queries = 0;
    std::vector<vurl_data*> to_insert;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
    = _related_queries.begin();
    while (hit!=_related_queries.end())
      {
        query_data *qd = (*hit).second;

        if (!qd->_visited_urls)
          {
            ++hit;
            continue;
          }

        hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator vit
        = qd->_visited_urls->begin();
        while(vit!=qd->_visited_urls->end())
          {
            vurl_data *vd = (*vit).second;
            if (vd->_url[vd->_url.length()-1] == '/')
              {
                std::string url = vd->_url.substr(0,vd->_url.length()-1);  // fix url.
                hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator vit2 = vit;
                ++vit;
                qd->_visited_urls->erase(vit2);
                vd->_url = url;
                to_insert.push_back(vd);
                fixed_urls++;
              }
            else ++vit;
          }

        size_t tis = to_insert.size();
        if (tis > 0)
          {
            for (size_t i=0; i<tis; i++)
              qd->_visited_urls->insert(std::pair<const char*,vurl_data*>(to_insert.at(i)->_url.c_str(),
                                        to_insert.at(i)));
            fixed_queries++;
            to_insert.clear();
          }

        ++hit;
      }
    return fixed_queries;
  }

  /**
   * we dump non UTF8 queries, and try to fix non UTF8 URLs.
   */
  int db_query_record::fix_issue_154(uint32_t &fixed_urls, uint32_t &fixed_queries,
                                     uint32_t &removed_urls)
  {
    int dumped_queries = 0;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
    = _related_queries.begin();
    while (hit!=_related_queries.end())
      {
        query_data *qd = (*hit).second;
        char *conv_query = charset_conv::iconv_convert("UTF-8","UTF-8",qd->_query.c_str());

        bool dumped_q = false;
        if (!conv_query)
          {
            hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit2 = hit;
            ++hit;
            _related_queries.erase(hit2);
            delete qd;
            qd = NULL;
            dumped_q = true;
            dumped_queries++;
          }
        else free(conv_query);

        // check on URLs.
        if (qd && qd->_visited_urls)
          {
            std::vector<vurl_data*>to_insert_u;
            hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator vit
            = qd->_visited_urls->begin();
            while(vit!=qd->_visited_urls->end())
              {
                vurl_data *vd = (*vit).second;
                std::string vurl = vd->_url;
                char *conv_url = charset_conv::iconv_convert("UTF-8","UTF-8",vurl.c_str());
                if (!conv_url)
                  {
#ifdef FEATURE_ICU
                    // detect url charset.
                    int32_t c = 0;
                    const char *cs = charset_conv::icu_detection_best_match(qd->_query.c_str(),qd->_query.size(),&c);
                    if (cs)
                      {
                        //std::cerr << " detected url charset: " << cs << " with confidence: " << c << std::flush << std::endl;

                        // if not UTF-8, convert it.
                        int32_t clen = 0;
                        char *target = charset_conv::icu_conversion(cs,"UTF8",vd->_url.c_str(),&clen);

                        if (target)
                          {
                            vurl = std::string(target,clen);
                            //std::cerr << "icu converted url: " << vd->_url << std::flush << std::endl;
                            free(target);
                            to_insert_u.push_back(vd);
                            fixed_urls++;
                          }
                      }
#else
                    // try some blind conversions with iconv.
                    conv_url = charset_conv::iconv_convert("ISO_8859-1","UTF-8",vd->_url.c_str());
                    if (conv_url)
                      {
                        vurl= std::string(conv_url);
                        //std::cerr << "iconv converted url: " << qd->_query << std::endl;
                        to_insert_u.push_back(vd);
                        fixed_urls++;
                        free(conv_url);
                      }
#endif
                    // erase URL.
                    hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator vit2 = vit;
                    ++vit;
                    qd->_visited_urls->erase(vit2);
                    if (vd->_url != vurl)
                      vd->_url = vurl;
                    else
                      {
                        removed_urls++;
                        delete vd;
                      }
                  }
                else
                  {
                    free(conv_url);
                    ++vit;
                  }
              } // end while visited_urls.

            if (!to_insert_u.empty() && !dumped_q)
              fixed_queries++;
            for (size_t i=0; i<to_insert_u.size(); i++)
              {
                qd->_visited_urls->insert(std::pair<const char*,vurl_data*>(to_insert_u.at(i)->_url.c_str(),
                                          to_insert_u.at(i)));
              }
          }

        if (!dumped_q)
          ++hit;

      } // end iterate related_queries.

    return dumped_queries;
  }

  std::string db_query_record::fix_issue_575(uint32_t &fixed_queries)
  {
    std::string nkey;
    std::vector<query_data*> to_insert;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
    = _related_queries.begin();
    while (hit!=_related_queries.end())
      {
        query_data *qd = (*hit).second;
        std::string lc_query = qd->_query;
        miscutil::to_lower(lc_query);

        if (lc_query != qd->_query)
          {
            if (qd->_radius == 0)
              {
                // generate new record key.
                hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
                qprocess::generate_query_hashes(lc_query,0,0,features);
                if (!features.empty())
                  nkey = (*features.begin()).second.to_rstring();
              }

            hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit2 = hit;
            ++hit;
            _related_queries.erase(hit2);
            qd->_query = lc_query;
            to_insert.push_back(qd);
            fixed_queries++;
          }
        else ++hit;
      } // end iterate related_queries.
    size_t tis = to_insert.size();
    if (tis > 0)
      {
        for (size_t i=0; i<tis; i++)
          _related_queries.insert(std::pair<const char*,query_data*>(to_insert.at(i)->_query.c_str(),
                                  to_insert.at(i)));
      }
    return nkey;
  }

  void db_query_record::fetch_url_titles(uint32_t &fetched_urls,
                                         const long &timeout,
                                         const std::vector<std::list<const char*>*> *headers)
  {
    std::vector<vurl_data*> to_insert;
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
    = _related_queries.begin();
    while (hit!=_related_queries.end())
      {
        query_data *qd = (*hit).second;

        if (!qd->_visited_urls)
          {
            ++hit;
            continue;
          }

        hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator vit
        = qd->_visited_urls->begin();
        while(vit!=qd->_visited_urls->end())
          {
            vurl_data *vd = (*vit).second;
            if (vd->_title.empty())
              {
                std::vector<std::string> titles;
                std::vector<std::string> uris;
                uris.push_back(vd->_url);
                errlog::log_error(LOG_LEVEL_DEBUG,"fetching uri: %s",vd->_url.c_str());
                uc_err ferr = uri_capture::fetch_uri_html_title(uris,titles,timeout,headers);
                if (ferr == SP_ERR_OK)
                  {
                    fetched_urls++;
                    if (titles.at(0) != "404")
                      vd->_title = titles.at(0);
                  }
              }
            ++vit;
          }
        ++hit;
      }
  }


} /* end of namespace. */
