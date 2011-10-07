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
#include "errlog.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>

#include <map>
#include <string>
#include <iostream>

using namespace seekscli;
using sp::errlog;

void print_usage()
{
  std::cout << "usage: seeks_cli [--output <html,json,xml> [--x <get,put,delete,post>] <command> <url> [<query>] [<args>]\n";
  std::cout << "seeks_cli info <url>\n";
  std::cout << "seeks_cli peers <url>\n";
  std::cout << "seeks_cli [--x <get>] recommend <url> <query> [--nreco <nreco>] [--radius <radius>] [--peers <local|ring>] [--lang <lang>] [--order <rank|new-date|old-date|new-activity|old-activity>]\n";
  std::cout << "seeks_cli --x <post> recommend <url> <query> --title <title> [--radius <radius>] [--peers <local|ring>] [--lang <lang>] [--url-check <0|1>]\n";
  std::cout << "seeks_cli --x <delete> recommend <url> <query> --url <url> [--lang <lang>]\n";
  std::cout << "seeks_cli suggest <url> <query> [--nsugg <nsugg>] [--radius <radius>] [--peers <local|ring>]\n";
  std::cout << "seeks_cli [--x <get,put,delete,post>] search <url> <query> [--sid <snippet_id> | --surl <url>] [--engines <list of separated engines>] [--rpp <rpp>] [--page <page>] [--prs <on|off>] [--lang <lang>] [--thumbs <on|off>] [--expansion <expansion>] [--peers <local|ring>] [--order <rank|new-date|old-date|new-activity|old-activity>] [--redirect <url>] [--cpost <url>]\n";
  std::cout << "seeks_cli words <url> <query> [--sid <snippet_id> | --surl <url>] [--lang <lang>]\n";
  std::cout << "seeks_cli recent_queries <url> [--nq <nq>]\n";
  std::cout << "seeks_cli cluster_types <url> <query> [--lang <lang>]\n";
  std::cout << "seeks_cli cluster_auto <url> <query> [--nclusters <nclusters>] [--lang <lang>]\n";
  std::cout << "seeks_cli similar <url> <query> [--sid <snippet_id> | --surl <url>] [--lang <lang>]\n";
  std::cout << "seeks_cli cache <url> <query> --url <url> [--lang <lang>]\n";

  //TODO: examples.
}

