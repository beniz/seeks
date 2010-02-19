/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
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
#include "FingerTable.h"
#include "seeks_proxy.h"
#include "proxy_configuration.h"

using sp::seeks_proxy;
using sp::proxy_configuration;

namespace dht
{
   DHTNode::DHTNode()
     : _dht_config(NULL),_n_estimate(1)
     {
	_dht_config_filename = std::string(seeks_proxy::_basedir) + "/dht/dht-config";
	DHTNode::_dht_config = new dht_configuration(_dht_config_filename);
	
	/**
	 * create the location table.
	 */
	_lt = new LocationTable();

	/**
	 * We get the local net interface information.
	 * Beware: TODO: Win32.
	 */
	/* std::string iname = "";
	std::string ipaddr = "";
	if (DHTNetUtils::getLocalNetInterfaceInfo(iname, ipaddr) == -1)
	  {
	     std::cerr << "[Error]:DHTNode: Can't get the local interface and ip address.\n"
	       << "Please specify a network interface or check your system network interfaces.\n"
	       << "Exiting.\n";
	     exit(-1);
	  } */
	
	// this node net l1 address.
	_l1_na.setNetAddress(seeks_proxy::_config->_haddr);
	_l1_na.setPort(_dht_config->_l1_port);
	
	/**
	 * Create stabilizer before structures in vnodes register to it.
	 */
	_stabilizer = new Stabilizer();
	
	/**
	 * create the virtual nodes.
	 * TODO: persistance of vnodes and associated tables.
	 */
	for (unsigned int i=0; i<NVNODES; i++)
	  {
	     /**
	      * creating virtual nodes.
	      */
	     DHTVirtualNode* dvn = new DHTVirtualNode(this);
	     _vnodes[&dvn->getIdKey()] = dvn;
	     
	     /**
	      * TODO: register vnode routing structures to the stabilizer:
	      * - stabilize on successor continuously,
	      * - stabilize more slowly on the rest of the finger table.
	      */
	     _stabilizer->add_fast(dvn->getFingerTable());
	     _stabilizer->add_slow(dvn->getFingerTable());
	  }
	
	/**
	 * start rpc server.
	 */
	//TODO.
	//_rpc_server_thread_id = start_rpc_server(DHTSettings::_default_com_port, logfile);
     }

   DHTNode::~DHTNode()
     {
	delete _lt; //TODO: should clean the table.
		
	hash_map<const DHTKey*,DHTVirtualNode*,hash<const DHTKey*>,eqdhtkey>::iterator hit
	  = _vnodes.begin();
	while(hit!=_vnodes.end())
	  {
	     delete (*hit).second;
	     ++hit;
	  }
     }
      
   /**----------------------------**/
   /**
    * RPC virtual functions (callbacks).
    */
   dht_err DHTNode::getSuccessor_cb(const DHTKey& recipientKey,
				    DHTKey& dkres, NetAddress& na,
				    int& status)
     {
	status = -1;
	
	/**
	 * get virtual node and deal with possible errors.
	 */
	DHTVirtualNode* vnode = findVNode(recipientKey);
	if (!vnode)
	  {
	     dkres = DHTKey();
	     na = NetAddress();
	     status = DHT_ERR_UNKNOWN_PEER;
	     return status;
	  }	
	
	/**
	 * return successor info.
	 */
	DHTKey* dkres_ptr = vnode->getSuccessor();
	if (!dkres_ptr)
	  {
	     //TODO: this node has no successor yet: this means it must have joined 
	     //      the network a short time ago or that it is alone on the network.
	     
	     dkres = DHTKey();
	     na = NetAddress();
	     status = DHT_ERR_NO_SUCCESSOR_FOUND;
	     return status;
	  }
	
	/**
	 * copy found successor key.
	 */
	dkres = *dkres_ptr;
	
	/**
	 * fetch location information.
	 */
	Location* resloc = vnode->findLocation(dkres);
	if (!resloc)
	  {
	     std::cout << "[Error]:RPC_getSuccessor_cb: our own successor is an unknown location !\n"; 
	     na = NetAddress();
	     status = DHT_ERR_UNKNOWN_PEER_LOCATION;
	     return status;
	  }
	
	/**
	 * Setting RPC results.
	 */
	na = resloc->getNetAddress();
	if (status == -1)
	  status = DHT_ERR_OK;
	
	return status;
     }

