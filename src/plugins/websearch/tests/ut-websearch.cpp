/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#include "wb_err.h"
#include "websearch.h"
#include "websearch_configuration.h"
#include "se_handler.h"
#include "proxy_configuration.h"
#include "content_handler.h"
#include "sweeper.h"
#include "errlog.h"

using namespace seeks_plugins;
using sp::proxy_configuration;
using sp::sweeper;
using sp::errlog;

std::string seeks_url = "http://www.seeks-project.info/";

class WBTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO | LOG_LEVEL_DEBUG);
      websearch::_wconfig = new websearch_configuration(""); // default websearch configuration.
      websearch::_wconfig->_se_connect_timeout = 1;
      websearch::_wconfig->_se_transfer_timeout = 1;
      websearch::_wconfig->_se_enabled = feeds("dummy","url1");
      websearch::_wconfig->_se_enabled.add_feed("dummy1","url1");
      websearch::_wconfig->_se_default = feeds("dummy","url1");
      _pconfig = new proxy_configuration(""); // default proxy configuration.
      _pconfig->_templdir = strdup("");
    }

    virtual void TearDown()
    {
      delete websearch::_wconfig;
      delete _pconfig;
    }

    proxy_configuration *_pconfig;
};

class WBExistTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO | LOG_LEVEL_DEBUG);
      websearch::_wconfig = new websearch_configuration(""); // default websearch configuration.
      websearch::_wconfig->_se_connect_timeout = 1;
      websearch::_wconfig->_se_transfer_timeout = 1;
      websearch::_wconfig->_se_enabled = feeds("dummy","url1");
      websearch::_wconfig->_se_enabled.add_feed("dummy1","url1");
      websearch::_wconfig->_se_default = feeds("dummy","url1");
      _pconfig = new proxy_configuration(""); // default proxy configuration.
      _pconfig->_templdir = strdup("");
      _qc = new query_context();
      std::string query = "test";
      std::string lang = "en";
      _qc->_query = query;
      _qc->_query_key = query_context::assemble_query(query,lang);
      _qc->_query_hash = query_context::hash_query_for_context(_qc->_query_key);
      _qc->register_qc();
    }

    virtual void TearDown()
    {
      delete websearch::_wconfig;
      delete _pconfig;
      sweeper::unregister_sweepable(_qc);
      delete _qc;
    }

    proxy_configuration *_pconfig;
    query_context *_qc;
};

//TODO: check lookup_qc()

TEST_F(WBTest,perform_websearch_bad_param_new)
{
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,&parameters,render);
  ASSERT_EQ(SP_ERR_CGI_PARAMS,err);
  se_handler::cleanup_handlers();
  sweeper::sweep_all();
}

TEST_F(WBTest,perform_websearch_no_engine_fail_new)
{
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  miscutil::add_map_entry(parameters,"engines",1,"",1);
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,parameters,render);
  ASSERT_EQ(WB_ERR_NO_ENGINE,err);
  miscutil::free_map(parameters);
  se_handler::cleanup_handlers();
  sweeper::sweep_all();
}

TEST_F(WBTest,perform_websearch_no_engine_output_fail_new)
{
  client_state csp;
  csp._config = _pconfig;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  //miscutil::add_map_entry(parameters,"action",1,"expand",1);
  miscutil::add_map_entry(parameters,"engines",1,"dummy1",1);
  miscutil::add_map_entry(parameters,"prs",1,"off",1); // no personalization, otherwise connection failure is bypassed as results may come from local db.
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,parameters,render);
  ASSERT_EQ(WB_ERR_SE_CONNECT,err);
  miscutil::free_map(parameters);
  se_handler::cleanup_handlers();
  sweeper::sweep_all();
}

TEST_F(WBExistTest,perform_websearch_bad_param_new)
{
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  //miscutil::add_map_entry(parameters,"action",1,"expand",1);
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,parameters,render);
  ASSERT_EQ(SP_ERR_CGI_PARAMS,err);
  miscutil::free_map(parameters);
  se_handler::cleanup_handlers();
  sweeper::sweep_all();
}

TEST_F(WBExistTest,perform_websearch_no_engine_fail_new)
{
  client_state csp;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  //miscutil::add_map_entry(parameters,"action",1,"expand",1);
  miscutil::add_map_entry(parameters,"engines",1,"",1);
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,parameters,render);
  ASSERT_EQ(WB_ERR_NO_ENGINE,err);
  miscutil::free_map(parameters);
  se_handler::cleanup_handlers();
  sweeper::sweep_all();
}

