/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010 Loic Dachary <loic@dachary.org>
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

#include "Location.h"
#include <functional>

#include <queue>
#include <vector>
#include <functional>

using namespace dht;

TEST(LocationTest, less)
{
  std::less<Location> f;
  ASSERT_TRUE(f(Location(2),Location(3)));
  ASSERT_FALSE(f(Location(3),Location(2)));
  ASSERT_FALSE(f(Location(3),Location(3)));
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
