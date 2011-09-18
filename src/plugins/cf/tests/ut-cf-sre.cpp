/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include <iconv.h>

#include "cf.h"
#include "cf_configuration.h"
#include "rank_estimators.h"
#include "query_capture.h"
#include "user_db.h"
#include "query_context.h"
#include "seeks_proxy.h"
#include "plugin_manager.h"
#include "proxy_configuration.h"
#include "lsh_configuration.h"
#include "qprocess.h"
#include "urlmatch.h"
#include "errlog.h"

using namespace seeks_plugins;
using namespace sp;
using namespace lsh;

const std::string dbfile = "seeks_test.db";
const std::string basedir = "../../../";

static std::string queries[3] =
{
  "seeks",
  "seeks project",
  "seeks project search"
};

static std::string uris[3] =
{
  "http://www.seeks-project.info/",
  "http://seeks-project.info/wiki/index.php/Documentation",
  "http://www.seeks-project.info/wiki/index.php/Download"
};

static std::string titles[3] =
{
  "Seeks Project",
  "Seeks Project Documentation",
  "Seeks Project Download"
};

class SRETest : public testing::Test
{
  protected:
    SRETest()
    {

    }

    virtual ~SRETest()
    {

    }

    virtual void SetUp()
    {
      unlink(dbfile.c_str());
      seeks_proxy::_configfile = basedir + "config";
      seeks_proxy::_lshconfigfile = basedir + "lsh/lsh-config";
      seeks_proxy::initialize_mutexes();
      errlog::init_log_module();
      errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO);
      seeks_proxy::_basedir = basedir.c_str();
      plugin_manager::_plugin_repository = basedir + "/plugins/";
      seeks_proxy::_config = new proxy_configuration(seeks_proxy::_configfile);
      seeks_proxy::_lsh_config = new lsh_configuration(seeks_proxy::_lshconfigfile);
      cf_configuration::_config = new cf_configuration("");
      cf_configuration::_config->_record_cache_timeout = 0;

      seeks_proxy::_user_db = new user_db(dbfile);
      seeks_proxy::_user_db->open_db();
      plugin_manager::load_all_plugins();
      plugin_manager::start_plugins();

      plugin *pl = plugin_manager::get_plugin("query-capture");
      ASSERT_TRUE(NULL!=pl);
      qcpl = static_cast<query_capture*>(pl);
      ASSERT_TRUE(NULL!=qcpl);
      qcelt = qcpl->_qelt;

      // check that the db is empty.
      ASSERT_TRUE(seeks_proxy::_user_db!=NULL);
      ASSERT_EQ(0,seeks_proxy::_user_db->number_records());

      // feed the db with queries and urls.
      std::string url = uris[1];
      std::string host,path;
      query_capture::process_url(url,host,path);
      hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
      = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
      miscutil::add_map_entry(parameters,"q",1,queries[0].c_str(),1);
      std::list<const char*> headers;
      query_context qc(parameters,headers);
      search_snippet *sp = new search_snippet();
      sp->set_url(url);
      sp->set_title(titles[1]);
      qc._cached_snippets.push_back(sp);
      qc.add_to_unordered_cache(sp);
      try
        {
          qcelt->store_queries(&qc,url,host,"query-capture");
        }
      catch (sp_exception &e)
        {
          ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
        }
      miscutil::free_map(parameters);
      std::string url2 = uris[2];
      query_capture::process_url(url2,host,path);
      parameters = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
      miscutil::add_map_entry(parameters,"q",1,queries[1].c_str(),1);
      query_context qc2(parameters,headers);
      sp = new search_snippet();
      sp->set_url(url2);
      sp->set_title(titles[2]);
      qc2._cached_snippets.push_back(sp);
      qc2.add_to_unordered_cache(sp);
      try
        {
          qcelt->store_queries(&qc2,url2,host,"query-capture");
        }
      catch (sp_exception &e)
        {
          ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
        }
      ASSERT_EQ(3,seeks_proxy::_user_db->number_records()); // seeks, seeks project, project.
      miscutil::free_map(parameters);
    }

    virtual void TearDown()
    {
      plugin_manager::close_all_plugins();
      delete cf_configuration::_config;
      delete seeks_proxy::_user_db;
      delete seeks_proxy::_lsh_config;
      delete seeks_proxy::_config;
      unlink(dbfile.c_str());
    }

    query_capture *qcpl;
    query_capture_element *qcelt;
};

