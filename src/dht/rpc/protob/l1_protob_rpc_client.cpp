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

#if 0

#include "l1_protob_rpc_client.h"
#include "errlog.h"

//#define DEBUG

using sp::errlog;

namespace dht
{
  l1_protob_rpc_client::l1_protob_rpc_client()
  {
  }

  l1_protob_rpc_client::~l1_protob_rpc_client()
  {
  }

  void l1_protob_rpc_client::RPC_call(const uint32_t &fct_id,
                                      const DHTKey &recipientKey,
                                      const NetAddress& recipient,
                                      const DHTKey &nodeKey,
                                      l1::l1_response *l1r) throw (dht_exception)
  {
    // serialize.
    l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,
                        recipientKey,recipient,
                        nodeKey);
    RPC_call(l1q,recipient,l1r);
  }

  void l1_protob_rpc_client::RPC_call(const uint32_t &fct_id,
                                      const DHTKey &recipientKey,
                                      const NetAddress &recipient,
                                      l1::l1_response *l1r) throw (dht_exception)
  {
    // serialize.
    l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,
                        recipientKey,recipient);

    RPC_call(l1q,recipient,l1r);
  }

  void l1_protob_rpc_client::RPC_call(const uint32_t &fct_id,
                                      const DHTKey &recipientKey,
                                      const NetAddress& recipient,
                                      const DHTKey &senderKey,
                                      const NetAddress& senderAddress,
                                      l1::l1_response *l1r) throw (dht_exception)
  {
    // serialize.
    DHTKey nodeKey;
    l1::l1_query *l1q = l1_protob_wrapper::create_l1_query(fct_id,
                        recipientKey,recipient,
                        senderKey,senderAddress,
                        nodeKey); // empty nodeKey.
    RPC_call(l1q,recipient,l1r);
  }

  void l1_protob_rpc_client::RPC_call(l1::l1_query *&l1q,
                                      const NetAddress &recipient,
                                      l1::l1_response *l1r) throw (dht_exception)
  {
    std::string msg_str;
    l1_protob_wrapper::serialize_to_string(l1q,msg_str);
    std::string resp_str;

    try
      {
        do_rpc_call(recipient,msg_str,true,resp_str);
      }
    catch (dht_exception &e)
      {
        delete l1q;
        if (e.code() == DHT_ERR_COM_TIMEOUT)
          {
            l1r->set_error_status(e.code());
            return;
          }
        else
          {
            throw e;
          }
      }
    delete l1q;

    // deserialize response.
    try
      {
        l1_protob_wrapper::deserialize(resp_str,l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"rpc l1 error: %s",e.what().c_str());
        throw dht_exception(DHT_ERR_NETWORK, e.what());
      }
  }

  void l1_protob_rpc_client::RPC_getSuccessor(const DHTKey &recipientKey,
      const NetAddress& recipient,
      DHTKey& dkres, NetAddress& na,
      int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_getSuccessor call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        l1_protob_rpc_client::RPC_call(hash_get_successor,
                                       recipientKey,recipient,
                                       &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed getSuccessor call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed getSuccessor call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status,
                                        dkres,na);
    status = error_status; // remote error status.
  }

  void l1_protob_rpc_client::RPC_getPredecessor(const DHTKey& recipientKey,
      const NetAddress& recipient,
      DHTKey& dkres, NetAddress& na,
      int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_getPredecessor call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        l1_protob_rpc_client::RPC_call(hash_get_predecessor,
                                       recipientKey,recipient,
                                       &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed getPredecessor call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed getPredecessor call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status,
                                        dkres,na);
    status = error_status;
  }

  void l1_protob_rpc_client::RPC_notify(const DHTKey& recipientKey,
                                        const NetAddress& recipient,
                                        const DHTKey& senderKey,
                                        const NetAddress& senderAddress,
                                        int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_notify call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        l1_protob_rpc_client::RPC_call(hash_notify,
                                       recipientKey,recipient,
                                       senderKey,senderAddress,
                                       &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed notify call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed notify call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status);
    status = error_status;
  }

  void l1_protob_rpc_client::RPC_getSuccList(const DHTKey &recipientKey,
      const NetAddress &recipient,
      std::list<DHTKey> &dkres_list,
      std::list<NetAddress> &na_list,
      int &status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_getSuccList call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        l1_protob_rpc_client::RPC_call(hash_get_succlist,
                                       recipientKey,recipient,
                                       &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed getSuccList call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed getSuccList call to " + recipient.toString() + ":" + e.what());
      }

    /// handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status,
                                        dkres_list,na_list);
    status = error_status; // remote error status.
  }

  void l1_protob_rpc_client::RPC_findClosestPredecessor(const DHTKey& recipientKey,
      const NetAddress& recipient,
      const DHTKey& nodeKey,
      DHTKey& dkres, NetAddress& na,
      DHTKey& dkres_succ,
      NetAddress &dkres_succ_na,
      int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_findClosestPredecessor call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        l1_protob_rpc_client::RPC_call(hash_find_closest_predecessor,
                                       recipientKey,recipient,
                                       nodeKey,&l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed findClosestPredecessor call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed findClosestPredecessor call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status,
                                        dkres,na,
                                        dkres_succ,dkres_succ_na);
    status = error_status;
  }

  void l1_protob_rpc_client::RPC_joinGetSucc(const DHTKey& recipientKey,
      const NetAddress& recipient,
      const DHTKey &senderKey,
      DHTKey& dkres, NetAddress& na,
      int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_joinGetSucc call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    // empty address.
    const NetAddress senderAddress = NetAddress();

    try
      {
        l1_protob_rpc_client::RPC_call(hash_join_get_succ,
                                       recipientKey,recipient,
                                       senderKey, senderAddress,
                                       &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed joinGetSucc call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed joinGetSucc call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status,
                                        dkres,na);
    status = error_status;
  }

  void l1_protob_rpc_client::RPC_ping(const DHTKey& recipientKey,
                                      const NetAddress& recipient,
                                      int& status) throw (dht_exception)
  {
#ifdef DEBUG
    //debug
    std::cerr << "[Debug]: RPC_ping call\n";
    //debug
#endif

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        l1_protob_rpc_client::RPC_call(hash_ping,
                                       recipientKey,recipient,
                                       &l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed ping call to %s: %s",
                          recipient.toString().c_str(), e.what().c_str());
        throw dht_exception(e.code(), "Failed ping call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status);
    status = error_status;
  }

} /* end of namespace. */

#endif /* 0 */
