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

      inline void addUpLink(int up) {
		 upLinks_.push_back(up);
      }
      inline void addDnLink(int dn) {
		 dnLinks_.push_back(dn);
      }

      inline short int nUpLinks() { return upLinks_.size(); }
      inline short int nDnLinks() { return dnLinks_.size(); }

      inline int predecessor() { return predecessor_; }
      inline T& cost() { return cost_; }

      // The caller is responsible for index boundaries

      inline int upLink(short int i) { return upLinks_[i]; }
      inline int dnLink(short int i) { return dnLinks_[i]; }
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

      inline int upNode() { return upNode_; }
      inline int dnNode() { return dnNode_; }

      inline int predecessor() { return predecessor_; }

      inline char dnLegal(short int i) {
		 return (dnLegal_ & (1 << i)); // the ith bit
      }
      inline char dnIndex() { return dnIndex_; }

      inline T& penalty(short int i) { return penalties_[i]; }
      inline T& cost() { return cost_; }
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

      int nNodes() const { return nodes_.size(); }
      int nLinks() const { return links_.size(); }
  
      // Pointer to the objects in array (in this class)

      GraphNode<T, I> *node(int i) { return &nodes_[i]; }
      GraphLink<T, I> *link(int i) { return &links_[i]; }

      // For ith node

      inline T& nodeCost(int i) {
		 return nodes_[i].cost();
      }
      inline T& costToNode(int i) {
		 return nodeCost(i);
      }
      inline int predecessorToNode(int i) {
		 return  nodes_[i].predecessor();
      }

      // For ith link

      inline T& linkCost(int i) {
		 return links_[i].cost();
      }
      inline T& costToLink(int i) {
		 return linkCost(i);
      }
      inline int predecessorToLink(int i) {
		 return links_[i].predecessor();
      }

      // Initialize a link. The last arg legal indicates wether this
      // link can connect to a downstream link at the node.  Each bit
      // of legal corresponds to one downstream link, with the
      // first bit (from the right most) corresponds to the first
      // downstream link.

      void addLink(int i, int u, int d,
				   const T w = 1, char grade = 0,
				   char legal = (char) 0xFF, 
				   char index = 0);

      // Update the link cost

      void linkCost(int i, const T w);

      // Add or update the turning penalty

      void penalty(int i, int j, const T w);

      // Calculating one-to-many dynamic shortest path (link-based)

      void labelCorrecting(int root, double t = 0, I *info = NULL);

      // Searching links within a given scope (link-based, using the
      // default link cost)
      
      void labelSetting(int root, const T scp = 0x7FFF);

      // Print path information for debugging

      void printPathToNode(int v, ostream &os = cout);
      void printPathToLink(int e, ostream &os = cout);
      void printNodePathTree(int ro = 1, ostream &os = cout);
      void printLinkPathTree(int ro = 1, ostream &os = cout);
      void printLinkCode(int i, ostream &os = cout);
      void printCost(T &x, ostream &os = cout);

		// added by Wilco Burghout 2001-11-13
		vector<int> shortest_path_vector(int des);
		inline const bool reachable(int des)
			{return (node(des)->predecessor_!=SP_UNSET);}
      // added by Wilco 2004_06_01
      void set_downlink_indices();
      void set_turning_prohibitor(int inlink, int outlink);
};

#include "Graph.cpp"

#endif
