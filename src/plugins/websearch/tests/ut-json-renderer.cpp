/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Loic Dachary <loic@dachary.org>
 *               2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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
  std::string reng = json_renderer::render_engines(websearch::_wconfig->_se_default);
  EXPECT_EQ("\"bing\",\"google\",\"yahoo\"",reng);

  /*  websearch::_wconfig = new websearch_configuration("");
  feeds fd;
  fd.add_feed("google","http://www.google.com/search?q=%query&start=%start&num=%num&hl=%lang&ie=%encoding&oe=%encoding");
  fd.add_feed("bing","http://www.bing.com/search?q=%query&first=%start&mkt=%lang");
  fd.add_feed("yahoo","http://search.yahoo.com/search?n=10&ei=UTF-8&va_vt=any&vo_vt=any&ve_vt=any&vp_vt=any&vd=all&vst=0&vf=all&vm\
  =p&fl=1&vl=lang_%lang&p=%query&vs=");
  EXPECT_EQ("\"bing\",\"google\",\"yahoo\"", json_renderer::render_engines(fd));*/
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
  /*context._engines = feeds("yahoo","http://search.yahoo.com/search?n=10&ei=UTF-8&va_vt=any&vo_vt=any&ve_vt=any&vp_vt=any&vd=all&vst=0&vf=all&vm=p&fl=1&vl=lang_%lang&p=%query&vs=");*/
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
  EXPECT_NE(std::string::npos, json_opts.find("\"txt-parsers\""));
  delete csp->_config;
  delete csp;

  delete websearch::_wconfig;
}

TEST(JsonRendererTest, render_suggested_queries)
{
  query_context qc;
  qc._suggestions.insert(std::pair<double,std::string>(1.0,"seeks"));
  qc._suggestions.insert(std::pair<double,std::string>(0.5,"seeks project"));
  std::string json_str = json_renderer::render_suggested_queries(&qc,3);
  EXPECT_NE(std::string::npos,json_str.find("\"suggestions\":"));
  EXPECT_NE(std::string::npos,json_str.find("\"seeks\""));
  EXPECT_NE(std::string::npos,json_str.find("\"seeks project\""));
  json_str = json_renderer::render_suggested_queries(&qc,1);
  EXPECT_EQ(std::string::npos,json_str.find("\"seeks\""));
}

TEST(JsonRendererTest, render_recommendations)
{
  websearch::_wconfig = new websearch_configuration("../websearch-config");
  std::string url1 = "http://www.seeks.mx/";
  std::string url2 = "http://www.seeks-project.info/";
  query_context qc;
  qc._query = "seeks project";
  qc._npeers = 1;
  search_snippet *sp1 = new search_snippet();
  sp1->set_url(url1);
  sp1->set_title("Seeks Search");
  sp1->set_lang("es");
  sp1->set_radius(1);
  sp1->_engine.add_feed("seeks","s.s");
  search_snippet *sp2 = new search_snippet();
  sp2->set_url(url2);
  sp2->set_title("Seeks Project");
  sp2->set_radius(0);
  sp2->_engine.add_feed("seeks","s.s");
  qc.add_to_cache(sp1);
  qc.add_to_cache(sp2);
  int radius = 5;
  std::string lang;
  std::string json_str = json_renderer::render_recommendations(&qc,10,0.0,radius,lang);
  EXPECT_NE(std::string::npos,json_str.find("\"npeers\":1"));
  EXPECT_NE(std::string::npos,json_str.find("\"query\":\"seeks project\""));
  EXPECT_NE(std::string::npos,json_str.find("\"date\":"));
  EXPECT_NE(std::string::npos,json_str.find("\"qtime\":0"));
  EXPECT_NE(std::string::npos,json_str.find("\"snippets\":["));
  EXPECT_NE(std::string::npos,json_str.find(url1));
  EXPECT_NE(std::string::npos,json_str.find(url2));
  lang = "es";
  json_str = json_renderer::render_recommendations(&qc,10,0.0,radius,lang);
  EXPECT_NE(std::string::npos,json_str.find(url1));
  EXPECT_EQ(std::string::npos,json_str.find(url2));
  lang = "";
  radius = 0;
  json_str = json_renderer::render_recommendations(&qc,10,0.0,radius,lang);
  EXPECT_EQ(std::string::npos,json_str.find(url1));
  EXPECT_NE(std::string::npos,json_str.find(url2));
  radius = 0;
  lang = "es";
  json_str = json_renderer::render_recommendations(&qc,10,0.0,radius,lang);
  EXPECT_EQ(std::string::npos,json_str.find(url1));
  EXPECT_EQ(std::string::npos,json_str.find(url2));
  delete websearch::_wconfig;
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
