/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "qc_sg.h"
#include "qprocess.h"
#include "seeks_proxy.h" // for SGNode.
#include "SGNode.h"
#include "subscriber.h"
#include "Location.h"
#include "qprocess.h"
#include "sg_api.h"
#include "errlog.h"

using sp::seeks_proxy;
using sp::errlog;
using dht::SGNode;
using dht::Subscriber;
using dht::Location;
using dht::sg_api;
using lsh::qprocess;

namespace seeks_plugins
{
   short find_sg_arg::_max_trials = 3; // max number of trials to find a search group host (shouldn't fail unless the DHT is not well stabilized...).
   
   find_sg_arg::find_sg_arg(const DHTKey &sg_key,
			    const int &radius,
			    qc_sg *qc)
     :_sg_key(sg_key),_radius(radius),_qc(qc),_trials(0)
     {
     }
   
   find_sg_arg::~find_sg_arg()
     {
     }
   
   short subscribe_sg_arg::_max_trials = 5; // max number of update trials (to get search group peers).
   
   subscribe_sg_arg::subscribe_sg_arg(sg *sg,
				      qc_sg *qc)
     :_sg(sg),_qc(qc),_trials(0)
       {
       }
   
   subscribe_sg_arg::~subscribe_sg_arg()
     {
     }
      
   qc_sg::qc_sg()
     :query_context()
       {
       }
   
   qc_sg::qc_sg(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
		const std::list<const char*> &http_headers)
     :query_context(parameters,http_headers)
       {
	  _subscription = websearch::_wconfig->_sg_subscription; // configuration is default.
       }
   
   qc_sg::~qc_sg()
     {
	clear_sgs();
     }
   
   sg* qc_sg::find_sg(const DHTKey *key)
     {
	hash_map<const DHTKey*,sg*,hash<const DHTKey*>,eqdhtkey>::const_iterator hit;
	if ((hit=_sgs.find(key))!=_sgs.end())
	  return (*hit).second;
	else return NULL;
     }
      
   int qc_sg::add_sg(sg *s)
     {
	_sgs.insert(std::pair<const DHTKey*,sg*>(&s->_sg_key,s));
	return 1;
     }
   
   void qc_sg::clear_sgs() //TODO: locks & mutexes.
     {
	hash_map<const DHTKey*,sg*,hash<const DHTKey*>,eqdhtkey>::iterator hit
	  = _sgs.begin();
	while(hit!=_sgs.end())
	  {
	     sg *s = (*hit).second;
	     ++hit;
	     delete s;
	  }
     }
      
   void qc_sg::subscribe()
     {
	// compute query fragments as search group ids.
	size_t expansion = 0; //TODO: more granularity with expansion.
	hash_multimap<uint32_t,DHTKey,id_hash_uint> qf;
	while(expansion <= _page_expansion
	      && !qf.empty())
	  {
	     qf.clear();
	     qprocess::generate_query_hashes(_query,expansion,expansion,qf);
	     
	     // schedule calls to find search groups.
	     struct timeval tv;
	     gettimeofday(&tv,NULL); // now.
	     hash_multimap<uint32_t,DHTKey,id_hash_uint>::const_iterator hit = qf.begin();
	     while(hit!=qf.end())
	       {
		  schedule_find_sg((*hit).second,(*hit).first,tv);
		  _qf.insert(std::pair<uint32_t,DHTKey>((*hit).first,(*hit).second));
		  ++hit;
	       }
	     	     
	     expansion++;
	  }
     }
      
   void qc_sg::schedule_find_sg(DHTKey key, const int &radius,
				const struct timeval &tv)
     {
	find_sg_arg *fsa = new find_sg_arg(key,radius,this);
	schedule_find_sg(fsa,tv);
     }
   
   void qc_sg::schedule_find_sg(find_sg_arg *fsa,
				const struct timeval &tv)
     {
	qc_event::add_callback_timer(&qc_sg::find_sg_cb,fsa,tv);
     }
      
