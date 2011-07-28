/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Loic Dachary <loic@dachary.org>
 *               2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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
#include "json_renderer_private.h"

#include "seeks_proxy.h"
#include "proxy_configuration.h"

using namespace seeks_plugins;
using namespace json_renderer_private;
using sp::seeks_proxy;
using sp::proxy_configuration;

TEST(JsonRendererTest, render_engines)
{
  websearch::_wconfig = new websearch_configuration(""); // default configuration.
  //websearch::_wconfig->set_default_engines();
  /*fd.add_feed("google","url0");
  fd.add_feed("bing","url1");
  fd.add_feed("yauba","url2");
  fd.add_feed("yahoo","url3");
  fd.add_feed("exalead","url4");
  fd.add_feed("twitter","url5");*/
  std::string reng = json_renderer::render_engines(websearch::_wconfig->_se_default);
  EXPECT_EQ("\"bing\",\"google\",\"yahoo\"",reng);
  delete websearch::_wconfig;
}

TEST(JsonRendererTest, render_snippets)
{
  websearch::_wconfig = new websearch_configuration("not a real filename");
  websearch::_wconfig->_se_enabled = feeds("dummy","URL1");
  websearch::_wconfig->_se_default = feeds("dummy","URL1");
  websearch::_wconfig->_se_enabled.add_feed("dummy","URL2");
  websearch::_wconfig->_se_default.add_feed("dummy","URL2");
  websearch::_wconfig->_se_enabled.add_feed("dummy","URL3");
  websearch::_wconfig->_se_default.add_feed("dummy","URL3");
  std::string json_str;

  int current_page = 1;
  std::vector<search_snippet*> snippets;
  const std::string query_clean;
  hash_map<const char*, const char*, hash<const char*>, eqstr> parameters;

  // zero snippet
  EXPECT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  EXPECT_EQ("\"snippets\":[]", json_str);

  // 1 snippet, page 1
  json_str = "";
  snippets.resize(1);
  search_snippet s1;
  s1._engine = feeds("dummy","URL1");
  s1.set_url("URL1");
  snippets[0] = &s1;
  EXPECT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  EXPECT_TRUE(json_str.find(s1._url) != std::string::npos);
  EXPECT_EQ(std::string::npos, json_str.find("thumb"));

  // 1 snippet, page 1, thumbs on
  json_str = "";
  parameters.insert(std::pair<const char*,const char*>("thumbs", "on"));
  EXPECT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  EXPECT_TRUE(json_str.find(s1._url) != std::string::npos);
  EXPECT_NE(std::string::npos, json_str.find("thumb"));

  // 3 snippets, page 2, 2 results per page, thumbs on
  snippets.resize(3);
  search_snippet s2;
  s2._engine = feeds("dummy","URL2");
  s2.set_url("URL2");
  snippets[1] = &s2;
  search_snippet s3;
  s3._engine = feeds("dummy","URL3");
  s3.set_url("URL3");
  snippets[2] = &s3;
  parameters.insert(std::pair<const char*,const char*>("rpp", "2"));

  current_page = 2;
  json_str = "";
  EXPECT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  EXPECT_EQ(std::string::npos, json_str.find(s1._url));
  EXPECT_EQ(std::string::npos, json_str.find(s2._url));
  EXPECT_NE(std::string::npos, json_str.find(s3._url));

  // 3 snippets, page 1, 2 results per page, thumbs on
  current_page = 1;
  json_str = "";
  EXPECT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  EXPECT_NE(std::string::npos, json_str.find(s1._url));
  EXPECT_NE(std::string::npos, json_str.find(s2._url));
  EXPECT_EQ(std::string::npos, json_str.find(s3._url));

  // 3 snippets, page 1, 2 result per page, second snippet rejected, thumbs on
  current_page = 1;
  s1._doc_type = REJECTED;
  json_str = "";
  EXPECT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  EXPECT_EQ(std::string::npos, json_str.find(s1._url));
  EXPECT_NE(std::string::npos, json_str.find(s2._url));
  EXPECT_NE(std::string::npos, json_str.find(s3._url));
  s1._doc_type = WEBPAGE;

  // 3 snippets, page 1, 2 result per page, similarity on, thumbs on
  current_page = 1;
  s1._seeks_ir = 1.0;
  s2._seeks_ir = 0.0;
  s3._seeks_ir = 1.0;
  json_str = "";
  EXPECT_EQ(SP_ERR_OK, json_renderer::render_snippets(query_clean, current_page, snippets, json_str, &parameters));
  EXPECT_NE(std::string::npos, json_str.find(s1._url));
  EXPECT_EQ(std::string::npos, json_str.find(s2._url));
  EXPECT_NE(std::string::npos, json_str.find(s3._url));

  delete websearch::_wconfig;
}

