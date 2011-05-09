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

#include "cf.h"
#include "websearch.h"
#include "cf_configuration.h"
#include "rank_estimators.h"
#include "query_recommender.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "plugin_manager.h"
#include "cgi.h"
#include "encode.h"
#include "miscutil.h"
#include "errlog.h"

#include <sys/stat.h>
#include <iostream>

namespace seeks_plugins
{

  plugin* cf::_uc_plugin = NULL;

  cf::cf()
    :plugin()
  {
    _name = "cf";
    _version_major = "0";
    _version_minor = "1";

    // configuration.
    if (seeks_proxy::_datadir.empty())
      _config_filename = plugin_manager::_plugin_repository + "cf/cf-config";
    else
      _config_filename = seeks_proxy::_datadir + "/plugins/cf/cf-config";

#ifdef SEEKS_CONFIGDIR
    struct stat stFileInfo;
    if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
      {
        _config_filename = SEEKS_CONFIGDIR "/cf-config";
      }
#endif

    if (cf_configuration::_config == NULL)
      cf_configuration::_config = new cf_configuration(_config_filename);
    _configuration = cf_configuration::_config;

    // cgi dispatchers.
    _cgi_dispatchers.reserve(1);
    cgi_dispatcher *cgid_tbd
    = new cgi_dispatcher("tbd",&cf::cgi_tbd,NULL,TRUE);
    _cgi_dispatchers.push_back(cgid_tbd);
  }

  cf::~cf()
  {
    cf_configuration::_config = NULL; // configuration is deleted in parent class.
  }

  void cf::start()
  {
    //TODO: check on user_db.

    // look for dependent plugins.
    cf::_uc_plugin = plugin_manager::get_plugin("uri-capture");
  }

  void cf::stop()
  {
  }

  sp_err cf::cgi_tbd(client_state *csp,
                     http_response *rsp,
                     const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
  {
    if (!parameters->empty())
      {
        std::string url,query,lang;
        sp_err err = cf::tbd(parameters,url,query,lang);
        if (err != SP_ERR_OK && err == SP_ERR_CGI_PARAMS)
          {
            errlog::log_error(LOG_LEVEL_INFO,"bad parameter to tbd callback");
            return err;
          }

        // redirect to current query url.
        miscutil::unmap(const_cast<hash_map<const char*,const char*,hash<const char*>,eqstr>*>(parameters),"url");
        std::string base_url = query_context::detect_base_url_http(csp);

        const char *output = miscutil::lookup(parameters,"output");
        std::string output_str = output ? std::string(output) : "html";
        std::transform(output_str.begin(),output_str.end(),output_str.begin(),tolower);
        return websearch::cgi_websearch_search(csp,rsp,parameters);
      }
    else return SP_ERR_CGI_PARAMS;
  }

  sp_err cf::tbd(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
                 std::string &url, std::string &query, std::string &lang)
  {
    const char *urlp = miscutil::lookup(parameters,"url");
    if (!urlp)
      return SP_ERR_CGI_PARAMS;
    const char *queryp = miscutil::lookup(parameters,"q");
    if (!queryp)
      return SP_ERR_CGI_PARAMS;

    char *dec_urlp = encode::url_decode_but_not_plus(urlp);
    url = std::string(dec_urlp);
    free(dec_urlp);
    query = std::string(queryp);
    const char *langp = miscutil::lookup(parameters,"lang");
    if (!langp)
      {
        // XXX: this should not happen.
        return SP_ERR_CGI_PARAMS;
      }
    lang = std::string(langp);
    try
      {
        cf::thumb_down_url(query,lang,url);
      }
    catch(sp_exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR,e.to_string().c_str());
        return e.code();
      }
    return SP_ERR_OK;
  }

  void cf::personalize(const std::string &query,
                       const std::string &lang,
                       const uint32_t &expansion,
                       uint32_t &npeers,
                       std::vector<search_snippet*> &snippets,
                       std::multimap<double,std::string,std::less<double> > &related_queries,
                       hash_map<uint32_t,search_snippet*,id_hash_uint> &reco_snippets) throw (sp_exception)
  {
    simple_re sre;
    sre.peers_personalize(query,lang,expansion,npeers,snippets,related_queries,reco_snippets);
  }

  void cf::estimate_ranks(const std::string &query,
                          const std::string &lang,
                          const uint32_t &expansion,
                          std::vector<search_snippet*> &snippets,
                          const std::string &host,
                          const int &port) throw (sp_exception)
  {
    simple_re sre; // estimator.
    sre.estimate_ranks(query,lang,expansion,snippets,host,port);
  }

  void cf::get_related_queries(const std::string &query,
                               const std::string &lang,
                               const uint32_t &expansion,
                               std::multimap<double,std::string,std::less<double> > &related_queries,
                               const std::string &host,
                               const int &port) throw (sp_exception)
  {
    query_recommender::recommend_queries(query,lang,expansion,related_queries,host,port);
  }

  void cf::get_recommended_urls(const std::string &query,
                                const std::string &lang,
                                const uint32_t &expansion,
                                hash_map<uint32_t,search_snippet*,id_hash_uint> &snippets,
                                const std::string &host,
                                const int &port) throw (sp_exception)
  {
    simple_re sre; // estimator.
    sre.recommend_urls(query,lang,expansion,snippets,host,port);
  }

  void cf::thumb_down_url(const std::string &query,
                          const std::string &lang,
                          const std::string &url) throw (sp_exception)
  {
    simple_re sre; // estimator.
    sre.thumb_down_url(query,lang,url);
  }

  /* plugin registration. */
  extern "C"
  {
    plugin* maker()
    {
      return new cf;
    }
  }

} /* end of namespace. */
