//-*-C++-*------------------------------------------------------------
// NAME: Template Classes
// NOTE: A basic graph class
// AUTH: Yang, Qi
// FILE: Graph.cc
// DATE: Thu Sep 28 18:02:13 1995
//--------------------------------------------------------------------

#ifndef GRAPH_IMPLEMENTATION
#define GRAPH_IMPLEMENTATION

#include "Graph.h"
#include <assert.h>

template <class T, class I>
Graph<T, I>::Graph(const int n, const int m, const T u, const T p)
   :  infinity_(u), downGradePenalty_(p),  scope_(0x7FFF) ,root_(SP_UNSET)
{
#ifdef RogueWave
   nodes_.resize(n);
   links_.resize(m);
#else
   nodes_.reserve(n);
   GraphNode<T, I> dummy_node;
   int i;
   for (i = 0; i < n; i ++) {
	  nodes_.push_back(dummy_node);
   }

   links_.reserve(m);
   GraphLink<T, I> dummy_link;
   for (i = 0; i < m; i ++) {
	  links_.push_back(dummy_link);
   }
#endif
}



//--------------------------------------------------------------------
// Purpose : Add an link to the graph
// Requires: Link i and nodes u and d are indices.
// Modifies: Variables in this link and its end nodes
// Effects : None
//--------------------------------------------------------------------

template <class T, class I>
void Graph<T, I>::addLink(const int i, const int u,const  int d, // required
						  const T w,	// following is optional
						  const char grade,
						  const unsigned int legal,
						  const unsigned int index)
{
   GraphLink<T, I> *arc = link(i);
   arc->upNode_ = u;
   arc->dnNode_ = d;
   arc->cost_ = w;
   arc->grade_ = grade;
   arc->dnLegal_ = legal;
   arc->dnIndex_ = index;

   node(u)->addDnLink(i);
   node(d)->addUpLink(i);
}


template <class T, class I>
void Graph<T, I>::linkCost(const int i, const T w)
{
   link(i)->cost_ = w;
}


//--------------------------------------------------------------------
// Requires: link i and j are connected to the same node
// Modifies: penalties_ of link i.
// Effects : set turning penalty to the movement from link i to j
//--------------------------------------------------------------------

template <class T, class I>
void Graph<T, I>::penalty(const int up, const int dn, const T w)
{
   GraphLink<T, I> *l = link(up);
   GraphNode<T, I> *n = node(l->dnNode_);

   // Create the array at the first time a link calls this function

   if (!(l->penalties_)) {
       int i, num = n->nDnLinks();
      l->penalties_ = new T[num];
      for (i = 0; i < num; i ++) {
		 l->penalties_[i] = 0;
      }
   }

   // Set the penalty to the ith outgoing link

   l->penalties_[link(dn)->dnIndex()] = w;
}


//--------------------------------------------------------------------
// Requires each link has cost_ defined
//
// Modifies predecessor_ of every link, cost_ and predecessor_ of
// every node
//
// Effects : finds the shortest path tree within a given scope
// (defined by sum of link costs from upstream end of the root link
// s).  When finished:
//
// (1) in link array: predecessor_ is the previous link on
//     the shortest path to link s.  cost_ is unchanged.
// 
// (2) in node array: cost_ is minimum cost from link s
//     (start at upstream end) to the node, and predecessor_
//     (link index) is the last link on the path.
// 
// (3) The vertice outside the searching scope is treated
//     as unreachable and has no predecessor defined (SP_UNSET)
//--------------------------------------------------------------------

