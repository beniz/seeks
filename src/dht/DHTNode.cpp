/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010 Emmanuel Benazera, juban@free.fr
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

#include "DHTNode.h"
#include "Transport.h"
#include "FingerTable.h"
#include "net_utils.h"
#include "errlog.h"
#include "proxy_configuration.h"
#include "l1_protob_rpc_server.h"
#include "l1_protob_rpc_client.h"
#include "l1_data_protob_wrapper.h"

#include <sys/time.h>
#include <algorithm>
#include <fstream>

using sp::errlog;
using sp::proxy_configuration;

//#define DEBUG

namespace dht
{
  std::string DHTNode::_dht_config_filename = "";

  DHTNode::DHTNode(const char *net_addr,
                   const short &net_port,
                   const bool &start_dht_node,
                   const std::string &vnodes_table_file)
    : _nnodes(0),_nnvnodes(0),_transport(NULL),_connected(false),_has_persistent_data(false)
  {
    if (DHTNode::_dht_config_filename.empty())
      {
#ifdef SEEKS_CONFIGDIR
        DHTNode::_dht_config_filename = SEEKS_CONFIGDIR "dht-config"; // TODO: changes for packaging.
#else
        DHTNode::_dht_config_filename = "dht-config";
#endif
      }

    _vnodes_table_file = vnodes_table_file;
#ifdef SEEKS_LOCALSTATEDIR
    _vnodes_table_file = SEEKS_LOCALSTATDIR "/" _vnodes_table_file;
#else
    // do nothing.
#endif

    //debug
    std::cerr << "[Debug]: vnodes_table_file: " << _vnodes_table_file << std::endl;
    //debug

    if (!dht_configuration::_dht_config)
      dht_configuration::_dht_config = new dht_configuration(DHTNode::_dht_config_filename);

    // this node net l1 address.
    _l1_na.setNetAddress(net_addr);
    if (net_port == 0) // no netport specified, default to config port.
      _l1_na.setPort(dht_configuration::_dht_config->_l1_port);
    else _l1_na.setPort(net_port);

    if (start_dht_node)
      start_node();
  }

  DHTNode::~DHTNode()
  {
    stop_node();
  }

  void DHTNode::start_node()
  {
    /**
     * Create stabilizer before structures in vnodes register to it.
     */
    _stabilizer = new Stabilizer();

    /**
     * Sets successor list size.
     */
    SuccList::_max_list_size = dht_configuration::_dht_config->_succlist_size;

    /**
     * create the virtual nodes.
     * Persistance of vnodes and associated location tables works as follows:
     * - default is to save vnodes' ids after they are created, and location tables
     *   after a clean exit, i.e. if possible.
     * - at starting time, if we have persistent data, we try to load them and bootstrap
     *   from them unless:
     *   + number of vnodes has changed, in this case we generate vnodes from scratch.
     *   + it is corrupted, same consequences.
     * - at starting time, if we do not have persistent data or if we are told to generate
     *   the vnodes, we do so from scratch.
     */
    _has_persistent_data = load_vnodes_table();
    if (!_has_persistent_data)
      create_vnodes(); // create the vnodes from scratch only if we couldn't deal with the persistent data. (virtual).
    _transport->init_sorted_vnodes();

    /**
     * start rpc client & server.
     * (virtual call).
     */
    init_server();

    _transport->run_thread();
  }

  void DHTNode::stop_node()
  {
    /**
    	* kill stabilizer.
    	*/
    delete _stabilizer;
    _stabilizer = NULL;

    /**
     * stops server.
     */
    _transport->stop_thread();

    /**
     * persistence of virtual nodes.
     */
    hibernate_vnodes_table();

    /**
     * destroy virtual nodes.
     */
    destroy_vnodes();

    delete _transport;
    _transport = NULL;
  }

  void DHTNode::create_vnodes()
  {
    for (int i=0; i<dht_configuration::_dht_config->_nvnodes; i++)
      {
        /**
         * creating virtual nodes.
         */
        DHTVirtualNode* dvn = create_vnode();
        _transport->_vnodes.insert(std::pair<const DHTKey*,DHTVirtualNode*>(new DHTKey(dvn->getIdKey()),
                                   dvn));

        /**
         * register vnode routing structures to the stabilizer:
         * - stabilize on successor continuously,
         * - stabilize more slowly on the rest of the finger table.
         */
        _stabilizer->add_fast(dvn->getFingerTable());
        _stabilizer->add_slow(dvn->getFingerTable());

        /**
         * register successor list with the stabilizer.
         */
        _stabilizer->add_slow(&dvn->_successors);
      }
    errlog::log_error(LOG_LEVEL_DHT, "Successfully generated %u virtual nodes", _transport->_vnodes.size());
  }

