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

#include "peer_list.h"
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

TEST(PLTest,add_remove)
{
  peer_list pl;
  peer pe(hosts[0],ports[0],"","bsn");
  pl.add(&pe);
  ASSERT_EQ(1,pl._peers.size());
  pl.remove(hosts[0],ports[0],"");
  ASSERT_TRUE(pl._peers.empty());
}

TEST(DPTest,sweep)
{
  peer_list pl;
  peer_list dpl;
  dead_peer::_pl = &pl;
  dead_peer::_dpl = &dpl;

  dead_peer *dpe = new dead_peer(hosts[0],ports[0],"","bsn");
  dead_peer::_dpl->add(dpe);
  delete dpe;
  ASSERT_TRUE(dead_peer::_dpl->_peers.empty());
  ASSERT_EQ(1,dead_peer::_pl->_peers.size());
  peer *pe = (*dead_peer::_pl->_peers.begin()).second;
  dead_peer::_pl->remove(pe->_host,pe->_port,pe->_path);
  delete pe;
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