template <class T, class I>
void Graph<T, I>::labelSetting(const int s, const T scp)
{
   cout << "starting label setting" << endl;
   scope_ = scp;

   int nlinks = nLinks();

   // cost: an array that holds cost from the root link (upstream end)
   // to every link (downstream end)

   T *label = new T[nlinks];

   // array that holds information for a list of links sorted by
   // current label (labe, sorted in ascending order).
   //
   // prev[i]: 
   //   nLinks_  if link i is the last link in the list
   //   SP_UNSET link i is not in the list
   //   j        link j is the next link in the list
   //
   // next[i]:
   //   nLinks_  if link i is the first link in the list
   //   SP_UNSET link i is not in the list
   //   j        link j is the prev link in the list

   int *prev = new int[nlinks];
   int *next = new int[nlinks];
   int front, p, n;

   register int i;
   int num, u, v;
   T newlabel, *penalties;
   GraphNode<T, I> *pivot;
   GraphLink<T, I> *s_link, *p_link;
   GraphLink<T, I> *root = link(s);

   // initialize predecessors and label
   for (i = 0; i < nlinks; i ++) {
      link(i)->predecessor_ = SP_UNSET;
      label[i] = infinity_;
      prev[i] = SP_UNSET;
      next[i] = SP_UNSET;
   }

   label[s] = root->cost_;

   // put root link in the list
   prev[s] = nlinks;		// mark as the first one in list
   next[s] = nlinks;		// mark as the last one in list
   u = s;

   // Dijkstra algorithm starts

   while (u != nlinks) {		// list is not empty

      // take u out from the list
      front = next[u];		// next candidate
      prev[front] = nlinks;

      s_link = link(u);
      pivot =  node(s_link->dnNode_); // downstream node
      penalties = s_link->penalties_; // penalties to dn links 
      num = pivot->nDnLinks(); // number of dn links

      // add dn links to list and modifies their costs if necessary

      for (i = 0; i < num; i ++) { // check each dn links

		 v = pivot->dnLinks_[i];
		 p_link = link(v); // ith dn link

		 // index position as an outgoing link at the upstream node 

		 int k = p_link->dnIndex();

		 // skip an link if connection from s_link to the link is not
		 // allowed.
	 
		 if (!s_link->dnLegal(k)) continue;

		 newlabel = label[u] + p_link->cost_;

		 if (penalties) {
			newlabel += penalties[k];
		 }

		 if (s_link->grade_ < p_link->grade_) {
			newlabel += downGradePenalty_;
		 }

		 if (newlabel < label[v] && // can be better off
			 !(newlabel > scp)) { // in scope

			p_link->predecessor_ = u; // create/update predecessor
			label[v] = newlabel;		// record the label

			if (prev[v] == SP_UNSET) {	// v is not in the list
			   n = front;
			   p = nlinks;
			   while (n != nlinks && label[n] < newlabel) {
				  p = n;
				  n = next[n];
			   }

			   // insert v between p and n
			   prev[v] = p;
			   next[v] = n;
			   if (n != nlinks) prev[n] = v;
			   if (p != nlinks) next[p] = v;
			   else front = v;

			} else if ((p = prev[v]) != nlinks &&
					   label[p] > newlabel) { // need to promote v

			   // remove v from the list

			   next[p] = n = next[v];
			   if (n != nlinks) prev[n] = p;

			   // find new position for v

			   while (p != nlinks && label[p] > newlabel) {
				  n = p;
				  p = prev[p];
			   }

			   // insert v between p and n

			   prev[v] = p;
			   next[v] = n;
			   if (n != nlinks) prev[n] = v;
			   if (p != nlinks) next[p] = v;
			   else front = v;
			}
		 }
      }
      u = front;
   }
   cout << "cleaning up" << endl;
   delete [] prev;
   delete [] next;

   // now array label[i] stores label from link root to link i and
   // predecessor_ of each link stores the path tree.

   // calculate cost_ and predecessor_ in node.

   calcCostToNodes(label);

   // now for each node cost_ is minimum label from link s (starting
   // from the upstream end) to the node, and predecessor_ is the
   // last link on the path.

   // returns the space allocated in this function

   delete [] label;

   root_ = s;			// root link of the current path tree
}


//--------------------------------------------------------------------
// REQUIRES: Costs for every link during each time period are provided
// by info->graph_cost(i, t), where i the index to a link and t is the time
// to enter the link.
//
// CAUTION: If info is not defined, default link cost specified in
// GraphLink is used instead.
//
// MODIFIES: cost_ and predecessor_ of every node, predecessor_ of
// every link.
//
// EFFECTS: Finds the dynamic shortest path tree from link s to every
// link and node.  When finished:

// (1) in link array: cost_ is the minimum label from link s (starting
// from the upstream end) to the end of the link, and predecessor_ is
// the previous link on the shortest path to link s.
// 
// (2) in node array: cost_ is minimum label from link s to the
// node, and predecessor_ is the last link on the path.
//
// NOTE: a modified cost correcting algorithm is used.
// NOTE: links leading to the source link will be removed from
// consideration in order not to generate paths with cycles with 
// respect to nodes.  
//--------------------------------------------------------------------