   dht_err DHTNode::getPredecessor_cb(const DHTKey& recipientKey,
				      DHTKey& dkres, NetAddress& na,
				      int& status)
     {
	status = -1;
	
	/**
	 * get virtual node and deal with possible errors.
	 */
	DHTVirtualNode* vnode = findVNode(recipientKey);
	if (!vnode)
	  {     
	     dkres = DHTKey();
	     na = NetAddress();
	     status = DHT_ERR_UNKNOWN_PEER;
	     return status;
	  }
	
	/**
	 * return predecessor info.
	 */
	DHTKey* dkres_ptr = vnode->getPredecessor();
	if (!dkres_ptr)
	  {
	     dkres = DHTKey();
	     na = NetAddress();
	     status = DHT_ERR_NO_SUCCESSOR_FOUND;
	     return status;
	  }
	
	/**
	 * copy found predecessor key.
	 */
	dkres = *dkres_ptr;
	
	/**
	 * fetch location information.
	 */
	Location* resloc = vnode->findLocation(dkres);
	if (!resloc)
	  {
	     std::cout << "[Error]:RPC_getPredecessor_cb: our own predecessor is an unknown location !\n";
	     na = NetAddress();
	     status = DHT_ERR_UNKNOWN_PEER_LOCATION;
	     return status;
	  }
	
	/**
	 * Setting RPC results.
	 */
	na = resloc->getNetAddress();
	if (status == -1)
	  status = DHT_ERR_OK;
	
	return status;
     }
      
   int DHTNode::notify_cb(const DHTKey& recipientKey,
			  const NetAddress& senderAddress,
			  const DHTKey& senderKey,
			  int& status)
     { 
	status = -1;
	
	/**
	 * get virtual node.
	 */
	DHTVirtualNode* vnode = findVNode(recipientKey);
	if (!vnode)
	  {
	     status = DHT_ERR_UNKNOWN_PEER;
	     return status;
	  }
	
	/**
	 * notifies this node that the argument node (key) thinks it is 
	 * its predecessor.
	 */
	vnode->notify(senderKey, senderAddress);
	status = 0;
	
	return status;
     }
   
   int DHTNode::findClosestPredecessor_cb(const DHTKey& recipientKey,
					  const DHTKey& nodeKey,
					  DHTKey& dkres, NetAddress& na,
					  DHTKey& dkres_succ,
					  int& status)
     {
	status = -1;
	
	/**
	 * get virtual node.
	 */
	DHTVirtualNode* vnode = findVNode(recipientKey);
	if (!vnode)
	  {
	     dkres = DHTKey();
	     na = NetAddress();
	     status = DHT_ERR_UNKNOWN_PEER;
	     return status;
	  }
	
	/**
	 * return closest predecessor.
	 */
	vnode->findClosestPredecessor(nodeKey, dkres, na, dkres_succ, status);	
	return 0;
     }
   
   /**-- Main routines using RPCs --**/
   int DHTNode::join(const NetAddress& na_bootstrap,
		     int& status)
     {
	/**
	 * TODO: we need to get the key from the boostrap node.
	 * This MUST be done through a direct call to the bootstrap peer.
	 */
	DHTKey dk_bootstrap;  /* TODO!!!! */
	
	/**
	 * We're basically bootstraping all the virtual nodes here.
	 */
	hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit
	  = _vnodes.begin();
	while(hit != _vnodes.end())
	  {
	     /**
	      * TODO: join.
	      */
	     DHTVirtualNode* vnode = (*hit).second;
	     vnode->join(dk_bootstrap, *(*hit).first, status);
	     
	     /**
	      * TODO: check on status and reset.
	      */
	  }
	
	/* TODO. */
	return 0;
     }
   
   int DHTNode::find_successor(const DHTKey& recipientKey,
			       const DHTKey& nodeKey,
			       DHTKey& dkres, NetAddress& na)
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
   
   int DHTNode::find_predecessor(const DHTKey& recipientKey,
				 const DHTKey& nodeKey,
				 DHTKey& dkres, NetAddress& na)
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

   int DHTNode::stabilize(const DHTKey& recipientKey)
     {
	/**
	 * get virtual node.
	 */
	DHTVirtualNode* vnode = findVNode(recipientKey);
	if (!vnode)
	  {
	     return 3;
	  }

	int status = vnode->stabilize();
	return status;
     }
   
   
   /**----------------------------**/   
   
   DHTVirtualNode* DHTNode::findVNode(const DHTKey& dk) const
     {
	hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit;
	if ((hit = _vnodes.find(&dk)) != _vnodes.end())
	  return (*hit).second;
	else
	  {
	     std::cout << "[Error]:DHTNode::findVNode: virtual node: " << dk
	       << " is unknown on this node.\n";
	     return NULL;  /* BEWARE, TODO: abort related RPC or return error message. */
	  }
	
     }
   
   
} /* end of namespace. */