TEST_F(SRETest,fetch_user_db_record)
{
  hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
  rank_estimator::fetch_user_db_record(queries[1],seeks_proxy::_user_db,records);
  ASSERT_EQ(3,records.size());
  rank_estimator::destroy_records(records);
}

TEST_F(SRETest,extract_queries)
{
  hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
  rank_estimator::fetch_user_db_record(queries[2],seeks_proxy::_user_db,records);
  ASSERT_EQ(3,records.size());

  hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
  hash_map<const char*,std::vector<query_data*>,hash<const char*>,eqstr> inv_qdata;
  rank_estimator::extract_queries(queries[2],"en",1,seeks_proxy::_user_db,records,qdata,inv_qdata);
  ASSERT_EQ(2,qdata.size());
  hash_map<const char*,query_data*,hash<const char*>,eqstr>::const_iterator hit
  = qdata.begin();
  ASSERT_EQ(queries[1],(*hit).second->_query);
  ASSERT_EQ(1,(*hit).second->_radius);
  ++hit;
  ASSERT_EQ(queries[0],(*hit).second->_query);
  ASSERT_EQ(2,(*hit).second->_radius);

  rank_estimator::destroy_records(records);
  rank_estimator::destroy_query_data(qdata);
  rank_estimator::destroy_inv_qdata_key(inv_qdata);
}

TEST(SREAPITest,damerau_levenshtein_distance)
{
  std::string s1 = "ca";
  std::string s2 = "abc";
  uint32_t dist = simple_re::damerau_levenshtein_distance(s1,s2,256);
  ASSERT_EQ(2,dist);
  dist = simple_re::damerau_levenshtein_distance("seeks","seesk",256);
  ASSERT_EQ(1,dist);
  dist = simple_re::damerau_levenshtein_distance("seeks","seek",256);
  ASSERT_EQ(1,dist);
  dist = simple_re::damerau_levenshtein_distance("seks","seeks",256);
  ASSERT_EQ(1,dist);
  dist = simple_re::damerau_levenshtein_distance("seke","seeks",256);
  ASSERT_EQ(2,dist);
  dist = simple_re::damerau_levenshtein_distance("seeks","project",256);
  ASSERT_EQ(6,dist);
  dist = simple_re::damerau_levenshtein_distance("seeks","seeks",256);
  ASSERT_EQ(0,dist);
  dist = simple_re::damerau_levenshtein_distance("données","donnât",256);
  ASSERT_EQ(3,dist);
}

TEST_F(SRETest,query_distance)
{
  uint32_t dist = simple_re::query_distance(queries[0],queries[0]);
  ASSERT_EQ(0,dist);
  dist = simple_re::query_distance(queries[0],queries[1]);
  ASSERT_EQ(8,dist);
  dist = simple_re::query_distance(queries[0],queries[2]);
  ASSERT_EQ(15,dist);
  dist = simple_re::query_distance(queries[1],queries[2]);
  ASSERT_EQ(7,dist);

  std::string s1 = "the seeks project";
  stopwordlist *swl = seeks_proxy::_lsh_config->get_wordlist("en");
  ASSERT_TRUE(swl!=NULL);
  dist = simple_re::query_distance(queries[1],s1,swl);
  ASSERT_EQ(0,dist);
}

TEST(SREAPITest,query_halo_weight)
{
  float tw1 = log(3.0) / (log(simple_re::query_distance(queries[1],queries[2]) + 1.0) + 1.0);
  float w1 = simple_re::query_halo_weight(queries[1],queries[2],1); // seeks project vs. seeks project search.
  ASSERT_EQ(tw1,w1);
  float w2 = simple_re::query_halo_weight(queries[0],queries[2],1); // seeks vs. seeks project search.
  ASSERT_TRUE(w2<w1);
}

