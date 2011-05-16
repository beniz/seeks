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

#include "udb_server.h"
#include "query_capture.h"
#include "user_db.h"
#include "seeks_proxy.h"
#include "plugin_manager.h"
#include "proxy_configuration.h"
#include "lsh_configuration.h"
#include "qprocess.h"
#include "urlmatch.h"
#include "errlog.h"

using namespace seeks_plugins;

static std::string queries[3] =
{
  "seeks",
  "seeks project",
  "seeks project search"
};

static std::string uris[3] =
{
  "http://www.seeks-project.info/",
  "http://seeks-project.info/wiki/index.php/Documentation",
  "http://www.seeks-project.info/wiki/index.php/Download"
};

const std::string dbfile = "seeks_test.db";
const std::string basedir = "../../../";

class UDBSTest : public testing::Test
{
  protected:
    UDBSTest()
    {
    }

    virtual ~UDBSTest()
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
      plugin_manager::start_plugins();

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
      try
        {
          qcelt->store_queries(queries[0],url,host,"query-capture");
        }
      catch (sp_exception &e)
        {
          ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
        }
      ASSERT_EQ(1,seeks_proxy::_user_db->number_records());
      /*std::string url2 = uris[2];
      query_capture::process_url(url2,host,path);
      try
        {
          qcelt->store_queries(queries[1],url2,host,"query-capture");
        }
      catch (sp_exception &e)
        {
          ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
        }
      	ASSERT_EQ(3,seeks_proxy::_user_db->number_records()); */ // seeks, seeks project, project.
    }

    virtual void TearDown()
    {
      plugin_manager::close_all_plugins();
      delete seeks_proxy::_user_db;
      delete seeks_proxy::_lsh_config;
      delete seeks_proxy::_config;
      unlink(dbfile.c_str());
    }

    query_capture *qcpl;
    query_capture_element *qcelt;
};

TEST_F(UDBSTest,find_dbr_cb_no_rec)
{
  std::string key = "bla";
  std::string pn = "query-capture";
  http_response rsp;
  db_err err = udb_server::find_dbr_cb(key.c_str(),pn.c_str(),&rsp);
  ASSERT_EQ(DB_ERR_NO_REC,err);
}

TEST_F(UDBSTest,find_dbr_cb)
{
  std::string key = "1645a6897e62417931f26bcbdf4687c9c026b626"; // key for 'seeks'.
  std::string pn = "query-capture";
  http_response rsp;
  db_err err = udb_server::find_dbr_cb(key.c_str(),pn.c_str(),&rsp);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string str(rsp._body,rsp._content_length);
  ASSERT_FALSE(str.empty());
  plugin *pl = plugin_manager::get_plugin(pn);
  ASSERT_FALSE(NULL == pl);
  db_record * dbr = pl->create_db_record();
  int serr = dbr->deserialize(str);
  ASSERT_EQ(0,serr);
  delete dbr;
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
