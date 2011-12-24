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

#include "cli.h"
#include "curl_mget.h"
#include "encode.h"
#include "miscutil.h"
#include "superfasthash.h"

#include <cstdlib>
#include <iostream>

using sp::curl_mget;
using sp::encode;
using sp::miscutil;

namespace seekscli
{
  std::string cli::_proxy_addr = "";
  short cli::_proxy_port = 0;

  void cli::make_call(const int &timeout,
                      const std::string &url,
                      const std::string &http_method,
                      int &status,
                      std::string *&result)
  {
    curl_mget cmg(1,timeout,0,timeout,0);
    result = cmg.www_simple(url,NULL,status,http_method,NULL,-1,"",
                            cli::_proxy_addr,cli::_proxy_port);
  }

  std::string cli::url_encode(const std::string &str)
  {
    char *enc_str_c = encode::url_encode(str.c_str());
    std::string enc_str = std::string(enc_str_c);
    free(enc_str_c);
    return enc_str;
  }

  std::string cli::strip_url(const std::string &url)
  {
    std::string surl = url;
    if (strncmp(surl.c_str(),"http://",7)==0)
      surl = surl.substr(7);
    else if (strncmp(surl.c_str(),"https://",8)==0)
      surl = surl.substr(8);
    if (miscutil::strncmpic(surl.c_str(),"www.",4)==0)
      surl = surl.substr(4);
    if (surl[surl.length()-1]=='/') // remove trailing '/'.
      surl = surl.substr(0,surl.length()-1);
    return surl;
  }

  std::string cli::url_to_id(const std::string &url)
  {
    std::string url_lc = url;
    miscutil::to_lower(url_lc);
    url_lc = cli::strip_url(url_lc);
    uint32_t id = SuperFastHash(url_lc.c_str(),url_lc.size());
    return miscutil::to_string(id);
  }

  void cli::set_proxy(const std::string &proxy_addr,
                      const short &proxy_port)
  {
    cli::_proxy_addr = proxy_addr;
    cli::_proxy_port = proxy_port;
  }

  int cli::get_info(const std::string &seeks_url,
                    const std::string &output,
                    const int &timeout,
                    std::string *&result)
  {
    std::string url = seeks_url + "/info";
    int status = 0;
    cli::make_call(timeout,url,"GET",status,result);
    return status;
  }

  int cli::get_search_txt(const std::string &seeks_url,
                          const std::string &output,
                          const int &timeout,
                          const std::string &http_method,
                          const std::string &query,
                          const std::string &snippet_id,
                          const std::string &snippet_url,
                          const std::string &engines,
                          const std::string &rpp,
                          const std::string &page,
                          const std::string &lang,
                          const std::string &thumbs,
                          const std::string &expansion,
                          const std::string &peers,
                          const std::string &order,
                          const std::string &redirect,
                          const std::string &cpost,
                          const std::string &swords,
                          std::string *&result)
  {
    std::string enc_query = cli::url_encode(query);
    std::string url = seeks_url + "/search/txt/" + enc_query;
    if (!snippet_id.empty())
      url += "/" + snippet_id;
    else if (!snippet_url.empty())
      url += "/" + cli::url_to_id(snippet_url);
    url += "?output=" + output;
    if (!engines.empty())
      url += "&engines=" + engines;
    if (!rpp.empty())
      url += "&rpp=" + rpp;
    if (!page.empty())
      url += "&page=" + page;
    if (!lang.empty())
      url += "&lang=" + lang;
    if (!thumbs.empty())
      url += "&thumbs=" + thumbs;
    if (!expansion.empty())
      url += "&expansion=" + expansion;
    if (!peers.empty())
      url += "&peers=" + peers;
    if (!order.empty())
      url += "&order=" + order;
    if (!redirect.empty())
      url += "&redirect=" + redirect;
    if (!cpost.empty())
      {
        std::string enc_cpost = cli::url_encode(cpost);
        url += "&cpost=" + enc_cpost;
      }
    if (!swords.empty())
      url += "&swords=" + swords;
    int status = 0;
    cli::make_call(timeout,url,http_method,status,result);
    return status;
  }

