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
#include "adfilter.h"
#include "curl_mget.h"
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

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

using namespace sp;
using sp::proxy_configuration;
using sp::seeks_proxy;

namespace seeks_plugins
{

  /*
   * Constructor
   * ----------------
   */
  adblock_downloader::adblock_downloader(adfilter* parent, std::string filename)
  {
    struct sigevent sev;
    struct itimerspec its;

    // parent is adfilter plugin.
    _parent = parent;

    // ADBlock rules filename
    _listfilename = (seeks_proxy::_datadir.empty() ?
                     plugin_manager::_plugin_repository + filename :
                     seeks_proxy::_datadir + "/plugins/" + filename);

    // Timer creation (SIG is SIGRTMIN)
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = this;
    timer_create(CLOCKID, &sev, &(_tid));

    // Timer frequency definition
    its.it_value.tv_sec = _parent->get_config()->_update_frequency; // Tick every x seconds as specified in config.
    its.it_value.tv_nsec = 0; // and 0 milliseconds
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    timer_settime(_tid, 0, &its, NULL);

    sigset_t mask;
    // SIG set initialization
    sigemptyset(&mask);
    // Add SIG signal to the set
    sigaddset(&mask, SIG);
    // Add signal to the mask and unblock it
    sigprocmask(SIG_SETMASK, &mask, NULL);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    _timer_running = false;

    // check on lists at startup.
    download_lists_if_needed();
  }

  adblock_downloader::~adblock_downloader()
  {
    stop_timer();
    timer_delete(_tid);
    _tid = NULL;
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

    _timer_running = true;
  }

  void adblock_downloader::stop_timer()
  {
    // Just remove the signal handler
    signal(SIG, SIG_IGN);

    _timer_running = false;
  }

  bool adblock_downloader::list_file_needs_update(adblock_downloader *adbdownl)
  {
    struct stat attrib;

    // Check list file attributes
    stat(adbdownl->_listfilename.c_str(), &attrib);

    // If there is more than _update_frequency seconds between now and the modification date of the adblock rules file
    // or if this file is simply missing
    // this is time to refresh this file
    if((time(NULL) - attrib.st_mtime >= adbdownl->_parent->get_config()->_update_frequency)
        || errno == ENOENT)
      return true;
    else return false;
  }

// uc param is for being passed to sigaction, required.
  void adblock_downloader::tick(int sig, siginfo_t *si, void *uc)
  {
    // Pointer to this instanciated class cast
    adblock_downloader* c = reinterpret_cast<adblock_downloader*>(si->si_value.sival_ptr);

    // check whether file has changed.
    c->download_lists_if_needed();
  }

  int adblock_downloader::download_lists_if_needed()
  {
    int nbdled = 0;

    // check whether file has changed.
    if (list_file_needs_update(this))
      {
        nbdled = download_lists();
        errlog::log_error(LOG_LEVEL_INFO, "adfilter: %d adblock files downloaded successfully", nbdled);
      }
    return nbdled;
  }

  int adblock_downloader::download_lists()
  {
    std::vector<std::string> adblists = _parent->get_config()->_adblock_lists;
    std::vector<std::string>::iterator it;
    int nb = 0;

    // Stop the timer during the lists download
    stop_timer();

    // Open ouput file
    std::ofstream outfile;
    outfile.open(_listfilename.c_str(), std::ios_base::trunc);

    // fetch lists.
    std::vector<int> status;
    curl_mget cmg(adblists.size(),seeks_proxy::_config->_ct_connect_timeout,0,
                  seeks_proxy::_config->_ct_transfer_timeout,0);
    std::string **output = cmg.www_mget(adblists,adblists.size(),NULL,"",-1,status);
    for (size_t i=0; i<adblists.size(); i++)
      {
        if (status.at(i)==0)
          {
            outfile.write(output[i]->c_str(),output[i]->length());
            errlog::log_error(LOG_LEVEL_INFO, "adfilter: %s downloaded", adblists.at(i).c_str());
            nb++;
          }
        else errlog::log_error(LOG_LEVEL_ERROR, "adfilter: error downloading %s.", adblists.at(i).c_str());
        delete output[i];
      }
    delete[] output;

    // close  file.
    outfile.close();

    // Everything is fine, we can start the timer again
    start_timer();

    // Parse newly downloaded rules
    errlog::log_error(LOG_LEVEL_INFO, "adfilter: %d rules parsed successfully",
                      _parent->get_parser()->parse_file(_parent->get_config()->_use_filter, _parent->get_config()->_use_blocker));

    return nb;
  }

} // end of namespace.