  void DHTNode::destroy_vnodes()
  {
    hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::iterator hit
    = _transport->_vnodes.begin();
    while(hit!=_transport->_vnodes.end())
      {
        delete (*hit).second;
        ++hit;
      }
  }

  DHTVirtualNode* DHTNode::create_vnode()
  {
    return new DHTVirtualNode(_transport);
  }

  DHTVirtualNode* DHTNode::create_vnode(const DHTKey &idkey,
                                        LocationTable *lt)
  {
    return new DHTVirtualNode(_transport,idkey,lt);
  }

#if 0
  void DHTNode::init_sorted_vnodes()
  {
    hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit
    = _vnodes.begin();
    while(hit!=_vnodes.end())
      {
        _sorted_vnodes_vec.push_back((*hit).second->getIdKeyPtr());
        ++hit;
      }
    std::sort(_sorted_vnodes_vec.begin(),_sorted_vnodes_vec.end(),DHTKey::lowdhtkey);
  }
#endif

  void DHTNode::init_server()
  {
    _transport = new Transport(_l1_na);
  }

  bool DHTNode::load_vnodes_table() throw (dht_exception)
  {
    std::ifstream ifs;
    ifs.open(_vnodes_table_file.c_str(),std::ios::binary);
    if (!ifs.is_open())
      return false;

    /**
     * Otherwise let's try to read from the persistent table.
     */
    l1::table::vnodes_table *persistent_vt = new l1::table::vnodes_table();

    try
      {
        l1_data_protob_wrapper::deserialize_from_stream(ifs,persistent_vt);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed deserializing vnode and location tables");
        throw dht_exception(e.code(), "Failed deserializing vnode and location tables" + e.what());
      }
    std::vector<const DHTKey*> vnode_ids;
    std::vector<LocationTable*> vnode_ltables;
    l1_data_protob_wrapper::read_vnodes_table(persistent_vt,vnode_ids,vnode_ltables); // TODO: intercept failures.
    if (vnode_ids.size()!=vnode_ltables.size())
      return false; // there's a problem in persistent data, let's take another path.

    /**
     * fill up tables with our persistent data if the number of vnodes has not
     * changed in configuration.
     */
    if (dht_configuration::_dht_config->_nvnodes == (int)vnode_ids.size())
      {
        load_vnodes_and_tables(vnode_ids,vnode_ltables);
        errlog::log_error(LOG_LEVEL_DHT, "Successfully loaded %u persistent virtual nodes", vnode_ids.size());
        return true;
      }
    else
      {
        errlog::log_error(LOG_LEVEL_DHT, "Found hibernated table of virtual nodes that doesn't match the requested number of virtual nodes (%d != %d). Virtual nodes will be recreated.",dht_configuration::_dht_config->_nvnodes,(int)vnode_ids.size());
        return false;
      }
  }

  void DHTNode::load_vnodes_and_tables(const std::vector<const DHTKey*> &vnode_ids,
                                       const std::vector<LocationTable*> &vnode_ltables)
  {
    for (int i=0; i<dht_configuration::_dht_config->_nvnodes; i++)
      {
        /**
         * create virtual nodes but specifies key and location table.
         */
        const DHTKey idkey = *vnode_ids.at(i);
        LocationTable *lt = vnode_ltables.at(i);
        DHTVirtualNode *dvn = create_vnode(idkey,lt);
        _transport->_vnodes.insert(std::pair<const DHTKey*,DHTVirtualNode*>(new DHTKey(dvn->getIdKey()),
                                   dvn));

        errlog::log_error(LOG_LEVEL_DHT, "Loaded persistent virtual node %s", dvn->getIdKey().to_rstring().c_str());

        /**
         * register vnode routing structures to the stabilizer:
         * - stabilize on successor continuously,
         * - stabilize more slowly on the rest of the finger table.
         */
        _stabilizer->add_fast(dvn->getFingerTable());
        _stabilizer->add_slow(dvn->getFingerTable());

        /**
         * register successor list with the stabilizer.
         */
        _stabilizer->add_slow(&dvn->_successors);
      }
  }