TEST(SREAPITest,estimate_rank)
{
  simple_re sre;
  vurl_data vd_url(uris[0],1);
  vurl_data vd_host(uris[1],1);
  search_snippet s;
  bool pers = false;
  float posterior = sre.estimate_rank(&s,NULL,1,&vd_url,&vd_host,2,0.3,pers);
  float posterior_check = ((log(2) + 1) / (log(3) + 1)) * 0.3 * (log(2) + 1) / (log(3) + 1);
  ASSERT_EQ(posterior_check,posterior);
  posterior = sre.estimate_rank(&s,NULL,1,&vd_url,&vd_host,-2,0.3,pers);
  ASSERT_EQ(posterior_check,posterior);
  posterior = sre.estimate_rank(&s,NULL,1,&vd_url,&vd_host,2,0.0,pers);
  ASSERT_TRUE(posterior > 0.0);
  posterior_check = (log(2) + 1) / (log(3) + 1);
  ASSERT_EQ(posterior_check,posterior);
  vd_url._hits = -1;
  posterior = sre.estimate_rank(&s,NULL,1,&vd_url,&vd_host,-2,0.3,pers);
  ASSERT_TRUE(posterior < 0.0);
  vd_host._hits = -1;
  posterior = sre.estimate_rank(&s,NULL,1,&vd_url,&vd_host,-2,0.3,pers);
  ASSERT_TRUE(posterior < 0.0);
}

TEST_F(SRETest,recommend_urls)
{
  static std::string lang = "en";
  simple_re sre;
  hash_map<uint32_t,search_snippet*,id_hash_uint> snippets;
  sre.recommend_urls(queries[0],lang,5,snippets);
  ASSERT_EQ(2,snippets.size());

  // scoring is not done in recommend_urls anymore.
  /*float url1_score = log(2)*(log(2.0)+1.0) / (log(3.0)+5.0)
                     * cf_configuration::_config->_domain_name_weight / (log(3.0)+5.0);
  float url2_score = url1_score / (log(9)+1); // weighting down with query radius.

  std::string url1 = uris[1];
  std::string host,path;
  query_capture::process_url(url1,host,path);
  hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit
  = snippets.begin();
  ASSERT_EQ(url1,(*hit).second->_url);
  ASSERT_EQ(url1_score,(*hit).second->_seeks_rank);
  ++hit;
  std::string url2 = uris[2];
  query_capture::process_url(url2,host,path);
  ASSERT_EQ(url2,(*hit).second->_url);
  ASSERT_EQ(url2_score,(*hit).second->_seeks_rank);*/

  hash_map<uint32_t,search_snippet*,id_hash_uint>::iterator hit = snippets.begin();
  while(hit!=snippets.end())
    {
      delete (*hit).second;
      ++hit;
    }
}

TEST_F(SRETest,thumb_down_url)
{
  static std::string lang = "en";
  simple_re sre;
  std::string url1 = uris[1];
  std::string host,path;
  query_capture::process_url(url1,host,path);
  sre.thumb_down_url(queries[0],lang,url1);

  hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
  qprocess::generate_query_hashes(queries[0],0,5,features);
  ASSERT_EQ(1,features.size());
  DHTKey key = (*features.begin()).second;
  std::string key_str = key.to_rstring();
  db_record *dbr = seeks_proxy::_user_db->find_dbr(key_str,"query-capture");
  ASSERT_TRUE(dbr!=NULL);
  db_query_record *dbqr = dynamic_cast<db_query_record*>(dbr);
  ASSERT_TRUE(dbqr!=NULL);
  hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit
  = dbqr->_related_queries.find(queries[0].c_str());
  ASSERT_TRUE((*hit).second->_visited_urls==NULL);
  delete dbqr;

  hash_map<const char*,vurl_data*,hash<const char*>,eqstr>::iterator vit;
  for (int i=0; i<2; i++)
    {
      sre.thumb_down_url(queries[0],lang,url1);
      dbr = seeks_proxy::_user_db->find_dbr(key_str,"query-capture");
      ASSERT_TRUE(dbr!=NULL);
      dbqr = dynamic_cast<db_query_record*>(dbr);
      ASSERT_TRUE(dbqr!=NULL);
      hit = dbqr->_related_queries.find(queries[0].c_str());
      ASSERT_TRUE((*hit).second->_visited_urls!=NULL);
      ASSERT_EQ(2,(*hit).second->_visited_urls->size());
      vit = (*hit).second->_visited_urls->begin();
      ASSERT_EQ(-(i+1),(*vit).second->_hits);
      ++vit;
      ASSERT_EQ(-(i+1),(*vit).second->_hits);
      delete dbqr;
    }

  std::string url2 = uris[2];
  query_capture::process_url(url2,host,path);
  sre.thumb_down_url(queries[0],lang,url2);
  dbr = seeks_proxy::_user_db->find_dbr(key_str,"query-capture");
  ASSERT_TRUE(dbr!=NULL);
  dbqr = dynamic_cast<db_query_record*>(dbr);
  ASSERT_TRUE(dbqr!=NULL);
  hit = dbqr->_related_queries.find(queries[0].c_str());
  ASSERT_TRUE((*hit).second->_visited_urls!=NULL);
  ASSERT_EQ(3,(*hit).second->_visited_urls->size());
  std::string purl = uris[0];
  query_capture::process_url(purl,host,path);
  purl = urlmatch::strip_url(purl);
  std::cerr << purl << std::endl;
  vit = (*hit).second->_visited_urls->find(purl.c_str());
  ASSERT_TRUE(vit!=(*hit).second->_visited_urls->end());
  ASSERT_EQ(-3,(*vit).second->_hits); // domain
  purl = uris[1];
  query_capture::process_url(purl,host,path);
  vit = (*hit).second->_visited_urls->find(purl.c_str());
  ASSERT_TRUE(vit!=(*hit).second->_visited_urls->end());
  ASSERT_EQ(-2,(*vit).second->_hits); // documentation
  purl = uris[2];
  query_capture::process_url(purl,host,path);
  vit = (*hit).second->_visited_urls->find(purl.c_str());
  ASSERT_TRUE(vit!=(*hit).second->_visited_urls->end());
  ASSERT_EQ(-1,(*vit).second->_hits); // download
  delete dbqr;
}

