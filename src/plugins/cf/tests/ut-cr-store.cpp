/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "cr_store.h"
#include "seeks_proxy.h"
#include "rank_estimators.h"
#include "query_capture.h"
#include "user_db.h"
#include "plugin_manager.h"
#include "proxy_configuration.h"

using namespace seeks_plugins;
using namespace sp;

std::string hosts[2] =
{
  "seeks.node1",
  "seeks.node2"
};

int ports[2] =
{
  8250,
  8251
};

TEST(CRTest,cr_cache)
{
  cr_store crs;
  cr_cache *crc = new cr_cache(cr_store::generate_peer(hosts[0],ports[0]),&crs);
  std::string key = "1645a6897e62417931f26bcbdf4687c9c026b626";
  db_record *rec = new db_record();
  crc->add(key,rec);
  ASSERT_EQ(1,crc->_records.size());
  ASSERT_EQ(1,seeks_proxy::_memory_dust.size()); // automatically registered through sweeper.
  time_t lu = (*crc->_records.begin()).second->_last_use;
  cached_record *cr = crc->find(key);
  ASSERT_TRUE(NULL != cr);
  ASSERT_EQ(rec,cr->_rec);
  ASSERT_TRUE(cr->_last_use >= lu);
  sweeper::sweep_all(); // delete cr and unregister from dust.
  ASSERT_TRUE(seeks_proxy::_memory_dust.empty());
  ASSERT_TRUE(crc->_records.empty());
}

TEST(CRTest,cr_record)
{
  cr_store crs;
  std::string key = "1645a6897e62417931f26bcbdf4687c9c026b626";
  db_record *rec = new db_record();
  crs.add(cr_store::generate_peer(hosts[0],ports[0]),key,rec);
  ASSERT_EQ(1,crs._store.size());
  ASSERT_EQ(1,seeks_proxy::_memory_dust.size());
  db_record *rec_f = crs.find(cr_store::generate_peer(hosts[0],ports[0]),key);
  ASSERT_TRUE(rec == rec_f);
  sweeper::sweep_all(); // delete cr and unregister from dust.
  ASSERT_TRUE(crs._store.empty());
}

TEST(CRTest,find_dbr)
{
  std::string dbfile = "seeks_test.db";
  unlink(dbfile.c_str()); // just in case.
  user_db udb(dbfile);
  udb.open_db();
  seeks_proxy::_user_db = &udb;

  // load plugins.
  std::string basedir = "../../../";
  seeks_proxy::_basedir = basedir.c_str();
  seeks_proxy::_configfile = basedir + "config";
  seeks_proxy::_config = new proxy_configuration(seeks_proxy::_configfile);
  plugin_manager::_plugin_repository = basedir + "/plugins/";
  plugin_manager::load_all_plugins();
  plugin_manager::start_plugins();

  // access to plugins.
  plugin *pl = plugin_manager::get_plugin("query-capture");
  ASSERT_TRUE(NULL!=pl);
  query_capture *qcpl = static_cast<query_capture*>(pl);
  ASSERT_TRUE(NULL!=qcpl);
  query_capture_element *qcelt = static_cast<query_capture_element*>(qcpl->_interceptor_plugin);

  // add a record to the db.
  std::string query = "seeks";
  std::string url = "http://www.seeks-project.info/";
  std::string host,path;
  query_capture::process_url(url,host,path);
  try
    {
      qcelt->store_queries(query,url,host,"query-capture");
    }
  catch (sp_exception &e)
    {
      ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
    }

  // test find_dbr based on cr_store.
  std::string key = "1645a6897e62417931f26bcbdf4687c9c026b626";
  bool in_store = false;
  db_record *dbr = rank_estimator::find_dbr(&udb,key,"query-capture",in_store);
  ASSERT_TRUE(NULL!=dbr);
  db_query_record *dqr = dynamic_cast<db_query_record*>(dbr);
  ASSERT_TRUE(NULL!=dqr);
  ASSERT_EQ(1,dqr->_related_queries.size());
  query_data *qd = (*dqr->_related_queries.begin()).second;
  ASSERT_TRUE(NULL!=qd);
  ASSERT_EQ("seeks",qd->_query);
  ASSERT_EQ(2,qd->_visited_urls->size());
  std::string rkey = user_db::generate_rkey(key,"query-capture");
  rank_estimator::_store.add(host,-1,"",rkey,dbr);
  user_db udbr(false,host,-1,"","sn");
  db_record *dbr2 = rank_estimator::find_dbr(&udbr,key,"query-capture",in_store);
  ASSERT_EQ(dbr,dbr2);
  ASSERT_EQ(1,rank_estimator::_store._store.size());

  // clear all.
  sweeper::sweep_all();

  udb.close_db();
  plugin_manager::close_all_plugins();
  delete seeks_proxy::_config;
  unlink(dbfile.c_str());
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