int main(int argc, char **argv)
{
  if (argc < 2)
    {
      print_usage();
      exit(0);
    }

  errlog::init_log_module();
  errlog::set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO | LOG_LEVEL_DEBUG);

  // catch options.
  std::string output = "json";
  std::string http_method = "get";
  int i = 0;
  while(++i < argc)
    {
      std::string o = argv[i];
      if (o.substr(0,2)!="--")
        break;
      if (o == "--x")
        {
          const char *hm = argv[++i];
          if (hm)
            {
              http_method = hm;
              if (http_method != "put" && http_method != "get"
                  && http_method != "delete" && http_method != "post")
                {
                  std::cout << "wrong argument: --x " + http_method << std::endl;
                  exit(-1);
                }
            }
          else
            {
              std::cout << "missing argument: --x\n";
              exit(-1);
            }
        }
      else if (o == "--output")
        {
          const char *ou = argv[++i];
          if (ou)
            {
              output = ou;
              if (output != "json" && output != "html" && output != "xml")
                {
                  std::cout << "wrong argument: --output " + output << std::endl;
                  exit(-1);
                }
            }
          else
            {
              std::cout << "missing argument: --output\n";
              exit(-1);
            }
        }
    }

  std::string command = argv[i];
  std::string node = argv[++i];
  std::string query;
  if (++i < argc)
    query = argv[i];

  // parameters.
  std::map<std::string,std::string> params;
  while(++i < argc)
    {
      // XXX: would be more elegant to use a hashtable.
      std::string p = argv[i];
      if (p == "--nreco" || p == "--radius" || p == "--peers"
          || p == "--lang" || p == "--order" || p == "--nsugg"
          || p == "--url" || p == "--title" || p == "--url-check"
          || p == "--nclusters" || p == "--sid" || p == "--engines"
          || p == "--rpp" || p == "--page" || p == "--prs"
          || p == "--thumbs" || p == "--nq" || p == "--redirect"
          || p == "--cpost" || p == "--surl")
        {
          std::string v = argv[++i];
          params.insert(std::pair<std::string,std::string>(p,v));
        }
      else
        {
          std::cout << "unknown parameter: " << p << std::endl;
          exit(-1);
        }
    }

  // call duration.
  struct tms st_cpu;
  struct tms en_cpu;
  clock_t start_time = times(&st_cpu);

  int timeout = 5; // default.
  std::map<std::string,std::string>::const_iterator mit;
  std::string *result = NULL;
  int err = 0;
  if (command == "info")
    {
      err = cli::get_info(node,output,timeout,result);
    }
  else if (command == "peers")
    {
      err = cli::get_peers(node,output,timeout,result);
    }
  else if (command == "recommend")
    {
      std::string nreco,radius,peers,lang,order,purl,title,url_check;
      if ((mit=params.find("--nreco"))!=params.end())
        nreco = (*mit).second;
      if ((mit=params.find("--radius"))!=params.end())
        radius = (*mit).second;
      if ((mit=params.find("--peers"))!=params.end())
        peers = (*mit).second;
      if ((mit=params.find("--lang"))!=params.end())
        lang = (*mit).second;
      if ((mit=params.find("--order"))!=params.end())
        order = (*mit).second;
      if ((mit=params.find("--url"))!=params.end())
        purl = (*mit).second;
      if ((mit=params.find("--title"))!=params.end())
        title = (*mit).second;
      if ((mit=params.find("--url_check"))!=params.end())
        url_check = (*mit).second;
      if (http_method == "get")
        err = cli::get_recommendation(node,output,timeout,
                                      query,nreco,radius,peers,lang,order,
                                      result);
      else if (http_method == "delete")
        err = cli::delete_recommendation(node,output,timeout,query,purl,lang,result);
      else if (http_method == "post")
        err = cli::post_recommendation(node,output,timeout,query,purl,title,
                                       radius,url_check,lang,result);
    }
  else if (command == "suggest")
    {
      std::string nsugg,peers,radius;
      if ((mit=params.find("--peers"))!=params.end())
        peers = (*mit).second;
      if ((mit=params.find("--nsugg"))!=params.end())
        nsugg = (*mit).second;
      if ((mit=params.find("--radius"))!=params.end())
        radius = (*mit).second;
      err = cli::get_suggestion(node,output,timeout,
                                query,nsugg,radius,peers,result);
    }
  else if (command == "search")
    {
      std::string sid,rpp,peers,expansion,engines,page,prs,
          lang,thumbs,order,redirect,cpost,surl;
      if ((mit=params.find("--sid"))!=params.end())
        sid = (*mit).second;
      else if ((mit=params.find("--surl"))!=params.end())
        surl = (*mit).second;
      if ((mit=params.find("--peers"))!=params.end())
        peers = (*mit).second;
      if ((mit=params.find("--rpp"))!=params.end())
        rpp = (*mit).second;
      if ((mit=params.find("--expansion"))!=params.end())
        expansion = (*mit).second;
      if ((mit=params.find("--engines"))!=params.end())
        engines = (*mit).second;
      if ((mit=params.find("--page"))!=params.end())
        page = (*mit).second;
      if ((mit=params.find("--prs"))!=params.end())
        prs = (*mit).second;
      if ((mit=params.find("--lang"))!=params.end())
        lang = (*mit).second;
      if ((mit=params.find("--thumbs"))!=params.end())
        thumbs = (*mit).second;
      if ((mit=params.find("--order"))!=params.end())
        order = (*mit).second;
      if ((mit=params.find("--redirect"))!=params.end())
        redirect = (*mit).second;
      if ((mit=params.find("--cpost"))!=params.end())
        cpost = (*mit).second;
      if (http_method == "get")
        {
          if (sid.empty() && surl.empty())
            err = cli::get_search_txt_query(node,output,timeout,
                                            query,engines,rpp,page,lang,thumbs,
                                            expansion,peers,order,result);
          else err = cli::get_search_txt_snippet(node,output,timeout,
                                                   query,sid,surl,lang,result);
        }
      else if (http_method == "put")
        {
          err = cli::put_search_txt_query(node,output,timeout,
                                          query,engines,rpp,page,lang,thumbs,
                                          expansion,peers,order,result);
        }
      else if (http_method == "post" && (!sid.empty() || !surl.empty()))
        {
          err = cli::post_search_snippet(node,output,timeout,
                                         query,sid,surl,lang,redirect,cpost,result);
        }
      else if (http_method == "delete" && (!sid.empty() || !surl.empty()))
        {
          err = cli::delete_search_snippet(node,output,timeout,
                                           query,sid,surl,lang,result);
        }
      else
        {
          std::cout << "wrong combination of http method " << http_method
                    << " and parameters, maybe a missing --sid <snippet_id> or --surl <url> ?\n";
        }
    }
  else if (command == "words")
    {
      std::string lang,sid,surl;
      if ((mit=params.find("--sid"))!=params.end())
        sid = (*mit).second;
      if ((mit=params.find("--surl"))!=params.end())
        surl = (*mit).second;
      if ((mit=params.find("--lang"))!=params.end())
        lang = (*mit).second;
      if (sid.empty() && surl.empty())
        err = cli::get_words_query(node,output,timeout,
                                   query,lang,result);
      else err = cli::get_words_snippet(node,output,timeout,
                                          query,sid,surl,lang,result);
    }
  else if (command == "recent_queries")
    {
      std::string nq;
      if ((mit=params.find("--nq"))!=params.end())
        nq = (*mit).second;
      err = cli::get_recent_queries(node,output,timeout,
                                    nq,result);
    }
  else if (command == "cluster_types")
    {
      std::string lang;
      if ((mit=params.find("--lang"))!=params.end())
        lang = (*mit).second;
      err = cli::get_cluster_types(node,output,timeout,
                                   query,lang,result);
    }
  else if (command == "cluster_auto")
    {
      std::string lang,nclusters;
      if ((mit=params.find("--lang"))!=params.end())
        lang = (*mit).second;
      if ((mit=params.find("--nclusters"))!=params.end())
        nclusters = (*mit).second;
      err = cli::get_cluster_auto(node,output,timeout,
                                  query,lang,nclusters,result);
    }
  else if (command == "similar")
    {
      std::string sid,lang,surl;
      if ((mit=params.find("--lang"))!=params.end())
        lang = (*mit).second;
      if ((mit=params.find("--sid"))!=params.end())
        sid = (*mit).second;
      if ((mit=params.find("--surl"))!=params.end())
        surl = (*mit).second;
      err = cli::get_similar_txt_snippet(node,output,timeout,
                                         query,sid,surl,lang,result);
    }
  else if (command == "cache")
    {
      std::string lang,url;
      if ((mit=params.find("--lang"))!=params.end())
        lang = (*mit).second;
      if ((mit=params.find("--url"))!=params.end())
        url = (*mit).second;
      err = cli::get_cache_txt(node,output,timeout,
                               query,url,lang,result);
    }
  else
    {
      std::cout << "unknown command: " << command << std::endl;
      exit(0);
    }

  clock_t end_time = times(&en_cpu);
  double qtime = (end_time-start_time)/static_cast<double>(sysconf(_SC_CLK_TCK));

  // output and errors.
  std::cout << "status: ";
  if (err == 0)
    std::cout << "OK"; //TODO: more statuses.
  else std::cout << err;
  std::cout << std::endl;
  std::cout << "call duration: " << qtime << " sec\n";
  if (!query.empty())
    std::cout << "query: " << query << std::endl;
  if (result)
    std::cout << *result << std::endl;
  delete result;
}
