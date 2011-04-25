/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#include "search_snippet.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "websearch.h"
#include "websearch_configuration.h"

using namespace seeks_plugins;

TEST(SearchSnippetTest,is_se_enabled)
{
  hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
  search_snippet s0;
  s0._engine = feeds("zoot","url1");
  websearch::_wconfig = new websearch_configuration("");
  ASSERT_FALSE(s0.is_se_enabled(&parameters));
  websearch::_wconfig->_se_enabled.add_feed("seeks","urls");
  parameters.insert(std::pair<const char*,const char*>("engines","seeks,dummy"));
  search_snippet s1;
  s1._engine = feeds("seeks","url1");
  ASSERT_FALSE(s1.is_se_enabled(&parameters));
  search_snippet s2;
  s2._engine = feeds("yahoo","url2");
  ASSERT_FALSE(s2.is_se_enabled(&parameters));
  delete websearch::_wconfig;
  websearch::_wconfig = NULL;
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
