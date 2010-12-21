/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
 * Copyright (C) 2010  Loic Dachary <loic@dachary.org>
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

#include "Stabilizer.h"
#include "Random.h"
#include "DHTVirtualNode.h"
#include "dht_configuration.h"
#include "dht_exception.h"
#include <math.h>
#include <iostream>
#include <time.h>
#include <assert.h>

//#define DEBUG

using lsh::Random;

namespace dht
{
  Stabilizable::Stabilizable()
    :_stabilizing_fast(0),_stabilizing_slow(0)
  {
  }

  void Stabilizable::stabilize_fast_ct()
  {
    _stabilizing_fast++;
    stabilize_fast();
    _stabilizing_fast--;
  }

  void Stabilizable::stabilize_slow_ct()
  {
    _stabilizing_slow++;
    stabilize_slow();
    _stabilizing_slow--;
  }

  /**
   * For now we're using the same parameters and values as in Chord.
   * We'll change these as needed.
   */
  const int Stabilizer::_decrease_timer = 1000;
  const double Stabilizer::_slowdown_factor = 1.2;

  const int Stabilizer::_fast_timer_init = 1 * 1000;
  const int Stabilizer::_fast_timer_base = 20 * 1000;
  const int Stabilizer::_fast_timer_max = 2 * Stabilizer::_fast_timer_base;

  const int Stabilizer::_slow_timer_init = 1 * 1000;
  const int Stabilizer::_slow_timer_base = 8 * 1000;
  const int Stabilizer::_slow_timer_max = 2 * Stabilizer::_slow_timer_base;

  Stabilizer::Stabilizer(const bool &start)
    : BstTimeCbTree(),_slow_clicks(0),_fast_clicks(0)
  {
    if (start)
      {
        start_fast_stabilizer();
        start_slow_stabilizer();
      }
  }

  Stabilizer::~Stabilizer()
  {
    stop_threaded_timecheck_loop();
  }

  void Stabilizer::start_fast_stabilizer()
  {
    fast_stabilize(static_cast<double>(_fast_timer_init));
  }

  void Stabilizer::start_slow_stabilizer()
  {
    slow_stabilize(static_cast<double>(_slow_timer_init));
  }

  int Stabilizer::fast_stabilize(double tround)
  {
    _fast_clicks++;

    /**
     * If we've on-going rpcs wrt. stabilization,
     * we delay the next round by slowing down the calls.
     */
    if (fast_stabilizing())
      {
        tround *= 2.0;
      }
    else
      {
        /**
         * decide if we decrease or increase the time between the calls.
         */
        if (tround >= Stabilizer::_fast_timer_base) tround -= Stabilizer::_decrease_timer;
        if (tround < Stabilizer::_fast_timer_base) tround *= Stabilizer::_slowdown_factor;

        /**
         * run fast stabilization on individual structures.
         */
        for (unsigned int i=0; i<_stab_elts_fast.size(); i++)
          {
            try
              {
                _stab_elts_fast[i]->stabilize_fast();
              }
            catch (dht_exception &e)
              {

              }
          }
      }

    if (tround > Stabilizer::_fast_timer_max)
      tround = static_cast<double>(Stabilizer::_fast_timer_max);

    /**
     * generate a time for the next stabilization round.
     */
    //no need reseed random generator ?
    double t1 = Random::genUniformDbl32(0.5 * tround, 1.5 * tround);
    int sec = static_cast<int>(ceil(t1));
    int sec2 = static_cast<int>(ceil(sec / 1000));
    int nsec = (sec % 1000) * 1000000;

    //debug
    /* std::cout << "[Debug]:Stabilizer::fast_stabilize: sec: " << sec << " -- nsec: " << nsec
     << " -- t1: " << t1 << std::endl; */
    //debug

    /**
     * Schedule a later callback of this same function.
     */
    timespec tsp, tsnow;
    clock_gettime(CLOCK_REALTIME, &tsnow);
    tsp.tv_sec = tsnow.tv_sec + sec2;
    tsp.tv_nsec = tsnow.tv_nsec + nsec;
    if (tsp.tv_nsec >= 1000000000)
      {
        tsp.tv_nsec -= 1000000000;
        tsp.tv_sec++;
      }

    //debug
    /* if (_bstcbtree)
     std::cout << "tree size before insertion: " << _bstcbtree->size() << std::endl; */
    //debug

    callback<int>::ref fscb= wrap(this, &Stabilizer::fast_stabilize, tround);
    insert(tsp, fscb);

    //debug
    /* if (_bstcbtree)
     std::cout << "tree size after insertion: " << _bstcbtree->size() << std::endl; */
    //debug

#ifdef DEBUG
    //debug
    std::cerr << "[Debug]:Stabilizer::fast_stabilize: tround: " << tround << std::endl;
    std::cerr << "[Debug]:Stabilizer::fast_stabilize: scheduling next call around: "
              << tsp << std::endl;
    //std::cout << tround << std::endl;
    //debug
#endif

    return 0;
  }

