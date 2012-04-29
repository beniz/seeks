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

#ifndef ADBLOCK_DOWNLOADER_H
#define ADBLOCK_DOWNLOADER_H

#include "seeks_proxy.h"

#include <signal.h>
#include <time.h>
#include <string>

namespace seeks_plugins
{
  class adfilter;

  class adblock_downloader
  {
    public:
      adblock_downloader(adfilter *parent, std::string filename);
      ~adblock_downloader();
      void start_timer();                                 // Start the timer loop
      void stop_timer();                                  // Stop the timer loop
    private:
      bool _timer_running;                                // True if the timer runs
      static void tick(int sig, siginfo_t *si, void *uc); // Static function for timer ticks
      timer_t _tid;                                       // Current timer ID
      int download_lists();                               // Download all lists
      std::string _listfilename;                          // Local lists file path
      adfilter* _parent;                                  // Parent (sp::plugin)
  };
} // end of namespace.

#endif