  bool DHTNode::hibernate_vnodes_table() throw (dht_exception)
  {
    std::vector<const DHTKey*> vnode_ids;
    std::vector<LocationTable*> vnode_ltables;
    hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::const_iterator
    hit = _transport->_vnodes.begin();
    while(hit!=_transport->_vnodes.end())
      {
        vnode_ids.push_back((*hit).first);
        vnode_ltables.push_back((*hit).second->_lt);
        ++hit;
      }
    l1::table::vnodes_table *vt
    = l1_data_protob_wrapper::create_vnodes_table(vnode_ids,vnode_ltables);
    std::ofstream ofs;
    ofs.open(_vnodes_table_file.c_str(),std::ios::trunc | std::ios::binary); // beware.
    if (!ofs.is_open())
      {
        delete vt;
        return false;
      }
    try
      {
        l1_data_protob_wrapper::serialize_to_stream(vt,ofs);
      }
    catch (dht_exception &e)
      {
        delete vt;
        errlog::log_error(LOG_LEVEL_DHT, "Failed serializing vnode and location tables");
        throw dht_exception(e.code(), "Failed serializing vnode and location tables" + e.what());
      }
    delete vt;
    return true;
  }

#if 0
  DHTVirtualNode* DHTNode::find_closest_vnode(const DHTKey &key) const
  {
    DHTVirtualNode *vnode = NULL;
    std::vector<const DHTKey*>::const_iterator vit = _sorted_vnodes_vec.begin();
    DHTKey vidkey = *(*vit);
    if (key < *(*vit))
      vidkey = *(_sorted_vnodes_vec.back());
    else
      {
        while(vit!=_sorted_vnodes_vec.end()
              && *(*vit) < key)
          {
            vidkey = *(*vit);
            ++vit;
          }
      }
    vnode = findVNode(vidkey);
    return vnode;
  }
#endif

  dht_err DHTNode::join_start(const std::vector<NetAddress> &btrap_nodelist,
                              const bool &reset)
  {
    std::cerr << "[Debug]: join_start\n";

    std::vector<NetAddress> bootstrap_nodelist = btrap_nodelist;

    /**
     * try location table if possible/asked to.
     * To do so, known locations are added to the bootstrap nodelist,
     * before being removed from location tables and removed.
     * XXX: there may be other applicable strategies here.
     */
    if (!reset)
      {
        //debug
        std::cerr << "[Debug]:join_start: trying location table.\n";
        //debug

        hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit
        = _transport->_vnodes.begin();
        while(hit != _transport->_vnodes.end())
          {
            std::vector<Location*> dead_locs;
            LocationTable *lt = (*hit).second->_lt;
            hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::iterator lit
            = lt->_hlt.begin();
            while(lit!=lt->_hlt.end())
              {
                Location *loc = (*lit).second;
                if (loc->getDHTKey()!=(*hit).second->getIdKey())
                  {
                    bootstrap_nodelist.push_back(loc->getNetAddress());
                    dead_locs.push_back(loc);
                  }
                ++lit;
              }

            /**
             * clear location tables.
             * XXX: other strategies are applicable here, left for future work.
             */
            for (size_t i=0; i<dead_locs.size(); i++)
              lt->removeLocation(dead_locs.at(i));

            ++hit;
          }
      }

    // try to bootstrap from the nodelist in configuration.
    std::vector<NetAddress>::const_iterator nit = bootstrap_nodelist.begin();
    while(nit!=bootstrap_nodelist.end())
      {
        NetAddress na = (*nit);

        // XXX: we don't ping the bootstrap node because we do not have its key.
        // we could ping the machine but the result would be the same as trying to
        // bootstrap from it, since any alive node can be used as a bootstrap.

        // output.
        errlog::log_error(LOG_LEVEL_DHT,"trying to bootstrap from %s",na.toString().c_str());

        // join.
        DHTKey key;
        dht_err status = join(na,key); // empty key is handled by the called node.
        if (status == DHT_ERR_OK)
          return status; // we're done, join was successful, stabilization will take over the job.

        ++nit;
      }

    std::cerr << "[Debug]: end join_start (failed)\n";

    // hermaphrodite, builds its own circle.
    self_bootstrap();

    return DHT_ERR_BOOTSTRAP; // TODO: check on error status.
  }

#if 0
  dht_err DHTNode::rejoin()
  {
    hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::iterator
    hit = _vnodes.begin();
    while(hit!=_vnodes.end())
      {
        _stabilizer->rejoin((*hit).second);
        ++hit;
      }
    return DHT_ERR_OK;
  }
#endif

