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

#include "plugin_manager.h"
#include "seeks_proxy.h"

using namespace sp;

TEST(plugin_manager,find_plugin_cgi_dispatcher)
{
  cgi_dispatcher cgd1("rsc1",NULL,NULL,TRUE);
  cgi_dispatcher cgd2("rsc1/rsc2",NULL,NULL,TRUE);
  plugin_manager::_cgi_dispatchers.insert(std::pair<const char*,cgi_dispatcher*>(cgd1._name,&cgd1));
  plugin_manager::_cgi_dispatchers.insert(std::pair<const char*,cgi_dispatcher*>(cgd2._name,&cgd2));
  cgi_dispatcher *cgd = plugin_manager::find_plugin_cgi_dispatcher("rsc1");
  ASSERT_EQ("rsc1",cgd->_name);
  cgd = plugin_manager::find_plugin_cgi_dispatcher("rsc1/rsc2/test");
  ASSERT_EQ("rsc1/rsc2",cgd->_name);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