TEST(JsonRendererTest, render_clustered_snippets)
{
  websearch::_wconfig = new websearch_configuration("not a real filename");
  websearch::_wconfig->_se_enabled = feeds("dummy","URL1");
  websearch::_wconfig->_se_default = feeds("dummy","URL1");
  websearch::_wconfig->_se_enabled.add_feed("dummy","URL2");
  websearch::_wconfig->_se_default.add_feed("dummy","URL2");
  websearch::_wconfig->_se_enabled.add_feed("dummy","URL3");
  websearch::_wconfig->_se_default.add_feed("dummy","URL3");
  query_context context;
  cluster clusters[3];

  std::string json_str;

  std::vector<search_snippet*> snippets;
  const std::string query_clean;
  hash_map<const char*, const char*, hash<const char*>, eqstr> parameters;

  search_snippet s1;
  s1._engine = feeds("dummy","URL1");
  s1.set_url("URL1");
  context.add_to_unordered_cache(&s1);
  clusters[0].add_point(s1._id, NULL);
  clusters[0]._label = "CLUSTER1";

  search_snippet s2;
  s2._engine = feeds("dummy","URL2");
  s2.set_url("URL2");
  context.add_to_unordered_cache(&s2);
  clusters[1].add_point(s2._id, NULL);
  search_snippet s3;
  s3._engine = feeds("dummy","URL3");
  s3.set_url("URL3");
  context.add_to_unordered_cache(&s3);
  clusters[1].add_point(s3._id, NULL);
  clusters[1]._label = "CLUSTER2";

  clusters[2]._label = "CLUSTER3";

  //parameters.insert(std::pair<const char*,const char*>("rpp", "1"));

  EXPECT_EQ(SP_ERR_OK, json_renderer::render_clustered_snippets(query_clean, clusters, 3, &context, json_str, &parameters));
  EXPECT_NE(std::string::npos, json_str.find(clusters[0]._label));
  EXPECT_NE(std::string::npos, json_str.find(s1._url));
  EXPECT_NE(std::string::npos, json_str.find(clusters[1]._label));
  EXPECT_NE(std::string::npos, json_str.find(s2._url));
  EXPECT_NE(std::string::npos, json_str.find(s3._url));
  EXPECT_EQ(std::string::npos, json_str.find(clusters[2]._label));

  delete websearch::_wconfig;
}

