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

#include "se_parser.h"
#include "errlog.h"

using namespace seeks_plugins;
using sp::errlog;

class ParserTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO | LOG_LEVEL_PARSER);
    }
};

TEST_F(ParserTest, parser_output_xml_normal)
{
  std::string page = "<xml>b</xml>";
  se_parser sep;
  std::vector<search_snippet*> snippets;
  try
    {
      sep.parse_output_xml((char*)page.c_str(),&snippets,0);
    }
  catch (sp_exception &e)
    {
      ASSERT_EQ(SP_ERR_OK,e.code()); // will fail if exception.
    }
}

TEST_F(ParserTest, parser_output_xml_fail)
{
  std::string page = "<li></ut>";
  se_parser sep;
  std::vector<search_snippet*> snippets;
  int code = SP_ERR_OK;
  try
    {
      sep.parse_output_xml((char*)page.c_str(),&snippets,0);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_PARSE,code);
}

TEST_F(ParserTest, parser_output_normal)
{
  std::string page = "<html>b</html>";
  se_parser sep;
  std::vector<search_snippet*> snippets;
  int code = SP_ERR_OK;
  try
    {
      sep.parse_output((char*)page.c_str(),&snippets,0);
    }
  catch (sp_exception &e)
    {
      ASSERT_EQ(SP_ERR_OK,e.code()); // will fail if exception.
    }
}

TEST_F(ParserTest, parser_output_fail)
{
  std::string page = "<li></ut>";
  se_parser sep;
  std::vector<search_snippet*> snippets;
  int code = SP_ERR_OK;
  try
    {
      sep.parse_output((char*)page.c_str(),&snippets,0);
    }
  catch (sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_PARSE,code);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
