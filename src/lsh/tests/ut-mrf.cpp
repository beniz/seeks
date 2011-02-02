/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "mrf.h"

using namespace lsh;

TEST(MrfTest, str_chain)
{
  str_chain s("seeks project",0,true);
  EXPECT_FALSE(s.has_skip());
  str_chain rs = s.rank_alpha();
  EXPECT_EQ("project seeks",rs.print_str());

  str_chain s2("seeks search",0,true);
  str_chain si = s2.intersect(s);
  EXPECT_EQ(1,si.size());
  EXPECT_EQ("seeks",si.at(0));

  si = s.intersect(s2);
  EXPECT_EQ(1,si.size());

  str_chain cs1("10 10 ub",0,true);
  str_chain cs2("ub 10",0,true);
  si = cs1.intersect(cs2);
  EXPECT_EQ(2,si.size());

  s2.remove_token(2);
  EXPECT_EQ("seeks search",s2.print_str());
  s2.remove_token(1);
  EXPECT_EQ("seeks",s2.print_str());
  str_chain s3("seeks search",0,true);
  s3.add_token("<skip>");
  EXPECT_TRUE(s3.has_skip());
  s3.remove_token(2);
  EXPECT_EQ("seeks search",s3.print_str());
  EXPECT_FALSE(s3.has_skip());
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
