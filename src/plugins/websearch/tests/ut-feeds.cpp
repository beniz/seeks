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

#include "feeds.h"

using namespace seeks_plugins;

TEST(FeedsTest,add_feed)
{
  feeds f1("dummy","url1");
  bool ret = f1.add_feed("seeks","url2");
  ASSERT_TRUE(ret);
  ASSERT_EQ(2,f1.count());
  ASSERT_EQ(2,f1.size());
  ret = f1.add_feed("dummy","url3");
  ASSERT_TRUE(ret);
  ASSERT_EQ(2,f1.count());
  ASSERT_EQ(3,f1.size());
  ret = f1.add_feed("dummy","url1");
  ASSERT_FALSE(ret);
  ASSERT_EQ(2,f1.count());
  ASSERT_EQ(3,f1.size());
  feed_parser fe;
  ret = f1.add_feed(fe);
  ASSERT_FALSE(ret);
}

TEST(FeedsTest,sunion)
{
  feeds f1("dummy","url1");
  feeds f2("seeks","url2");
  feeds f3 = f1.sunion(f2);
  ASSERT_EQ(2,f3.size());
  ASSERT_TRUE(f3.has_feed("seeks"));
  ASSERT_TRUE(f3.has_feed("dummy"));
}

TEST(FeedsTest,sunion2)
{
  feeds f1("seeks","url1");
  feeds f2("seeks","url2");
  feeds f3 = f1.sunion(f2);
  ASSERT_EQ(2,f3.size());
  ASSERT_EQ(1,f3.count());
  ASSERT_TRUE(f3.has_feed("seeks"));
}

TEST(FeedsTest,inter)
{
  feeds f1("seeks","url1");
  feeds f2("seeks","url2");
  feeds f3 = f1.inter(f2);
  ASSERT_EQ(0,f3.size());
}

TEST(FeedsTest,inter2)
{
  feeds f1("seeks","url1");
  feeds f2("seeks","url1");
  f2.add_feed("seeks","url2");
  feeds f3 = f1.inter(f2);
  ASSERT_EQ(1,f3.size());
  ASSERT_TRUE(f3.has_feed("seeks"));
  ASSERT_EQ("url1",(*f3._feedset.begin()).get_url());
}

TEST(FeedsTest,diff)
{
  feeds f1("seeks","url1");
  feeds f2("seeks","url1");
  feeds f3 = f1.diff(f2);
  ASSERT_EQ(0,f3.size());
  f2.add_feed("seeks","url2");
  ASSERT_EQ(2,f2.size());
  f3 = f1.diff(f2);
  ASSERT_EQ(1,f3.size());
  ASSERT_TRUE(f3.has_feed("seeks"));
  ASSERT_EQ("url2",(*f3._feedset.begin()).get_url());
  feeds fe;
  f3 = f1.diff(fe);
  ASSERT_EQ(f1.size(),f3.size());
  f1.add_feed("seeks project","url3");
  f3 = f2.diff(f1);
  ASSERT_EQ(2,f3.size());
}

TEST(FeedsTest,equal)
{
  feeds f1("seeks","url1");
  feeds f2("seeks","url1");
  ASSERT_TRUE(f1.equal(f2));
  f2.add_feed("seeks","url2");
  ASSERT_FALSE(f1.equal(f2));
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
