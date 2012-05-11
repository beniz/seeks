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

#ifndef CLI_H
#define CLI_H

#include <string>

namespace seekscli
{

  class cli
  {
    public:
      /* generic */
      static void make_call(const int &timeout,
                            const std::string &url,
                            const std::string &http_method,
                            int &status,
                            std::string *&result,
                            std::string *content=NULL,
                            const int &content_size=-1);

      static std::string url_encode(const std::string &str);

      static std::string strip_url(const std::string &url);

      static std::string url_to_id(const std::string &url);

      static void set_proxy(const std::string &proxy_addr,
                            const short &proxy_port);

      /* info */
      static int get_info(const std::string &seeks_url,
                          const std::string &output,
                          const int &timeout,
                          std::string *&result);

      /* websearch */
    private:
      static int get_search_txt(const std::string &seeks_url,
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
                                std::string *&result);

    public:
      static int get_search_txt_query(const std::string &seeks_url,
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
                                      std::string *&result);

      static int put_search_txt_query(const std::string &seeks_url,
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
                                      std::string *&result);

      static int get_search_txt_snippet(const std::string &seeks_url,
                                        const std::string &output,
                                        const int &timeout,
                                        const std::string &query,
                                        const std::string &snippet_id,
                                        const std::string &snippet_url,
                                        const std::string &lang,
                                        const std::string &swords,
                                        std::string *&result);

      static int post_search_snippet(const std::string &seeks_url,
                                     const std::string &output,
                                     const int &timeout,
                                     const std::string &query,
                                     const std::string &snippet_id,
                                     const std::string &snippet_url,
                                     const std::string &lang,
                                     const std::string &redirect,
                                     const std::string &cpost,
                                     std::string *&result);

      static int delete_search_snippet(const std::string &seeks_url,
                                       const std::string &output,
                                       const int &timeout,
                                       const std::string &query,
                                       const std::string &snippet_id,
                                       const std::string &snippet_url,
                                       const std::string &lang,
                                       std::string *&result);

    private:
      static int get_words(const std::string &seeks_url,
                           const std::string &output,
                           const int &timeout,
                           const std::string &query,
                           const std::string &snippet_id,
                           const std::string &snippet_url,
                           const std::string &lang,
                           std::string *&result);

    public:
      static int get_words_query(const std::string &seeks_url,
                                 const std::string &output,
                                 const int &timeout,
                                 const std::string &query,
                                 const std::string &lang,
                                 std::string *&result);

      static int get_words_snippet(const std::string &seeks_url,
                                   const std::string &output,
                                   const int &timeout,
                                   const std::string &query,
                                   const std::string &snippet_id,
                                   const std::string &snippet_url,
                                   const std::string &lang,
                                   std::string *&result);

      static int get_recent_queries(const std::string &seeks_url,
                                    const std::string &output,
                                    const int &timeout,
                                    const std::string &nq,
                                    std::string *&result);

    private:
      static int get_cluster(const std::string &seeks_url,
                             const std::string &output,
                             const int &timeout,
                             const std::string &cluster_type,
                             const std::string &query,
                             const std::string &lang,
                             const std::string &nclusters,
                             const std::string &engines,
                             const std::string &expansion,
                             const std::string &peers,
                             std::string *&result);

    public:
      static int get_cluster_types(const std::string &seeks_url,
                                   const std::string &output,
                                   const int &timeout,
                                   const std::string &query,
                                   const std::string &lang,
                                   const std::string &engines,
                                   const std::string &expansion,
                                   const std::string &peers,
                                   std::string *&result);

      static int get_cluster_auto(const std::string &seeks_url,
                                  const std::string &output,
                                  const int &timeout,
                                  const std::string &query,
                                  const std::string &lang,
                                  const std::string &nclusters,
                                  const std::string &engines,
                                  const std::string &expansion,
                                  const std::string &peers,
                                  std::string *&result);

      static int get_similar_txt_snippet(const std::string &seeks_url,
                                         const std::string &output,
                                         const int &timeout,
                                         const std::string &query,
                                         const std::string &snippet_id,
                                         const std::string &snippet_url,
                                         const std::string &lang,
                                         std::string *&result);

      static int get_cache_txt(const std::string &seeks_url,
                               const std::string &output,
                               const int &timeout,
                               const std::string &query,
                               const std::string &url,
                               const std::string &lang,
                               std::string *&result);

      /* collaborative filter */
      static int get_peers(const std::string &seeks_url,
                           const std::string &output,
                           const int &timeout,
                           std::string *&result);

      static int get_suggestion(const std::string &seeks_url,
                                const std::string &output,
                                const int &timeout,
                                const std::string &query,
                                const std::string &nsugg,
                                const std::string &radius,
                                const std::string &peers,
                                const std::string &swords,
                                std::string *&result);

      static int get_recommendation(const std::string &seeks_url,
                                    const std::string &output,
                                    const int &timeout,
                                    const std::string &query,
                                    const std::string &nreco,
                                    const std::string &radius,
                                    const std::string &peers,
                                    const std::string &lang,
                                    const std::string &order,
                                    const std::string &swords,
                                    std::string *&result);

      static int post_recommendation(const std::string &seeks_url,
                                     const std::string &output,
                                     const int &timeout,
                                     const std::string &query,
                                     const std::string &url,
                                     const std::string &title,
                                     const std::string &radius,
                                     const std::string &url_check,
                                     const std::string &lang,
                                     std::string *&result);

      static int delete_recommendation(const std::string &seeks_url,
                                       const std::string &output,
                                       const int &timeout,
                                       const std::string &query,
                                       const std::string &url,
                                       const std::string &lang,
                                       std::string *&result);
    private:
      static int recommendation(const std::string &seeks_url,
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
                                std::string *&result);

      static std::string _proxy_addr;
      static short _proxy_port;
  };

} /* end of namespace. */

#endif