template <class T, class I>
void Graph<T, I>::labelCorrecting(const int s, const double entry, I *info)
{
   int nlinks = nLinks();

   // label: an array that holds label from the root link (upstream
   // end) to every link (downstream end)

   T *label = new T[nlinks];

   // queue: an array that holds information on the current status of
   // a link with respect to the sequence list:
   //
   // queue[i] =
   //
   //   nlinks   if link i is the last link in the sequence list
   //   SP_UNSET if link i was never in the sequence list
   //   SP_START if link i was in the sequence list but not any more
   //   j        if link i is in the sequence list and link j is the
   //            next one in the list
   //

   int *queue = new int[nlinks];

   int front;			// current first link in list
   int rear;			// current last link in list
   register int i;
   int num, u, v;
   T newlabel, *penalties;
   GraphLink<T, I> *s_link, *p_link;
   GraphLink<T, I> *root = link(s);
   GraphNode<T, I> *pivot;
   int rootnode = root->upNode_;

   // initialize list and costs

   for(i = 0; i < nlinks; i ++) {
      link(i)->predecessor_ = SP_UNSET;
      label[i] = infinity_;
      queue[i] = SP_UNSET;
   }

   // for the root link

   root->predecessor_ = SP_START;
   if (info) {			// cost may be time variant
      label[s] = info->graph_cost(s, entry);
   } else {
      label[s] = root->cost_;
   }
   queue[s] = nlinks;		// mark as the last one in list
   u = rear = s;

   // label correcting starts

   while(u != nlinks) {		// list is not empty
      front = queue[u];		// next candidate
      queue[u] = SP_START;	// was in list but not any more
      s_link = link(u);
      pivot =  node(s_link->dnNode_);
      penalties = s_link->penalties_;
      num = pivot->nDnLinks();
      for (i = 0; i < num; i ++) {

		 v = pivot->dnLinks_[i];
		 p_link = link(v);

		 // disregard links pointing to the source node
		 // in order to avoid generating cyclic paths
		
		 // Wilco Burghout 2001_12_13 I WANT to make paths with U-turns possible (in case of incident)
		 // commented out:
		   
		  if( p_link->dnNode_ == rootnode )
		   continue;
       

		 // index position as an outgoing link at the upstream node

		 int k = p_link->dnIndex();

		 // skip an link if connection from s_link to the link is not
		 // allowed.

		 if (!s_link->dnLegal(k)) continue;

		 if (info) {
			newlabel = label[u] + info->graph_cost(v, entry + label[u]);
		 } else {
			newlabel = label[u] + link(v)->cost_;
		 }

		 if (penalties) {
			newlabel += penalties[k];
		 }

		 if (s_link->grade_ < p_link->grade_) {
			newlabel += downGradePenalty_;
		 }

		 if (newlabel < label[v]) {

			// label need to be corrected

			p_link->predecessor_ = u;
			label[v] = newlabel;
			if(queue[v] == SP_UNSET) {

			   // link v has never been in list

			   queue[rear] = v;	// link rear becomes 2nd from bottom
			   queue[v] = nlinks; // put link v at bottom of list
			   rear = v;	// link v is at bottom of list
			   if (front == nlinks) front = v;

			} else if (queue[v] == SP_START) { // link v was in list
			   queue[v] = front; // link front becomes 2nd in list
			   front = v;	// put link v at the top of list
			}
		 } // finished correcting a label 
      } // finished one outgoing link
      u = front;
   }

   // now array label[i] stores label from link root to link i and
   // predecessor_ of each link stores the path tree.

   // calculate label_ and predecessor_ in node.

   calcCostToNodes(label);
  
   // now for each node cost_ is minimum label from link s (starting
   // from the upstream end) to the node, and predecessor_ is the last
   // link on the path.

   // returns the space used

   delete [] label;
   delete [] queue;

   scope_ = infinity_;		// scope of the current path tree
   root_ = s;			// root link of the current path tree
}