  int Stabilizer::slow_stabilize(double tround)
  {
    _slow_clicks++;

    bool stable = isstable_slow();

    /**
     * if we have on-going rpcs wrt. stabilization,
     * we delay the next round by slowing down the calls.
     */
    if (slow_stabilizing())
      {
        tround *= 2.0;
      }
    else
      {
        /**
         * decide over the increase/decrease of the next call.
         */
        if (stable && tround <= Stabilizer::_slow_timer_max)
          tround *= Stabilizer::_slowdown_factor;
        else if (tround > Stabilizer::_slow_timer_base)
          tround -= Stabilizer::_decrease_timer;

        /**
         * run slow stabilization on individual structures.
         */
        for (unsigned int i=0; i<_stab_elts_slow.size(); i++)
          {
            try
              {
                _stab_elts_slow[i]->stabilize_slow();
              }
            catch (dht_exception &e)
              {

              }
          }
      }

    if (tround > Stabilizer::_slow_timer_max)
      tround = static_cast<double>(Stabilizer::_slow_timer_max);

    /**
     * generate a time for the next call.
     */
    //no need to reseed random generator.
    double t1 = Random::genUniformDbl32(0.5 * tround, 1.5 * tround);
    int sec = static_cast<int>(ceil(t1));
    int sec2 = static_cast<int>(ceil(sec / 1000));
    int nsec = (sec % 1000) * 1000000;

    /**
     * schedule a later callback of this same function.
     */
    timespec tsp, tsnow;
    clock_gettime(CLOCK_REALTIME, &tsnow);
    tsp.tv_sec = tsnow.tv_sec + sec2;
    tsp.tv_nsec = tsnow.tv_nsec + nsec;
    if (tsp.tv_nsec >= 1000000000)
      {
        tsp.tv_nsec -= 1000000000;
        tsp.tv_sec++;
      }

    //debug
    /* if (_bstcbtree)
     std::cout << "tree size before insertion (slow): " << _bstcbtree->size() << std::endl; */
    //debug

    callback<int>::ref fscb= wrap(this, &Stabilizer::slow_stabilize, tround);
    insert(tsp, fscb);

    //debug
    /* if (_bstcbtree)
     std::cout << "tree size after insertion (slow): " << _bstcbtree->size() << std::endl; */
    //debug

#ifdef DEBUG
    //debug
    std::cerr << "[Debug]:Stabilizer::slow_stabilize: tround: " << tround << std::endl;
    std::cerr << "[Debug]:Stabilizer::slow_stabilize: scheduling next call around: "
              << tsp << std::endl;
    //std::cout << tround << std::endl;
    //debug
#endif

    return 0;
  }

  bool Stabilizer::fast_stabilizing() const
  {
    for (unsigned int i=0; i<_stab_elts_fast.size(); i++)
      if (_stab_elts_fast[i]->isStabilizingFast())
        return true;
    return false;
  }

  bool Stabilizer::slow_stabilizing() const
  {
    for (unsigned int i=0; i<_stab_elts_slow.size(); i++)
      if (_stab_elts_slow[i]->isStabilizingSlow())
        return true;
    return false;
  }

  bool Stabilizer::isstable_slow() const
  {
    for (unsigned int i=0; i<_stab_elts_slow.size(); i++)
      if (!_stab_elts_slow[i]->isStable())
        return false;
    return true;
  }

  int Stabilizer::rejoin(DHTVirtualNode *vnode)
  {
    // grab a bootstrap node list.
    std::vector<NetAddress> bootstrap_nodelist
    = dht_configuration::_dht_config->_dht_config->_bootstrap_nodelist;

    // try to bootstrap.
    bool success = false;
    std::vector<NetAddress>::const_iterator vit = bootstrap_nodelist.begin();
    while(vit!=bootstrap_nodelist.end())
      {
        NetAddress na_boot = (*vit);

        // try to bootstrap from every node in the list.
        dht_err status = DHT_ERR_OK;
        DHTKey dk_bootstrap; // empty key is handled by the called node when asking for a bootstrap.
        vnode->join(dk_bootstrap,na_boot,
                    vnode->getIdKey(),status);

        // check on error and reschedule a rejoin call if needed.
        if (status != DHT_ERR_OK)
          {
            // failure, try next bootstrap node.
            ++vit;
            continue;
          }

        success = true;
        break;
      }

    // if unsuccessful, schedule another rejoin call.
    if (!success)
      {
        timespec tsp, tsnow;
        clock_gettime(CLOCK_REALTIME,&tsnow);
        tsp.tv_sec = tsnow.tv_sec + dht_configuration::_dht_config->_rejoin_timeout; // XXX: we leave nanoseconds to 0.
        callback<int>::ref fscb= wrap(this, &Stabilizer::rejoin, vnode);
        insert(tsp, fscb);
        return 0;
      }

    return 1;
  }

} /* end of namespace. */
