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

#ifndef QC_SG_H
#define QC_SG_H

#include "stl_hash.h"
#include "DHTKey.h"
#include "query_context.h"
#include "qc_event.h"
#include "sg.h"
#include "websearch.h"

namespace seeks_plugins
{
   class find_sg_arg;
   class subscribe_sg_arg;
   
   class qc_sg : public query_context
     {
      public:
	// dummy constructor.
	qc_sg();
	
	// constructor.
	qc_sg(const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
	      const std::list<const char*> &http_headers);
	  
	// destructor.
	virtual ~qc_sg();
	
	// sweeper.
	//virtual bool sweep_me();
	
	// management of search groups.
	sg* find_sg(const DHTKey *key);
	
	int add_sg(sg *s);
	
	int remove_sg(sg *s); //TODO.
	
	void clear_sgs();
	
	// search group subscription.
	void subscribe();
	void schedule_find_sg(DHTKey key, const int &radius,
			      const struct timeval &tv);
	void schedule_find_sg(find_sg_arg *fsa,
			      const struct timeval &tv);
	void schedule_subscribe_sg(sg *sg, const struct timeval &tv);
	void schedule_subscribe_sg(subscribe_sg_arg *ssa,
				   const struct timeval &tv);
	
	// callbacks.
	static void find_sg_cb(int fd, short event, void *arg); //TODO.
	static void subscribe_cb(int fd, short event, void *arg); //TODO.
	
      public:
	hash_map<const DHTKey*,sg*,hash<const DHTKey*>,eqdhtkey> _sgs; // active (found) search groups (even empty).
	bool _subscription; // whether this context does subscribe to search groups or not.
	hash_multimap<uint32_t,DHTKey,id_hash_uint> _qf; // radius, key table.
     };

   class find_sg_arg : public qc_event_arg
     {
      public:
	find_sg_arg(const DHTKey &sg_key,
		    const int &radius,
		    qc_sg *qc);
	
	~find_sg_arg();
	
	DHTKey _sg_key;
	int _radius;
	qc_sg *_qc;
	short _trials;
	
	static short _max_trials;
     };
        
   class subscribe_sg_arg : public qc_event_arg
     {
      public:
	subscribe_sg_arg(sg *sg,
			 qc_sg *qc);
	
	~subscribe_sg_arg();
	
	sg *_sg;
	qc_sg *_qc;
	short _trials;
	
	static short _max_trials;
     };
      
} /* end of namespace. */

#endif