template <class T, class I>
void Graph<T, I>::calcCostToNodes(T *label)
{
   GraphNode<T, I> *pivot;
   T newlabel;
   int p, u, v, i, num;

   for (v = 0; v < nNodes(); v ++) {
      pivot = node(v);

      // find which path to node has the minimum label

      newlabel = infinity_;
      p = SP_UNSET;
      num = pivot->nUpLinks();

      for (i = 0; i < num; i ++) {
		 u = pivot->upLinks_[i];
		 if (label[u] < newlabel) {
			newlabel = label[u];
			p = u;
		 }
      }
      pivot->predecessor_ = p;
      pivot->cost_ = newlabel;
   }
}


//--------------------------------------------------------------------
// Requires: path tree has been built
// Modifies: os
// Effects : print path information from root link to node des
//--------------------------------------------------------------------

template <class T, class I>
void Graph<T, I>::printPathToNode( const int des, ostream &os)
{
   if (root_ == SP_UNSET) return;
   int p = node(des)->predecessor_;
   if (p == SP_UNSET) {
      os << "No path from <" << root_ << "> to [" << des << "]";
      if (scope_ < infinity_) {
		 os << " or [" << des
			<< "] is outside scope (" << scope_ << ")";
      }
      os << endl;
      return;
   }
   GraphLink<T, I> *e;
   os << "Path from <" << root_
      << "> to [" << des << "]" << endl;
   os << "\tTo" << "\tTotal cost" << endl;
   while (p > 0) {
      e = link(p);
      os << "\t"; printLinkCode(p, os);
      os << "\t"; printCost(e->cost_, os);
      p = e->predecessor_;
   }
   os << endl;
}


//--------------------------------------------------------------------
// Requires: path tree has been built
// Modifies: os
// Effects : print path information from root link to link des
//--------------------------------------------------------------------

template <class T, class I>
void Graph<T, I>::printPathToLink(const int des, ostream &os)
{
   if (root_ == SP_UNSET) return;
   int p = link(des)->predecessor_;
   if (p == SP_UNSET) {
      os << "No path from ";
      printLinkCode(root_);
      os << " to ";
      printLinkCode(des);
      if (scope_ < infinity_) {
		 os << " or <" << des
			<< "> is outside scope (" << scope_ << ")";
      }
      os << endl;
      return;
   }
   GraphLink<T, I> *e;
   p = des;
   os << "Path from ";
   printLinkCode(root_);
   os << " to ";
   printLinkCode(des);
   os << endl << "\tTo" << "\tTotal cost" << endl;
   while (p > 0) {
      e = link(p);
      os << "\t"; printLinkCode(p, os);
      os << "\t"; printCost(e->cost_, os);
      p = e->predecessor_;
   }
   os << endl;
}


//--------------------------------------------------------------------
// Requires: path tree has been built
// Modifies: os
// Effects : print path tree information
//--------------------------------------------------------------------

template <class T, class I>
void Graph<T, I>::printLinkPathTree
(const int reachable_only, ostream &os)
{
   if (root_ == SP_UNSET) return;
   int i, p;
   os << "shortest path tree rooted from ";
   printLinkCode(root_);
   if (scope_ < infinity_) {
      os << " in scope (" << scope_ << ")";
   }
   os << " :" << endl;
   os << "\tTo" << "\tPredecessor"
      << "\tTotal cost" << endl;
   GraphLink<T, I> *e;
   for (i = 0; i < nLinks(); i ++) {
      e = link(i);
      p = e->predecessor_;
      if (p == SP_UNSET && reachable_only) continue;
      os << "\t"; printLinkCode(i, os);
      os << "\t"; printLinkCode(p, os);
      os << "\t"; printCost(e->cost_, os);
      os << endl;
   }
}

template <class T, class I>
void Graph<T, I>::printNodePathTree
(const int reachable_only, ostream &os)
{
   if (root_ == SP_UNSET) return;
   int i, p;
   os << "shortest path tree rooted from ";
   printLinkCode(root_);
   if (scope_ < infinity_) {
      os << " in scope (" << scope_ << ")";
   }
   os << " :" << endl;
   os << "\tTo" << "\tPredecessor"
      << "\tTotal label" << endl;
   GraphNode<T, I> *v;
   for (i = 0; i < nNodes(); i ++) {
      v = node(i);
      p = v->predecessor_;
      if (p == SP_UNSET && reachable_only) continue;
      os << "\t[" << i << "]";
      os << "\t"; printLinkCode(p, os);
      os << "\t"; printCost(v->cost_, os);
      os << endl;
   }
}

