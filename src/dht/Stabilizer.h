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

/**
 * \brief This is a stabilizer for some of the DHT internal structures that need
 *        to be periodically updated with regard to the changes on the overlay network.
 *        This is inspired from the same mechanism in Chord.
 *        It uses a binary search tree and a threaded loop to call on functions at more
 *        or less (thread dependent) the correct planned date.
 * 
 *        We're scheduling operations on all the virtual nodes at once.
 * 
 * \author E. Benazera, juban@free.fr
 */

#ifndef STABILIZER_H
#define STABILIZER_H

#include "BstTimeCb.h"

namespace dht
{
   class Stabilizable
     {
      public:
	Stabilizable(); 
	
	virtual ~Stabilizable() {};
	
	virtual void stabilize_fast() {};
	
	virtual void stabilize_slow() {};
	
	virtual bool isStable() { return false; }
	
	void stabilize_fast_ct();
	
	void stabilize_slow_ct();
	
	int getStabilizingFast() const { return _stabilizing_fast; }
	
	int getStabilizingSlow() const { return _stabilizing_slow; }
	
	bool isStabilizingFast() const { return _stabilizing_fast > 0; }
	
	bool isStabilizingSlow() const { return _stabilizing_slow > 0; }
	
      protected:
	/**
	 * Indicators of on-going rpcs. Useful for counting
	 * running asynchronous calls.
	 */
	int _stabilizing_fast;
	int _stabilizing_slow;
     };

   class DHTVirtualNode;
   
   class Stabilizer : public BstTimeCbTree 
     {
      public:
	Stabilizer(const bool &start=true);
	
	~Stabilizer();
	
	void add_fast(Stabilizable* stab) { _stab_elts_fast.push_back(stab); }
	void add_slow(Stabilizable* stab) { _stab_elts_slow.push_back(stab); }  
	
	/**
	 * \brief
	 */
	void start_fast_stabilizer();
	
	/**
	 * \brief 
	 */
	void start_slow_stabilizer();
	
	/**
	 * \brief 
	 */
	int fast_stabilize(double tround);
	
	/**
	 * \brief 
	 */
	int slow_stabilize(double tround);

	/**
	 * whether the structure is stable or not.
	 */
	bool isstable_slow() const;
	
	/**
	 * \brief if one elt is stabilizing, return true, else false.
	 */
	bool fast_stabilizing() const;
	bool slow_stabilizing() const;
	
	/**
	 * \brief rejoin scheme for virtual nodes that for some random reason (network problem,
	 * latency, ...) may have left the circle.
	 */
	int rejoin(DHTVirtualNode *vnode);
	
      private:
	/**
	 * vectors of stabilizable structures.
	 * We dissociate the two types of structures so we don't make useless
	 * virtual calls.
	 */
	std::vector<Stabilizable*> _stab_elts_fast;
	
	std::vector<Stabilizable*> _stab_elts_slow;  

	/**
	 * constants. Time values are in milliseconds.
	 * \Warning: these are for now copied from Chord. 
	 */
	static const int _decrease_timer;
	static const double _slowdown_factor;

	static const int _fast_timer_init;
	static const int _fast_timer_base;
	static const int _fast_timer_max;

	static const int _slow_timer_init;
	static const int _slow_timer_base;
	static const int _slow_timer_max;
     };
      
} /* end of namespace. */

#endif
