/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010 Emmanuel Benazera, juban@free.fr
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
#include "net_utils.h"
#include "errlog.h"
#include "proxy_configuration.h"
#include "l1_protob_rpc_server.h"
#include "l1_protob_rpc_client.h"

#include <sys/time.h>

using sp::errlog;
using sp::seeks_proxy;
using sp::proxy_configuration;

namespace dht
{
   std::string DHTNode::_dht_config_filename = "";
   dht_configuration* DHTNode::_dht_config = NULL;
   
   DHTNode::DHTNode(const char *net_addr,
		    const short &net_port)
     : _n_estimate(1),_l1_server(NULL),_l1_client(NULL)
     {
	if (DHTNode::_dht_config_filename.empty())
	  {
#ifdef SEEKS_CONFIGDIR
	     DHTNode::_dht_config_filename = SEEKS_CONFIGDIR "dht-config"; // TODO: changes for packaging.
#else
	     DHTNode::_dht_config_filename = "dht-config";
#endif
	  }
	
	if (!DHTNode::_dht_config)
	  DHTNode::_dht_config = new dht_configuration(_dht_config_filename);
	
	/**
	 * create the location table.
	 */
	_lt = new LocationTable();
	
	// this node net l1 address.
	//_l1_na.setNetAddress(seeks_proxy::_config->_haddr);
	_l1_na.setNetAddress(net_addr);
	//_l1_na.setPort(_dht_config->_l1_port);
	_l1_na.setPort(net_port);
	
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
	     _vnodes.insert(std::pair<const DHTKey*,DHTVirtualNode*>(new DHTKey(dvn->getIdKey()), // memory leak?
								     dvn));
	     	     
	     /**
	      * TODO: register vnode routing structures to the stabilizer:
	      * - stabilize on successor continuously,
	      * - stabilize more slowly on the rest of the finger table.
	      */
	     _stabilizer->add_fast(dvn->getFingerTable());
	     _stabilizer->add_slow(dvn->getFingerTable());
	  }
	
	/**
	 * start rpc client & server.
	 */
	_l1_server = new l1_protob_rpc_server(net_addr,net_port,this);
	
	/**
	 * run the server in its own thread.
	 */
	rpc_server::run_static(_l1_server);
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

   dht_err DHTNode::join_start(std::vector<NetAddress> &bootstrap_nodelist,
			       bool &reset)
     {
	// try location table if possible/asked to.
	if (!reset || _lt->is_empty())
	  {
	     hash_map<const DHTKey*, Location*, hash<const DHTKey*>, eqdhtkey>::const_iterator lit
	       = _lt->_hlt.begin();
	     while(lit!=_lt->_hlt.end())
	       {
		  Location *loc = (*lit).second;
		  
		  // TODO: test location beforehand.
	     	  // 
		  // join.
	     	  dht_err status = join(loc->getNetAddress(),loc->getDHTKey());
		  if (status == DHT_ERR_OK)
		    return status; // we're done, join was successful, stabilization will take over the job.
		  		  
		  ++lit;
	       }
	  }
	
	// try to bootstrap from the nodelist in configuration.
	std::vector<NetAddress>::const_iterator nit = DHTNode::_dht_config->_bootstrap_nodelist.begin();
	while(nit!=DHTNode::_dht_config->_bootstrap_nodelist.end())
	  {
	     NetAddress na = (*nit);
	     
	     // TODO: test address beforehand.
	      
	     // join.
	     DHTKey key;
	     dht_err status = join(na,key); // empty key is handled by the called node.
	     if (status == DHT_ERR_OK)
	       return status; // we're done, join was successful, stabilization will take over the job.
	     
	     ++nit;
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
	if (status == -1)  // TODO: ???
	  status = DHT_ERR_OK;
	
	return status;
     }
      
   int DHTNode::notify_cb(const DHTKey& recipientKey,
			  const DHTKey& senderKey,
			  const NetAddress& senderAddress,
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
					  DHTKey& dkres_succ, NetAddress &dkres_succ_na,
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
	vnode->findClosestPredecessor(nodeKey, dkres, na, dkres_succ, dkres_succ_na, status);
	return 0;
     }
   
   int DHTNode::joinGetSucc_cb(const DHTKey &recipientKey,
			       const DHTKey &senderKey,
			       DHTKey &dkres, NetAddress &na,
			       int &status)
     {
	// TODO: if recipientKey is not specified, find the closes VNode to senderkey.
	DHTVirtualNode *vnode = NULL;
	if (recipientKey.count() == 0) // test for unspecified recipient.
	  {
	     // TODO.
	  }
	else 
	  {
	     vnode = findVNode(recipientKey);
	     if (!vnode)
	       {
		  dkres = DHTKey();
		  na = NetAddress();
		  status = DHT_ERR_UNKNOWN_PEER;
		  return status;
	       }
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
	     return status;
	  }
	
	status = DHT_ERR_OK;
	return status;
     }
   
   /**-- Main routines using RPCs --**/
   int DHTNode::join(const NetAddress& dk_bootstrap_na,
		     const DHTKey &dk_bootstrap)
     {
	/**
	 * We're basically bootstraping all the virtual nodes here.
	 */
	hash_map<const DHTKey*, DHTVirtualNode*, hash<const DHTKey*>, eqdhtkey>::const_iterator hit
	  = _vnodes.begin();
	while(hit != _vnodes.end())
	  {
	     DHTVirtualNode* vnode = (*hit).second;
	     int status = 0;
	     vnode->join(dk_bootstrap, dk_bootstrap_na, *(*hit).first, status); // TODO: could make a single call instead ?
	     
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
	     for (int j=0;j<rbits;j++)
	       rkey.set(j,DHTKey::irbit2(&iseed));
	     
	     //debug
	     //std::cout << "rkey: " << rkey.to_string() << std::endl;
	     //debug
	     
	     for (int j=0;j<nchars;j++)
	       {
		  std::bitset<8> cbits;
		  short pos = 0;
		  for (int k=j*8;k<(j+1)*8;k++)
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
   
} /* end of namespace. */