  void DHTNode::self_bootstrap()
  {
    //debug
    std::cerr << "[Debug]:self-bootstrap in hermaphrodite mode...\n";
    //debug

    // For every virtual node, appoint local virtual node as successor.
    std::vector<const DHTKey*> vnode_keys_ord;
    _transport->rank_vnodes(vnode_keys_ord);
    int nv = vnode_keys_ord.size(); // should be dht_config::_nvnodes, but safer to use the vector size.
    int nvsucclist = std::min(nv-2,dht_configuration::_dht_config->_succlist_size-1);

    for (int i=0; i<nv; i++)
      {
        const DHTKey *dkey = vnode_keys_ord.at(i);
        DHTVirtualNode *vnode = _transport->findVNode(*dkey);

#ifdef DEBUG
        //debug
        std::cerr << "vnode key: " << vnode->getIdKey() << std::endl;
        //debug
#endif

        vnode->clearSuccsList();

        int j = i+2;
        if (i == nv-1)
          {
            vnode->setSuccessor(*vnode_keys_ord.at(0),_l1_na); // close the circle.
            j = 1;
          }
        else
          vnode->setSuccessor(*vnode_keys_ord.at(i+1),_l1_na);
        if (i == 0)
          vnode->setPredecessor(*vnode_keys_ord.at(nv-1),_l1_na);
        else
          vnode->setPredecessor(*vnode_keys_ord.at(i-1),_l1_na);

        vnode->_connected = true;

        vnode->update_successor_list_head(); // sets successor in successor list.

#ifdef DEBUG
        //debug
        std::cerr << "predecessor: " << *vnode->getPredecessor() << std::endl;
        std::cerr << "successor: " << *vnode->getSuccessor() << std::endl;
        //debug
#endif

        // fill up successor list.
        int c = 0;
        while(c<nvsucclist)
          {
            if (j >= nv)
              j = 0;
            vnode->addOrFindToLocationTable(*vnode_keys_ord.at(j),_l1_na);
            vnode->_successors._succs.push_back(vnode_keys_ord.at(j));
            c++;
            j++;
          }

#ifdef DEBUG
        //debug
        std::cerr << "successor list (" << vnode->_successors.size() << "): ";
        std::list<const DHTKey*>::const_iterator lit = vnode->_successors._succs.begin();
        while(lit!=vnode->_successors._succs.end())
          {
            std::cerr << *(*lit) << std::endl;
            ++lit;
          }
        //debug
#endif
      }

    //debug
    std::cerr << "[Debug]:self-bootstrap complete.\n";
    //debug

    // we are 'connected', to ourselves, but we are...
    _connected = true;
  }

  dht_err DHTNode::leave() const
  {
    dht_err err = DHT_ERR_OK;

    /**
     * Since several virtual nodes may be successors / predecessors
     * on the circle, we ask them to leave in ascending order.
     */
    std::vector<const DHTKey*>::const_iterator vit = _transport->_sorted_vnodes_vec.begin();
    while(vit!=_transport->_sorted_vnodes_vec.end())
      {
        const DHTKey *vkey = (*vit);
        DHTVirtualNode *vnode = _transport->findVNode(*vkey);
        if (vnode)
          {
            err += vnode->leave();
          }
        else
          {
            // XXX: this should never happen, but since we're leaving
            // we're not bitching too much about it...
            errlog::log_error(LOG_LEVEL_ERROR, "leave: can't find virtual node %s",
                              vkey->to_rstring().c_str());
          }
        ++vit;
      }

    return err;
  }

#if 0
  bool DHTNode::isSuccStable() const
  {
    hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit
    = _vnodes.begin();
    while(hit!=_vnodes.end())
      {
        if (!(*hit).second->isSuccStable())
          return false;
        ++hit;
      }
    return true;
  }

