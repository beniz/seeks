/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010, 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>

#include "query_capture.h"
#include "db_query_record.h"
#include "qprocess.h"
#include "user_db.h"
#include "seeks_proxy.h"
#include "plugin_manager.h"
#include "proxy_configuration.h"
#include "errlog.h"

using namespace seeks_plugins;
using namespace sp;
using namespace lsh;

const std::string dbfile = "seeks_test.db";
const std::string basedir = "../../../";

static std::string queries[1] =
{
  "seeks"
};

static std::string keys[1] =
{
  "1645a6897e62417931f26bcbdf4687c9c026b626"
};

static std::string uris[3] =
{
  "http://www.seeks-project.info/",
  "http://seeks-project.info/wiki/index.php/Documentation",
  "http://www.seeks-project.info/wiki/index.php/Download"
};

class QCTest : public testing::Test
{
  protected:
    QCTest()
    {
    }

    virtual ~QCTest()
    {
    }

    virtual void SetUp()
    {
      unlink(dbfile.c_str());
      seeks_proxy::_configfile = basedir + "config";
      seeks_proxy::initialize_mutexes();
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO);
      seeks_proxy::_basedir = basedir.c_str();
      plugin_manager::_plugin_repository = basedir + "/plugins/";
      seeks_proxy::_config = new proxy_configuration(seeks_proxy::_configfile);
      //query_capture_configuration *qcc = new query_capture_configuration("");

      seeks_proxy::_user_db = new user_db(dbfile);
      seeks_proxy::_user_db->open_db();
      plugin_manager::load_all_plugins();
      plugin_manager::start_plugins();

      plugin *pl = plugin_manager::get_plugin("query-capture");
      ASSERT_TRUE(NULL!=pl);
      qcpl = static_cast<query_capture*>(pl);
      ASSERT_TRUE(NULL!=qcpl);
      qcelt = static_cast<query_capture_element*>(qcpl->_interceptor_plugin);

      // check that the db is empty.
      ASSERT_TRUE(seeks_proxy::_user_db!=NULL);
      ASSERT_EQ(0,seeks_proxy::_user_db->number_records());
    }

    virtual void TearDown()
    {
      plugin_manager::close_all_plugins(); // XXX: beware, not very clean.
      seeks_proxy::_user_db->close_db();
      delete seeks_proxy::_user_db;
      delete seeks_proxy::_config;
      unlink(dbfile.c_str());
    }

    query_capture *qcpl;
    query_capture_element *qcelt;
};

TEST(QCAPITest,process_url)
{
  std::string url = "http://www.seeks-project.info/pr/";
  std::string host, path;
  query_capture::process_url(url,host,path);
  EXPECT_EQ("seeks-project.info",host);
  EXPECT_EQ("/pr/",path);
  EXPECT_EQ("http://www.seeks-project.info/pr",url);
}

TEST_F(QCTest,store_url_sp)
{
  search_snippet sp;
  sp.set_url(uris[0]);
  std::string title = "The Seeks Project";
  sp.set_title(title);
  std::string summary = "Seeks Project homepage";
  sp.set_summary(summary);
  DHTKey key = DHTKey::from_rstring(keys[0]);
  std::string host;
  std::string url = uris[0];
  query_capture::process_url(url,host);
  uint32_t radius = 0;
  try
    {
      qcelt->store_url(key,queries[0],
                       url,host,radius,"query-capture",&sp);
    }
  catch (sp_exception &e)
    {
      ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
    }
  ASSERT_EQ(1,seeks_proxy::_user_db->number_records());
  db_record *dbr = seeks_proxy::_user_db->find_dbr(keys[0],"query-capture");
  ASSERT_TRUE(dbr!=NULL);
  db_query_record *dbqr = dynamic_cast<db_query_record*>(dbr);
  ASSERT_TRUE(dbqr!=NULL);
  hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
  = dbqr->_related_queries.find(queries[0].c_str());
  ASSERT_FALSE(dbqr->_related_queries.end()==hit);
  ASSERT_TRUE((*hit).second!=NULL);
  ASSERT_EQ(queries[0],(*hit).second->_query);
  ASSERT_EQ(0,(*hit).second->_radius);
  ASSERT_EQ(1,(*hit).second->_hits);
  ASSERT_TRUE((*hit).second->_visited_urls!=NULL);
  ASSERT_EQ(1,(*hit).second->_visited_urls->size());
  vurl_data *vd = (*(*hit).second->_visited_urls->begin()).second;
  ASSERT_TRUE(vd!=NULL);
  ASSERT_EQ(url,vd->_url);
  ASSERT_EQ(title,vd->_title);
  ASSERT_EQ(summary,vd->_summary);
}