// test estimate ranks.
TEST_F(SRETest,estimate_ranks)
{
  static std::string lang = "en";
  simple_re sre;
  sre.thumb_down_url(queries[0],lang,uris[2]); // thumb down download.

  std::vector<search_snippet*> snippets;
  snippets.reserve(3);
  search_snippet s0;
  s0.set_url(uris[0]);  // http://www.seeks-project.info/
  s0.set_title(titles[0]);
  search_snippet s1;
  s1.set_url(uris[1]);  // http://seeks-project.info/wiki/index.php/Documentation
  s1.set_title(titles[1]);
  search_snippet s2;
  s2.set_url(uris[2]);  // http://www.seeks-project.info/wiki/index.php/Download
  s2.set_title(titles[2]);
  snippets.push_back(&s0);
  snippets.push_back(&s1);
  snippets.push_back(&s2);
  sre.estimate_ranks(queries[1],lang,1,snippets);
  ASSERT_EQ(3,snippets.size());

  std::cerr << "ranks, s2: " << s2._seeks_rank << " -- s1: " << s1._seeks_rank << " -- s0: " << s0._seeks_rank << std::endl;

  ASSERT_TRUE(s2._seeks_rank > s1._seeks_rank);
  ASSERT_TRUE(s1._seeks_rank > s0._seeks_rank);
}

TEST_F(SRETest, utf8)
{
  std::string q = "\xe6\x97\xa5\xd1\x88\xfa";

  std::string url = uris[1];
  std::string host, path;
  query_capture::process_url(url,host,path);
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"q",1,q.c_str(),1);
  std::list<const char*> headers;
  query_context qc(parameters,headers);
  try
    {
      qcelt->store_queries(&qc,url,host,"query-capture");
    }
  catch (sp_exception &e)
    {
      ASSERT_EQ(SP_ERR_OK,e.code()); // would fail.
    }
  miscutil::free_map(parameters);

  hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
  rank_estimator::fetch_user_db_record(q,seeks_proxy::_user_db,records);
  ASSERT_EQ(1,records.size());
  rank_estimator::destroy_records(records);
}

