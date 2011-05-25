/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009-2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "curl_mget.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "miscutil.h"
#include "errlog.h"

#include <pthread.h>

#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <assert.h>

namespace sp
{
  static size_t write_data(void *ptr, size_t size, size_t nmemb, void *userp)
  {
    char *buffer = static_cast<char*>(ptr);
    cbget *arg = static_cast<cbget*>(userp);
    size *= nmemb;

    if (!arg->_output)
      arg->_output = new std::string();

    arg->_output->append(buffer,size);

    return size;
  }

  curl_mget::curl_mget(const int &nrequests,
                       const long &connect_timeout_sec,
                       const long &connect_timeout_ms,
                       const long &transfer_timeout_sec,
                       const long &transfer_timeout_ms)
    :_nrequests(nrequests),_connect_timeout_sec(connect_timeout_sec),
     _connect_timeout_ms(connect_timeout_ms),_transfer_timeout_sec(transfer_timeout_sec),
     _transfer_timeout_ms(transfer_timeout_ms)
  {
    _outputs = new std::string*[_nrequests];
    for (int i=0; i<_nrequests; i++)
      _outputs[i] = NULL;
    _cbgets = new cbget*[_nrequests];
  }

  curl_mget::~curl_mget()
  {
    delete[] _cbgets;
  }

  void* pull_one_url(void *arg_cbget)
  {
    if (!arg_cbget)
      return NULL;

    cbget *arg = static_cast<cbget*>(arg_cbget);

    CURL *curl = NULL;

    if (!arg->_handler)
      {
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS,5);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); // do not check on SSL certificate.
      }
    else curl = arg->_handler;

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, arg->_connect_timeout_sec);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, arg->_transfer_timeout_sec);
    curl_easy_setopt(curl, CURLOPT_URL, arg->_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, arg);

    if (!arg->_cookies.empty())
      curl_easy_setopt(curl, CURLOPT_COOKIE, arg->_cookies.c_str());

    if (!arg->_proxy_addr.empty())
      {
        //std::string proxy_str = seeks_proxy::_config->_haddr;
        std::string proxy_str = arg->_proxy_addr + ":" + miscutil::to_string(arg->_proxy_port);//+ miscutil::to_string(seeks_proxy::_config->_hport);
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy_str.c_str());
      }

    if (arg->_content) // POST request.
      {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (void*)arg->_content->c_str());
        if (arg->_content_size >= 0)
          curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, arg->_content_size);
        /*std::cerr << "curl_mget POST size: " << arg->_content_size << std::endl
          << "content: " << *arg->_content << std::endl;*/
      }

    struct curl_slist *slist=NULL;

    // useful headers.
    if (arg->_headers)
      {
        std::list<const char*>::const_iterator sit = arg->_headers->begin();
        while (sit!=arg->_headers->end())
          {
            slist = curl_slist_append(slist,(*sit));
            ++sit;
          }
        if (arg->_content)
          {
            slist = curl_slist_append(slist,strdup(arg->_content_type.c_str()));
            slist = curl_slist_append(slist,strdup("Expect:"));
          }
      }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

    char errorbuffer[CURL_ERROR_SIZE];
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &errorbuffer);

    try
      {
        int status = curl_easy_perform(curl);
        if (status != 0)  // an error occurred.
          {
            errlog::log_error(LOG_LEVEL_ERROR, "curl error: %s", errorbuffer);

            if (arg->_output)
              {
                delete arg->_output;
                arg->_output = NULL;
              }
          }
      }
    catch (std::exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR, "Error %s in fetching remote data with curl.", e.what());
        if (arg->_output)
          {
            delete arg->_output;
            arg->_output = NULL;
          }
      }
    catch (...)
      {
        if (arg->_output)
          {
            delete arg->_output;
            arg->_output = NULL;
          }
      }

    if (!arg->_handler)
      curl_easy_cleanup(curl);

    if (slist)
      curl_slist_free_all(slist);

    return NULL;
  }

  std::string** curl_mget::www_mget(const std::vector<std::string> &urls, const int &nrequests,
                                    const std::vector<std::list<const char*>*> *headers,
                                    const std::string &proxy_addr, const short &proxy_port,
                                    std::vector<CURL*> *chandlers,
                                    std::vector<std::string> *cookies,
                                    const bool &post,
                                    std::string *content,
                                    const int &content_size,
                                    const std::string &content_type)
  {
    assert((int)urls.size() == nrequests); // check.

    pthread_t tid[nrequests];

    /* Must initialize libcurl before any threads are started */
    curl_global_init(CURL_GLOBAL_ALL);

    for (int i=0; i<nrequests; i++)
      {
        cbget *arg_cbget = new cbget();
        arg_cbget->_url = urls[i].c_str();
        arg_cbget->_transfer_timeout_sec = _transfer_timeout_sec;
        arg_cbget->_connect_timeout_sec = _connect_timeout_sec;
        arg_cbget->_proxy_addr = proxy_addr;
        arg_cbget->_proxy_port = proxy_port;
        if (headers)
          arg_cbget->_headers = headers->at(i); // headers are url dependent.
        if (chandlers)
          arg_cbget->_handler = chandlers->at(i);
        if (cookies)
          arg_cbget->_cookies = cookies->at(i);
        if (content)
          {
            arg_cbget->_content = content;
            arg_cbget->_content_size = content_size;
          }
        _cbgets[i] = arg_cbget;

        int error = pthread_create(&tid[i],
                                   NULL, /* default attributes please */
                                   pull_one_url,
                                   (void *)arg_cbget);

        if (error != 0)
          errlog::log_error(LOG_LEVEL_ERROR,"Couldn't run thread number %g",i,", errno %g",error);
        //else std::cout << "Thread " << i << ", gets " << urls[i] << std::endl;
      }

    /* now wait for all threads to terminate */
    for (int i=0; i<nrequests; i++)
      {
        int error = pthread_join(tid[i], NULL);
        //std::cout << "Thread " << i << " terminated\n";
      }

    for (int i=0; i<nrequests; i++)
      {
        _outputs[i] = _cbgets[i]->_output;
        delete _cbgets[i];
      }

    return _outputs;
  }

} /* end of namespace. */