TEST(JsonRendererTest, render_json_results)
{
  websearch::_wconfig = new websearch_configuration("not a real filename");
  websearch::_wconfig->_se_enabled = feeds("dummy","URL1");
  websearch::_wconfig->_se_default = feeds("dummy","URL1");
  websearch::_wconfig->_se_enabled.add_feed("dummy","URL2");
  websearch::_wconfig->_se_default.add_feed("dummy","URL2");
  http_response* rsp;
  query_context context;
  double qtime = 1234;

  std::vector<search_snippet*> snippets;
  hash_map<const char*, const char*, hash<const char*>, eqstr> parameters;

  snippets.resize(2);

  search_snippet s1;
  s1._engine = feeds("dummy","URL1");
  s1.set_url("URL1");
  snippets[0] = &s1;

  search_snippet s2;
  s2._engine = feeds("dummy","URL2");
  s2.set_url("URL2");
  snippets[1] = &s2;
  parameters.insert(std::pair<const char*,const char*>("rpp", "1"));
  parameters.insert(std::pair<const char*,const char*>("callback", "JSONP"));
  context._query = "<QUERY>";

  // select page 1
  rsp = new http_response();
  EXPECT_EQ(SP_ERR_OK, json_renderer::render_json_results(snippets, NULL, rsp, &parameters, &context, qtime));
  EXPECT_NE(std::string::npos, std::string(rsp->_body).find("JSONP({"));
  EXPECT_NE(std::string::npos, std::string(rsp->_body).find("\"qtime\":1234"));
  EXPECT_NE(std::string::npos, std::string(rsp->_body).find("\"<QUERY>\""));
  EXPECT_NE(std::string::npos, std::string(rsp->_body).find(s1._url));
  EXPECT_EQ(std::string::npos, std::string(rsp->_body).find(s2._url));
  delete rsp;

  // select page 2
  parameters.insert(std::pair<const char*,const char*>("page", "2"));

  rsp = new http_response();
  EXPECT_EQ(SP_ERR_OK, json_renderer::render_json_results(snippets, NULL, rsp, &parameters, &context, qtime));
  EXPECT_EQ(std::string::npos, std::string(rsp->_body).find(s1._url));
  EXPECT_NE(std::string::npos, std::string(rsp->_body).find(s2._url));
  delete rsp;

  delete websearch::_wconfig;
}

TEST(JsonRendererTest, render_clustered_json_results)
{
  websearch::_wconfig = new websearch_configuration("not a real filename");
  websearch::_wconfig->_se_enabled = feeds("dummy","URL1");
  websearch::_wconfig->_se_default = feeds("dummy","URL1");

  cluster clusters[1];
  http_response* rsp;
  query_context context;
  double qtime = 1234;

  std::vector<search_snippet*> snippets;
  const std::string query_clean;
  hash_map<const char*, const char*, hash<const char*>, eqstr> parameters;

  search_snippet s1;
  s1._engine = feeds("dummy","URL1");
  s1.set_url("URL1");
  context.add_to_unordered_cache(&s1);
  clusters[0].add_point(s1._id, NULL);
  clusters[0]._label = "CLUSTER1";

  parameters.insert(std::pair<const char*,const char*>("callback", "JSONP"));
  context._query = "<QUERY>";

  rsp = new http_response();
  EXPECT_EQ(SP_ERR_OK, json_renderer::render_clustered_json_results(clusters, 1, NULL, rsp, &parameters, &context, qtime));
  EXPECT_NE(std::string::npos, std::string(rsp->_body).find("JSONP({"));
  EXPECT_NE(std::string::npos, std::string(rsp->_body).find("\"qtime\":1234"));
  EXPECT_NE(std::string::npos, std::string(rsp->_body).find("\"<QUERY>\""));
  EXPECT_NE(std::string::npos, std::string(rsp->_body).find(s1._url));
  EXPECT_NE(std::string::npos, std::string(rsp->_body).find(clusters[0]._label));
  //std::cerr << rsp->_body << std::endl;
  delete rsp;

  delete websearch::_wconfig;
}

TEST(JsonRendererTest, response)
{
  http_response rsp;
  response(&rsp, "JSON");
  EXPECT_EQ(rsp._content_length, strlen(rsp._body));
  EXPECT_STREQ("JSON", rsp._body);
}

TEST(JsonRendererTest, jsonp)
{
  std::string input("WHAT");
  const char* callback = "CALLBACK";
  EXPECT_EQ("CALLBACK(WHAT)", jsonp(input, callback));
  EXPECT_EQ("WHAT", jsonp(input, NULL));
}

