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

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>

#include "cf.h"
#include "cf_configuration.h"
#include "rank_estimators.h"
#include "query_capture.h"
#include "user_db.h"
#include "seeks_proxy.h"
#include "plugin_manager.h"
#include "proxy_configuration.h"
#include "lsh_configuration.h"
#include "qprocess.h"
#include "errlog.h"

using namespace seeks_plugins;
using namespace sp;
using namespace lsh;

const std::string dbfile = "seeks_test.db";
const std::string basedir = "../../../";

static std::string queries[2] =
  {
    "seeks",
    "seeks project"
  };

static std::string uris[3] =
  {
    "http://www.seeks-project.info/",
    "http://seeks-project.info/wiki/index.php/Documentation",
    "http://www.seeks-project.info/wiki/index.php/Download"
  };

class SRETest : public testing::Test
{
protected:
  SRETest()
  {
    
  }

  virtual ~SRETest()
  {

  }

  virtual void SetUp()
  {
    unlink(dbfile.c_str());
    seeks_proxy::_configfile = basedir + "config";
    seeks_proxy::_lshconfigfile = basedir + "lsh/lsh-config";
    seeks_proxy::initialize_mutexes();
    errlog::init_log_module();
    errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO);
    seeks_proxy::_basedir = basedir.c_str();
    plugin_manager::_plugin_repository = basedir + "/plugins/";
    seeks_proxy::_config = new proxy_configuration(seeks_proxy::_configfile);
    seeks_proxy::_lsh_config = new lsh_configuration(seeks_proxy::_lshconfigfile);

    seeks_proxy::_user_db = new user_db(dbfile);
    seeks_proxy::_user_db->open_db();
    plugin_manager::load_all_plugins();
    plugin_manager::instanciate_plugins();

    plugin *pl = plugin_manager::get_plugin("query-capture");
    ASSERT_TRUE(NULL!=pl);
    qcpl = static_cast<query_capture*>(pl);
    ASSERT_TRUE(NULL!=qcpl);
    qcelt = static_cast<query_capture_element*>(qcpl->_interceptor_plugin);
    
    // check that the db is empty.
    ASSERT_TRUE(seeks_proxy::_user_db!=NULL);
    ASSERT_EQ(0,seeks_proxy::_user_db->number_records());
    
    // feed the db with queries and urls.
    std::string url = uris[1];
    std::string host,path;
    query_capture::process_url(url,host,path);
    qcelt->store_queries(queries[0],url,host,"query-capture");
    std::string url2 = uris[2];
    query_capture::process_url(url2,host,path);
    qcelt->store_queries(queries[1],url2,host,"query-capture");
    ASSERT_EQ(3,seeks_proxy::_user_db->number_records()); // seeks, seeks project, project.
  }
  
  virtual void TearDown()
  {
    plugin_manager::close_all_plugins(); // XXX: beware, not very clean.
    delete seeks_proxy::_user_db;
    delete seeks_proxy::_lsh_config;
    delete seeks_proxy::_config;
    unlink(dbfile.c_str());
  }
  
  query_capture *qcpl;
  query_capture_element *qcelt;
};

//TODO: test estimate ranks.

TEST_F(SRETest,recommend_urls)
{
  static std::string lang = "en";
  simple_re sre;
  hash_map<uint32_t,search_snippet*,id_hash_uint> snippets;
  sre.recommend_urls(queries[0],lang,snippets);
  ASSERT_EQ(2,snippets.size());
  
  float url1_score = (log(2.0)+1.0) / (log(3.0)+5.0) 
    * cf_configuration::_config->_domain_name_weight / (log(3.0)+5.0);
  float url2_score = url1_score / (log(2)+1); // weighting down with query radius.

  std::string url1 = uris[1];
  std::string host,path;
  query_capture::process_url(url1,host,path);
  hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit
    = snippets.begin();
  ASSERT_EQ(url1,(*hit).second->_url);
  ASSERT_EQ(url1_score,(*hit).second->_seeks_rank);
  ++hit;
  std::string url2 = uris[2];
  query_capture::process_url(url2,host,path);
  ASSERT_EQ(url2,(*hit).second->_url);
  ASSERT_EQ(url2_score,(*hit).second->_seeks_rank);
}

TEST_F(SRETest,thumb_down_url)
{
  static std::string lang = "en";
  simple_re sre;
  std::string url1 = uris[1];
  std::string host,path;
  query_capture::process_url(url1,host,path);
  sre.thumb_down_url(queries[0],lang,url1);
 
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
  ASSERT_TRUE((*hit).second->_visited_urls==NULL);
  delete dbqr;

  std::string url2 = uris[2];
  query_capture::process_url(url2,host,path);
  sre.thumb_down_url(queries[0],lang,url2);
  dbr = seeks_proxy::_user_db->find_dbr(key_str,"query-capture");
  ASSERT_TRUE(dbr!=NULL);
  dbqr = dynamic_cast<db_query_record*>(dbr);
  ASSERT_TRUE(dbqr!=NULL);
  hit = dbqr->_related_queries.find(queries[0].c_str());
  ASSERT_TRUE((*hit).second->_visited_urls!=NULL);
  ASSERT_EQ(2,(*hit).second->_visited_urls->size());
  hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator vit 
    = (*hit).second->_visited_urls->begin();
  ASSERT_EQ(-1,(*vit).second->_hits);
  ++vit;
  ASSERT_EQ(-1,(*vit).second->_hits);
  delete dbqr;
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