  int cli::get_search_txt_query(const std::string &seeks_url,
                                const std::string &output,
                                const int &timeout,
                                const std::string &query,
                                const std::string &engines,
                                const std::string &rpp,
                                const std::string &page,
                                const std::string &lang,
                                const std::string &thumbs,
                                const std::string &expansion,
                                const std::string &peers,
                                const std::string &order,
                                const std::string &swords,
                                std::string *&result)
  {
    return cli::get_search_txt(seeks_url,output,timeout,"GET",query,"","",engines,
                               rpp,page,lang,thumbs,expansion,peers,order,
                               "","",swords,result);
  }

  int cli::put_search_txt_query(const std::string &seeks_url,
                                const std::string &output,
                                const int &timeout,
                                const std::string &query,
                                const std::string &engines,
                                const std::string &rpp,
                                const std::string &page,
                                const std::string &lang,
                                const std::string &thumbs,
                                const std::string &expansion,
                                const std::string &peers,
                                const std::string &order,
                                std::string *&result)
  {
    return cli::get_search_txt(seeks_url,output,timeout,"PUT",query,"","",engines,
                               rpp,page,lang,thumbs,expansion,peers,order,
                               "","","",result);
  }

  int cli::get_search_txt_snippet(const std::string &seeks_url,
                                  const std::string &output,
                                  const int &timeout,
                                  const std::string &query,
                                  const std::string &snippet_id,
                                  const std::string &snippet_url,
                                  const std::string &lang,
                                  const std::string &swords,
                                  std::string *&result)
  {
    return cli::get_search_txt(seeks_url,output,timeout,"GET",query,snippet_id,
                               snippet_url,"","","",lang,"","","","","","",swords,result);
  }

  int cli::post_search_snippet(const std::string &seeks_url,
                               const std::string &output,
                               const int &timeout,
                               const std::string &query,
                               const std::string &snippet_id,
                               const std::string &snippet_url,
                               const std::string &lang,
                               const std::string &redirect,
                               const std::string &cpost,
                               std::string *&result)
  {
    return cli::get_search_txt(seeks_url,output,timeout,"POST",query,snippet_id,
                               snippet_url,"","","",lang,"","","","",redirect,cpost,"",result);
  }

  int cli::delete_search_snippet(const std::string &seeks_url,
                                 const std::string &output,
                                 const int &timeout,
                                 const std::string &query,
                                 const std::string &snippet_id,
                                 const std::string &snippet_url,
                                 const std::string &lang,
                                 std::string *&result)
  {
    return cli::get_search_txt(seeks_url,output,timeout,"DELETE",query,snippet_id,
                               snippet_url,"","","",lang,"","","","","","","",result);
  }

  int cli::get_words(const std::string &seeks_url,
                     const std::string &output,
                     const int &timeout,
                     const std::string &query,
                     const std::string &snippet_id,
                     const std::string &snippet_url,
                     const std::string &lang,
                     std::string *&result)
  {
    std::string enc_query = cli::url_encode(query);
    std::string url = seeks_url + "/words/" + enc_query;
    if (!snippet_id.empty())
      url += "/" + snippet_id;
    else if (!snippet_url.empty())
      url += "/" + cli::url_to_id(snippet_url);
    url += "?output=" + output;
    if (!lang.empty())
      url += "&lang=" + lang;
    int status = 0;
    cli::make_call(timeout,url,"GET",status,result);
    return status;
  }

  int cli::get_words_query(const std::string &seeks_url,
                           const std::string &output,
                           const int &timeout,
                           const std::string &query,
                           const std::string &lang,
                           std::string *&result)
  {
    return cli::get_words(seeks_url,output,timeout,query,"","",
                          lang,result);
  }