TEST(JsonRendererTest, collect_json_results)
{
  websearch::_wconfig = new websearch_configuration("not a real filename");
  std::string result;
  std::list<std::string> results;
  query_context context;
  double qtime = 1234;
  hash_map<const char*, const char*, hash<const char*>, eqstr> parameters;

  context._query = "<QUERY>";
  context._auto_lang = "LANG";

  // select page 1
  EXPECT_EQ(SP_ERR_OK, collect_json_results(results, &parameters, &context, qtime));
  result = miscutil::join_string_list(",", results);
  EXPECT_NE(std::string::npos, result.find("\"LANG\""));
  EXPECT_NE(std::string::npos, result.find("\"date\""));
  EXPECT_NE(std::string::npos, result.find("\"qtime\":1234"));
  EXPECT_NE(std::string::npos, result.find("\"pers\":\"on\""));
  EXPECT_NE(std::string::npos, result.find("\"<QUERY>\""));
  EXPECT_EQ(std::string::npos, result.find("\"CMD\""));
  EXPECT_EQ(std::string::npos, result.find("\"suggestion\""));
  EXPECT_EQ(std::string::npos, result.find("\"engines\""));
  EXPECT_EQ(std::string::npos, result.find("\"yahoo\""));

  // prs precedence logic
  websearch::_wconfig->_personalization = false;
  results.clear();
  EXPECT_EQ(SP_ERR_OK, collect_json_results(results, &parameters, &context, qtime));
  result = miscutil::join_string_list(",", results);
  EXPECT_NE(std::string::npos, result.find("\"pers\":\"off\""));

  parameters.insert(std::pair<const char*,const char*>("prs", "on"));
  results.clear();
  EXPECT_EQ(SP_ERR_OK, collect_json_results(results, &parameters, &context, qtime));
  result = miscutil::join_string_list(",", results);
  EXPECT_NE(std::string::npos, result.find("\"pers\":\"on\""));

  // suggestion
  context._suggestions.insert(std::pair<double,std::string>(2.0,"SUGGESTION1"));
  context._suggestions.insert(std::pair<double,std::string>(1.0,"SUGGESTION2"));
  results.clear();
  EXPECT_EQ(SP_ERR_OK, collect_json_results(results, &parameters, &context, qtime));
  result = miscutil::join_string_list(",", results);
  EXPECT_NE(std::string::npos, result.find("suggestion"));
  EXPECT_NE(std::string::npos, result.find("SUGGESTION1"));
  EXPECT_NE(std::string::npos, result.find("SUGGESTION2"));

  // engines
  context._engines = websearch::_wconfig->_se_default;
  results.clear();
  EXPECT_EQ(SP_ERR_OK, collect_json_results(results, &parameters, &context, qtime));
  result = miscutil::join_string_list(",", results);
  EXPECT_NE(std::string::npos, result.find("\"yahoo\""));

  delete websearch::_wconfig;
}

TEST(JsonRendererTest, render_node_options)
{
  // init logging module.
  /* errlog::init_log_module();
  errlog::set_debug_level(LOG_LEVEL_ERROR | LOG_LEVEL_ERROR | LOG_LEVEL_INFO); */

  websearch::_wconfig = new websearch_configuration("../websearch-config");
  client_state *csp = new client_state();
  csp->_config = new proxy_configuration("../../../config");
  csp->_cfd = 0;
  std::list<std::string> opts;
  std::string json_opts;

  EXPECT_EQ(SP_ERR_OK, json_renderer::render_node_options(csp,opts));
  json_opts = miscutil::join_string_list(",",opts);
  //std::cerr << json_opts << std::endl;
  EXPECT_NE(std::string::npos, json_opts.find("\"version\""));
  EXPECT_EQ(std::string::npos, json_opts.find("\"my-ip-address\""));
  EXPECT_NE(std::string::npos, json_opts.find("\"code-status\""));
  EXPECT_EQ(std::string::npos, json_opts.find("\"admin-address\""));
  EXPECT_NE(std::string::npos, json_opts.find("\"url-source-code\""));

  EXPECT_NE(std::string::npos, json_opts.find("\"thumbs\""));
  EXPECT_NE(std::string::npos, json_opts.find("\"content-analysis\""));
  EXPECT_NE(std::string::npos, json_opts.find("\"clustering\""));
  delete csp->_config;
  delete csp;

  delete websearch::_wconfig;
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
