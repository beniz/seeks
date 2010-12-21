/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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

#include "SuccList.h"
#include "DHTVirtualNode.h"
#include "FingerTable.h"
#include "Transport.h"
#include "errlog.h"

#include <iostream>

using sp::errlog;

//#define DEBUG

namespace dht
{
  short SuccList::_max_list_size = 1; // default is successor only, reset by DHTNode constructor.

  SuccList::SuccList(DHTVirtualNode *vnode)
    : Stabilizable(), _vnode(vnode), _stable(false)
  {
    mutex_init(&_stable_mutex);
    push_back(DHTKey());
  }

  SuccList::~SuccList()
  {
  }

  void SuccList::update_successors()
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]:SuccList::update_successors()\n";
    //debug
#endif

    /**
     * RPC call to get successor list.
     */
    int status = 0;
    DHTKey dk_succ = _vnode->getSuccessorS();
    if (!dk_succ.count())
      {
        // TODO: errlog.
        // XXX: should never reach here...
        std::cerr << "[Error]:SuccList::update_successors: this virtual node has no successor:"
                  << dk_succ << ".Exiting\n";
        exit(-1);
      }

    Location *loc_succ = _vnode->findLocation(dk_succ);

    std::list<DHTKey> dkres_list;
    std::list<NetAddress> na_list;
    _vnode->RPC_getSuccList(dk_succ, loc_succ->getNetAddress(),
                            dkres_list, na_list, status);

    /**
     * XXX: we could handle failure, retry, and move to next successor in the list.
     * For now, this is done in stabilize, in FingerTable.
     */
    if (status != DHT_ERR_OK)
      {
#ifdef DEBUG
        //debug
        std::cerr << "getSuccList failed: " << status << "\n";
        //debug
#endif

        errlog::log_error(LOG_LEVEL_DHT, "getSuccList failed");
        return;
      }
    else
      {
#if 0
        Location *uloc = _vnode->findLocation(dk_succ);
        if (uloc)
          uloc->update_check_time();
#endif
      }

    // merge succlist.
    merge_succ_list(dkres_list,na_list);
  }

  void SuccList::merge_succ_list(std::list<DHTKey> &dkres_list, std::list<NetAddress> &na_list)
  {
    std::list<Location> l;
    std::list<DHTKey>::const_iterator ki;
    std::list<NetAddress>::const_iterator ni;
    for(ki = dkres_list.begin(), ni = na_list.begin();
        ki != dkres_list.end() && ni != na_list.end();
        ki++, ni++)
      {
        l.push_back(Location(*ki,*ni));
      }
    merge_list(l);
    check();
  }

  void SuccList::merge_list(std::list<Location>& other)
  {
    other.sort();
    sort();
    merge(other);
    unique();
    if(size() > 1)
      remove(_vnode->getIdKey());
    if(back() > _vnode->getIdKey())
      {
        for(iterator it = begin(); it != end();)
          {
            if(*it > _vnode->getIdKey())
              break;
            else
              {
                push_back(*it);
                it = erase(it);
              }
          }
      }
  }

  void SuccList::check()
  {
    mutex_lock(&_stable_mutex);
    _stable = false;
    mutex_unlock(&_stable_mutex);

    int alive = 0;
    iterator it;
    for(it = begin(); it != end() && alive < SuccList::_max_list_size;)
      {
        dht_err status;
        _vnode->is_dead(*it, it->getNetAddress(), status);
        if(status != DHT_ERR_OK)
          {
            it = erase(it);
          }
        else
          {
            alive++;
            it++;
          }
      }
    // limit the size of the list to max_list_size
    erase(it, end());

    if(size() == 0)
      {
        push_back(DHTKey());
        throw dht_exception(DHT_ERR_RETRY, "exhausted successor list while updating it");
      }

    mutex_lock(&_stable_mutex);
    _stable = size() > SuccList::_max_list_size / 2;
    mutex_unlock(&_stable_mutex);
  }

  bool SuccList::has_key(const DHTKey &key) const
  {
    std::list<Location>::const_iterator sit = begin();
    while(sit != end())
      {
        if (*sit == key)
          return true;
        ++sit;
      }
    return false;
  }

  void SuccList::removeKey(const DHTKey &key)
  {
    std::list<Location>::iterator sit = begin();
    while(sit != end())
      {
        if (*sit == key)
          sit = erase(sit);
        else ++sit;
      }
  }

  void SuccList::findClosestPredecessor(const DHTKey &nodeKey,
                                        DHTKey &dkres, NetAddress &na,
                                        DHTKey &dkres_succ, NetAddress &dkres_succ_na,
                                        int &status)
  {
    status =  DHT_ERR_OK;
    std::list<Location>::const_iterator sit = end();
    std::list<Location>::const_iterator sit2 = sit;
    while(sit != begin())
      {
        --sit;
        const Location& loc = *sit;

        if (loc.between(_vnode->getIdKey(),nodeKey))
          {
            dkres = loc;
            na = loc.getNetAddress();
            if (sit2 != end())
              {
                const Location& loc2 = *sit2;
                dkres_succ = loc2;
                dkres_succ_na = loc2.getNetAddress();
              }
            return;
          }
        sit2 = sit;
      }
  }

} /* end of namespace. */
