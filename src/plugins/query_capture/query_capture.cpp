/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010, 2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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

#include "query_capture.h"
#include "query_capture_configuration.h"
#include "db_query_record.h"
#include "qc_err.h"
#include "websearch.h"
#include "query_context.h"
#include "seeks_proxy.h" // for user_db.
#include "plugin_manager.h"
#include "proxy_configuration.h"
#include "user_db.h"
#include "proxy_dts.h"
#include "urlmatch.h"
#include "encode.h"
#include "cgi.h"
#include "cgisimple.h"
#include "qprocess.h"
#include "curl_mget.h"
#include "miscutil.h"
#include "charset_conv.h"
#include "errlog.h"

#include <algorithm>
#include <iterator>
#include <iostream>

#include <sys/time.h>
#include <sys/stat.h>

using sp::seeks_proxy;
using sp::user_db;
using sp::db_record;
using sp::urlmatch;
using sp::encode;
using sp::cgi;
using sp::cgisimple;
using lsh::qprocess;
using sp::curl_mget;
using sp::miscutil;
using sp::charset_conv;
using sp::errlog;

namespace seeks_plugins
{

  /*- query_db_sweepable -*/
  query_db_sweepable::query_db_sweepable()
    :user_db_sweepable()
  {
    // set last sweep to now.
    // sweeping is onde when plugin starts.
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    _last_sweep = tv_now.tv_sec;
  }

  query_db_sweepable::~query_db_sweepable()
  {
  }