  bool DHTNode::isStable() const
  {
    hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit
    = _vnodes.begin();
    while(hit!=_vnodes.end())
      {
        if (!(*hit).second->isStable())
          return false;
        ++hit;
      }
    return true;
  }
#endif

#if 0
  void DHTNode::rank_vnodes(std::vector<const DHTKey*> &vnode_keys_ord)
  {
    vnode_keys_ord.reserve(_vnodes.size());
    hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey>::const_iterator vit
    = _vnodes.begin();
    while(vit!=_vnodes.end())
      {
        vnode_keys_ord.push_back((*vit).first);
        ++vit;
      }
    std::stable_sort(vnode_keys_ord.begin(),vnode_keys_ord.end(),
                     DHTKey::lowdhtkey);
  }


  /**----------------------------**/
  /**
   * RPC virtual functions (callbacks).
   */
  void DHTNode::getSuccessor_cb(const DHTKey& recipientKey,
                                DHTKey& dkres, NetAddress& na,
                                int& status) throw (dht_exception)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node and deal with possible errors.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        dkres = DHTKey();
        na = NetAddress();
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    /**
     * return successor info.
     */
    dkres = vnode->getSuccessorS();
    if (!dkres.count())
      {
        //TODO: this node has no successor yet: this means it must have joined
        //      the network a short time ago or that it is alone on the network.
        na = NetAddress();
        status = DHT_ERR_NO_SUCCESSOR_FOUND;
        return;
      }

    /**
     * fetch location information.
     */
    Location* resloc = vnode->findLocation(dkres);
    if (!resloc)
      {
        // XXX: we should never reach here.
        std::cout << "[Error]:RPC_getSuccessor_cb: our own successor is an unknown location !\n";
        throw dht_exception(DHT_ERR_UNKNOWN_PEER_LOCATION,"RPC_getSuccessor_cb: our own successor is an unknown location");
      }

    /**
     * Setting RPC results.
     */
    na = resloc->getNetAddress();

    //debug
    //assert(dkres.count()>0);
    //debug
  }

  void DHTNode::getPredecessor_cb(const DHTKey& recipientKey,
                                  DHTKey& dkres, NetAddress& na,
                                  int& status)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node and deal with possible errors.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        dkres = DHTKey();
        na = NetAddress();
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    /**
     * return predecessor info.
     */
    dkres = vnode->getPredecessorS();
    if (!dkres.count())
      {
        na = NetAddress();
        status = DHT_ERR_NO_PREDECESSOR_FOUND;
        return;
      }

    /**
     * fetch location information.
     */
    Location* resloc = vnode->findLocation(dkres);
    if (!resloc)
      {
        std::cout << "[Error]:RPC_getPredecessor_cb: our own predecessor is an unknown location !\n";
        na = NetAddress();
        status = DHT_ERR_UNKNOWN_PEER_LOCATION;
        return;
      }

    /**
     * Setting RPC results.
     */
    na = resloc->getNetAddress();
  }

  void DHTNode::notify_cb(const DHTKey& recipientKey,
                          const DHTKey& senderKey,
                          const NetAddress& senderAddress,
                          int& status)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    /**
     * notifies this node that the argument node (key) thinks it is
     * its predecessor.
     */
    status = vnode->notify(senderKey, senderAddress);

  }

  void DHTNode::getSuccList_cb(const DHTKey &recipientKey,
                               std::list<DHTKey> &dkres_list,
                               std::list<NetAddress> &na_list,
                               int &status)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node and deal with possible errors.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        dkres_list = std::list<DHTKey>();
        na_list = std::list<NetAddress>();
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    if (vnode->_successors.empty())
      {
        dkres_list = std::list<DHTKey>();
        na_list = std::list<NetAddress>();
        status = DHT_ERR_OK;
        return;
      }

    /**
     * return successor list.
     */
    std::list<const DHTKey*>::const_iterator sit = vnode->_successors._succs.begin();
    while(sit!=vnode->_successors._succs.end())
      {
        dkres_list.push_back(*(*sit));
        ++sit;
      }

    if (dkres_list.empty())
      {
        na_list = std::list<NetAddress>();
        status = DHT_ERR_NO_SUCCESSOR_FOUND;
        return;
      }

    /**
     * fetch location information.
     */
    std::list<DHTKey>::const_iterator tit = dkres_list.begin();
    while(tit!=dkres_list.end())
      {
        Location *resloc = vnode->findLocation((*tit));
        if (!resloc)
          {
            dkres_list.clear();
            na_list = std::list<NetAddress>();
            status = DHT_ERR_UNKNOWN_PEER_LOCATION;
            return;
          }
        na_list.push_back(resloc->getNetAddress());
        ++tit;
      }

    //debug
    assert(!na_list.empty());
    //debug
  }

  void DHTNode::findClosestPredecessor_cb(const DHTKey& recipientKey,
                                          const DHTKey& nodeKey,
                                          DHTKey& dkres, NetAddress& na,
                                          DHTKey& dkres_succ, NetAddress &dkres_succ_na,
                                          int& status)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        dkres = DHTKey();
        na = NetAddress();
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    /**
     * return closest predecessor.
     */
    vnode->findClosestPredecessor(nodeKey, dkres, na, dkres_succ, dkres_succ_na, status);

    if(status != DHT_ERR_OK && status != DHT_ERR_UNKNOWN_PEER_LOCATION)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed findClosestPredecessor_cb");
        return;
      }

    //debug
    assert(dkres.count()>0);
    //debug
  }

  void DHTNode::joinGetSucc_cb(const DHTKey &recipientKey,
                               const DHTKey &senderKey,
                               DHTKey &dkres, NetAddress &na,
                               int &status)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]:executing joinGetSucc_cb.\n";
    //debug
