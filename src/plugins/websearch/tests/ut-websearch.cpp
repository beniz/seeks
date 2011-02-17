/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 **/

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>

#include "websearch.h"
#include "websearch_configuration.h"
#include "proxy_configuration.h"
#include "sweeper.h"
#include "errlog.h"

using namespace seeks_plugins;
using sp::proxy_configuration;
using sp::sweeper;
using sp::errlog;

class WBTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO | LOG_LEVEL_DEBUG);
      websearch::_wconfig = new websearch_configuration(""); // default websearch configuration.
      websearch::_wconfig->_se_connect_timeout = 1;
      websearch::_wconfig->_se_transfer_timeout = 1;
      _pconfig = new proxy_configuration(""); // default proxy configuration.
      _pconfig->_templdir = strdup("");
    }

    virtual void TearDown()
    {
      delete websearch::_wconfig;
      delete _pconfig;
    }

    proxy_configuration *_pconfig;
};

class WBExistTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO | LOG_LEVEL_DEBUG);
      websearch::_wconfig = new websearch_configuration(""); // default websearch configuration.
      websearch::_wconfig->_se_connect_timeout = 1;
      websearch::_wconfig->_se_transfer_timeout = 1;
      _pconfig = new proxy_configuration(""); // default proxy configuration.
      _pconfig->_templdir = strdup("");
      _qc = new query_context();
      std::string query = "test";
      std::string lang = "en";
      _qc->_query_key = query_context::assemble_query(query,lang);
      _qc->_query_hash = query_context::hash_query_for_context(_qc->_query_key);
      _qc->register_qc();
    }

    virtual void TearDown()
    {
      delete websearch::_wconfig;
      delete _pconfig;
      sweeper::unregister_sweepable(_qc);
      delete _qc;
    }

    proxy_configuration *_pconfig;
    query_context *_qc;
};

TEST_F(WBTest,perform_websearch_bad_param_new)
{
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,&parameters,render);
  ASSERT_EQ(SP_ERR_CGI_PARAMS,err);
}

TEST_F(WBTest,perform_websearch_no_engine_fail_new)
{
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  miscutil::add_map_entry(parameters,"engines",1,"",1);
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,parameters,render);
  ASSERT_EQ(WB_ERR_NO_ENGINE,err);
  miscutil::free_map(parameters);
}

TEST_F(WBTest,perform_websearch_no_engine_output_fail_new)
{
  client_state csp;
  csp._config = _pconfig;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  miscutil::add_map_entry(parameters,"action",1,"expand",1);
  miscutil::add_map_entry(parameters,"engines",1,"dummy",1);
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,parameters,render);
  ASSERT_EQ(WB_ERR_SE_CONNECT,err);
  miscutil::free_map(parameters);
}

TEST_F(WBExistTest,perform_websearch_bad_param_new)
{
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"action",1,"expand",1);
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,parameters,render);
  ASSERT_EQ(SP_ERR_CGI_PARAMS,err);
  miscutil::free_map(parameters);
}

TEST_F(WBExistTest,perform_websearch_no_engine_fail_new)
{
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  miscutil::add_map_entry(parameters,"action",1,"expand",1);
  miscutil::add_map_entry(parameters,"engines",1,"",1);
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,parameters,render);
  ASSERT_EQ(WB_ERR_NO_ENGINE,err);
  miscutil::free_map(parameters);
}

TEST_F(WBExistTest,perform_websearch_no_engine_output_fail_new)
{
  client_state csp;
  csp._config = _pconfig;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  miscutil::add_map_entry(parameters,"action",1,"expand",1);
  miscutil::add_map_entry(parameters,"engines",1,"dummy",1);
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,parameters,render);
  ASSERT_EQ(WB_ERR_SE_CONNECT,err);
  miscutil::free_map(parameters);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