TEST_F(QCTest,store_queries)
{
  try
    {
      qcelt->store_queries(queries[0],"query-capture");
    }
  catch (sp_exception &e)
    {
      ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
    }

  ASSERT_EQ(1,seeks_proxy::_user_db->number_records());

  hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
  qprocess::generate_query_hashes(queries[0],0,5,features);
  ASSERT_EQ(1,features.size());
  DHTKey key = (*features.begin()).second;
  std::string key_str = key.to_rstring();
  db_record *dbr = seeks_proxy::_user_db->find_dbr(key_str,"query-capture");
  ASSERT_TRUE(dbr!=NULL);
  db_query_record *dbqr = dynamic_cast<db_query_record*>(dbr);
  ASSERT_TRUE(dbqr!=NULL);
  ASSERT_EQ(1,dbqr->_related_queries.size());
  hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
  = dbqr->_related_queries.find(queries[0].c_str());
  ASSERT_FALSE(dbqr->_related_queries.end()==hit);
  ASSERT_TRUE((*hit).second!=NULL);
  ASSERT_EQ(queries[0],(*hit).second->_query);
  ASSERT_EQ(0,(*hit).second->_radius);
  ASSERT_EQ(1,(*hit).second->_hits);
  ASSERT_TRUE((*hit).second->_visited_urls==NULL);
  delete dbqr;
}

TEST_F(QCTest,store_queries_url)
{
  std::string url = uris[1];
  std::string host,path;
  query_capture::process_url(url,host,path);
  try
    {
      qcelt->store_queries(queries[0],url,host,"query-capture");
    }
  catch (sp_exception &e)
    {
      ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
    }
  ASSERT_EQ(1,seeks_proxy::_user_db->number_records());

  hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
  qprocess::generate_query_hashes(queries[0],0,5,features);
  ASSERT_EQ(1,features.size());
  DHTKey key = (*features.begin()).second;
  std::string key_str = key.to_rstring();
  db_record *dbr = seeks_proxy::_user_db->find_dbr(key_str,"query-capture");
  ASSERT_TRUE(dbr!=NULL);
  db_query_record *dbqr = dynamic_cast<db_query_record*>(dbr);
  ASSERT_TRUE(dbqr!=NULL);

  hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
  = dbqr->_related_queries.find(queries[0].c_str());
  ASSERT_TRUE((*hit).second->_visited_urls!=NULL);
  ASSERT_EQ(2,(*hit).second->_visited_urls->size()); // host and url.
  delete dbqr;
}

