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

#include "urlmatch.h"
#include <string.h>

using sp::urlmatch;

TEST(urlmatch,parse_ip_host_port)
{
  char *haddr = strdup("localhost:8118");
  int hport = 0;
  urlmatch::parse_ip_host_port(haddr,hport);
  ASSERT_TRUE(haddr!=NULL);
  ASSERT_EQ("localhost",std::string(haddr));
  ASSERT_EQ(8118,hport);
  free(haddr);

  haddr = strdup("[::1]:8250");
  hport = 0;
  urlmatch::parse_ip_host_port(haddr,hport);
  ASSERT_TRUE(haddr!=NULL);
  ASSERT_EQ("::1",std::string(haddr));
  ASSERT_EQ(8250,hport);
  free(haddr);
}

TEST(urlmatch,parse_url_host_and_path)
{
  std::string url = "http://www.seeks-project.info/wiki/Documentation";
  std::string host,path;
  urlmatch::parse_url_host_and_path(url,host,path);
  ASSERT_EQ("www.seeks-project.info",host);
  ASSERT_EQ("/wiki/Documentation",path);
}

TEST(urlmatch,strip_url)
{
  std::string url1 = "http://www.seeks-project.info/wiki/Documentation";
  std::string url = urlmatch::strip_url(url1);
  ASSERT_EQ(url,"seeks-project.info/wiki/Documentation");
  std::string url2 = "http://seeks-project.info/wiki";
  url = urlmatch::strip_url(url2);
  ASSERT_EQ(url,"seeks-project.info/wiki");
  std::string url3 = "https://www.seeks-project.info/";
  url = urlmatch::strip_url(url3);
  ASSERT_EQ(url,"seeks-project.info");
}

TEST(urlmatch,next_elt_from_path)
{
  std::string path = "search/txt/seeks/7258314";
  std::string np = urlmatch::next_elt_from_path(path);
  ASSERT_EQ("search",np);
  ASSERT_EQ("txt/seeks/7258314",path);
  np = urlmatch::next_elt_from_path(path);
  ASSERT_EQ("txt",np);
  ASSERT_EQ("seeks/7258314",path);
  np = urlmatch::next_elt_from_path(path);
  ASSERT_EQ("seeks",np);
  ASSERT_EQ("7258314",path);
  np = urlmatch::next_elt_from_path(path);
  ASSERT_EQ("7258314",np);
  ASSERT_TRUE(path.empty());
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
