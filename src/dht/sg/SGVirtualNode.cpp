/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2010  Emmanuel Benazera, juban@free.fr
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

#include "SGVirtualNode.h"
#include "SGNode.h"
#include "errlog.h"
#include "Transport.h"
#include "l2_data_protob_wrapper.h"

using sp::errlog;

namespace dht
{
  SGVirtualNode::SGVirtualNode(Transport *transport, sg_manager *sgm)
    :DHTVirtualNode(transport),_sgm(sgm)
  {
    std::cerr << "creating SGVirtualNode\n";
  }

  SGVirtualNode::SGVirtualNode(Transport *transport, sg_manager *sgm, const DHTKey &idkey)
    :DHTVirtualNode(transport,idkey),_sgm(sgm)
  {
  }

  SGVirtualNode::~SGVirtualNode()
  {
  }

#define hash_subscribe                434989955ul   /* "subscribe" */
#define hash_replicate                780233041ul   /* "replicate" */

  void SGVirtualNode::execute_callback(const uint32_t &fct_id,
                                       const DHTKey &sender_key,
                                       const NetAddress &sender_na,
                                       const DHTKey &node_key,
                                       int& status,
                                       std::string &resp_msg,
                                       const std::string &inc_msg)
  {
#ifdef DEBUG
    //debug
    std::cout << "[Debug]:l2 lx_server_response:\n";
    //debug
#endif

    if (fct_id == hash_subscribe)
      {
#ifdef DEBUG
        //debug
        std::cerr << "subscribe\n";
        //debug
#endif

        // callback.
        std::vector<Subscriber*> peers;
        RPC_subscribe_cb(sender_key,sender_na,
                         node_key,peers,status);

        // create response.
        if (status == DHT_ERR_OK)
          {
            l2::l2_subscribe_response *l2r
            = l2_protob_wrapper::create_l2_subscribe_response(status,peers);

            // serialize the response.
            l2_protob_wrapper::serialize_to_string(l2r,resp_msg);
            delete l2r;
          }
        else
          {
            l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(status);

            // serialize the response.
            l1_protob_wrapper::serialize_to_string(l1r,resp_msg);
            delete l1r;
          }
      }
    else if (fct_id == hash_replicate)
      {
        //debug
        std::cerr << "replicate\n";
        //debug

        // need to re-deserialize the message to get the extra
        // information it contains.
        l1::l1_query *l1q = new l1::l1_query();
        try
          {
            l2_data_protob_wrapper::deserialize_from_string(inc_msg,l1q);
          }
        catch (dht_exception &e)
          {
            delete l1q;
            errlog::log_error(LOG_LEVEL_DHT,"l2_protob_rpc_server::lx_server_response exception %s",e.what().c_str());
            throw e;
          }
        uint32_t fct_id2;
        DHTKey recipient_key2;
        NetAddress recipient_na2;
        DHTKey sender_key2;
        NetAddress sender_na2;
        DHTKey owner_key;
        std::vector<Searchgroup*> sgs;
        bool sdiff = false;
        l2_data_protob_wrapper::read_l2_replication_query(l1q,fct_id2,recipient_key2,recipient_na2,
            sender_key2,sender_na2,owner_key,sgs,sdiff);

        // callback.
        RPC_replicate_cb(sender_key2,sender_na2,
                         owner_key,sgs,sdiff,status);

        // create response.
        l1::l1_response *l1r = l1_protob_wrapper::create_l1_response(status);

        // serialize the response.
        l1_protob_wrapper::serialize_to_string(l1r,resp_msg);
        delete l1r;
      }
    else
      {
        DHTVirtualNode::execute_callback(fct_id,sender_key,sender_na,node_key,status,resp_msg,inc_msg);
      }
    //TODO: other callback come here.
  }

  void SGVirtualNode::RPC_subscribe_cb(const DHTKey &senderKey,
                                       const NetAddress &sender,
                                       const DHTKey &sgKey,
                                       std::vector<Subscriber*> &peers,
                                       int &status)
  {
    // fill up the peers list.
    // subscribe or not.
    // trigger a sweep (condition to alleviate the load ?)

    /* check on parameters. */
    if (sgKey.count() == 0)
      {
        status = DHT_ERR_UNSPECIFIED_SEARCHGROUP;
        return;
      }

    /* check on subscription, i.e. if a sender address is specified. */
    bool subscribe = false;
    if (!sender.empty())
      subscribe = true;

    /* find / create searchgroup. */
    Searchgroup *sg = _sgm->find_load_or_create_sg(&sgKey);

    /* select peers. */
    if ((int)sg->_vec_subscribers.size() > sg_configuration::_sg_config->_max_returned_peers)
      sg->random_peer_selection(sg_configuration::_sg_config->_max_returned_peers,peers);
    else peers = sg->_vec_subscribers;

    /* subscription. */
    if (subscribe)
      {
        Subscriber *nsub = new Subscriber(senderKey,
                                          sender.getNetAddress(),sender.getPort());
        if (!sg->add_subscriber(nsub))
          delete nsub;
      }

    /* update usage. */
    sg->set_last_time_of_use();

    /* trigger a call to sweep (from sg_manager). */
    _sgm->_sgsw.sweep();
  }