TEST_F(SRETest,recommendation_post_ok)
{
  cf_configuration::_config = new cf_configuration("");
  cf_configuration::_config->_post_url_check = false; // disable URL checking.
  client_state csp;
  csp._config = seeks_proxy::_config;
  csp._http._gpc = strdup("post");
  csp._http._path = strdup("/recommendation/search%20engine");
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"url",1,"http://www.seeks.mx/",1);
  miscutil::add_map_entry(parameters,"title",1,"Seeks Search",1);
  miscutil::add_map_entry(parameters,"url-check",1,"0",1);
  miscutil::add_map_entry(parameters,"radius",1,"5",1);
  sp_err err = cf::cgi_recommendation(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
  rank_estimator::fetch_user_db_record("search engine",seeks_proxy::_user_db,records);
  ASSERT_EQ(3,records.size());
  miscutil::free_map(parameters);
  hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
  hash_map<const char*,std::vector<query_data*>,hash<const char*>,eqstr> inv_qdata;
  rank_estimator::extract_queries("search engine","",0,seeks_proxy::_user_db,records,
                                  qdata,inv_qdata);
  ASSERT_EQ(1,inv_qdata.size());
  std::vector<query_data*> vqd = (*inv_qdata.begin()).second;
  ASSERT_EQ(1,vqd.size());
  query_data *qd = vqd.at(0);
  ASSERT_TRUE(qd!=NULL);
  ASSERT_TRUE(qd->_visited_urls!=NULL);
  ASSERT_EQ(2,qd->_visited_urls->size());
  vurl_data *vd = (*qd->_visited_urls->begin()).second;
  ASSERT_TRUE(vd!=NULL);
  ASSERT_EQ("http://www.seeks.mx",vd->_url);
  ASSERT_EQ("Seeks Search",vd->_title);
  ASSERT_TRUE(vd->_url_lang.empty());
  ASSERT_TRUE(0 < vd->_url_date);
  rank_estimator::destroy_records(records);
  rank_estimator::destroy_query_data(qdata);
  rank_estimator::destroy_inv_qdata_key(inv_qdata);
}

TEST_F(SRETest,recommendation_post_ok_lang)
{
  cf_configuration::_config = new cf_configuration("");
  cf_configuration::_config->_post_url_check = false; // disable URL checking.
  client_state csp;
  csp._config = seeks_proxy::_config;
  csp._http._gpc = strdup("post");
  csp._http._path = strdup("/recommendation/search%20engine");
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"url",1,"http://www.seeks.mx/",1);
  miscutil::add_map_entry(parameters,"title",1,"Seeks Search",1);
  miscutil::add_map_entry(parameters,"url-check",1,"0",1);
  miscutil::add_map_entry(parameters,"radius",1,"5",1);
  miscutil::add_map_entry(parameters,"lang",1,"es",1);
  sp_err err = cf::cgi_recommendation(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
  rank_estimator::fetch_user_db_record("search engine",seeks_proxy::_user_db,records);
  ASSERT_EQ(3,records.size());
  miscutil::free_map(parameters);
  hash_map<const char*,query_data*,hash<const char*>,eqstr> qdata;
  hash_map<const char*,std::vector<query_data*>,hash<const char*>,eqstr> inv_qdata;
  rank_estimator::extract_queries("search engine","",0,seeks_proxy::_user_db,records,
                                  qdata,inv_qdata);
  ASSERT_EQ(1,inv_qdata.size());
  std::vector<query_data*> vqd = (*inv_qdata.begin()).second;
  ASSERT_EQ(1,vqd.size());
  query_data *qd = vqd.at(0);
  ASSERT_TRUE(qd!=NULL);
  ASSERT_TRUE(qd->_visited_urls!=NULL);
  ASSERT_EQ(2,qd->_visited_urls->size());
  vurl_data *vd = (*qd->_visited_urls->begin()).second;
  ASSERT_TRUE(vd!=NULL);
  ASSERT_EQ("http://www.seeks.mx",vd->_url);
  ASSERT_EQ("Seeks Search",vd->_title);
  ASSERT_EQ("es",vd->_url_lang);
  ASSERT_TRUE(0 < vd->_url_date);
  rank_estimator::destroy_records(records);
  rank_estimator::destroy_query_data(qdata);
  rank_estimator::destroy_inv_qdata_key(inv_qdata);
}

TEST_F(SRETest,recommendation_post_url_check_fail_400)
{
  cf_configuration::_config = new cf_configuration("");
  cf_configuration::_config->_post_url_check = true; // enable URL checking.
  client_state csp;
  csp._config = seeks_proxy::_config;
  csp._http._gpc = strdup("post");
  csp._http._path = strdup("/recommendation/search%20engine");
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"url",1,"http://www.seeks.mx/",1);
  miscutil::add_map_entry(parameters,"url-check",1,"1",1);
  miscutil::add_map_entry(parameters,"radius",1,"5",1);
  sp_err err = cf::cgi_recommendation(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string body = std::string(rsp._body,rsp._content_length);
  //std::cerr << "body: " << body << std::endl;
  EXPECT_NE(std::string::npos,body.find("bad parameter"));
  hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
  rank_estimator::fetch_user_db_record("search engine",seeks_proxy::_user_db,records);
  ASSERT_EQ(0,records.size());
  miscutil::free_map(parameters);
  rank_estimator::destroy_records(records);
}

