/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009-2011 Emmanuel Benazera <ebenazer@seeks-project.info>
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
    char* encoded = NULL;
    char* decoded  = NULL;

    if (!arg->_handler)
      {
        errlog::log_error(LOG_LEVEL_DEBUG, "pull_one_url(): Setting default (curl_easy_init()) CURL handler for %s", arg->_url);
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
        std::string proxy_str = arg->_proxy_addr + ":" + miscutil::to_string(arg->_proxy_port);
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy_str.c_str());
      }

    if (arg->_http_method == "POST") // POST request.
      {
        if (arg->_content)
          {
            errlog::log_error(LOG_LEVEL_DEBUG, "pull_one_url(): Enabling POST request ...");
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            encoded = curl_easy_escape(curl, arg->_content->c_str(), 0);
            if (!encoded)
              {
                errlog::log_error(LOG_LEVEL_ERROR, "pull_one_url(): @TODO curl_easy_escape() failed");
              }
            errlog::log_error(LOG_LEVEL_DEBUG, "pull_one_url(): Before: %s(%d)", arg->_content->c_str(), arg->_content->length());
            errlog::log_error(LOG_LEVEL_DEBUG, "pull_one_url(): After: %s(%d)", encoded, strlen(encoded));
            // Fix encoded length to URL-encoded
            arg->_content_size = strlen(encoded);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, encoded);
            // OLD: curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (void*)arg->_content->c_str());

            if (arg->_content_size >= 0)
              {
                errlog::log_error(LOG_LEVEL_DEBUG, "pull_one_url(): Setting CURLOPT_POSTFIELDSIZE=%d", arg->_content_size);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, arg->_content_size);
              }
            /*std::cerr << "curl_mget POST size: " << arg->_content_size << std::endl
              << "content: " << *arg->_content << std::endl;*/
          }
        else curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
      }
    else if (arg->_http_method == "PUT" || arg->_http_method == "DELETE")
      {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, arg->_http_method.c_str());
      }

    struct curl_slist *slist=NULL;

    // useful headers.
    errlog::log_error(LOG_LEVEL_DEBUG, "pull_one_url(): Checking arg->_headers=%s ...", arg->_headers);
    if (arg->_headers)
      {
        std::list<const char*>::const_iterator sit = arg->_headers->begin();
        while (sit!=arg->_headers->end())
          {
            slist = curl_slist_append(slist,(*sit));
            ++sit;
          }
        errlog::log_error(LOG_LEVEL_DEBUG, "pull_one_url(): Checking arg->_content=%s", arg->_content);
        if (arg->_content)
          {
            errlog::log_error(LOG_LEVEL_DEBUG, "pull_one_url(): Adding content-type=%s and Expect: header", arg->_content_type.c_str());
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
            arg->_status = status;
            errlog::log_error(LOG_LEVEL_ERROR, "curl error on url %s: %s",arg->_url,errorbuffer);

            if (arg->_output)
              {
                delete arg->_output;
                arg->_output = NULL;
              }
          }
        else
          {
            // "Transparent" URL decoding
            errlog::log_error(LOG_LEVEL_DEBUG, "pull_one_url(): Before: %s(%d)", arg->_output->c_str(), arg->_output->length());
            decoded = curl_easy_unescape(curl, arg->_output->c_str(), 0, NULL);
            if (!decoded)
              {
                errlog::log_error(LOG_LEVEL_ERROR, "pull_one_url(): URL decoding failed.");
                delete arg->_output;
                arg->_status = 500;
                arg->_output = NULL;
              }
            errlog::log_error(LOG_LEVEL_DEBUG, "pull_one_url(): After: %s(%d)", decoded, strlen(decoded));
            arg->_output = new std::string(decoded);
          }
      }
    catch (std::exception &e)
      {
        arg->_status = -1;
        errlog::log_error(LOG_LEVEL_ERROR, "Error %s in fetching remote data with curl.", e.what());
        if (arg->_output)
          {
            delete arg->_output;
            arg->_output = NULL;
          }
      }
    catch (...)
      {
        arg->_status = -2;
        if (arg->_output)
          {
            delete arg->_output;
            arg->_output = NULL;
          }
      }

            // Free memory
            curl_free(decoded);

            // Free memory
            curl_free(encoded);

    if (!arg->_handler)
      curl_easy_cleanup(curl);

    if (slist)
      curl_slist_free_all(slist);

    return NULL;
  }

  std::string** curl_mget::www_mget(const std::vector<std::string> &urls, const int &nrequests,
                                    const std::vector<std::list<const char*>*> *headers,
                                    const std::string &proxy_addr, const short &proxy_port,
                                    std::vector<int> &status,
                                    std::vector<CURL*> *chandlers,
                                    std::vector<std::string> *cookies,
                                    const std::string &http_method,
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
        arg_cbget->_http_method = http_method;
        if (content)
          {
            errlog::log_error(LOG_LEVEL_DEBUG, "curl_mget::www_mget(): Setting content of length %d bytes", content_size);
            arg_cbget->_content = content;
            arg_cbget->_content_size = content_size;
            if (content_type.length() > 0)
              {
                errlog::log_error(LOG_LEVEL_DEBUG, "curl_mget::www_mget(): Setting content type %s", content_type.c_str());
                arg_cbget->_content_type = content_type;
              }
          }
        _cbgets[i] = arg_cbget;

        int error = pthread_create(&tid[i],
                                   NULL, /* default attributes please */
                                   pull_one_url,
                                   (void *)arg_cbget);

        if (error != 0)
          errlog::log_error(LOG_LEVEL_ERROR,"Couldn't run thread number %g",i,", errno %g",error);
      }

    /* now wait for all threads to terminate */
    for (int i=0; i<nrequests; i++)
      {
        int error = pthread_join(tid[i], NULL);

        if (error != 0)
          errlog::log_error(LOG_LEVEL_ERROR,"Couldn't join thread number %g",i,", errno %g",error);
      }

    for (int i=0; i<nrequests; i++)
      {
        _outputs[i] = _cbgets[i]->_output;
        errlog::log_error(LOG_LEVEL_DEBUG, "curl_mget::www_mget(): i=%d,_output=%s", i, _outputs[i]->c_str());
        status.push_back(_cbgets[i]->_status);
        delete _cbgets[i];
      }

    return _outputs;
  }

  std::string* curl_mget::www_simple(const std::string &url,
                                     std::list<const char*> *headers,
                                     int &status,
                                     const std::string &http_method,
                                     std::string *content,
                                     const int &content_size,
                                     const std::string &content_type,
                                     const std::string &proxy_addr,
                                     const short &proxy_port)
  {
    std::vector<std::string> urls;
    urls.reserve(1);
    urls.push_back(url);
    std::vector<int> statuses;
    std::vector<std::list<const char*>*> *lheaders = NULL;
    if (headers)
      {
        lheaders = new std::vector<std::list<const char*>*>();
        lheaders->push_back(headers);
      }
    www_mget(urls,1,lheaders,proxy_addr,proxy_port,statuses,NULL,NULL,http_method);
    if (statuses[0] != 0)
      {
        // failed connection.
        status = statuses[0];
        delete[] _outputs;
        if (headers)
          delete lheaders;
        std::string msg = "failed connection to " + url;
        errlog::log_error(LOG_LEVEL_ERROR,msg.c_str());
        return NULL;
      }
    else if (statuses[0] == 0 && !_outputs)
      {
        // no result.
        status = statuses[0];
        std::string msg = "no output from " + url;
        if (headers)
          delete lheaders;
        errlog::log_error(LOG_LEVEL_ERROR,msg.c_str());
        delete _outputs[0];
        delete[] _outputs;
        return NULL;
      }
    status = statuses[0];
    std::string *result = _outputs[0];
    delete[] _outputs;
    if (headers)
      delete lheaders;
    return result;
  }


} /* end of namespace. */