  void SGVirtualNode::RPC_replicate_cb(const DHTKey &senderKey,
                                       const NetAddress &sender,
                                       const DHTKey &ownerKey,
                                       const std::vector<Searchgroup*> &sgs,
                                       const bool &sdiff,
                                       int &status)
  {
    // verify that sender address is non empty (empty means either not divulged,
    // either fake, eliminated by server).
    if (sender.empty())
      {
        errlog::log_error(LOG_LEVEL_DHT,"rejected replication call with empty sender address");
        status = DHT_ERR_SENDER_ADDR;
        return;
      }

    // if no peer in searchgroup, simply update the replication radius.
    int sgs_size = sgs.size();
    for (int i=0; i<sgs_size; i++)
      {
        Searchgroup *sg = sgs.at(i);
        if (sg->_hash_subscribers.empty())
          {
            //TODO: locate search group.
            Searchgroup *lsg = _sgm->find_sg(&sg->_idkey);
            if (!lsg)
              {

              }

            //TODO: update replication status.
            //lsg->_replication_level = ;
          }
        else
          {
            //TODO: add sg to local db if replication level is 0.
            if (sg->_replication_level == 0)
              _sgm->add_sg_db(sg);
            else
              {
                //TODO: add to replicated db.
              }
          }
      }
  }

  void SGVirtualNode::RPC_call2(const uint32_t &fct_id,
                                const DHTKey &recipientKey,
                                const NetAddress &recipient,
                                const DHTKey &sgKey,
                                l2::l2_subscribe_response *l2r) throw (dht_exception)
  {
    // serialize.
    l1::l1_query *l2q = l1_protob_wrapper::create_l1_query(fct_id,recipientKey,
                        recipient,sgKey);
    l1::header *l2q_head = l2q->mutable_head();
    l2q_head->set_layer_id(2);

    std::string msg;
    try
      {
        l1_protob_wrapper::serialize_to_string(l2q,msg);
      }
    catch(dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"rpc %u l2 error: %s",fct_id,e.what().c_str());
        throw dht_exception(DHT_ERR_CALL, "rpc " + sp::miscutil::to_string(fct_id) + " l2 error" + e.what());
      }