   void qc_sg::find_sg_cb(int fd, short event, void *arg) //TODO: threaded version...
     {
	find_sg_arg *fsa = static_cast<find_sg_arg*>(arg);
	Location node;
	dht_err err = sg_api::find_sg(*seeks_proxy::_dhtnode,fsa->_sg_key,node);
	if (err != DHT_ERR_OK)
	  {
	     // error: schedule a retry, up to x retries.
	     if (fsa->_trials < find_sg_arg::_max_trials)
	       {
		  fsa->_trials++;
		  struct timeval tv;
		  gettimeofday(&tv,NULL);
		  tv.tv_sec += websearch::_wconfig->_sg_retry_delay;
		  fsa->_qc->schedule_find_sg(fsa,tv);
	       }
	     return;
	  }
	
	// add newsgroup if it does not already exist.
	sg *nsg = fsa->_qc->find_sg(&fsa->_sg_key);
	if (!nsg)
	  {
	     nsg = new sg(fsa->_sg_key,node,fsa->_radius);
	     fsa->_qc->add_sg(nsg);
	  }
	else
	  {
	     // replace sg (this could happen in case of host failure).
	     if (nsg->_host != node)
	       {
		  nsg->_host = node;
		  nsg->clear_peers(); // XXX: clear the already known peers, this is not mandatory though.
	       }
	  }
			
	// schedule a call to get peers.
	struct timeval tv;
	gettimeofday(&tv,NULL); // now.
	fsa->_qc->schedule_subscribe_sg(nsg,tv);
	
	delete fsa;
     }
      
   void qc_sg::schedule_subscribe_sg(sg *nsg, const struct timeval &tv)
     {
	subscribe_sg_arg *ssa = new subscribe_sg_arg(nsg,this);
	schedule_subscribe_sg(ssa,tv);
     }
   
   void qc_sg::schedule_subscribe_sg(subscribe_sg_arg *ssa,
				     const struct timeval &tv)
     {
	qc_event::add_callback_timer(&qc_sg::subscribe_cb,ssa,tv);
     }
      
   void qc_sg::subscribe_cb(int fd, short event, void *arg) //TODO: threaded version...
     {
	subscribe_sg_arg *ssa = static_cast<subscribe_sg_arg*>(arg);
	std::vector<Subscriber*> peers;
	dht_err err = DHT_ERR_OK;
	if (ssa->_qc->_subscription)
	  err = sg_api::get_sg_peers_and_subscribe(*seeks_proxy::_dhtnode,ssa->_sg->_sg_key,
						   ssa->_sg->_host,peers);
	else
	  err = sg_api::get_sg_peers(*seeks_proxy::_dhtnode,ssa->_sg->_sg_key,
				     ssa->_sg->_host,peers);
	if (err != DHT_ERR_OK)
	  {
	     // error: schedule a retry, if all fail, try to find search group host again.
	     if (ssa->_trials < subscribe_sg_arg::_max_trials)
	       {
		  ssa->_trials++;
		  struct timeval tv;
		  gettimeofday(&tv,NULL);
		  tv.tv_sec += websearch::_wconfig->_sg_retry_delay;
		  ssa->_qc->schedule_subscribe_sg(ssa,tv);
	       }
	     else // schedule calls to find search group as it may have changed location.
	       {
		  struct timeval tv;
		  gettimeofday(&tv,NULL); // now.
		  ssa->_qc->schedule_find_sg(ssa->_sg->_sg_key,ssa->_sg->_radius,tv);
		  delete ssa;
	       }
	     return;
	  }
	
	ssa->_sg->add_peers(peers);
		
	// re-schedule a call to get peers.
	struct timeval tv;
	gettimeofday(&tv,NULL);
	tv.tv_sec += websearch::_wconfig->_sg_update_delay;
	ssa->_qc->schedule_subscribe_sg(ssa->_sg, tv);
     
	delete ssa;
     }
      
} /* end of namespace. */