template <class T, class I>
void Graph<T, I>::printLinkCode(const int i, ostream &os)
{
   if (i == SP_START) os << "<None>";
   else if (i == SP_UNSET) os << "<N/A>";
   else {
      GraphLink<T, I> *e = link(i);
      os << "<" << e->upNode_ << "*"
		 << i << "*" << e->dnNode_ << ">";
   }
}

template <class T, class I>
void Graph<T, I>::printCost(const T &x, ostream &os)
{
   if (x < infinity_) os << x;
   else os << "Inf";
}

// added by wilco burghout 2001-11-13
template<class T>
struct compare_val
{
 compare_val(int id_):id(id_) {}
 bool operator () (T arg)

 	{
 	 return (arg==id);
 	}

 int id;
};


template <class T, class I>
const vector<int> Graph<T, I>::shortest_path_vector(const int des)
{
   //cout << "entered the shortest_path_vector routine " << endl;
   assert (root_ != SP_UNSET);
   int p = node(des)->predecessor_;
   if (p==SP_UNSET)
   {
   	//	cout << " Trouble with shortest path: " << endl;
   		printPathToNode(des);
   		printNodePathTree(des);
   }
   assert (p != SP_UNSET);
   GraphLink<T, I> *e;
	vector <int> linkids;
   while (p >= 0) {
     // cout << "..inserting link: " << p << ", with cost " << link(p)->cost_ << endl;
	  vector<int>::iterator iter = find_if (linkids.begin(),linkids.end(),  compare_val <int> (p) );
	  if ( iter == linkids.end()) // the link is not yet in list
	  {
		  linkids.insert(linkids.begin(),p); // insert link id into vector
		  e = link(p);
          p = e->predecessor_;
	  }
	  else
	  {
			cout << "circular path" << endl;
			linkids.clear();
			p=0;
			return linkids;
	  }
   }
   return linkids;
}

// added by wilco 2004-06-01
template <class T, class I>
void Graph<T, I>::set_downlink_indices()
{
  // for all nodes
   typename vector<GraphNode<T, I> >::iterator n_iter;
  for (n_iter=nodes_.begin(); n_iter<nodes_.end();n_iter++)
  {
    // for all downlinks
     register int i = 0;
     vector<int>::iterator l_iter; 
     for ( l_iter = (*n_iter).dnLinks_.begin(); l_iter< (*n_iter).dnLinks_.end();l_iter++)
     {
       // set link->dnindex=i
       link(*l_iter)->dnIndex_=i;
       i++;
     }

  }
}

template <class T, class I>
const bool Graph<T, I>::set_turning_prohibitor(int inlink, int outlink)
{
  GraphLink<T, I> *l_in = link(inlink);
  GraphLink<T, I> *l_out = link(outlink);
  if (!l_in)
  {
	  eout << " void Graph<T, I>::set_turning_prohibitor " << inlink << " to " <<  outlink << " : inlink missing." << endl;
	  return false;
  }	  
  if (!l_out)
  {
	  eout << " void Graph<T, I>::set_turning_prohibitor " << inlink << " to " <<  outlink << " : outlink missing." << endl;
	  return false;
  }
  assert (l_in && l_out); // to make sure they exist
  int index=-1;
  index = l_out->dnIndex(); // this is the downstream index of this link.
//	eout << " void Graph<T, I>::set_turning_prohibitor " << inlink << " to " <<  outlink << " test index: " << index << endl;
   l_in->dnLegal_=(l_in->dnLegal_) ^ (1 << index);     // set the bit for this turning to 0 in the dnLegal_ variable of inlink.
   if (l_in->dnLegal(index) != 0)
   {
	   eout << " void Graph<T, I>::set_turning_prohibitor " << inlink << " to " <<  outlink << " problem setting turning penalty as dnLegal for the l_in is not 0, but " << l_in->dnLegal(index) << endl;
	   return false;
   }
  assert (l_in->dnLegal(index) == 0); // to check all went fine with the shaky bitshifting :) 
  return true;
}

#endif // GRAPH_IMPLEMENTATION
