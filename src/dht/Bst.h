/**
 * This file is part of the SEEKS project.
 * Copyright (C) 2006 Emmanuel Benazera, juban@free.fr
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

#ifndef BST_H
#define BST_H

#include <vector>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <ostream>
#include <assert.h>

namespace dht
{
   /**
    * \class Bst
    * \brief binary search tree template: 
    *        - D is the decision class (i.e. the node value).
    *        - S is the stored class (e.g. callbacks, etc...), in a vector.
    */
   template<class D, class S>
     class Bst
     {
      public:
	/**
	 * \brief root/leaf node constructor.
	 * @param val, decision value.
	 * @param data node data.
	 * //@param fnode father node, if any, NULL otherwise.
	 */
	Bst(const D& val, const S& data);
	
	/**
	 * \brief node constructor. USELESS ?
	 */
	Bst(const D& val, const S& data, 
	    Bst* rnode, Bst* lnode, Bst* fnode);

	/**
	 * \brief destructor: destroys the local tree node only, no child is deleted.
	 */
	~Bst();
	
	/**
	 * \static destroys the entire subtree, starting from and including bt.
	 */
	static void deleteBst(Bst* bt);
	
	/**
	 * \brief node insertion. This function creates a new node unless
	 *        there already exists a node with the same decision value 
	 *        as the new one.
	 * @param node current node being looked up.
	 * @param val decision value to be added.
	 * @param data data for the new node.
	 * @return a pointer to node.
	 */
	static Bst* insert(Bst* node, const D& val, const S& data);
	
	/**
	 * \brief node value transfer.
	 * @param bt node whose values must be copied to this node.
	 */
	void transfer(Bst* bt);
	
	/**
	 * \brief removal of a node based on a value.
	 */
	static Bst* remove(const D& val, Bst* tree, bool& root);
	
	/**
	 * \brief removal of a node based on a node pointer.
	 * \Warning this method doesn't conserve pointers on tree nodes.
	 */
	static Bst* remove(Bst* bt);
	
	/**
	 * \brief lookup of a decision value.
	 * @param result: 0 means an exact match, 1 means the closest neighbor.
	 */
	Bst* lookup(const D& val, int& result);	
	
	/**
	 * \brief get all elements below a certain D value.
	 * -> must return the upper node (local root).
	 */
	Bst* getAllEltsBelow(const D& val, std::vector<S>& elts);
	Bst* getAllNodesBelow(const D& val, std::vector<Bst*>& nodes);
	
	void getAllElts(std::vector<S>& elts);
	void getAllNodes(std::vector<Bst*>& nodes);

	//tree size.
	unsigned int size();
	unsigned int size(unsigned int& s);
	
	/**
	 * accessors.
	 */
	Bst* getRightChild() const { return _ptR; }
	void setRightChild(Bst* rc) { _ptR = rc; }
	Bst* getLeftChild() const { return _ptL; }
	void setLeftChild(Bst* lc) { _ptL = lc; }
	Bst* getFatherNode() const { return _ptF; }
	void setFatherNode(Bst* fnode) { _ptF = fnode; }
	D& getValue() { return _val; }
	std::vector<S>& getData() { return _data; }
	void add(const S& data) { _data.push_back(data); }

	bool isLeaf() const { return !_ptR && !_ptL; }

	/**
	 * print.
	 */
      public:
	std::ostream& print(std::ostream& output);
	static std::ostream& printBst(std::ostream& output, Bst* bt);
	
      protected:
	/**
	 * Right subtree.
	 */
	Bst* _ptR;
	
	/**
	 * Left subtree.
	 */
	Bst* _ptL;

	/**
	 * father node.
	 */
	Bst* _ptF;
	
	/**
	 * decision value (e.g. int, time_t, etc...).
	 */
	D _val;
     
	/**
	 * data (e.g. callbacks, etc...).
	 */
	std::vector<S> _data;  //all nodes have data.
     };
   
   template<class D, class S>
     Bst<D,S>::Bst(const D& value, const S& data)
       : _ptR(NULL), _ptL(NULL), _ptF(NULL), _val(value)
	 {
	    _data.push_back(data);
	 }
   
   template<class D, class S>
     Bst<D,S>::Bst(const D& value, const S& data,
		   Bst *rnode, Bst* lnode, Bst* fnode)
       : _ptR(rnode), _ptL(lnode), _ptF(fnode), _val(value)
	 {
	    _data.push_back(data);
	 }

   template<class D, class S>
     Bst<D,S>::~Bst<D,S>()
       {
       }

   /**
    * tree destructor: static procedure that deletes the argument node and its subtree.
    * Warning: argument pointer points to NULL on procedure termination.
    */
   template<class D, class S>
     void Bst<D,S>::deleteBst(Bst* bt)
       {
	  Bst* pright = bt->getRightChild();
	  Bst* pleft = bt->getLeftChild();
	  
	  delete bt;
	  bt = NULL;
	  
	  if (pright)
	    Bst<D,S>::deleteBst(pright);
	  if (pleft)
	    Bst<D,S>::deleteBst(pleft);
       }
   
   template<class D, class S>
     Bst<D,S>* Bst<D,S>::insert(Bst* node, const D& val, const S& data)
       {
	  //debug
	  /* std::cout << "[Debug]:Bst::insert: val: " << val 
	   << " -- node: " << node << std::endl; */
	  //debug
	  
	  if (!node)
	    {
	       node = new Bst(val, data);
	    }
	  else if (val == node->getValue())
	    {
	       node->add(data);
	    }
	  else if (val < node->getValue())
	    {
	       node->setLeftChild(Bst<D,S>::insert(node->getLeftChild(), val, data));
	       node->getLeftChild()->setFatherNode(node);
	    }
	  else 
	    {
	       node->setRightChild(Bst<D,S>::insert(node->getRightChild(), val, data));
	       node->getRightChild()->setFatherNode(node);
	    }
	  return node;
       }

   template<class D, class S>
     void Bst<D,S>::transfer(Bst* bt)
       {
	  _val = bt->getValue();
	  _data.clear(); //Warning: make sure there are no leaks here.
	  _data = bt->getData();
       }
   
   template<class D, class S>
     Bst<D,S>* Bst<D,S>::remove(const D& val, Bst* tree, bool& root)
       {
	  int result = -1;
	  Bst* bt = tree->lookup(val, result);
	  root = false;
	  
	  if (tree == bt)
	    {
	       root = true;
	    }
	  
	       
	  if (result)
	    {
	       //Warning: we need printing support through operator here.
	       std::cout << "[Warning]:Bst::remove can't find val: "
		 << val << " in tree. Skipping removal.\n";
	       return NULL;
	    }
	  
	  return Bst<D,S>::remove(bt);
       }

   /**
    * \Warning: does not conserve the pointers !
    */
   template<class D, class S>
     Bst<D,S>* Bst<D,S>::remove(Bst* bt)
       {
	  Bst* temp = bt;
	  Bst* fnode = bt->getFatherNode();
	  if (bt->getLeftChild() == NULL)
	    {
	       bt = bt->getRightChild();
	       if (bt)
		 bt->setFatherNode(fnode);
	       if (fnode)
		 {
		    assert(fnode != bt);
		    if (fnode->getRightChild() == temp)
		      fnode->setRightChild(bt);
		    else 
		      fnode->setLeftChild(bt);
		 }
	       delete temp;
	       temp = NULL;
	    }
	  else if (bt->getRightChild() == NULL)
	    {
	       bt = bt->getLeftChild();
	       if (bt)
		 bt->setFatherNode(fnode);
	       if (fnode)
		 {
		    assert(fnode != bt);
		    if (fnode->getRightChild() == temp)
		      fnode->setRightChild(bt);
		    else fnode->setLeftChild(bt);
		 }
	       delete temp;
	       temp = NULL;
	    }
	  else
	    {
	       temp = bt->getLeftChild();
	       while(temp->getRightChild())
		 temp = temp->getRightChild();
	       bt->transfer(temp);	       
	       Bst<D,S>::remove(temp);
	    }
	  return bt;
       }
   
   template<class D, class S>
     Bst<D,S>* Bst<D,S>::lookup(const D& val, int& result)
       {
	  //Warning: we need an equality operator!
	  if (val == _val)
	    {
	       result = 0;
	       return this;
	    }
	  else if (val < _val)
	    {
	       if (isLeaf())
		 {
		    result = 1;
		    return this;
		 }
	       else return _ptL->lookup(val, result);
	    }
	  else 
	    {
	       if (isLeaf())
		 {
		    result = 1;
		    return this;
		 }
	       else return _ptR->lookup(val, result);
	    }
       }

   template<class D, class S>
     Bst<D,S>* Bst<D,S>::getAllEltsBelow(const D& val, std::vector<S>& elts)
       {
	  if (_val <= val)
	    {
	       typename std::vector<S>::iterator sit = _data.begin();
	       while (sit != _data.end())
		 {
		    elts.push_back((*sit));
		    sit++;
		 }
	       if (_ptL)
		 _ptL->getAllElts(elts);
	       if (_ptR && _ptR->getValue() <= val)
		 _ptR->getAllEltsBelow(val, elts);
	       return this;
	    }
	  else 
	    {
	       if (_ptL)
		 return _ptL->getAllEltsBelow(val, elts);
	       return NULL;
	    }
       }
   
   template<class D, class S>
     Bst<D,S>* Bst<D,S>::getAllNodesBelow(const D& val, std::vector<Bst*>& nodes)
       {
	  if (_val <= val)
	    {
	       nodes.push_back(this);
	       if (_ptL)
		 _ptL->getAllNodes(nodes);
	       if (_ptR && _ptR->getValue() <= val)
		 _ptR->getAllNodesBelow(val, nodes);
	       return this;
	    }
	  else
	    {
	       if (_ptL)
		 return _ptL->getAllNodesBelow(val, nodes);
	       return NULL;
	    }
       }
   
   template<class D, class S>
     void Bst<D,S>::getAllElts(std::vector<S>& elts)
       {
	  typename std::vector<S>::iterator sit = _data.begin();
	  while (sit != _data.end())
	    {
	       elts.push_back((*sit));
	       sit++;
	    }
	  if (_ptL)
	    _ptL->getAllElts(elts);
	  if (_ptR)
	    _ptR->getAllElts(elts);
       }

   template<class D, class S>
     void Bst<D,S>::getAllNodes(std::vector<Bst*>& nodes)
       {
	  nodes.push_back(this);
	  if (_ptL)
	    _ptL->getAllNodes(nodes);
	  if (_ptR)
	    _ptR->getAllNodes(nodes);
       }
      
   template<class D, class S>
     unsigned int Bst<D,S>::size()
       {
	  unsigned int sz = 1;
	  return size(sz);
       }
   
   template<class D, class S>
     unsigned int Bst<D,S>::size(unsigned int& s)
       {  
	  if (_ptL)
	    {
	       s++;
	       _ptL->size(s);
	    }
	  if (_ptR)
	    {
	       s++;
	       _ptR->size(s);
	    }
	  return s;
       }
   
   template<class D, class S>
     std::ostream& Bst<D,S>::print(std::ostream& output)
       {
	  output << "*** Node: " << this << " ***\n";
	  output << "right node: " << _ptR << std::endl;
	  output << "left node: " << _ptL << std::endl;
	  output << "father node: " << _ptF << std::endl;
	  output << "data: ";
	  std::copy(_data.begin(), _data.end(), std::ostream_iterator<S>(output, ""));
	  return output;
       }
   
   template<class D, class S>
     std::ostream& Bst<D,S>::printBst(std::ostream& output, Bst<D,S>* bt)
       {
	  bt->print(output);
	  output << std::endl;
	  if (bt->getRightChild())
	    Bst<D,S>::printBst(output, bt->getRightChild());
	  if (bt->getLeftChild())
	    Bst<D,S>::printBst(output, bt->getLeftChild());
	  return output;
       }
   
   
} /* end of namespace. */

#endif
