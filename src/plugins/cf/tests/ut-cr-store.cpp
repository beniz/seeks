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

TEST(CRTtest,cr_record)
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

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
