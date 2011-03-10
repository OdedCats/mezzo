/*
	Mezzo Mesoscopic Traffic Simulation 
	Copyright (C) 2008  Wilco Burghout

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GRAPH_HEADER
#define GRAPH_HEADER

#include <iostream>
#include <vector>
using namespace std;
//#include "STL_Configure.h"

enum { SP_UNSET = -2, SP_START = -1 };

template <class T, class I> class Graph;
template <class T, class I> class GraphNode;
template <class T, class I> class GraphLink;

// The time dependent shortest path algorithm requires an information
// object (I) provides the time (in)variant cost on each link.  The
// following dummy class can be used bypass this requirement.

template <class T>
class GraphNoInfo
{
   public:
      GraphNoInfo() { }
      ~GraphNoInfo() { }
      T cost(int, double) { return 1; }
};

template <class T, class I>
class GraphNode
{
      friend class GraphLink<T, I>;
      friend class Graph<T, I>;

   protected:

      vector<int /*ALLOCATOR*/> upLinks_;
      vector<int /*ALLOCATOR*/> dnLinks_;

      // These are the output of graph algorithms.

      int predecessor_;
      T cost_;

   public:

      GraphNode() : predecessor_(SP_UNSET), cost_(0) { }
      virtual ~GraphNode() { }

      inline void addUpLink(const int up) {
		 upLinks_.push_back(up);
      }
      inline void addDnLink(const int dn) {
		 dnLinks_.push_back(dn);
      }

      inline  const int nUpLinks() const { return upLinks_.size(); }
      inline  const int nDnLinks() const { return dnLinks_.size(); }

      inline const int predecessor() const { return predecessor_; }
      inline const T& cost() { return cost_; }

      // The caller is responsible for index boundaries

      inline const int upLink( const int i) const { return upLinks_[i]; }
      inline const int dnLink( const int i) const { return dnLinks_[i]; }
};


template <class T, class I>
class GraphLink
{
      friend class GraphNode<T, I>;
      friend class Graph<T, I>;

   protected:

      // pointers to connected nodes.

      int upNode_;
      int dnNode_;

      T *penalties_;		// turning penalties to dnLinks
      char grade_;		// link grade (0=freeway 1=otherwise)

      char dnLegal_;		// is downstream links connected
      char dnIndex_;		// index as an downstream link

      T cost_;			// default cost
      int predecessor_;		// for storing path

   public:

      GraphLink() :
		 upNode_(SP_UNSET), dnNode_(SP_UNSET),
		 penalties_(NULL), predecessor_(SP_UNSET) {
	  }

      virtual ~GraphLink() {
		 if (penalties_) {
			delete [] penalties_;
		 }
      }

      inline const int upNode() const { return upNode_; }
      inline const int dnNode() const { return dnNode_; }

	  inline const int predecessor() const { return predecessor_; }

      inline const char dnLegal( const int i) const {
		 return (dnLegal_ & (1 << i)); // the ith bit
      }
      inline const char dnIndex() const { return dnIndex_; }

      inline const T& penalty( const int i) const { return penalties_[i]; }
      inline const T& cost() const { return cost_; }
};


template <class T, class I>
class Graph
{
   protected:
      
      vector<GraphNode<T, I> /*ALLOCATOR*/> nodes_;
      vector<GraphLink<T, I> /*ALLOCATOR*/> links_;

      T	infinity_;		// a value represents infinity large
      T downGradePenalty_;	// for change to a lower grade link
      T	scope_;			// maximum range in label setting

   private:

      void calcCostToNodes(T *);
      int root_;		// current root link

   public:

      Graph(const int n = 2, const int m = 1,
			const T u = 0x7FFF / 3,
			const T p = 0);
      virtual ~Graph() {}

     const int nNodes() const { return nodes_.size(); }
     const int nLinks() const { return links_.size(); }
  
      // Pointer to the objects in array (in this class)

      GraphNode<T, I> *node(const int i) { return &nodes_[i]; }
      GraphLink<T, I> *link(const int i) { return &links_[i]; }

      // For ith node

      inline const T& nodeCost(const int i) const {
		 return nodes_[i].cost();
      }
      inline const T& costToNode(const int i) const {
		 return nodeCost(i);
      }
      inline const int predecessorToNode(const int i) const {
		 return  nodes_[i].predecessor();
      }

      // For ith link

      inline const T& linkCost(const int i) const {
		 return links_[i].cost();
      }
      inline const T& costToLink(const int i) const {
		 return linkCost(i);
      }
      inline const int predecessorToLink(const int i) const {
		 return links_[i].predecessor();
      }

      // Initialize a link. The last arg legal indicates wether this
      // link can connect to a downstream link at the node.  Each bit
      // of legal corresponds to one downstream link, with the
      // first bit (from the right most) corresponds to the first
      // downstream link.

      void addLink(const int i, const int u,const int d,
				   const T w = 1, const char grade = 0,
				  const char legal = (char) 0xFF, 
				   const char index = 0);

      // Update the link cost

      void linkCost(const int i, const T w);

      // Add or update the turning penalty

      void penalty(const int i, const int j, const T w);

      // Calculating one-to-many dynamic shortest path (link-based)

      void labelCorrecting(const int root, const double entry = 0, I *info = NULL);

      // Searching links within a given scope (link-based, using the
      // default link cost)
      
      void labelSetting(const int root, const T scp = 0x7FFF);

      // Print path information for debugging

      void printPathToNode(const int v, ostream &os = cout);
      void printPathToLink(const int e, ostream &os = cout);
      void printNodePathTree(const int ro = 1, ostream &os = cout);
      void printLinkPathTree(const int ro = 1, ostream &os = cout);
      void printLinkCode(const int i, ostream &os = cout);
      void printCost(const T &x, ostream &os = cout);

		// added by Wilco Burghout 2001-11-13
		const vector<int> shortest_path_vector(const int des);
		inline const bool reachable(const int des)
			{return (node(des)->predecessor_!=SP_UNSET);}
      // added by Wilco 2004_06_01
      void set_downlink_indices();
      const bool set_turning_prohibitor(const int inlink, const int outlink);
};

#include "Graph.cpp"

#endif