TEST_F(SRETest,recommendation_post_url_check_retrieve)
{
  cf_configuration::_config = new cf_configuration("");
  cf_configuration::_config->_post_url_check = true; // enable URL checking.
  client_state csp;
  csp._config = seeks_proxy::_config;
  csp._http._gpc = strdup("post");
  csp._http._path = strdup("/recommendation/search%20engine");
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"url",1,"http://www.seeks.fr/",1);
  miscutil::add_map_entry(parameters,"url-check",1,"1",1);
  miscutil::add_map_entry(parameters,"radius",1,"5",1);
  miscutil::add_map_entry(parameters,"output",1,"json",1);
  sp_err err = cf::cgi_recommendation(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string body = std::string(rsp._body,rsp._content_length);
  //std::cerr << "body: " << body << std::endl;
  EXPECT_EQ(std::string::npos,body.find("not found"));
  hash_map<const DHTKey*,db_record*,hash<const DHTKey*>,eqdhtkey> records;
  rank_estimator::fetch_user_db_record("search engine",seeks_proxy::_user_db,records);
  ASSERT_EQ(3,records.size());
  miscutil::free_map(parameters);
  rank_estimator::destroy_records(records);
}

TEST_F(SRETest,recommendation_get_ok)
{
  cf_configuration::_config = new cf_configuration("");
  cf_configuration::_config->_post_url_check = false; // disable URL checking.
  client_state csp;
  csp._config = seeks_proxy::_config;
  csp._http._gpc = strdup("post");
  csp._http._path = strdup("/recommendation/search%20engine");
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"url",1,"http://www.seeks.mx/",1);
  miscutil::add_map_entry(parameters,"title",1,"Seeks Search",1);
  miscutil::add_map_entry(parameters,"url-check",1,"0",1);
  miscutil::add_map_entry(parameters,"radius",1,"5",1);
  sp_err err = cf::cgi_recommendation(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  miscutil::unmap(parameters,"title");
  miscutil::unmap(parameters,"url-check");
  miscutil::unmap(parameters,"radius");
  miscutil::unmap(parameters,"lang");
  miscutil::add_map_entry(parameters,"peers",1,"local",1);
  client_state csp2;
  csp2._http._gpc = strdup("get");
  csp2._http._path = strdup("/recommendation/search%20engine");
  http_response rsp2;
  err = cf::cgi_recommendation(&csp2,&rsp2,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string body = std::string(rsp2._body,rsp2._content_length);
  std::cerr << "body: " << body << std::endl;
  EXPECT_NE(std::string::npos,body.find("http://www.seeks.mx"));
  EXPECT_NE(std::string::npos,body.find("Seeks Search"));
  EXPECT_NE(std::string::npos,body.find("\"hits\":1"));
  miscutil::free_map(parameters);
}

TEST_F(SRETest,recommendation_get_ok_lang)
{
  cf_configuration::_config = new cf_configuration("");
  cf_configuration::_config->_post_url_check = false; // disable URL checking.
  client_state csp;
  csp._config = seeks_proxy::_config;
  csp._http._gpc = strdup("post");
  csp._http._path = strdup("/recommendation/search%20engine");
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"url",1,"http://www.seeks.mx/",1);
  miscutil::add_map_entry(parameters,"title",1,"Seeks Search",1);
  miscutil::add_map_entry(parameters,"url-check",1,"0",1);
  miscutil::add_map_entry(parameters,"radius",1,"5",1);
  sp_err err = cf::cgi_recommendation(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  miscutil::unmap(parameters,"title");
  miscutil::unmap(parameters,"url-check");
  miscutil::unmap(parameters,"radius");
  miscutil::add_map_entry(parameters,"peers",1,"local",1);
  miscutil::add_map_entry(parameters,"lang",1,"es",1);
  client_state csp2;
  csp2._http._gpc = strdup("get");
  csp2._http._path = strdup("/recommendation/search%20engine");
  http_response rsp2;
  err = cf::cgi_recommendation(&csp2,&rsp2,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string body = std::string(rsp2._body,rsp2._content_length);
  //std::cerr << "body: " << body << std::endl;
  EXPECT_EQ(std::string::npos,body.find("http://www.seeks.mx"));
  EXPECT_EQ(std::string::npos,body.find("Seeks Search"));
  EXPECT_EQ(std::string::npos,body.find("\"hits\":1"));
  miscutil::free_map(parameters);
}

TEST_F(SRETest,recommendation_delete_ok)
{
  cf_configuration::_config = new cf_configuration("");
  cf_configuration::_config->_post_url_check = false; // disable URL checking.
  client_state csp;
  csp._config = seeks_proxy::_config;
  csp._http._gpc = strdup("post");
  csp._http._path = strdup("/recommendation/search%20engine");
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"url",1,"http://www.seeks.mx/",1);
  miscutil::add_map_entry(parameters,"title",1,"Seeks Search",1);
  miscutil::add_map_entry(parameters,"url-check",1,"0",1);
  miscutil::add_map_entry(parameters,"radius",1,"5",1);
  sp_err err = cf::cgi_recommendation(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  miscutil::unmap(parameters,"title");
  miscutil::unmap(parameters,"url-check");
  miscutil::unmap(parameters,"radius");
  miscutil::unmap(parameters,"lang");
  client_state csp2;
  csp2._http._gpc = strdup("delete");
  csp2._http._path = strdup("/recommendation/search%20engine");
  http_response rsp2;
  err = cf::cgi_recommendation(&csp2,&rsp2,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  miscutil::unmap(parameters,"url");
  miscutil::add_map_entry(parameters,"peers",1,"local",1);
  client_state csp3;
  csp3._http._gpc = strdup("get");
  csp3._http._path = strdup("/recommendation/search%20engine");
  http_response rsp3;
  err = cf::cgi_recommendation(&csp3,&rsp3,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string body = std::string(rsp3._body,rsp3._content_length);
  EXPECT_EQ(std::string::npos,body.find("http://www.seeks.mx"));
  EXPECT_EQ(std::string::npos,body.find("Seeks Search"));
  EXPECT_EQ(std::string::npos,body.find("\"hits\""));
  miscutil::free_map(parameters);
}

TEST_F(SRETest,recommendation_delete_ok_lang)
{
  cf_configuration::_config = new cf_configuration("");
  cf_configuration::_config->_post_url_check = false; // disable URL checking.
  client_state csp;
  csp._config = seeks_proxy::_config;
  csp._http._gpc = strdup("post");
  csp._http._path = strdup("/recommendation/search%20engine");
  http_response rsp;
  hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
  miscutil::add_map_entry(parameters,"url",1,"http://www.seeks.mx/",1);
  miscutil::add_map_entry(parameters,"title",1,"Seeks Search",1);
  miscutil::add_map_entry(parameters,"url-check",1,"0",1);
  miscutil::add_map_entry(parameters,"radius",1,"5",1);
  sp_err err = cf::cgi_recommendation(&csp,&rsp,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  miscutil::unmap(parameters,"title");
  miscutil::unmap(parameters,"url-check");
  miscutil::unmap(parameters,"radius");
  miscutil::unmap(parameters,"lang");
  miscutil::add_map_entry(parameters,"lang",1,"es",1);
  client_state csp2;
  csp2._http._gpc = strdup("delete");
  csp2._http._path = strdup("/recommendation/search%20engine");
  http_response rsp2;
  err = cf::cgi_recommendation(&csp2,&rsp2,parameters);
  ASSERT_EQ(DB_ERR_NO_REC,err);
  miscutil::unmap(parameters,"url");
  miscutil::unmap(parameters,"lang");
  miscutil::add_map_entry(parameters,"peers",1,"local",1);
  client_state csp3;
  csp3._http._gpc = strdup("get");
  csp3._http._path = strdup("/recommendation/search%20engine");
  http_response rsp3;
  err = cf::cgi_recommendation(&csp3,&rsp3,parameters);
  ASSERT_EQ(SP_ERR_OK,err);
  std::string body = std::string(rsp3._body,rsp3._content_length);
  EXPECT_NE(std::string::npos,body.find("http://www.seeks.mx"));
  EXPECT_NE(std::string::npos,body.find("Seeks Search"));
  EXPECT_NE(std::string::npos,body.find("\"hits\""));
  miscutil::free_map(parameters);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