    RPC_call2(msg,recipientKey,recipient,l2r);
  }

  void SGVirtualNode::RPC_call2(const uint32_t &fct_id,
                                const DHTKey &recipientKey,
                                const NetAddress &recipient,
                                const DHTKey &senderKey,
                                const NetAddress &sender,
                                const DHTKey &sgKey,
                                l2::l2_subscribe_response *l2r) throw (dht_exception)
  {
    // serialize.
    l1::l1_query *l2q = l1_protob_wrapper::create_l1_query(fct_id,recipientKey,recipient,
                        senderKey,sender,sgKey);
    l1::header *l2q_head = l2q->mutable_head();
    l2q_head->set_layer_id(2);

    std::string msg;
    try
      {
        l1_protob_wrapper::serialize_to_string(l2q,msg);
      }
    catch(dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"rpc %u l2 error: %s",fct_id,e.what().c_str());
        throw dht_exception(DHT_ERR_CALL, "rpc " + sp::miscutil::to_string(fct_id) + " l2 error " + e.what());
      }

    RPC_call2(msg,recipientKey,recipient,l2r);
  }

  void SGVirtualNode::RPC_call2(const uint32_t &fct_id,
                                const DHTKey &recipientKey,
                                const NetAddress &recipient,
                                const DHTKey &senderKey,
                                const NetAddress &sender,
                                const DHTKey &ownerKey,
                                const std::vector<Searchgroup*> &sgs,
                                const bool &sdiff,
                                l1::l1_response *l1r) throw (dht_exception)
  {
    // serialize.
    l1::l1_query *l2q = l2_data_protob_wrapper::create_l2_replication_query(fct_id,recipientKey,recipient,
                        senderKey,sender,ownerKey,
                        sgs,sdiff);
    l1::header *l2q_head = l2q->mutable_head();
    l2q_head->set_layer_id(2);

    std::string msg;
    try
      {
        l2_data_protob_wrapper::serialize_to_string(l2q,msg);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"rpc %u l2 error: %s",fct_id,e.what().c_str());
        throw dht_exception(DHT_ERR_CALL, "rpc " + sp::miscutil::to_string(fct_id) + " l2 error " + e.what());
      }
    RPC_call(msg,recipientKey,recipient,l1r);
  }

  void SGVirtualNode::RPC_call2(const std::string &msg_str,
                                const DHTKey &recipientKey,
                                const NetAddress &recipient,
                                l2::l2_subscribe_response *l2r) throw (dht_exception)
  {
    // send & get response.
    std::string resp_str;
    try
      {
        _transport->do_rpc_call(recipient,recipientKey,msg_str,true,resp_str);
      }
    catch (dht_exception &e)
      {
        if (e.code() == DHT_ERR_COM_TIMEOUT)
          {
            l2r->set_error_status(e.code());
            return;
          }
        else
          {
            throw e;
          }
      }

    // deserialize response.
    try
      {
        l2_protob_wrapper::deserialize(resp_str,l2r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_ERROR,"rpc l2 error: %s",e.what().c_str());
        throw dht_exception(DHT_ERR_NETWORK, "rpc l2 error " + e.what());
      }
  }

  void SGVirtualNode::RPC_subscribe(const DHTKey &recipientKey,
                                    const NetAddress &recipient,
                                    const DHTKey &senderKey,
                                    const NetAddress &senderAddress,
                                    const DHTKey &sgKey,
                                    std::vector<Subscriber*> &peers,
                                    int &status) throw (dht_exception)
  {
    //debug
    std::cerr << "[Debug]: RPC_subscribe call\n";
    //debug

    // do call, wait and get response.
    l2::l2_subscribe_response l2r;

    try
      {
        if (senderKey.count() == 0)
          SGVirtualNode::RPC_call2(hash_subscribe,
                                   recipientKey,recipient,
                                   sgKey,&l2r);
        else
          SGVirtualNode::RPC_call2(hash_subscribe,
                                   recipientKey,recipient,
                                   senderKey,senderAddress,sgKey,&l2r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed subscribe call to %s: %s",
                          recipient.toString().c_str(),e.what().c_str());
        throw dht_exception(DHT_ERR_CALL, "Failed subscribe call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l2_protob_wrapper::read_l2_subscribe_response(&l2r,error_status,peers);
    status = error_status;
  }

  void SGVirtualNode::RPC_replicate(const DHTKey &recipientKey,
                                    const NetAddress &recipient,
                                    const DHTKey &senderKey,
                                    const NetAddress &senderAddress,
                                    const DHTKey &ownerKey,
                                    const std::vector<Searchgroup*> &sgs,
                                    const bool &sdiff,
                                    int &status) throw (dht_exception)
  {
    //debug
    std::cerr << "[Debug]: RPC_replicate call\n";
    //debug

    // do call, wait and get response.
    l1::l1_response l1r;

    try
      {
        SGVirtualNode::RPC_call2(hash_replicate,
                                 recipientKey,recipient,
                                 senderKey,senderAddress,
                                 ownerKey,sgs,sdiff,&l1r);
      }
    catch (dht_exception &e)
      {
        errlog::log_error(LOG_LEVEL_DHT, "Failed replicate call to %s: %s",
                          recipient.toString().c_str(),e.what().c_str());
        throw dht_exception(DHT_ERR_CALL, "Failed replicate call to " + recipient.toString() + ":" + e.what());
      }

    // handle the response.
    uint32_t error_status;
    l1_protob_wrapper::read_l1_response(&l1r,error_status);
    status = error_status;
  }

  dht_err SGVirtualNode::replication_host_keys(const DHTKey &start_key)
  {
    // decrement replication radius of all other replicated search groups.
    // when replication_radius is 0, the search group is hosted by this virtual node.
    if (!_sgm->replication_decrement_all_sgs(_idkey))
      return DHT_ERR_REPLICATION;
    return DHT_ERR_OK;
  }

  void SGVirtualNode::replication_move_keys_backward(const DHTKey &start_key,
      const DHTKey &end_key,
      const NetAddress &senderAddress)
  {
    // select keys and searchgroups...
    // XXX: we could use a single representation here, by using a hash_map
    // in the call back.
    hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey> h_sgs;
    _sgm->find_sg_range(start_key,end_key,h_sgs);
    std::vector<Searchgroup*> v_sgs;
    hash_map<const DHTKey*,Searchgroup*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit
    = h_sgs.begin();
    while(hit!=h_sgs.end())
      {
        v_sgs.push_back((*hit).second);
        ++hit;
      }

    // RPC call to predecessor to pass the keys.
    // TODO: local call if the virtual node belongs to this DHT node.
    int status = DHT_ERR_OK;
    DHTKey ownerKey; //TODO: unused.
    RPC_replicate(end_key,senderAddress,
                  _idkey,getNetAddress(),
                  ownerKey,v_sgs,
                  false,status);
    //TODO: catch errors.


    //TODO: update local replication level of nodes,
    // and remove those with level > k.
    // Searchgroup objects are destroyed.
    _sgm->increment_replicated_sgs(_idkey,h_sgs);
    h_sgs.clear();
  }

  void SGVirtualNode::replication_move_keys_forward(const DHTKey &end_key)
  {

  }

  void SGVirtualNode::replication_trickle_forward(const DHTKey &start_key,
      const short &start_replication_radius)
  {

  }

} /* end of namespace. */
