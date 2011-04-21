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
 */

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>

#include "se_handler.h"
#include "query_context.h"
#include "websearch.h"
#include "websearch_configuration.h"
#include "miscutil.h"
#include "errlog.h"

using namespace seeks_plugins;
using sp::miscutil;
using sp::errlog;

class SEHandlerTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO | LOG_LEVEL_DEBUG);
    }
};

TEST_F(SEHandlerTest,query_to_ses_fail_no_engine)
{
  feeds engines;
  hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
  int nresults = 0;
  query_context *qc = NULL;
  std::string **output = NULL;
  int code = SP_ERR_OK;
  try
    {
      output = se_handler::query_to_ses(&parameters,nresults,qc,engines);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_NO_ENGINE,code);
  parameters.clear();
}

TEST_F(SEHandlerTest,query_to_ses_fail_connect)
{
  feeds engines("dummy","url1");
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>(2);
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"",1);
  websearch::_wconfig = new websearch_configuration("");
  websearch::_wconfig->_se_connect_timeout = 1;
  websearch::_wconfig->_se_transfer_timeout = 1;
  ASSERT_TRUE(NULL!=websearch::_wconfig);
  int nresults = 0;
  query_context qc;
  std::string **output = NULL;
  int code = SP_ERR_OK;
  try
    {
      output = se_handler::query_to_ses(parameters,nresults,&qc,engines);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_NO_ENGINE_OUTPUT,code);
  delete websearch::_wconfig;
  miscutil::free_map(parameters);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
