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

#include "SGNode.h"
#include "SGVirtualNode.h"
#include "errlog.h"

using sp::errlog;

namespace dht
{
  std::string SGNode::_sg_config_filename = "";

  SGNode::SGNode(const char *net_addr, const short &net_port)
    :DHTNode(net_addr,net_port,false)
  {
    if (SGNode::_sg_config_filename.empty())
      {
        SGNode::_sg_config_filename = DHTNode::_dht_config_filename;
      }

    if (!sg_configuration::_sg_config)
      sg_configuration::_sg_config = new sg_configuration(SGNode::_sg_config_filename);

    /* check whether our search groups are in sync with our virtual nodes. */
    if (!_has_persistent_data) // set in DHTNode constructor.
      reset_vnodes_dependent(); // resets data that is dependent on virtual nodes.

    /* init server. */
    start_node();

    /* init sweeper. */
    sg_sweeper::init(&_sgmanager);
  }

  SGNode::~SGNode()
  {
  }

  DHTVirtualNode* SGNode::create_vnode()
  {
    return new SGVirtualNode(_transport, &_sgmanager);
  }

  DHTVirtualNode* SGNode::create_vnode(const DHTKey &idkey,
                                       LocationTable *lt)
  {
    errlog::log_error(LOG_LEVEL_DHT,"loading lt not implemented in SGVirtualNode");
    return new SGVirtualNode(_transport, &_sgmanager, idkey);
  }

  void SGNode::reset_vnodes_dependent()
  {
    // resets searchgroup data, as it is dependent on the virtual nodes.
    _sgmanager.clear_sg_db();
  }

} /* end of namespace. */
