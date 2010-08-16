/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include "sweeper.h"
#include "mem_utils.h"
#include "seeks_proxy.h"
#include "miscutil.h"

#include <iostream>

namespace sp
{
   /*- sweepable. -*/
   sweepable::sweepable()
     {
     }
   
   sweepable::~sweepable()
     {
     }
      
   /*- sweeper. -*/

   void sweeper::register_sweepable(sweepable *spable)
     {
	seeks_proxy::_memory_dust.push_back(spable);
     }
   
   void sweeper::unregister_sweepable(sweepable *spable)
     {
	std::vector<sweepable*>::iterator vit
	  = seeks_proxy::_memory_dust.begin();
	while(vit!=seeks_proxy::_memory_dust.end())
	  {
	     if ((*vit) == spable)
	       {
		  seeks_proxy::_memory_dust.erase(vit);
		  return;
	       }
	     ++vit;
	  }
     }
      
   /*********************************************************************
    * Function    :  sweep
    *
    * Description :  Basically a mark and sweep garbage collector, it is run
    *                (by the parent thread) every once in a while to reclaim 
    *                memory.
    *
    * It uses a mark and sweep strategy:
    *   1) mark all files as inactive
    *
    *   2) check with each client:
    *       if it is active,   mark its files as active
    *       if it is inactive, free its resources
    *
    *   3) free the resources.
    *
    * Returns     :  The number of threads that are still active.
    *
    *********************************************************************/
   unsigned int sweeper::sweep()
     {
       client_state *csp, *last_active;
       unsigned int active_threads = 0;

       last_active = &seeks_proxy::_clients;
       csp = seeks_proxy::_clients._next;

       while (NULL != csp)
	 {
	   if (csp->_flags & CSP_FLAG_ACTIVE)
	     {
	       active_threads++;

	       last_active = csp;
	       csp = csp->_next;
	     }
	   else
	     /*
	      * This client is not active. Free its resources.
	      */
	     //TODO: should go into client_state destructor.
	     {
	       last_active->_next = csp->_next;

	       freez(csp->_ip_addr_str);
	       freez(csp->_iob._buf);
	       //freez(csp->_error_message);

	       if (csp->_action._flags & ACTION_FORWARD_OVERRIDE &&
		   NULL != csp->_fwd)
		 {
		   delete csp->_fwd;
		 }

	       miscutil::list_remove_all(&csp->_headers);
	       miscutil::list_remove_all(&csp->_tags);

#ifdef FEATURE_STATISTICS
	       seeks_proxy::_urls_read++;
	       if (csp->_flags & CSP_FLAG_REJECTED)
		 {
		   seeks_proxy::_urls_rejected++;
		 }
#endif /* def FEATURE_STATISTICS */

	       delete csp;
	       csp = last_active->_next;
	     }
	 }

	/* sweeps other memory dust grains, as necessary. */

	//debug
	/* std::cerr << "[Debug]:sweeper: cleaning memory dust: "
	 << seeks_proxy::_memory_dust.size() << " remaining items\n"; */
	//debug
	
	std::vector<std::vector<sweepable*>::iterator> dead_iterators;
	std::vector<sweepable*>::iterator vit = seeks_proxy::_memory_dust.begin();
	while(vit!=seeks_proxy::_memory_dust.end())
	  {
	     sweepable *spable = (*vit);
	     if (spable->sweep_me())
	       {
		  delete spable;
		  vit = seeks_proxy::_memory_dust.erase(vit);
	       }
	     else ++vit;
	  }
	
	//debug
	//std::cerr << "[Debug]:sweeper: removed " << dead_iterators.size() << " items\n";
	//debug
	
       return active_threads;
     }
   
} /* end of namespace. */
