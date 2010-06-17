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

#ifndef QC_EVENT_H
#define QC_EVENT_H

#include <sys/time.h>
#include <string>
#include <event.h>

namespace seeks_plugins
{
   
   class qc_event_arg
     {
      public:
	qc_event_arg();
	~qc_event_arg();
	
	struct event _ev;
     };
      
   typedef void (*cb_func_ptr)(int,short,void*);
   
   class qc_event
     {
      public:
	static int init();
     
	//TODO: clean exit.
	
	static int add_callback_timer(cb_func_ptr cb, qc_event_arg *qcarg,
				      const struct timeval &tv)
	  {
	     struct event ev;
	     //event_set(&ev,qc_event::_socket,EV_READ,cb,qcarg);
	     qcarg->_ev = ev;
	     evtimer_set(&qcarg->_ev,cb,qcarg);
	     evtimer_add(&qcarg->_ev,&tv);
	     return 0;
	  }

	//TODO: scheme to remove callback timer (e.g. abort...) (requires the event struct).
	
	/* callbacks. */
	//static void subscribe_sg(int fd, short event, void *arg);
		
	/* static std::string _event_file;
	static int _socket; */
     };
      
} /* end of namespace. */

#endif
  
