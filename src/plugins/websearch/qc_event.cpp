/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "qc_event.h"
#include "errlog.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

using sp::errlog;

namespace seeks_plugins
{
   
   /* std::string qc_event::_event_file = "qcevent.fifo";
    int qc_event::_socket = -1; */
   
   int qc_event::init()
     {
	// test the event file.
	/* struct stat st;
	if (lstat(qc_event::_event_file.c_str(),&st)==0)
	  {
	     if ((st.st_mode & S_IFMT) == S_IFREG) 
	       {
		  // file exists.
		  unlink(qc_event::_event_file.c_str());
	       }
	     if (mkfifo(qc_event::_event_file.c_str(),0600) == -1)
	       {
		  errlog::log_error(LOG_LEVEL_ERROR,"Failed creating fifo file %s for query context events",
				    qc_event::_event_file.c_str());
		  return -1;
	       }
	     
	     qc_event::_socket = open(qc_event::_event_file.c_str(),O_RDWR | O_NONBLOCK,0);
	     if (qc_event::_socket == -1)
	       {
		  errlog::log_error(LOG_LEVEL_ERROR,"Failed to open fifo file %s for query context events",
				    qc_event::_event_file.c_str());
		  return -2;
	       } */
	     
	// initialize the event library.
	// XXX: beware, this conflicts with initialization in httpserv plugin.
	// and later on maybe in the main DHT code.
	event_init(); //TODO: memory leak.
	
	// start the dispatching loop in a thread.
	pthread_t event_dispatch_thread;
	int err = pthread_create(&event_dispatch_thread,NULL,
				 (void*(*)(void*))&event_dispatch,NULL);
	pthread_detach(event_dispatch_thread);
	
	return err;
	//}
     }

   qc_event_arg::qc_event_arg()
     {
     }
   
   qc_event_arg::~qc_event_arg()
     {
     }
      
} /* end of namespace. */  