TEST_F(QCTest,store_queries_url_merge)
{
  std::string url = uris[1];
  std::string host,path;
  query_capture::process_url(url,host,path);
  try
    {
      qcelt->store_queries(queries[0],url,host,"query-capture");
    }
  catch (sp_exception &e)
    {
      ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
    }
  ASSERT_EQ(1,seeks_proxy::_user_db->number_records());

  std::string url2 = uris[2];
  query_capture::process_url(url2,host,path);
  try
    {
      qcelt->store_queries(queries[0],url2,host,"query-capture");
    }
  catch (sp_exception &e)
    {
      ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
    }
  ASSERT_EQ(1,seeks_proxy::_user_db->number_records());

  hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
  qprocess::generate_query_hashes(queries[0],0,5,features);
  ASSERT_EQ(1,features.size());
  DHTKey key = (*features.begin()).second;
  std::string key_str = key.to_rstring();
  db_record *dbr = seeks_proxy::_user_db->find_dbr(key_str,"query-capture");
  ASSERT_TRUE(dbr!=NULL);
  db_query_record *dbqr = dynamic_cast<db_query_record*>(dbr);
  ASSERT_TRUE(dbqr!=NULL);
  ASSERT_EQ(1,dbqr->_related_queries.size());
  hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
  = dbqr->_related_queries.find(queries[0].c_str());
  ASSERT_TRUE((*hit).second->_visited_urls!=NULL);
  ASSERT_EQ(3,(*hit).second->_visited_urls->size()); // one domain, two urls.
  hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator vit
  = (*hit).second->_visited_urls->find(host.c_str());
  ASSERT_TRUE(vit!=(*hit).second->_visited_urls->end());
  ASSERT_EQ(2,(*vit).second->_hits);
  delete dbqr;
}

TEST_F(QCTest,remove_url)
{
  std::string url = uris[1];
  std::string host,path;
  query_capture::process_url(url,host,path);
  try
    {
      qcelt->store_queries(queries[0],url,host,"query-capture");
    }
  catch (sp_exception &e)
    {
      ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
    }
  ASSERT_EQ(1,seeks_proxy::_user_db->number_records());

  hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
  qprocess::generate_query_hashes(queries[0],0,5,features);
  ASSERT_EQ(1,features.size());
  DHTKey key = (*features.begin()).second;
  qcelt->remove_url(key,queries[0],url,host,
                    1,0,"query-capture");
  ASSERT_EQ(1,seeks_proxy::_user_db->number_records());

  std::string key_str = key.to_rstring();
  db_record *dbr = seeks_proxy::_user_db->find_dbr(key_str,"query-capture");
  ASSERT_TRUE(dbr!=NULL);
  db_query_record *dbqr = dynamic_cast<db_query_record*>(dbr);
  ASSERT_TRUE(dbqr!=NULL);
  ASSERT_EQ(1,dbqr->_related_queries.size());
  hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
  = dbqr->_related_queries.find(queries[0].c_str());
  ASSERT_TRUE((*hit).second->_visited_urls==NULL); // protobuffers read no urls in record.
  delete dbqr;
}

TEST(DBRTest,serialize_deserialize)
{
  db_query_record dbr("query-capture",queries[0],0,
                      uris[1],1,1,"Seeks Project Documentation",
                      "Seeks project documentation",0);
  std::string msg;
  int err = dbr.serialize(msg);
  ASSERT_EQ(0,err);
  std::cerr << "msg length: " << msg.length() << std::endl;
  err = dbr.deserialize(msg);
  ASSERT_EQ(0,err);
}

TEST(DBRTest,serialize_deserialize_compressed)
{
  db_query_record dbr("query-capture",queries[0],0,
                      uris[1],1,1,"Seeks Project Documentation",
                      "Seeks project documentation",0);
  std::string msg;
  int err = dbr.serialize_compressed(msg);
  ASSERT_EQ(0,err);
  ASSERT_FALSE(msg.empty());
  std::cerr << "msg length: " << msg.length() << std::endl;
  err = dbr.deserialize_compressed(msg);
  ASSERT_EQ(0,err);
}

TEST(DBRTest,serialize_deserialize_compressed_mix)
{
  db_query_record dbr("query-capture",queries[0],0,
                      uris[1],1,1,"Seeks Project Documentation",
                      "Seeks project documentation",0);
  std::string msg;
  int err = dbr.serialize(msg);
  ASSERT_EQ(0,err);
  ASSERT_FALSE(msg.empty());
  std::cerr << "msg length: " << msg.length() << std::endl;
  err = dbr.deserialize_compressed(msg);
  ASSERT_EQ(0,err);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