  int cli::get_words_snippet(const std::string &seeks_url,
                             const std::string &output,
                             const int &timeout,
                             const std::string &query,
                             const std::string &snippet_id,
                             const std::string &snippet_url,
                             const std::string &lang,
                             std::string *&result)
  {
    return cli::get_words(seeks_url,output,timeout,query,snippet_id,
                          snippet_url,lang,result);
  }

  int cli::get_recent_queries(const std::string &seeks_url,
                              const std::string &output,
                              const int &timeout,
                              const std::string &nq,
                              std::string *&result)
  {
    std::string url = seeks_url + "/recent/queries?output=" + output;
    if (!nq.empty())
      url += "&nq=" + nq;
    int status = 0;
    cli::make_call(timeout,url,"GET",status,result);
    return status;
  }

  int cli::get_cluster(const std::string &seeks_url,
                       const std::string &output,
                       const int &timeout,
                       const std::string &cluster_type,
                       const std::string &query,
                       const std::string &lang,
                       const std::string &nclusters,
                       const std::string &engines,
                       const std::string &expansion,
                       const std::string &peers,
                       std::string *&result)
  {
    std::string enc_query = cli::url_encode(query);
    std::string url = seeks_url + "/cluster/" + cluster_type + "/"
                      + enc_query + "?output=" + output;
    if (!lang.empty())
      url += "&lang=" + lang;
    if (!nclusters.empty())
      url += "&clusters=" + nclusters;
    if (!engines.empty())
      url += "&engines=" + engines;
    if (!expansion.empty())
      url += "&expansion=" + expansion;
    if (!peers.empty())
      url += "&peers=" + peers;

    int status = 0;
    cli::make_call(timeout,url,"GET",status,result);
    return status;
  }

  int cli::get_cluster_types(const std::string &seeks_url,
                             const std::string &output,
                             const int &timeout,
                             const std::string &query,
                             const std::string &lang,
                             const std::string &engines,
                             const std::string &expansion,
                             const std::string &peers,
                             std::string *&result)
  {
    return cli::get_cluster(seeks_url,output,timeout,"types",
                            query,lang,"",engines,expansion,peers,result);
  }

  int cli::get_cluster_auto(const std::string &seeks_url,
                            const std::string &output,
                            const int &timeout,
                            const std::string &query,
                            const std::string &lang,
                            const std::string &nclusters,
                            const std::string &engines,
                            const std::string &expansion,
                            const std::string &peers,
                            std::string *&result)
  {
    return cli::get_cluster(seeks_url,output,timeout,"auto",
                            query,lang,nclusters,engines,expansion,peers,result);
  }

  int cli::get_similar_txt_snippet(const std::string &seeks_url,
                                   const std::string &output,
                                   const int &timeout,
                                   const std::string &query,
                                   const std::string &snippet_id,
                                   const std::string &snippet_url,
                                   const std::string &lang,
                                   std::string *&result)
  {
    std::string enc_query = cli::url_encode(query);
    std::string url = seeks_url + "/similar/txt/" + enc_query;
    if (!snippet_id.empty())
      url += "/" + snippet_id;
    else if (!snippet_url.empty())
      url += "/" + cli::url_to_id(snippet_url);
    url += "?output=" + output;
    if (!lang.empty())
      url += "&lang=" + lang;
    int status = 0;
    cli::make_call(timeout,url,"GET",status,result);
    return status;
  }

  int cli::get_cache_txt(const std::string &seeks_url,
                         const std::string &output,
                         const int &timeout,
                         const std::string &query,
                         const std::string &url,
                         const std::string &lang,
                         std::string *&result)
  {
    std::string enc_query = cli::url_encode(query);
    std::string rurl = seeks_url + "/cache/txt/" + enc_query + "?output=" + output;
    if (!lang.empty())
      rurl += "&lang=" + lang;
    if (!url.empty())
      {
        std::string enc_url = cli::url_encode(url);
        rurl += "&url=" + enc_url;
      }
    int status = 0;
    cli::make_call(timeout,rurl,"GET",status,result);
    return status;
  }