TEST_F(WBExistTest,perform_websearch_no_engine_output_fail_new)
{
  client_state csp;
  csp._config = _pconfig;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  //miscutil::add_map_entry(parameters,"action",1,"expand",1);
  miscutil::add_map_entry(parameters,"engines",1,"dummy1",1);
  miscutil::add_map_entry(parameters,"prs",1,"off",1); // no personalization, otherwise connection failure is bypassed as results may come from local db.
  bool render = false;
  sp_err err = websearch::perform_websearch(&csp,&rsp,parameters,render);
  ASSERT_EQ(WB_ERR_SE_CONNECT,err);
  miscutil::free_map(parameters);
  se_handler::cleanup_handlers();
  sweeper::sweep_all();
}

TEST_F(WBTest,preprocess_parameters_ok)
{
  client_state csp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"test",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  //miscutil::add_map_entry(parameters,"action",1,"expand",1);
  int code = SP_ERR_OK;
  try
    {
      websearch::preprocess_parameters(parameters,&csp);
    }
  catch(sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(SP_ERR_OK,code);
  miscutil::free_map(parameters);
}

TEST_F(WBTest,preprocess_parameters_bad_charset)
{
  client_state csp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"a\x80\xe0\xa0\xc0\xaf\xed\xa0\x80z",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  //miscutil::add_map_entry(parameters,"action",1,"expand",1);
  int code = SP_ERR_OK;
  try
    {
      websearch::preprocess_parameters(parameters,&csp);
    }
  catch(sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(WB_ERR_QUERY_ENCODING,code);
  miscutil::free_map(parameters);
}

/*TEST_F(WBTest,preprocess_parameters_dyn_ui_no_expand_action)
{
  client_state csp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,"seeks",1);
  miscutil::add_map_entry(parameters,"expansion",1,"1",1);
  miscutil::add_map_entry(parameters,"action",1,"similarity",1);
  miscutil::add_map_entry(parameters,"ui",1,"dyn",1);
  miscutil::add_map_entry(parameters,"output",1,"html",1);
  int code = SP_ERR_OK;
  try
    {
      websearch::preprocess_parameters(parameters,&csp);
    }
  catch(sp_exception &e)
    {
      code = e.code();
    }
  ASSERT_EQ(SP_ERR_OK,code);
  const char *action = miscutil::lookup(parameters,"action");
  ASSERT_TRUE(strcmp(action,"expand")==0);
  miscutil::free_map(parameters);
  }*/

TEST_F(WBTest,cgi_websearch_node_info)
{
  client_state csp;
  csp._config = _pconfig;
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
  sp_err err = websearch::cgi_websearch_node_info(&csp,&rsp,&parameters);
  ASSERT_EQ(SP_ERR_OK,err);
}

TEST_F(WBExistTest,cgi_websearch_search_query)
{
  client_state csp;
  csp._config = _pconfig;
  csp._http._gpc = strdup("get");
  csp._http._path = strdup("/search/txt/seeks");
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"output",1,"json",1);
  miscutil::add_map_entry(parameters,"engines",1,"dummy",1);
  sp_err err = websearch::cgi_websearch_search(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string body = std::string(rsp._body,rsp._content_length);
  EXPECT_NE(std::string::npos,body.find("\"query\":\"seeks\""));
  EXPECT_NE(std::string::npos,body.find("\"lang\":\"en\""));
  EXPECT_NE(std::string::npos,body.find("\"pers\":\"on\""));
  EXPECT_NE(std::string::npos,body.find("\"expansion\":\"1\""));
  EXPECT_NE(std::string::npos,body.find("\"npeers\":\"0\""));
  EXPECT_NE(std::string::npos,body.find("\"engines\":[]"));
  EXPECT_NE(std::string::npos,body.find("\"date\":"));
  EXPECT_NE(std::string::npos,body.find("\"snippets\":[]"));
  miscutil::free_map(parameters);
}

TEST_F(WBExistTest,cgi_websearch_search_snippet)
{
  search_snippet *sp = new search_snippet();
  sp->set_url(seeks_url);
  uint32_t sid = sp->_id;
  _qc->_cached_snippets.push_back(sp);
  _qc->add_to_unordered_cache(sp);

  client_state csp;
  csp._config = _pconfig;
  csp._http._gpc = strdup("get");
  std::string api_url = "/search/txt/test/" + miscutil::to_string(sid);
  csp._http._path = strdup(api_url.c_str());
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"output",1,"json",1);
  miscutil::add_map_entry(parameters,"engines",1,"dummy",1);
  sp_err err = websearch::cgi_websearch_search(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string body = std::string(rsp._body,rsp._content_length);
  EXPECT_NE(std::string::npos,body.find("\"id\":1289851243"));
  EXPECT_NE(std::string::npos,body.find("\"title\":\"\""));
  EXPECT_NE(std::string::npos,body.find("\"url\":\"http://www.seeks-project.info/\""));
  EXPECT_NE(std::string::npos,body.find("\"summary\":\"\""));
  EXPECT_NE(std::string::npos,body.find("\"seeks_meta\":"));
  EXPECT_NE(std::string::npos,body.find("\"seeks_score\":0"));
  EXPECT_NE(std::string::npos,body.find("\"cite\":\"http://www.seeks-project.info/\""));
  EXPECT_NE(std::string::npos,body.find("\"engines\":[]"));
  EXPECT_NE(std::string::npos,body.find("\"type\":\"webpage\""));
  EXPECT_NE(std::string::npos,body.find("\"personalized\":\"no\""));
  miscutil::free_map(parameters);
}

TEST_F(WBTest,cgi_websearch_search_bad_param)
{
  client_state csp;
  http_response rsp;
  csp._http._gpc = strdup("get");
  csp._http._path = strdup("/search/txt/");
  hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
  sp_err err = websearch::cgi_websearch_search(&csp,&rsp,&parameters);
  ASSERT_EQ(SP_ERR_CGI_PARAMS,err);
}

TEST_F(WBTest,cgi_websearch_search_bad_charset)
{
  client_state csp;
  http_response rsp;
  csp._http._gpc = strdup("get");
  csp._http._path = strdup("/search/txt/a\x80\xe0\xa0\xc0\xaf\xed\xa0\x80z");
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  //miscutil::add_map_entry(parameters,"action",1,"expand",1);
  //miscutil::add_map_entry(parameters,"q",1,"a\x80\xe0\xa0\xc0\xaf\xed\xa0\x80z",1);
  sp_err err = websearch::cgi_websearch_search(&csp,&rsp,parameters);
  ASSERT_EQ(WB_ERR_QUERY_ENCODING,err);
  //TODO: check on JSON output.
  miscutil::free_map(parameters);
}

TEST_F(WBExistTest,cgi_websearch_words_query_snippet)
{
  search_snippet *sp1 = new search_snippet();
  sp1->set_url("url1");
  sp1->set_title("jaguar cars for sale");
  sp1->set_summary("buy jaguar cars for a good price");
  search_snippet *sp2 = new search_snippet();
  sp2->set_url("url2");
  sp2->set_title("Jaguars on the verge of extinction");
  sp2->set_summary("A large cat from the jungle");
  _qc->_cached_snippets.push_back(sp1);
  _qc->add_to_unordered_cache(sp1);
  _qc->_cached_snippets.push_back(sp2);
  _qc->add_to_unordered_cache(sp2);

  content_handler::fetch_all_snippets_summary_and_features(_qc);

  client_state csp;
  csp._config = _pconfig;
  csp._http._gpc = strdup("get");
  std::string api_url = "/words/test";
  csp._http._path = strdup(api_url.c_str());
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"output",1,"json",1);
  miscutil::add_map_entry(parameters,"engines",1,"dummy",1);
  sp_err err = websearch::cgi_websearch_words(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string body = std::string(rsp._body,rsp._content_length);
  //std::cerr << "body: " << body << std::endl;
  EXPECT_NE(std::string::npos,body.find("\"words\":"));
  EXPECT_NE(std::string::npos,body.find("\"buy\""));
  EXPECT_NE(std::string::npos,body.find("\"jungle\""));
  miscutil::free_map(parameters);
}

TEST_F(WBExistTest,cgi_websearch_words_snippet)
{
  search_snippet *sp1 = new search_snippet();
  sp1->set_url("url1");
  sp1->set_title("jaguar cars for sale");
  sp1->set_summary("buy jaguar cars for a good price");
  uint32_t s1_id = sp1->_id;
  search_snippet *sp2 = new search_snippet();
  sp2->set_url("url2");
  sp2->set_title("Jaguars on the verge of extinction");
  sp2->set_summary("A large cat from the jungle");
  _qc->_cached_snippets.push_back(sp1);
  _qc->add_to_unordered_cache(sp1);
  _qc->_cached_snippets.push_back(sp2);
  _qc->add_to_unordered_cache(sp2);

  content_handler::fetch_all_snippets_summary_and_features(_qc);

  client_state csp;
  csp._config = _pconfig;
  csp._http._gpc = strdup("get");
  std::string api_url = "/words/test/" + miscutil::to_string(s1_id);
  csp._http._path = strdup(api_url.c_str());
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"output",1,"json",1);
  miscutil::add_map_entry(parameters,"engines",1,"dummy",1);
  sp_err err = websearch::cgi_websearch_words(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string body = std::string(rsp._body,rsp._content_length);
  //std::cerr << "body: " << body << std::endl;
  EXPECT_NE(std::string::npos,body.find("\"words\":"));
  EXPECT_NE(std::string::npos,body.find("\"buy\""));
  EXPECT_EQ(std::string::npos,body.find("\"jungle\""));
  miscutil::free_map(parameters);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