  bool query_db_sweepable::sweep_me()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    if ((tv_now.tv_sec - _last_sweep)
        > query_capture_configuration::_config->_sweep_cycle)
      {
        _last_sweep = tv_now.tv_sec;
        return true;
      }
    else return false;
  }

  int query_db_sweepable::sweep_records()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    if (query_capture_configuration::_config->_retention > 0)
      {
        time_t sweep_date = tv_now.tv_sec - query_capture_configuration::_config->_retention;
        int err = seeks_proxy::_user_db->prune_db("query-capture",sweep_date);
        return err;
      }
    else return SP_ERR_OK;
  }

  /*- query_capture -*/
  query_capture::query_capture()
    :plugin(),_qelt(NULL)
  {
    _name = "query-capture";
    _version_major = "0";
    _version_minor = "1";

    if (seeks_proxy::_datadir.empty())
      _config_filename = plugin_manager::_plugin_repository + "query_capture/query-capture-config";
    else
      _config_filename = seeks_proxy::_datadir + "/plugins/query_capture/query-capture-config";

#ifdef SEEKS_CONFIGDIR
    struct stat stFileInfo;
    if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
      {
        _config_filename = SEEKS_CONFIGDIR "/query-capture-config";
      }
#endif

    if (query_capture_configuration::_config == NULL)
      query_capture_configuration::_config = new query_capture_configuration(_config_filename);
    _configuration = query_capture_configuration::_config;
    _qelt = new query_capture_element();
  }

  query_capture::~query_capture()
  {
    query_capture_configuration::_config = NULL; // configuration is deleted in parent class.
    delete _qelt;
  }

  void query_capture::start()
  {
    // check for user db.
    if (!seeks_proxy::_user_db || !seeks_proxy::_user_db->_opened)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"user db is not opened for query capture plugin to work with it");
        return;
      }
    else if (seeks_proxy::_config->_user_db_startup_check)
      {
        // preventive sweep of records.
        _qelt->_qds.sweep_records();
      }

    // get number of captured queries already in user_db.
    uint64_t nr = seeks_proxy::_user_db->number_records(_name);

    errlog::log_error(LOG_LEVEL_INFO,"query_capture plugin: %u records",nr);
  }

  void query_capture::stop()
  {
  }

  sp_err query_capture::cgi_qc_redir(client_state *csp,
                                     http_response *rsp,
                                     const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        // check on config file, in case it did change.
        query_capture_configuration::_config->load_config();
        pthread_rwlock_rdlock(&query_capture_configuration::_config->_conf_rwlock);

        char *urlp = NULL;
        sp_err err = query_capture::qc_redir(csp,rsp,parameters,urlp);
        /*if (err == SP_ERR_CGI_PARAMS)
          {
            const char *output_str = miscutil::lookup(parameters,"output");
            if (output_str && strcmp(output_str,"json")==0)
              return cgi::cgi_error_bad_param(csp,rsp,"json");
            else return cgi::cgi_error_bad_param(csp,rsp,"html");
          }
        else if (err == SP_ERR_PARSE)
        return cgi::cgi_error_disabled(csp,rsp);*/ // wrong use of the resource.
        if (err != SP_ERR_OK)
          {
            pthread_rwlock_unlock(&query_capture_configuration::_config->_conf_rwlock);
            return err;
          }

        // redirect to requested url.
        urlp = encode::url_decode_but_not_plus(urlp);

        cgi::cgi_redirect(rsp,urlp);
        free(urlp);
        pthread_rwlock_unlock(&query_capture_configuration::_config->_conf_rwlock);
        return SP_ERR_OK;
      }
    else return cgi::cgi_error_bad_param(csp,rsp,parameters,"html");
  }

  sp_err query_capture::qc_redir(client_state *csp,
                                 http_response *rsp,
                                 const hash_map<const char*, const char*, hash<const char*>, eqstr> *parameters,
                                 char *&urlp)
  {
    urlp = (char*)miscutil::lookup(parameters,"url"); // grab the url.
    if (!urlp)
      return SP_ERR_CGI_PARAMS;
    const char *q = miscutil::lookup(parameters,"q"); // grab the query.
    if (!q)
      return SP_ERR_CGI_PARAMS;

    // protection against abusive usage, check on referer.
    // XXX: this does prevent forging the referer and the query, but for
    // now let's issue a warning.
    std::string chost, referer, cget, base_url;
    query_capture_element::get_useful_headers(csp->_headers,chost,referer,
        cget,base_url);
    std::string ref_host, ref_path;
    urlmatch::parse_url_host_and_path(referer,ref_host,ref_path);

    /* if (base_url.empty())
      base_url = query_capture_element::_cgi_site_host; */

    // XXX: we could check on the exact referer domain.
    // But attackers forging referer would forge this too anyways.
    // So we perform a basic test, discouraging many, not all.
    if (query_capture_configuration::_config->_protected_redirection)
      {
        size_t p = ref_path.find("search/txt");
        if (p == std::string::npos)
          {
            // old API.
            p = ref_path.find("search?");
            if (p == std::string::npos)
              {
                p = ref_path.find("search_img?");
                if (p==std::string::npos)
                  return SP_ERR_PARSE;
              }
          }
      }

    // capture queries and URL / HOST.
    // XXX: could be threaded and detached.
    query_context *qc = websearch::lookup_qc(parameters);
    std::string host,path;
    std::string url = urlp;
    query_capture::process_url(url,host,path);

    try
      {
        query_capture::store_queries(q,qc,url,host);
      }
    catch (sp_exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR,e.what().c_str());
        return e.code();
      }

    // crossposting requested.
    // XXX: could thread it and return.
    const char *cpost = miscutil::lookup(parameters,"cpost");
    if (!cpost && !query_capture_configuration::_config->_cross_post_url.empty())
      cpost = query_capture_configuration::_config->_cross_post_url.c_str();
    const char *sid = miscutil::lookup(parameters,"id"); // should always be non NULL at this point.
    if (cpost && sid)
      {
        std::string chost = cpost;
        errlog::log_error(LOG_LEVEL_DEBUG,"crossposting to %s",cpost);
        std::string query = q;
        char *enc_query = encode::url_encode(query.c_str());
        query = enc_query;
        free(enc_query);
        chost += "/search/txt/" + query + "/" + std::string(sid);
        const char *lang = miscutil::lookup(parameters,"lang"); // should always be non NULL at this point.
        if (lang)
          chost += "?lang=" + std::string(lang);
        curl_mget cmg(1,3,0,3,0); // timeout is 3 seconds.
        int status;
        std::string *output = cmg.www_simple(chost,NULL,status,"POST");
        delete output; // ignore output.
      }

    return SP_ERR_OK;
  }

  void query_capture::process_url(std::string &url, std::string &host)
  {
    if (url[url.length()-1]=='/') // remove trailing '/'.
      url = url.substr(0,url.length()-1);
    std::transform(url.begin(),url.end(),url.begin(),tolower);
    std::transform(host.begin(),host.end(),host.begin(),tolower);
  }

  void query_capture::process_url(std::string &url, std::string &host, std::string &path)
  {
    urlmatch::parse_url_host_and_path(url,host,path);
    host = urlmatch::strip_url(host);
    query_capture::process_url(url,host);
  }

  void query_capture::process_get(std::string &get)
  {
    size_t p = miscutil::replace_in_string(get," HTTP/1.1","");
    if (p == 0)
      miscutil::replace_in_string(get," HTTP/1.0","");
  }

  sp::db_record* query_capture::create_db_record()
  {
    return new db_query_record();
  }

  int query_capture::remove_all_query_records()
  {
    return seeks_proxy::_user_db->prune_db(_name);
  }

  void query_capture::store_queries(const std::string &q,
                                    const query_context *qc,
                                    const std::string &url, const std::string &host
                                   ) throw (sp_exception)
  {
    query_capture_element::store_queries(q,qc,url,host,"query-capture");
  }

  void query_capture::store_queries(const std::string &query) const throw (sp_exception)
  {
    pthread_rwlock_rdlock(&query_capture_configuration::_config->_conf_rwlock);
    query_capture_element::store_queries(query,get_name());
    pthread_rwlock_unlock(&query_capture_configuration::_config->_conf_rwlock);
  }

  /*- query_capture_element -*/
  std::string query_capture_element::_capt_filename = "query_capture/query-patterns";
  std::string query_capture_element::_cgi_site_host = CGI_SITE_1_HOST;

  query_capture_element::query_capture_element()
  {
    if (seeks_proxy::_user_db && query_capture_configuration::_config->_sweep_cycle > 0)
      seeks_proxy::_user_db->register_sweeper(&_qds);
  }

  query_capture_element::~query_capture_element()
  {
  }

  void query_capture_element::store_queries(const std::string &q,
      const query_context *qc,
      const std::string &url, const std::string &host,
      const std::string &plugin_name,
      const int &radius) throw (sp_exception)
  {
    std::string query = q;
    if (qc)
      query = qc->_lc_query;

    // generate query fragments.
    hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
    qprocess::generate_query_hashes(query,0,
                                    radius == -1 ? query_capture_configuration::_config->_max_radius : radius,
                                    features);

    // push URL into the user db buckets with query fragments as key.
    // URLs are stored only for queries of radius 0. This scheme allows to save
    // DB space. To recover URLs from a query of radius > 1, a second lookup is necessary,
    // for the recorded query of radius 0 that holds the URL counters.
    int uerr = 0;
    int qerr = 0;
    hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
    while (hit!=features.end())
      {
        if ((*hit).first == 0) // radius == 0.
          {
            try
              {
                if (!query_capture_configuration::_config->_save_url_data)
                  query_capture_element::store_url((*hit).second,query,url,host,(*hit).first,plugin_name);
                else
                  {
                    // grab snippet and title, if available from the websearch plugin cache.
                    search_snippet *sp = NULL;
                    if (qc)
                      {
                        sp = qc->get_cached_snippet(url);
                        query_capture_element::store_url((*hit).second,query,url,host,
                                                         (*hit).first,plugin_name,sp);
                      }
                    else
                      {
                        query_capture_element::store_url((*hit).second,query,url,host,
                                                         (*hit).first,plugin_name,NULL);
                      }
                  }
              }
            catch (sp_exception &e)
              {
                uerr++;
              }
          }
        else  // store query alone.
          {
            try
              {
                query_capture_element::store_query((*hit).second,query,(*hit).first,plugin_name);
              }
            catch (sp_exception &e)
              {
                qerr++;
              }
          }
        ++hit;
      }
    if (uerr && qerr)
      {
        std::string msg = "failed storing URL " + url + " and query fragments for query " + query;
        throw sp_exception(QC_ERR_STORE,msg);
      }
    else if (uerr)
      {
        std::string msg = "failed storing URL " + url + " for query " + query;
        throw sp_exception(QC_ERR_STORE_URL,msg);
      }
    else if (qerr)
      {
        std::string msg = "failed storing some or all query fragments for query " + query;
        throw sp_exception(QC_ERR_STORE_QUERY,msg);
      }
  }

  void query_capture_element::store_queries(const std::string &query,
      const std::string &plugin_name,
      const int &radius) throw (sp_exception)
  {
    // generate query fragments.
    hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
    qprocess::generate_query_hashes(query,0,
                                    radius == -1 ? query_capture_configuration::_config->_max_radius : radius,
                                    features);

    // store query with hash fragment as key.
    int err = 0;
    hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
    while (hit!=features.end())
      {
        try
          {
            query_capture_element::store_query((*hit).second,query,(*hit).first,plugin_name);
          }
        catch(sp_exception &e)
          {
            err++;
          }
        ++hit;
      }
    if (err != 0)
      {
        std::string msg = "failed storing some or all query fragments for query " + query;
        throw sp_exception(QC_ERR_STORE_QUERY,msg);
      }
  }

  void query_capture_element::remove_queries(const std::string &query,
      const std::string &plugin_name,
      const int &radius) throw (sp_exception)
  {
    // generate query fragments.
    hash_multimap<uint32_t,DHTKey,id_hash_uint> features;
    qprocess::generate_query_hashes(query,0,
                                    radius == -1 ? query_capture_configuration::_config->_max_radius : radius,
                                    features);

    // remove queries.
    int err = 0;
    hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = features.begin();
    while (hit!=features.end())
      {
        try
          {
            query_capture_element::remove_query((*hit).second,query,(*hit).first,plugin_name);
          }
        catch(sp_exception &e)
          {
            if (e.code()==DB_ERR_NO_REC)
              err = DB_ERR_NO_REC;
            else err++;
          }
        ++hit;
      }
    if (err == DB_ERR_NO_REC)
      throw sp_exception(err,"");
    if (err != 0 && err != DB_ERR_NO_REC)
      {
        std::string msg = "failed removing some or all query fragments for query " + query;
        throw sp_exception(QC_ERR_REMOVE_QUERY,msg);
      }
  }

  void query_capture_element::store_query(const DHTKey &key,
                                          const std::string &query,
                                          const uint32_t &radius,
                                          const std::string &plugin_name) throw (sp_exception)
  {
    std::string key_str = key.to_rstring();
    db_query_record dbqr(plugin_name,query,radius);
    db_err err = seeks_proxy::_user_db->add_dbr(key_str,dbqr);
    if (err != SP_ERR_OK)
      {
        std::string msg = "failed storage of captured query fragment " + key_str + " for query " + query + " with error "
                          + miscutil::to_string(err);
        errlog::log_error(LOG_LEVEL_ERROR,msg.c_str());
        throw sp_exception(err,msg);
      }
  }

  void query_capture_element::remove_query(const DHTKey &key,
      const std::string &query,
      const uint32_t &radius,
      const std::string &plugin_name) throw (sp_exception)
  {
    std::string key_str = key.to_rstring();
    db_record *dbr = seeks_proxy::_user_db->find_dbr(key_str,plugin_name);
    if (!dbr)
      throw sp_exception(DB_ERR_NO_REC,"");
    db_query_record *dbqr = static_cast<db_query_record*>(dbr);
    hash_map<const char*,query_data*,hash<const char*>,eqstr>::iterator hit;
    if ((hit=dbqr->_related_queries.find(query.c_str()))!=dbqr->_related_queries.end())
      {
        // erase the query from the list, then rewrite the
        // record.
        query_data *qdata = (*hit).second;
        dbqr->_related_queries.erase(hit);
        delete qdata;
        seeks_proxy::_user_db->remove_dbr(key_str,plugin_name);
        if (!dbqr->_related_queries.empty())
          seeks_proxy::_user_db->add_dbr(key_str,*dbqr);
      }
    delete dbr;
  }

  void query_capture_element::store_url(const DHTKey &key, const std::string &query,
                                        const std::string &url, const std::string &host,
                                        const uint32_t &radius,
                                        const std::string &plugin_name,
                                        const search_snippet *sp) throw (sp_exception)
  {
    std::string key_str = key.to_rstring();
    if (!url.empty())
      {
        db_err err = SP_ERR_OK;
        if (!sp)
          {
            db_query_record dbqr(plugin_name,query,radius,url);
            err = seeks_proxy::_user_db->add_dbr(key_str,dbqr);
          }
        else
          {
            // rec_date.
            struct timeval tv_now;
            gettimeofday(&tv_now, NULL);
            uint32_t rec_date = tv_now.tv_sec;
            uint32_t url_date = sp->_content_date;
            db_query_record dbqr(plugin_name,query,radius,url,
                                 1,1,sp->_title,sp->_summary,url_date,rec_date,sp->_lang);
            err = seeks_proxy::_user_db->add_dbr(key_str,dbqr);
          }
        if (err != SP_ERR_OK)
          {
            std::string msg = "failed storage of captured url " + url + " for query " + query + " with error "
                              + miscutil::to_string(err);
            throw sp_exception(err,msg);
          }
      }
    if (!host.empty() && host != url)
      {
        db_query_record dbqr(plugin_name,query,radius,host);
        db_err err = seeks_proxy::_user_db->add_dbr(key_str,dbqr);
        if (err != SP_ERR_OK)
          {
            std::string msg = "failed storage of captured host " + host + " for query " + query + " with error "
                              + miscutil::to_string(err);
            throw sp_exception(err,msg);
          }
      }
  }

  void query_capture_element::remove_url(const DHTKey &key, const std::string &query,
                                         const std::string &url, const std::string &host,
                                         const short &url_hits, const uint32_t &radius,
                                         const std::string &plugin_name) throw (sp_exception)
  {
    std::string key_str = key.to_rstring();
    if (!url.empty())
      {
        db_query_record dbqr(plugin_name,query,radius,url,1,-url_hits);
        db_err err = seeks_proxy::_user_db->add_dbr(key_str,dbqr);
        if (err != SP_ERR_OK)
          {
            std::string msg = "failed removal of captured url " + url + " for query " + query + " with error "
                              + miscutil::to_string(err);
            throw sp_exception(err,msg);
          }
      }
    if (!host.empty() && host != url)
      {
        db_query_record dbqr(plugin_name,query,radius,host,1,-url_hits);
        db_err err = seeks_proxy::_user_db->add_dbr(key_str,dbqr);
        if (err != SP_ERR_OK)
          {
            std::string msg = "failed storage of captured host " + host + " for query " + query + " with error "
                              + miscutil::to_string(err);
            throw sp_exception(err,msg);
          }
      }
  }

  void query_capture_element::get_useful_headers(const std::list<const char*> &headers,
      std::string &host, std::string &referer,
      std::string &get, std::string &base_url)
  {
    std::list<const char*>::const_iterator lit = headers.begin();
    while (lit!=headers.end())
      {
        if (miscutil::strncmpic((*lit),"get ",4) == 0)
          {
            get = (*lit);
            try
              {
                get = get.substr(4);
              }
            catch (std::exception &e)
              {
                get = "";
              }
          }
        else if (miscutil::strncmpic((*lit),"host:",5) == 0)
          {
            host = (*lit);
            try
              {
                host = host.substr(6);
              }
            catch (std::exception &e)
              {
                host = "";
              }
          }
        else if (miscutil::strncmpic((*lit),"referer:",8) == 0)
          {
            referer = (*lit);
            try
              {
                referer = referer.substr(9);
              }
            catch (std::exception &e)
              {
                referer = "";
              }
          }
        else if (miscutil::strncmpic((*lit),"Seeks-Remote-Location:",22) == 0)
          {
            base_url = (*lit);
            try
              {
                size_t pos = base_url.find_first_of(" ");
                base_url = base_url.substr(pos+1);
              }
            catch (std::exception &e)
              {
                base_url = "";
              }
          }
        ++lit;
      }
  }

  std::string query_capture_element::no_command_query(const std::string &oquery)
  {

    std::string cquery = oquery;
    if (cquery[0] == ':')
      {
        try
          {
            cquery = cquery.substr(4);
          }
        catch (std::exception &e)
          {
            // do nothing.
          }
      }
    miscutil::replace_in_string(cquery,"\"",""); // prune out quote. XXX: could go elsewhere.
    return cquery;
  }

  /* plugin registration. */
  extern "C"
  {
    plugin* maker()
    {
      return new query_capture;
    }
  }

} /* end of namespace. */