  int cli::get_peers(const std::string &seeks_url,
                     const std::string &output,
                     const int &timeout,
                     std::string *&result)
  {
    std::string url = seeks_url + "/peers";
    int status = 0;
    cli::make_call(timeout,url,"GET",status,result);
    return status;
  }

  int cli::recommendation(const std::string &seeks_url,
                          const std::string &output,
                          const int &timeout,
                          const std::string &http_method,
                          const std::string &query,
                          const std::string &nreco,
                          const std::string &radius,
                          const std::string &peers,
                          const std::string &lang,
                          const std::string &order,
                          const std::string &url,
                          const std::string &title,
                          const std::string &url_check,
                          const std::string &swords,
                          std::string *&result)
  {
    std::string enc_query = cli::url_encode(query);
    std::string rurl = seeks_url + "/recommendation/" + enc_query + "?output=" + output;
    if (!nreco.empty())
      rurl += "&nreco=" + nreco;
    if (!radius.empty())
      rurl += "&radius=" + radius;
    if (!peers.empty())
      rurl += "&peers=" + peers;
    if (!lang.empty())
      rurl += "&lang=" + lang;
    if (!order.empty())
      rurl += "&order=" + order;
    if (!url.empty())
      {
        std::string enc_url = cli::url_encode(url);
        rurl += "&url=" + enc_url;
      }
    if (!title.empty())
      {
        std::string enc_title = cli::url_encode(title);
        rurl += "&title=" + enc_title;
      }
    if (!url_check.empty())
      rurl += "&url_check=" + url_check;
    if (!swords.empty())
      rurl += "&swords=" + swords;
    int status = 0;
    cli::make_call(timeout,rurl,http_method,status,result);
    return status;
  }

  int cli::get_recommendation(const std::string &seeks_url,
                              const std::string &output,
                              const int &timeout,
                              const std::string &query,
                              const std::string &nreco,
                              const std::string &radius,
                              const std::string &peers,
                              const std::string &lang,
                              const std::string &order,
                              const std::string &swords,
                              std::string *&result)
  {
    return cli::recommendation(seeks_url,output,timeout,"GET",query,nreco,radius,peers,
                               lang,order,"","","",swords,result);
  }

  int cli::post_recommendation(const std::string &seeks_url,
                               const std::string &output,
                               const int &timeout,
                               const std::string &query,
                               const std::string &url,
                               const std::string &title,
                               const std::string &radius,
                               const std::string &url_check,
                               const std::string &lang,
                               std::string *&result)
  {
    return cli::recommendation(seeks_url,output,timeout,"POST",query,"",radius,"",
                               lang,"",url,title,url_check,"",result);
  }

  int cli::delete_recommendation(const std::string &seeks_url,
                                 const std::string &output,
                                 const int &timeout,
                                 const std::string &query,
                                 const std::string &url,
                                 const std::string &lang,
                                 std::string *&result)
  {
    return cli::recommendation(seeks_url,output,timeout,"DELETE",query,"","","",
                               lang,"",url,"","","",result);
  }

  int cli::get_suggestion(const std::string &seeks_url,
                          const std::string &output,
                          const int &timeout,
                          const std::string &query,
                          const std::string &nsugg,
                          const std::string &radius,
                          const std::string &peers,
                          const std::string &swords,
                          std::string *&result)
  {
    std::string enc_query = cli::url_encode(query);
    std::string url = seeks_url + "/suggestion/" + enc_query + "?output=" + output;
    if (!nsugg.empty())
      url += "&nsugg=" + nsugg;
    if (!radius.empty())
      url += "&radius=" + radius;
    if (!peers.empty())
      url += "&peers=" + peers;
    if (!swords.empty())
      url += "&swords=" + swords;
    int status = 0;
    cli::make_call(timeout,url,"GET",status,result);
    return status;
  }

} /* end of namespace. */
