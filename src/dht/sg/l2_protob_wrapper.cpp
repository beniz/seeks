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

#include "l2_protob_wrapper.h"
#include "dht_layer1.pb.h"
#include "errlog.h"

using sp::errlog;

namespace dht
{
  l2::l2_subscribe_response* l2_protob_wrapper::create_l2_subscribe_response(const uint32_t &error_status,
      const std::vector<Subscriber*> &peers)
  {
    l2::l2_subscribe_response *l2r = new l2::l2_subscribe_response();
    l2r->set_error_status(error_status);
    std::vector<unsigned char> dkresult;
    size_t npeers = peers.size();
    for (size_t i=0; i<npeers; i++)
      {
        l1::vnodeid *l2r_peer = l2r->add_sg_peers();
        dkresult = DHTKey::serialize(peers.at(i)->_idkey);
        std::string key_str(dkresult.begin(),dkresult.end());
        dkresult.clear();
        l1::dht_key *l2r_peer_key = l2r_peer->mutable_key();
        l2r_peer_key->set_key(key_str);
        l1::net_address *l2r_peer_addr = l2r_peer->mutable_addr();
        l2r_peer_addr->set_ip_addr(peers.at(i)->getNetAddress());
        l2r_peer_addr->set_net_port(peers.at(i)->getPort());
      }
    return l2r;
  }

  void l2_protob_wrapper::serialize_to_string(const l2::l2_subscribe_response *l2r, std::string &str) throw (dht_exception)
  {
    if (!l2r)
      {
        errlog::log_error(LOG_LEVEL_ERROR, "can't serialize null subscribe response");
        throw dht_exception(DHT_ERR_MSG,"can't serialize null subscribe response");
      }
    if (!l2r->SerializeToString(&str))
      throw dht_exception(DHT_ERR_MSG,"l2::SerializeToString");
  }

  void l2_protob_wrapper::read_l2_subscribe_response(const l2::l2_subscribe_response *l2r,
      uint32_t &error_status,
      std::vector<Subscriber*> &peers)
  {
    error_status = l2r->error_status();
    int npeers = l2r->sg_peers_size();
    peers.reserve(npeers);
    std::vector<unsigned char> ser;
    for (int i=0; i<npeers; i++)
      {
        l1::vnodeid l2r_peer = l2r->sg_peers(i);
        l1::dht_key l2r_key = l2r_peer.key();
        std::string key_str = l2r_key.key();
        std::copy(key_str.begin(),key_str.end(),std::back_inserter(ser));
        DHTKey skey = DHTKey::unserialize(ser);
        ser.clear();
        l1::net_address l2r_addr = l2r_peer.addr();
        std::string ip_addr = l2r_addr.ip_addr();
        uint32_t net_port = l2r_addr.net_port();
        Subscriber *su = new Subscriber(skey,ip_addr,net_port);
        peers.push_back(su);
      }
  }

  void l2_protob_wrapper::deserialize(const std::string &str, l2::l2_subscribe_response *l2r) throw (dht_exception)
  {
    if (!l2r)
      {
        errlog::log_error(LOG_LEVEL_ERROR, "can't deserialize to null response");
        throw dht_exception(DHT_ERR_MSG,"can't deserialize to null response");
      }
    if (!l2r->ParseFromString(str))
      throw dht_exception(DHT_ERR_MSG,"l2::ParseFromString(" + str + ")");
  }

} /* end of namespace. */
