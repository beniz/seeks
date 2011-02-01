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
#include "errlog.h"

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

  sp_mutex_t sweeper::_mem_dust_mutex;

  void sweeper::register_sweepable(sweepable *spable)
  {
    mutex_lock(&sweeper::_mem_dust_mutex);
    seeks_proxy::_memory_dust.push_back(spable);
    mutex_unlock(&sweeper::_mem_dust_mutex);
  }

  void sweeper::unregister_sweepable(sweepable *spable)
  {
    mutex_lock(&sweeper::_mem_dust_mutex);
    std::vector<sweepable*>::iterator vit
    = seeks_proxy::_memory_dust.begin();
    while (vit!=seeks_proxy::_memory_dust.end())
      {
        if ((*vit) == spable)
          {
            seeks_proxy::_memory_dust.erase(vit);
            mutex_unlock(&sweeper::_mem_dust_mutex);
            return;
          }
        ++vit;
      }
    mutex_unlock(&sweeper::_mem_dust_mutex);
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

            /* miscutil::list_remove_all(&csp->_headers);
             miscutil::list_remove_all(&csp->_tags); */

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

    mutex_lock(&sweeper::_mem_dust_mutex);
    std::vector<sweepable*>::iterator vit = seeks_proxy::_memory_dust.begin();
    while (vit!=seeks_proxy::_memory_dust.end())
      {
        sweepable *spable = (*vit);
        if (spable->sweep_me())
          {
            delete spable;
            vit = seeks_proxy::_memory_dust.erase(vit);
          }
        else ++vit;
      }
    mutex_unlock(&sweeper::_mem_dust_mutex);

#if defined(PROTOBUF) && defined(TC)
    // user_db sweep.
    seeks_proxy::_user_db->sweep_db();
#endif

    return active_threads;
  }

  unsigned int sweeper::sweep_all_memory_dust()
  {
    unsigned int nm = seeks_proxy::_memory_dust.size();
    std::vector<sweepable*>::iterator vit = seeks_proxy::_memory_dust.begin();
    while (vit!=seeks_proxy::_memory_dust.end())
      {
        sweepable *spable = (*vit);
        delete spable;
        vit = seeks_proxy::_memory_dust.erase(vit);
      }
    errlog::log_error(LOG_LEVEL_INFO, "sweep_all: destroyed %u elements",nm);
    return nm;
  }

  unsigned int sweeper::sweep_all_csps()
  {
    client_state *csp, *last_active;
    unsigned int active_threads = 0;

    last_active = &seeks_proxy::_clients;
    csp = seeks_proxy::_clients._next;

    while (NULL != csp)
      {
        active_threads++;

        /*
         * This client is not active. Free its resources.
         */
        //TODO: should go into client_state destructor.
        {
          last_active->_next = csp->_next;
          freez(csp->_ip_addr_str);
          freez(csp->_iob._buf);

          if (csp->_action._flags & ACTION_FORWARD_OVERRIDE &&
              NULL != csp->_fwd)
            {
              delete csp->_fwd;
            }
          delete csp;
          csp = last_active->_next;
        }
      }
    return active_threads;
  }

  unsigned int sweeper::sweep_all()
  {
    return sweeper::sweep_all_csps() + sweeper::sweep_all_memory_dust();
  }

} /* end of namespace. */