#endif

    // if recipientKey is not specified, find the closest VNode to senderkey.
    DHTVirtualNode *vnode = NULL;
    DHTKey contactKey = recipientKey;
    if (contactKey.count() == 0) // test for unspecified recipient.
      {
        // rank local nodes.
        std::vector<const DHTKey*> vnode_keys_ord;
        rank_vnodes(vnode_keys_ord);

        // pick up the closest one.
        DHTKey *curr = NULL, *prev = NULL;
        std::vector<const DHTKey*>::const_iterator dit = vnode_keys_ord.begin();
        while(dit!=vnode_keys_ord.end())
          {
            curr = const_cast<DHTKey*>((*dit));
            if (senderKey > *curr)
              break;
            prev = curr;
            ++dit;
          }

        if (!prev) // could not find a key smaller than senderKey, so curr is the successor.
          {
            dkres = *curr;
            na = getNetAddress();
            return;
          }

        // recipient key is prev.
        contactKey = *prev;
      }

    vnode = findVNode(contactKey);
    if (!vnode)
      {
        dkres = DHTKey();
        na = NetAddress();
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    status = vnode->find_successor(senderKey, dkres, na);
    if (status < 0) // TODO: more precise check.
      {
        dkres = DHTKey();
        na = NetAddress();
        status = DHT_ERR_UNKNOWN; // TODO: some other error ?
      }
    else if (dkres.count() == 0) // TODO: if not found.
      {
        na = NetAddress();
        status = DHT_ERR_NO_SUCCESSOR_FOUND;
        return;
      }

    status = DHT_ERR_OK;
  }

  void DHTNode::ping_cb(const DHTKey& recipientKey,
                        int& status)
  {
    status = DHT_ERR_OK;

    /**
     * get virtual node.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        status = DHT_ERR_UNKNOWN_PEER;
        return;
      }

    /**
     * ping this virtual node.
     */
    vnode->ping();
  }
#endif

  /**-- Main routines using RPCs --**/
  dht_err DHTNode::join(const NetAddress& dk_bootstrap_na,
                        const DHTKey &dk_bootstrap)
  {
    /**
     * We're basically bootstraping all the virtual nodes here.
     */
    hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit
    = _transport->_vnodes.begin();
    while(hit != _transport->_vnodes.end())
      {
        DHTVirtualNode *vnode = (*hit).second;
        int status = 0;
        try
          {
            vnode->join(dk_bootstrap, dk_bootstrap_na, *(*hit).first, status); // XXX: could make a single call instead ?
          }
        catch (dht_exception &e)
          {
            status = e.code();
          } // XXX: no throw ?

        if (status != DHT_ERR_OK)
          {
            return status;
          }

        ++hit;
      }

    // this node is connected (all its virtual nodes are.
    _connected = true;

    return DHT_ERR_OK;
  }

