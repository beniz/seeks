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

#include "query_context.h"
#include "websearch.h"
#include "websearch_configuration.h"
#include "errlog.h"

using namespace seeks_plugins;
using sp::errlog;

class QCTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO | LOG_LEVEL_DEBUG);
      websearch::_wconfig = new websearch_configuration("");
      websearch::_wconfig->_se_connect_timeout = 1;
      websearch::_wconfig->_se_transfer_timeout = 1;
    }

    virtual void TearDown()
    {
      delete websearch::_wconfig;
    }
};

TEST_F(QCTest,expand_no_engine_fail)
{
  query_context qc;
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
  std::bitset<NSEs> engines;
  int code = SP_ERR_OK;
  try
    {
      qc.expand(&csp,&rsp,&parameters,0,1,engines);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_NO_ENGINE,code);
}

TEST_F(QCTest,expand_no_engine_output_fail)
{
  query_context qc;
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
  miscutil::add_map_entry(&parameters,"q",1,"test",1);
  miscutil::add_map_entry(&parameters,"expansion",1,"",1);
  std::bitset<NSEs> engines;
  engines.set(3);
  int code = SP_ERR_OK;
  try
    {
      qc.expand(&csp,&rsp,&parameters,0,1,engines);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_NO_ENGINE_OUTPUT,code);
}

TEST_F(QCTest,generate_expansion_param_fail)
{
  query_context qc;
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
  bool expanded = false;
  int code = SP_ERR_OK;
  try
    {
      qc.generate(&csp,&rsp,&parameters,expanded);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(SP_ERR_CGI_PARAMS,code);
}

TEST_F(QCTest,generate_wrong_expansion_fail)
{
  query_context qc;
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"bla",1);
  bool expanded = false;
  int code = SP_ERR_OK;
  try
    {
      qc.generate(&csp,&rsp,parameters,expanded);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(SP_ERR_CGI_PARAMS,code);
  miscutil::free_map(parameters);
}

TEST_F(QCTest,generate_no_engine_fail)
{
  query_context qc;
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  miscutil::add_map_entry(parameters,"engines",1,"",1);
  bool expanded = false;
  int code = SP_ERR_OK;
  try
    {
      qc.generate(&csp,&rsp,parameters,expanded);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_NO_ENGINE,code);
  miscutil::free_map(parameters);
}

TEST_F(QCTest,generate_no_engine_output_fail)
{
  query_context qc;
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  miscutil::add_map_entry(parameters,"engines",1,"dummy",1);
  bool expanded = false;
  int code = SP_ERR_OK;
  try
    {
      qc.generate(&csp,&rsp,parameters,expanded);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_NO_ENGINE_OUTPUT,code);
  miscutil::free_map(parameters);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
