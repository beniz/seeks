/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Loic Dachary <loic@dacary.org>
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
 **/

#define _PCREPOSIX_H // avoid pcreposix.h conflict with regex.h used by gtest
#include <gtest/gtest.h>

#include "json_renderer.h"

using namespace seeks_plugins;

TEST(JsonRendererTest, render_engines) {
  EXPECT_EQ("\"google\",\"bing\",\"yauba\",\"yahoo\",\"exalead\",\"twitter\"", json_renderer::render_engines(SE_GOOGLE|SE_BING|SE_YAUBA|SE_YAHOO|SE_EXALEAD|SE_TWITTER));
}

TEST(JsonRendererTest, render_snippets) {
  websearch::_wconfig = new websearch_configuration("not a real filename");
  std::string json_str;

  int current_page = 1;
  std::vector<search_snippet*> snippets;
  const std::string query_clean;
  hash_map<const char*, const char*, hash<const char*>, eqstr> parameters;

  // zero snippet
  ASSERT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  ASSERT_EQ("\"snippets\":[]", json_str);

  // 1 snippet, page 1
  json_str = "";
  snippets.resize(1);
  search_snippet s1;
  s1.set_url("URL1");
  snippets[0] = &s1;
  ASSERT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  ASSERT_TRUE(json_str.find(s1._url) != std::string::npos);
  ASSERT_EQ(std::string::npos, json_str.find("thumb"));
  
  // 1 snippet, page 1, thumbs on
  json_str = "";
  parameters.insert(std::pair<const char*,const char*>("thumbs", "on"));
  ASSERT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  ASSERT_TRUE(json_str.find(s1._url) != std::string::npos);
  ASSERT_NE(std::string::npos, json_str.find("thumb"));

  // 3 snippets, page 2, 2 result per page, thumbs on
  snippets.resize(3);
  search_snippet s2;
  s2.set_url("URL2");
  snippets[1] = &s2;
  search_snippet s3;
  s3.set_url("URL3");
  snippets[2] = &s3;
  parameters.insert(std::pair<const char*,const char*>("rpp", "2"));

  current_page = 2;
  json_str = "";
  ASSERT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  ASSERT_EQ(std::string::npos, json_str.find(s1._url));
  ASSERT_EQ(std::string::npos, json_str.find(s2._url));
  ASSERT_NE(std::string::npos, json_str.find(s3._url));

  // 3 snippets, page 1, 2 result per page, thumbs on
  current_page = 1;
  json_str = "";
  ASSERT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  ASSERT_NE(std::string::npos, json_str.find(s1._url));
  ASSERT_NE(std::string::npos, json_str.find(s2._url));
  ASSERT_EQ(std::string::npos, json_str.find(s3._url));

  // 3 snippets, page 1, 2 result per page, similarity on, thumbs on
  current_page = 1;
  s1._seeks_ir = 1.0;
  s2._seeks_ir = 0.0;
  s3._seeks_ir = 1.0;
  json_str = "";
  ASSERT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  ASSERT_EQ("\"snippets\":[]", json_str);
  ASSERT_EQ(std::string::npos, json_str.find(s1._url));
  ASSERT_NE(std::string::npos, json_str.find(s2._url));
  ASSERT_EQ(std::string::npos, json_str.find(s3._url));
}

TEST(JsonRendererTest, render_clustered_snippets) {
  websearch::_wconfig = new websearch_configuration("not a real filename");
  query_context context;
  cluster clusters[2];

  std::string json_str;

  std::vector<search_snippet*> snippets;
  const std::string query_clean;
  hash_map<const char*, const char*, hash<const char*>, eqstr> parameters;
  
  search_snippet s1;
  s1.set_url("URL1");
  context.add_to_unordered_cache(&s1);
  clusters[0].add_point(s1._id, NULL);
  clusters[0]._label = "CLUSTER1";

  search_snippet s2;
  s2.set_url("URL2");
  context.add_to_unordered_cache(&s2);
  clusters[1].add_point(s2._id, NULL);
  clusters[1]._label = "CLUSTER2";

  ASSERT_EQ(SP_ERR_OK, json_renderer::render_clustered_snippets(query_clean, clusters, 2, &context, json_str, &parameters));
  ASSERT_NE(std::string::npos, json_str.find(clusters[0]._label));
  ASSERT_NE(std::string::npos, json_str.find(s1._url));
  ASSERT_NE(std::string::npos, json_str.find(clusters[1]._label));
  ASSERT_NE(std::string::npos, json_str.find(s2._url));
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  RUN_ALL_TESTS();
}