#if 0
  dht_err DHTNode::find_successor(const DHTKey& recipientKey,
                                  const DHTKey& nodeKey,
                                  DHTKey& dkres, NetAddress& na) throw (dht_exception)
  {
    /**
     * get the virtual node and deal with possible errors.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)  // TODO: error handling.
      {
        dkres = DHTKey();
        na = NetAddress();
        return 3;
      }

    return vnode->find_successor(nodeKey, dkres, na);
  }

  dht_err DHTNode::find_predecessor(const DHTKey& recipientKey,
                                    const DHTKey& nodeKey,
                                    DHTKey& dkres, NetAddress& na) throw (dht_exception)
  {
    /**
     * get the virtual node and deal with possible errors.
     */
    DHTVirtualNode* vnode = findVNode(recipientKey);
    if (!vnode)
      {
        dkres = DHTKey();
        na = NetAddress();
        return 3;
      }

    return vnode->find_predecessor(nodeKey, dkres, na);
  }
#endif

#if 0
  /**----------------------------**/

  DHTVirtualNode* DHTNode::findVNode(const DHTKey& dk) const
  {
    hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit;
    if ((hit = _vnodes.find(&dk)) != _vnodes.end())
      return (*hit).second;
    else
      {
#ifdef DEBUG
        std::cout << "[Debug]:DHTNode::findVNode: virtual node: " << dk
                  << " is unknown on this node.\n";
#endif
        return NULL;
      }

  }
#endif

  DHTKey DHTNode::generate_uniform_key()
  {
    /**
     * There is a key per virtual node.
     * A key is 48bit mac address + 112 random bits -> through ripemd-160.
     */

    /* first we try to get a mac address for this systems. */
    long stat;
    u_char addr[6];
    stat = mac_addr_sys(addr); // TODO: do it once only...
    if (stat < 0)
      {
        errlog::log_error(LOG_LEVEL_ERROR, "Can't determine mac address for this system, falling back to full key randomization");
        return DHTKey::randomKey();
      }

    /* printf( "MAC address = ");
    for (int i=0; i<6; ++i)
      {
         printf("%2.2x", addr[i]);
      }
    std::cout << std::endl; */

    int rbits = 112; // 160-48.

    /* generate random bits, seed is timeofday. BEWARE: this is no 'strong' randomization. */
    timeval tim;
    gettimeofday(&tim,NULL);
    unsigned long iseed = tim.tv_sec + tim.tv_usec;

    //debug
    //std::cout << "iseed: " << iseed << std::endl;
    //debug

    int nchars = rbits/8;
    char crkey[rbits];
    if (stat == 0)
      {
        std::bitset<112> rkey;
        for (int j=0; j<rbits; j++)
          rkey.set(j,DHTKey::irbit2(&iseed));

        //debug
        //std::cout << "rkey: " << rkey.to_string() << std::endl;
        //debug

        for (int j=0; j<nchars; j++)
          {
            std::bitset<8> cbits;
            short pos = 0;
            for (int k=j*8; k<(j+1)*8; k++)
              {
                //std::cout << "k: " << k << " -- rkey@k: " << rkey[k] << std::endl;
                if (rkey[k])
                  cbits.set(pos);
                else cbits.set(0);
                pos++;
              }
            unsigned long ckey = cbits.to_ulong();

            /* std::cout << "cbits: " << cbits.to_string() << std::endl;
             std::cout << "ckey: " << ckey << std::endl; */

            crkey[j] = (char)ckey;
          }

        std::string fkey = std::string((char*)addr) + std::string(crkey);

        //std::cout << "fkey: " << fkey << std::endl;

        //debug
        /* byte* hashcode = DHTKey::RMD((byte*)fkey.c_str());
         std::cout << "rmd: " << hashcode << std::endl; */
        //debug

        char *fckey = new char[20];
        memcpy(fckey,fkey.c_str(),20);
        DHTKey res_key = DHTKey::hashKey(fckey);

        delete[] fckey;

        return res_key;
      }
    return DHTKey(); // beware, we should never reach here.
  }

  void DHTNode::estimate_nodes(const unsigned long &nnodes,
                               const unsigned long &nnvnodes)
  {
    if (_nnodes == 0)
      {
        _nnodes = nnodes;
        _nnvnodes = nnvnodes;
      }
    else
      {
        _nnodes = std::min(_nnodes,(int)nnodes);
        _nnvnodes = std::min(_nnvnodes,(int)nnvnodes);
      }
  }

  bool DHTNode::on_ring_of_virtual_nodes()
  {
    hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::const_iterator
    hit = _transport->_vnodes.begin();
    while(hit!=_transport->_vnodes.end())
      {
        if (!(*hit).second->getLocationTable()->has_only_virtual_nodes(this))
          return false;
        ++hit;
      }
    return true;
  }

} /* end of namespace. */
