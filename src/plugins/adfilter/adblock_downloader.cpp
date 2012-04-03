/**                                                                                                                                                
* This file is part of the SEEKS project.                                                                             
* Copyright (C) 2011 Fabien Dupont <fab+seeks@kafe-in.net>
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

#include "adblock_downloader.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"
#include "errlog.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <errno.h>
#include <sys/stat.h>
#include <curl/curl.h>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

using namespace sp;
using sp::proxy_configuration;
using sp::seeks_proxy;

/*
 * Constructor
 * ----------------
 */
adblock_downloader::adblock_downloader(adfilter* parent, std::string filename)
{
  struct sigevent sev;
  struct itimerspec its;

  // ADBlock rules filename
  this->_listfilename = (seeks_proxy::_datadir.empty() ?
                        plugin_manager::_plugin_repository + filename :
                        seeks_proxy::_datadir + "/plugins/" + filename);

  // Timer creation (SIG is SIGRTMIN)
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIG;
  sev.sigev_value.sival_ptr = this;
  timer_create(CLOCKID, &sev, &(this->_tid));

  // Timer frequency definition
  its.it_value.tv_sec = 1; // Tick every 60 seconds
  its.it_value.tv_nsec = 0; // and 0 milliseconds
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;
  timer_settime(this->_tid, 0, &its, NULL);

  sigset_t mask;
  // SIG set initialization
  sigemptyset(&mask);
  // Add SIG signal to the set
  sigaddset(&mask, SIG);
  // Add signal to the mask and unblock it
  sigprocmask(SIG_SETMASK, &mask, NULL);
  sigprocmask(SIG_UNBLOCK, &mask, NULL);

  this->_timer_running = false;
  this->parent = parent;
}

adblock_downloader::~adblock_downloader()
{
  this->stop_timer();
  timer_delete(this->_tid);
  this->_tid = NULL;
}

// FIXME Timer comment
void adblock_downloader::start_timer()
{
  struct sigaction sa;

  // Signal handler on timer expiration
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = adblock_downloader::tick;
  sigemptyset(&sa.sa_mask);
  sigaction(SIG, &sa, NULL);

  this->_timer_running = true;
}

void adblock_downloader::stop_timer()
{
  // Just remove the signal handler
  signal(SIG, SIG_IGN);

  this->_timer_running = false;
}

void adblock_downloader::tick(int sig, siginfo_t *si, void *uc)
{
  struct stat attrib;

  // Pointer to this instanciated class cast
  adblock_downloader* c = reinterpret_cast<adblock_downloader*>(si->si_value.sival_ptr);

  // Check list file attributes
  stat(c->_listfilename.c_str(), &attrib);

  // If there is more than _update_frequency secondes between now and the modification date of the adblock rules file
  // or if this file is simply missing
  // this it's time to refresh this file
  if(time(NULL) - attrib.st_mtime >= c->parent->get_config()->_update_frequency or errno == ENOENT)
  {
    int nbdled;
    nbdled = c->download_lists();
    errlog::log_error(LOG_LEVEL_INFO, "adfilter: %d adblock files downloaded successfully", nbdled);
  }
}

// This is the writer call back function used by curl
// TODO comment
int adblock_downloader::_curl_writecb(char *data, size_t size, size_t nmemb, std::string *buffer)
{
  int result = 0;

  // Is there anything in the buffer?
  if (buffer != NULL)
  {
    // Append the data to the buffer
    buffer->append(data, (size_t)(size * nmemb));

    // How much did we write?
    result = size * nmemb;
  }

  return result;
}

int adblock_downloader::download_lists()
{
  std::vector<std::string> adblists = this->parent->get_config()->_adblock_lists;
  std::vector<std::string>::iterator it;
  int nb = 0;
  CURL *curl;
  CURLcode result;

  curl = curl_easy_init();

  if(!curl)
  {
    return -1;
  }

  // Stop the timer during the lists download
  this->stop_timer();

  // Open ouput file
  std::ofstream outfile;
  outfile.open(this->_listfilename.c_str(), std::ios_base::trunc);

  // fetch lists
  for(it = adblists.begin(); it != adblists.end(); it++)
  {
    adblock_downloader::_curl_buffer = "";

    // TODO add connexion timeouts option
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, adblock_downloader::_curl_errorBuffer);
    curl_easy_setopt(curl, CURLOPT_URL, (*it).c_str());
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, adblock_downloader::_curl_writecb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &(adblock_downloader::_curl_buffer));

    result = curl_easy_perform(curl);
  
    if(result == CURLE_OK)
    {
      // Save content into a temp file
        outfile.write(adblock_downloader::_curl_buffer.c_str(), adblock_downloader::_curl_buffer.length());
        errlog::log_error(LOG_LEVEL_INFO, "adfilter: %s downloaded", (*it).c_str());
        nb++;
    }
    else
    {
        errlog::log_error(LOG_LEVEL_ERROR, "adfilter: error downloading %s.", (*it).c_str());
    }
  }

  // Memory cleanup
  outfile.close();
  curl_easy_cleanup(curl);

  // Everything is fine, we can start the timer again
  this->start_timer();

  // Parse newly downloaded rules
  errlog::log_error(LOG_LEVEL_INFO, "adfilter: %d rules parsed successfully",
    this->parent->get_parser()->parse_file(this->parent->get_config()->_use_filter, this->parent->get_config()->_use_blocker));

  return nb;
}
