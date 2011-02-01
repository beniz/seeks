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

#include "uri_capture.h"
#include "db_uri_record.h"
#include "user_db.h"
#include "seeks_proxy.h"
#include "plugin_manager.h"
#include "proxy_configuration.h"
#include "errlog.h"

using namespace seeks_plugins;
using namespace sp;

const std::string dbfile = "seeks_test.db";
const std::string basedir = "../../../";

static std::string uris[3] =
{
  "http://www.seeks-project.info/",
  "http://seeks-project.info/wiki/index.php/Documentation",
  "http://www.seeks-project.info/wiki/index.php/Download"
};

class URICaptureTest : public testing::Test
{
  protected:
    URICaptureTest()
    {
    }

    virtual ~URICaptureTest()
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

      seeks_proxy::_user_db = new user_db(dbfile);
      seeks_proxy::_user_db->open_db();
      plugin_manager::load_all_plugins();
      plugin_manager::instanciate_plugins();

      plugin *pl = plugin_manager::get_plugin("uri-capture");
      ASSERT_TRUE(NULL!=pl);
      uripl = static_cast<uri_capture*>(pl);
      ASSERT_TRUE(NULL!=uripl);
      urielt = static_cast<uri_capture_element*>(uripl->_interceptor_plugin);

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

    uri_capture *uripl;
    uri_capture_element *urielt;
};

TEST(URIAPITest,prepare_uri)
{
  std::string purl0 = uri_capture_element::prepare_uri(uris[0]);
  ASSERT_EQ("seeks-project.info",purl0);
  std::string purl1 = uri_capture_element::prepare_uri(uris[1]);
  ASSERT_EQ("seeks-project.info/wiki/index.php/documentation",purl1);
}

TEST_F(URICaptureTest,store_remove)
{
  std::string url = uri_capture_element::prepare_uri(uris[1]);
  std::string host = uri_capture_element::prepare_uri(uris[0]);
  urielt->store_uri(url,host);
  ASSERT_EQ(2,seeks_proxy::_user_db->number_records());
  ASSERT_EQ(2,uripl->_nr);

  db_record *dbr = seeks_proxy::_user_db->find_dbr(url,"uri-capture");
  ASSERT_TRUE(dbr!=NULL);
  db_uri_record *uridbr = dynamic_cast<db_uri_record*>(dbr);
  ASSERT_TRUE(uridbr!=NULL);
  ASSERT_EQ(1,uridbr->_hits);
  delete uridbr;

  dbr = seeks_proxy::_user_db->find_dbr(host,"uri-capture");
  ASSERT_TRUE(dbr!=NULL);
  uridbr = dynamic_cast<db_uri_record*>(dbr);
  ASSERT_TRUE(uridbr!=NULL);
  ASSERT_EQ(1,uridbr->_hits);
  delete uridbr;

  urielt->remove_uri(url,host);
  ASSERT_EQ(0,seeks_proxy::_user_db->number_records());
  ASSERT_EQ(0,uripl->_nr);
}

TEST_F(URICaptureTest,store_merge_remove)
{
  std::string url = uri_capture_element::prepare_uri(uris[1]);
  std::string host = uri_capture_element::prepare_uri(uris[0]);
  urielt->store_uri(url,host);
  ASSERT_EQ(2,uripl->_nr);

  std::string url1 = uri_capture_element::prepare_uri(uris[2]);
  urielt->store_uri(url1,host);
  ASSERT_EQ(3,uripl->_nr);

  db_record *dbr = seeks_proxy::_user_db->find_dbr(host,"uri-capture");
  ASSERT_TRUE(dbr!=NULL);
  db_uri_record *uridbr = dynamic_cast<db_uri_record*>(dbr);
  ASSERT_TRUE(uridbr!=NULL);
  ASSERT_EQ(2,uridbr->_hits);
  delete uridbr;

  urielt->remove_uri(url,host);
  ASSERT_EQ(2,uripl->_nr);
  dbr = seeks_proxy::_user_db->find_dbr(host,"uri-capture");
  ASSERT_TRUE(dbr!=NULL);
  uridbr = dynamic_cast<db_uri_record*>(dbr);
  ASSERT_TRUE(uridbr!=NULL);
  ASSERT_EQ(1,uridbr->_hits);
  delete uridbr;
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
