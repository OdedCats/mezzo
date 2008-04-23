
#include "gettime.h"

#include "network.h"
#include <assert.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <set>
//#include <strstream> // OLD include for gcc2

#include "od.h"

//using namespace std;

// initialise the global variables and objects

long int randseed=0;
int vid=0;
VehicleRecycler recycler;    // Global vehicle recycler
double time_alpha=0.2; // smoothing factor for the output link times (uses hist_time & avg_time),
                                                  // 1 = only new times, 0= only old times.

Parameters* theParameters = new Parameters();

// compare is a helper functor (function object) to be used in STL algorithms as
// predicate (for instance in find_if(start_iterator,stop_iterator, Predicate))
// class T is the 'thing' Object of which the id is compared. get_id() should be defined
// for T

template<class T>
struct compare
{
 compare(int id_):id(id_) {}
 bool operator () (T* thing)

 	{
 	 return (thing->get_id()==id);
 	}

 int id;
};



struct compareod
{
 compareod(odval val_):val(val_) {}
 bool operator () (ODpair* thing)

 	{
 	 return (thing->odids()==val);
 	}

 odval val;
};


template<class T>
struct equalmembers
// Tests if the two containers of class T have equal members
// requires class T to have a function bool equals(T);
{
 equalmembers(T base_):base(base_) {}
 bool operator () (T* thing)

 	{
 	 return (thing->equals(base));
 	}

 T base;
};



struct compareroute
{
 compareroute(odval odvalue_):odvalue(odvalue_) {}
 bool operator () (Route* route)
 	{
 	 return (route->check(odvalue.first, odvalue.second)==true);
 	}
 odval odvalue;
};

bool route_less_than (Route* r1, Route* r2) 
// returns true if origin_id of route 1 < origin_id of route 2, or, if they are equal, 
// if destination_id1 < destination_id2
{
	return r1->less_than(r2);
}

bool od_less_than (ODpair* od1, ODpair* od2)
{
	return od1->less_than(od2);
}

bool route_id_less_than (Route* r1, Route* r2)
{
	return ( r1->get_id() < r2->get_id() );
}

Network::Network()
{
  #ifndef _NO_GUI
	drawing=new Drawing();
	#endif //_NO_GUI
	//eventhandler=new EventHandler(*drawing);
  #ifdef _PVM
	communicator=new PVM ("Mezzo", MSG_TAG_ZOOM_MITSIM, MSG_TAG_ZOOM_MEZZO);
  #endif // _NO_PVM

#ifdef _VISSIMCOM
	communicator= new VISSIMCOM("vissimconf.dat");
#endif //_VISSIMCOM
	linkinfo=new LinkTimeInfo();
	eventlist=new Eventlist;
	no_ass_links=0;
}

Network::~Network()
{
	//delete drawing;
	//delete eventhandler;
  //delete graph;
  delete linkinfo;
  delete eventlist;
  //	delete random;
  #ifdef _MIME
	delete communicator;
  #endif //_MIME
	for (vector <Origin*>::iterator iter=origins.begin();iter<origins.end();)
	{			
		delete (*iter); // calls automatically destructor
		iter=origins.erase(iter);	
	}
	for (vector <Destination*>::iterator iter1=destinations.begin();iter1<destinations.end();)
	{			
		delete (*iter1); // calls automatically destructor
		iter1=destinations.erase(iter1);	
	}
	for (vector <Junction*>::iterator iter2=junctions.begin();iter2<junctions.end();)
	{			
		delete (*iter2); // calls automatically destructor
		iter2=junctions.erase(iter2);	
	}
	for (vector <Node*>::iterator iter3=nodes.begin();iter3<nodes.end();)
	{			
		//delete (*iter); // calls automatically destructor
		iter3=nodes.erase(iter3);	
	}
	for (vector <Link*>::iterator iter4=links.begin();iter4<links.end();)
	{			
		delete (*iter4); // calls automatically destructor
		iter4=links.erase(iter4);	
	}
	for (vector <Incident*>::iterator iter5=incidents.begin();iter5<incidents.end();)
	{			
		delete (*iter5); // calls automatically destructor
		iter5=incidents.erase(iter5);	
	}	
	for (vector <ODpair*>::iterator iter6=odpairs.begin();iter6<odpairs.end();)
	{			
		delete (*iter6); // calls automatically destructor
		iter6=odpairs.erase(iter6);	
	}
	for (vector <Route*>::iterator iter7=routes.begin();iter7<routes.end();)
	{			
		delete (*iter7); // calls automatically destructor
		iter7=routes.erase(iter7);	
	}
	for (vector <Sdfunc*>::iterator iter8=sdfuncs.begin();iter8<sdfuncs.end();)
	{			
		delete (*iter8); // calls automatically destructor
		iter8=sdfuncs.erase(iter8);	
	}
	for (vector <Server*>::iterator iter9=servers.begin();iter9<servers.end();)
	{			
		delete (*iter9); // calls automatically destructor
		iter9=servers.erase(iter9);	
	}
	for (vector <Turning*>::iterator iter10=turnings.begin();iter10<turnings.end();)
	{			
		delete (*iter10); // calls automatically destructor
		iter10=turnings.erase(iter10);	
	}
	for (vector <Vtype*>::iterator iter11=vehtypes.vtypes.begin();iter11<vehtypes.vtypes.end();)
	{			
		delete (*iter11); // calls automatically destructor
		iter11=vehtypes.vtypes.erase(iter11);	
	}
    for (vector <TurnPenalty*>::iterator iter12= turnpenalties.begin(); iter12 < turnpenalties.end();)
    {
        delete (*iter12);
        iter12=turnpenalties.erase(iter12);
    }
}

// define all the inputfunctions according to the rules in the
// grammar . Each production rule has its own function

bool Network::readnodes(istream& in)
{
 string keyword;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="nodes:")
 	return false;
 int nr;
 in >> nr;
 for (int i=0; i<nr;i++)
 {
 	if (!readnode(in))
 		return false;
 }
 return true;
}

bool Network::readnode(istream& in)
{
#ifndef _NO_GUI
  stringstream os; // for the formatting of the icon text (such as "j:42" for junction with id 42)
	const int sz=8; // allows for 6-digit id numbers.
	char t[sz];
#endif //_NO_GUI
  char bracket;
  int nid, type;
  double x, y;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readnodes scanner jammed at " << bracket;
  	return false;
  }
   in  >> nid >> type >> x >> y;
   // check nid, type; assert ( !exists (nid) && exists(type) )
   assert ((type < 6) && (type > 0) );
#ifndef _UNSAFE
   //assert  ( (find_if (nodes.begin(),nodes.end(), compare <Node> (nid))) == nodes.end() ); // no node with nid exists
   assert (!nodemap.count(nid));
#endif // _UNSAFE
   if (type==1)
  {
	Origin* optr=new Origin(nid);
#ifndef _NO_GUI
	os << "o:"<< nid << endl;
	os.get(t,sz);
	optr->setxy(x,y);
	NodeIcon* niptr=new NodeIcon(static_cast<int>(x),static_cast<int>(y), optr);
	niptr->settext(t);
	optr->set_icon(niptr);
	drawing->add_icon(niptr);
#endif // _NO_GUI
	nodemap [nid] = optr; // later on take out the vectors. Now we use both map and old vectors
	originmap [nid] = optr;
  	nodes.insert(nodes.begin(),optr);
  	origins.insert(origins.begin(),optr);
#ifdef _DEBUG_NETWORK
  	cout << " origin " << nid;
#endif //_DEBUG_NETWORK
  }
  if (type==2)
  {
    int sid;
    in >>  sid;
    Destination* dptr=NULL;
    if (sid < 0) // why was this clause again? check what server == -1 means...
    	dptr=new Destination(nid);
    else
    {
		assert (servermap.count(sid));
     	//Server* sptr=(*(find_if (servers.begin(),servers.end(), compare <Server> (sid)))) ;
		Server* sptr = servermap [sid];
     	if (sptr!=NULL)
     		dptr=new Destination(nid,sptr);
    }	
    if (dptr==NULL)
    {

      cout << "Read nodes: scanner jammed at destination " << nid << endl;
      return false;
    }
#ifndef _NO_GUI
	os << "d:"<< nid << endl;
	os.get(t,sz);
 	dptr->setxy(x,y);
 	NodeIcon* niptr=new NodeIcon(static_cast<int>(x),static_cast<int>(y),dptr);
	niptr->settext(t);
	dptr->set_icon(niptr);
	drawing->add_icon(niptr);
#endif //_NO_GUI
	nodemap [nid] = dptr; // later on take out the vectors. Now we use both map and old vectors
	destinationmap [nid] = dptr;
  	nodes.insert(nodes.begin(),dptr);
  	destinations.insert(destinations.begin(),dptr);
#ifdef _DEBUG_NETWORK  	
  	 cout << " destination " << nid ;

#endif //_DEBUG_NETWORK  	
  }
  if (type==3)     // JUNCTION
  {
 	Junction* jptr=new Junction(nid);
#ifndef _NO_GUI
	os << "j:"<< nid << endl;
	os.get(t,sz);
 	jptr->setxy(x,y);
	NodeIcon* niptr=new NodeIcon(static_cast<int>(x),static_cast<int>(y),jptr);
	niptr->settext(t);
	jptr->set_icon(niptr);
	drawing->add_icon(niptr);
#endif // _NO_GUI
	nodemap [nid] = jptr; // later on take out the vectors. Now we use both map and old vectors
	junctionmap [nid] = jptr;
	nodes.insert(nodes.begin(),jptr);
	junctions.insert(junctions.begin(),jptr);
#ifdef _DEBUG_NETWORK
   	 cout << " junction " << nid ;
#endif //_DEBUG_NETWORK   	
  }
  if (type==4)
  {
   BoundaryIn* biptr=new BoundaryIn(nid);
#ifndef _NO_GUI
	os << "bi:"<< nid << endl;
	os.get(t,sz);
 	biptr->setxy(x,y);
 	NodeIcon* niptr=new NodeIcon(static_cast<int>(x),static_cast<int>(y),biptr);
	niptr->settext(t);
	biptr->set_icon(niptr);
	drawing->add_icon(niptr);
#endif //_NO_GUI
	nodemap [nid] = biptr; // later on take out the vectors. Now we use both map and old vectors
	boundaryinmap [nid] = biptr;
	originmap [nid] = biptr;

 	nodes.insert(nodes.begin(),biptr);
 	boundaryins.insert(boundaryins.begin(),biptr);
 	origins.insert(origins.begin(),biptr);
#ifdef _DEBUG_NETWORK  	
  	 cout << " boundaryin "  << nid;
#endif //_DEBUG_NETWORK  	
  }

  if (type==5) // BOUNDARY OUT
    // 2002-12-03 NEW: Boundary out has become a junction, turning movements replace the bo actions
  {
	BoundaryOut* jptr=new BoundaryOut(nid);
#ifndef _NO_GUI    
	os << "bo:"<< nid << endl;
	os.get(t,sz);
	jptr->setxy(x,y);  
 	NodeIcon* niptr=new NodeIcon(static_cast<int>(x),static_cast<int>(y),jptr);
	niptr->settext(t);
	jptr->set_icon(niptr);
	drawing->add_icon(niptr);
#endif //_NO_GUI
	nodemap [nid] = jptr; // later on take out the vectors. Now we use both map and old vectors
	boundaryoutmap [nid] = jptr;
	junctionmap [nid] = jptr;

 	nodes.insert(nodes.begin(),jptr);
 	boundaryouts.insert(boundaryouts.begin(),jptr);
 	junctions.insert(junctions.begin(),jptr);
#ifdef _DEBUG_NETWORK  	
  	 cout << " boundaryout " << nid ;
#endif //_DEBUG_NETWORK  	
   }

   in >> bracket;
   if (bracket != '}')
   {
  	cout << "readfile::readnodes scanner jammed at " << bracket;
  	return false;
   }

#ifdef _DEBUG_NETWORK
  cout << "read"<<endl;
#endif //_DEBUG_NETWORK
  return true;
}


bool Network::readsdfuncs(istream& in)
{
 string keyword;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="sdfuncs:")
 	return false;
 int nr;
 in >> nr;
 for (int i=0; i<nr;i++)
 {
 	if (!readsdfunc(in))
 		return false;
 }
 return true;
}

bool Network::readsdfunc(istream& in)

{
  char bracket;
  int sdid=0, type=0, vmax=0, vmin=0, romax=0, romin=0;
  double alpha=0.0, beta=0.0;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsdfuncs scanner jammed at " << bracket;
  	return false;
  }
   in  >> sdid >> type >> vmax >> vmin >> romax;
   if (type ==1)
   		in >> alpha >> beta;
   if (type==2)
   {
   		in  >> romin >> alpha >> beta;
   		//cout << "Dynamit sd func: vmin "<< vmin << " vmax " << vmax << " romax " << romax << "romin " << romin << endl;
   }

   // assert  ( (find_if (sdfuncs.begin(),sdfuncs.end(), compare <Sdfunc> (sdid))) == sdfuncs.end() );
	assert (!sdfuncmap.count(sdid));
    assert ( (vmin>0) && (vmax>=vmin) && (romin >= 0) && (romax>=romin) );
    assert ( (type==0) || (type==1) || (type==2));
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readsdfuncs scanner jammed at " << bracket;
  	return false;
  }
  Sdfunc* sdptr;
  if (type==0)
  {
	  sdptr = new Sdfunc(sdid,vmax,vmin,romax);
  }	else if (type==1)	
  {
	  sdptr = new GenSdfunc(sdid,vmax,vmin,romax,alpha,beta);
  }	else if (type==2)
  {
	  sdptr = new DynamitSdfunc(sdid,vmax,vmin,romax,romin,alpha,beta);
  }
  assert (sdptr);
  sdfuncmap [sdid] = sdptr;
  sdfuncs.insert(sdfuncs.begin(),sdptr );

#ifdef _DEBUG_NETWORK
  cout << " read a sdfunc"<<endl;
#endif //_DEBUG_NETWORK
  return true;
}

bool Network::readlinks(istream& in)
{

 string keyword;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="links:")
 	return false;
 int nr;
 in >> nr;
 for (int i=0; i<nr;i++)
 {
 	if (!readlink(in))
 		return false;
 }
 return true;
}



bool Network::readlink(istream& in)
{
  char bracket;
  int lid, innode, outnode, length,nrlanes,sdid;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readlinks scanner jammed at " << bracket;
  	return false;
  }
   in  >> lid >> innode >> outnode >> length >> nrlanes >> sdid;
   // check lid, vmax, vmin, romax;
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readlinks scanner jammed at " << bracket;
  	return false;
  }
  // find the nodes and sdfunc pointers
  
  assert ( (length>0) && (nrlanes > 0) );           // check that the length and nrlanes are positive
#ifndef _UNSAFE
	//assert  ( (find_if (links.begin(),links.end(), compare <Link> (lid))) == links.end() );    // no link with lid exists
	assert (!linkmap.count(lid));
#endif // _UNSAFE  
/*
  vector <Node*>::iterator iter=nodes.begin();
  iter= find_if (nodes.begin(),nodes.end(), compare <Node> (innode)) ;
  assert  ( iter < nodes.end() ); // innode exists
  Node* inptr=(*iter) ;   
  */
  assert (nodemap.count(innode));
  Node* inptr = nodemap [innode];
  /*
  iter= find_if (nodes.begin(),nodes.end(), compare <Node> (outnode)) ;
  assert  ( iter < nodes.end() ); // outnode exists
  Node* outptr=(*iter) ; */
  assert (nodemap.count(outnode));
  Node* outptr = nodemap [outnode];
  /*
  vector <Sdfunc*>::iterator iter2=sdfuncs.begin();
  iter2= find_if (sdfuncs.begin(),sdfuncs.end(), compare <Sdfunc> (sdid));
  assert  ( iter2 < sdfuncs.end() );  // sdfunc exists
  Sdfunc* sdptr=(*iter2) ;    
  */
  assert (sdfuncmap.count(sdid));
  Sdfunc* sdptr = sdfuncmap [sdid];
  // make the drawable icon for the link
#ifndef _NO_GUI  
  Coord st=inptr->getxy();
  int startx=static_cast <int> (st.x+theParameters->node_radius);
  int starty=static_cast <int> (st.y+theParameters->node_radius);
  st=outptr->getxy();
  int stopx=static_cast <int> (st.x+theParameters->node_radius);
  int stopy=static_cast <int> (st.y+theParameters->node_radius);
  LinkIcon* icon=new LinkIcon(startx, starty ,stopx, stopy);
  stringstream os; // for the formatting of the icon text (such as "j:42" for junction with id 42)
	const int sz=8; // allows for 6-digit id numbers.
	char t[sz];
  os << lid << endl;
   os.get(t,sz);
   icon->settext(t);
  // register the icon in the drawing
  drawing->add_icon(icon);
#endif // _NO_GUI  
  // create the link
  Link* link=new Link(lid, inptr, outptr, length,nrlanes,sdptr);
  // register the icon in the link
#ifndef _NO_GUI
  link->set_icon(icon);
#endif //_NO_GUI 
  linkmap [lid] = link;
  links.insert(links.end(),link);
#ifdef _DEBUG_NETWORK
  cout << " read a link"<<endl;
#endif //_DEBUG_NETWORK
  return true;
}

bool Network::readvirtuallinks(string name)
{
	ifstream in(name.c_str());
	assert (in);
  string keyword;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="virtuallinks:")
 	return false;
 int nr;
 in >> nr;
 for (int i=0; i<nr;i++)
 {
 	if (!readvirtuallink(in))
 		return false;
 }
 return true;
}

bool Network::readvirtuallink(istream& in)
{
  char bracket;
  int lid, innode, outnode, length,nrlanes,sdid;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readvirtuallinks scanner jammed at " << bracket;

  	return false;
  }
   in  >> lid >> innode >> outnode >> length >> nrlanes >> sdid;
   // check lid, vmax, vmin, romax;
	// assert (!exists (lid) &&exists(innode) && exists(outnode) && length >0 && 0<nrlanes && exists(sdid) )
#ifdef _VISSIMCOM
	// read the virtual path link ids and parking place as well
   long enterparkinglot, lastlink, nr_v_nodes, v_node;
   vector <long> ids;
   in >> enterparkinglot >> lastlink >> nr_v_nodes;
   in >> bracket;
	
   if (bracket != '{')
   {
  		cout << "readfile::readvirtuallinks scanner jammed at " << bracket;
  		return false;
   }
   for (long i=0; i<nr_v_nodes; i++)
   {
		in >> v_node;
		ids.push_back(v_node);
   }
   in >> bracket;
	
   if (bracket != '}')
   {
  		cout << "readfile::readvirtuallinks scanner jammed at " << bracket;
  		return false;
   }
#endif //_VISSIMCOM
   
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readvirtuallinks scanner jammed at " << bracket;
  	return false;
  }
  // find the nodes and sdfunc pointers
  assert ( (length>0) && (nrlanes > 0) );           // check that the length and nrlanes are positive
  //assert  ( (find_if (links.begin(),links.end(), compare <Link> (lid))) == links.end() );    // no link with lid exists
  assert (!virtuallinkmap.count(lid));
  //OPTIMIZE
  /*vector <Node*>::iterator iter=nodes.begin();
  iter= find_if (nodes.begin(),nodes.end(), compare <Node> (innode)) ;
  assert  ( iter < nodes.end() ); // innode exists
  Node* inptr=(*iter) ;
  */
  assert (nodemap.count(innode));
  Node* inptr = nodemap [innode];
  /*
  iter= find_if (nodes.begin(),nodes.end(), compare <Node> (outnode)) ;
  assert  ( iter < nodes.end() ); // outnode exists
  Node* outptr=(*iter) ;   
*/
  assert (nodemap.count(outnode));
  Node* outptr = nodemap [outnode];
/*
  vector <BoundaryIn*>::iterator iter1=boundaryins.begin();
  iter1= find_if (boundaryins.begin(),boundaryins.end(), compare <BoundaryIn> (outnode));
  assert  ( iter1 < boundaryins.end() ); // the outnode is a boundaryIn
  BoundaryIn* biptr=(*iter1);
*/
  assert (boundaryinmap.count(outnode));
  BoundaryIn* biptr = boundaryinmap [outnode];
/*
  vector <BoundaryOut*>::iterator iter2=boundaryouts.begin();
  iter2=find_if (boundaryouts.begin(),boundaryouts.end(), compare <BoundaryOut> (innode));
  assert  ( iter2 < boundaryouts.end() );   //innode exists in Boundaryouts
  BoundaryOut* boptr=(*iter2) ;  
*/
  assert (boundaryoutmap.count(innode));
  BoundaryOut* boptr = boundaryoutmap [innode];
/*
  vector <Sdfunc*>::iterator iter3=sdfuncs.begin();
  iter3= find_if (sdfuncs.begin(),sdfuncs.end(), compare <Sdfunc> (sdid));
  assert  ( iter3 < sdfuncs.end() );       // sdfunc exists
  Sdfunc* sdptr=(*iter3) ;    
  */
  assert (sdfuncmap.count(sdid));
  Sdfunc* sdptr = sdfuncmap [sdid];
  // make the drawable icon for the link
#ifndef _NO_GUI
  Coord st=inptr->getxy();
  int startx=static_cast <int> (st.x+theParameters->node_radius);
  int starty=static_cast <int> (st.y+theParameters->node_radius);
  st=outptr->getxy();
  int stopx=static_cast <int> (st.x+theParameters->node_radius);
  int stopy=static_cast <int> (st.y+theParameters->node_radius);

  VirtualLinkIcon* icon=new VirtualLinkIcon(startx, starty ,stopx, stopy);
  // register the icon in the drawing
  drawing->add_icon(icon);
#endif // _NO_GUI  
  // create the link
  VirtualLink* link=new VirtualLink(lid, inptr, outptr, length,nrlanes,sdptr);

#ifdef _VISSIMCOM
	// add the id tags
  link->parkinglot = enterparkinglot;
  link->lastlink = lastlink;
  link->set_v_path_ids(ids);
#endif //_VISSIMCOM
  
  // register the icon in the link
#ifndef _NO_GUI
  link->set_icon(icon);
#endif //_NO_GUI  
  linkmap [lid] = link;
  virtuallinkmap [lid] = link;

  links.insert(links.end(),link);
  virtuallinks.insert(virtuallinks.end(),link);
  biptr->register_virtual(link);
  boptr->register_virtual(link);

#ifdef _DEBUG_NETWORK
  cout << " read a virtual link"<<endl;
#endif //_DEBUG_NETWORK
  return true;
}


bool Network::readservers(istream& in)
{
 string keyword;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="servers:")
 	return false;
 int nr;
 in >> nr;
 for (int i=0; i<nr;i++)
 {
 	if (!readserver(in))
 		return false;
 }
 return true;
}


bool Network::readserver(istream& in)
{
  char bracket;
  int sid, stype;
  double mu, sd, delay;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readservers scanner jammed at " << bracket;
  	return false;
  }
   in  >> sid >> stype >> mu >> sd >> delay;
   //assert  ( (find_if (servers.begin(),servers.end(), compare <Server> (sid))) == servers.end() );   // no server with sid exists
   assert (!servermap.count(sid));
   assert ( (stype > -1) && (stype<3) && (mu>0.0) && (sd>=0.0) && (delay>=0.0)); // to be updated when more server types are added
   // check id, vmax, vmin, romax;
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readservers scanner jammed at " << bracket;
  	return false;
  }
	// type 0= dummy server: Const server
	// type 1=standard N(mu,sd) sever
   // type 2=deterministic (mu) server
	// type -1 (internal) = OD server
	// type -2 (internal)= Destination server
	Server* sptr;
    if (stype==0)
		sptr = new ConstServer(sid,stype,mu,sd,delay);
	if (stype==1)
  		sptr = new Server(sid,stype,mu,sd,delay);
	if (stype==2)
  		sptr = new DetServer(sid,stype,mu,sd,delay);
	assert (sptr);
	servermap [sid] = sptr;
	servers.insert(servers.begin(),new ConstServer(sid,stype,mu,sd,delay));
#ifdef _DEBUG_NETWORK
  cout << " read a server"<<endl;
#endif //_DEBUG_NETWORK
  return true;
}


bool Network::readturnings(string name)
{
	 ifstream inputfile(name.c_str());
	 assert (inputfile);
	 string keyword;
	 inputfile >> keyword;
	#ifdef _DEBUG_NETWORK
	 cout << keyword << endl;
	#endif //_DEBUG_NETWORK
	 if (keyword!="turnings:")
 		return false;
	 int nr;
	 inputfile >> nr;
	 for (int i=0; i<nr;i++)
	 {
 		if (!readturning(inputfile))
 			return false;
	 }
	 return true;
}


bool Network::readturning(istream& in)
{

  char bracket;
  int tid, nid, sid, inlink, outlink,size;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readturnings scanner jammed at " << bracket;
  	return false;
  }

   in  >> tid >> nid >> sid >> inlink >> outlink >>size;
   // check
	assert (size>0);
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readturnings scanner jammed at " << bracket;
  	return false;
  }
  // find the node , links and server pointers
/* vector <Node*>::iterator iter1=nodes.begin();
  iter1=  find_if (nodes.begin(),nodes.end(), compare <Node> (nid));
  assert  ( iter1 < nodes.end() ); // the node exists
  Node* nptr=(*iter1) ;
  */
   map <int, Node*>::iterator node_iter;
   node_iter=nodemap.find(nid);
   assert (node_iter != nodemap.end());
   Node* nptr = (*node_iter).second;
/*
  vector <Link*>::iterator iter2=links.begin();
  iter2=find_if (links.begin(),links.end(), compare <Link> (inlink));
  assert  ( iter2 < links.end() );  // inlink exists
  Link* inlinkptr=(*iter2) ;
*/
  map <int, Link*>::iterator link_iter;
  link_iter=linkmap.find(inlink);
  assert (link_iter != linkmap.end());
  Link* inlinkptr = (*link_iter).second;

/*
  iter2=find_if (links.begin(),links.end(), compare <Link> (outlink));
  assert  ( iter2 < links.end() );  // outlink exists
  Link* outlinkptr=(*iter2) ;
*/
  link_iter=linkmap.find(outlink);
  assert (link_iter != linkmap.end());
  Link* outlinkptr = (*link_iter).second;

  if (sid < 0) // special case: this means a turning prohibitor
  {
     TurnPenalty* tptr=new TurnPenalty();
     tptr->from_link=inlink;
     tptr->to_link=outlink;
     tptr->cost=theParameters->turn_penalty_cost;
     turnpenalties.insert(turnpenalties.begin(),tptr);
     return true;
  }
/*
  vector <Server*>::iterator iter3=servers.begin();
  iter3= find_if (servers.begin(),servers.end(), compare <Server> (sid));
  assert  ( iter3 < servers.end() );  // server exists
  Server* sptr=(*iter3) ;
  */
  assert (servermap.count(sid));
  Server* sptr = servermap[sid];
#ifndef _UNSAFE
  //assert  ( (find_if (turnings.begin(),turnings.end(), compare <Turning> (tid))) == turnings.end() );  // there exists no turning with tid
  assert (!turningmap.count(tid));
#endif // _UNSAFE
  Turning* tptr = new Turning(tid, nptr, sptr, inlinkptr, outlinkptr,size);
  turningmap [tid] = tptr;
  turnings.insert(turnings.begin(),tptr);
#ifdef _DEBUG_NETWORK
  cout << " read a turning"<<endl;
#endif //_DEBUG_NETWORK
  return true;
}

void Network::create_turnings()
/*
   creates automatically new turnings for all junctions, using server nr 0 from the servers list
*/
{
	cout << "network::create turnings :" << endl;
	int tid=turnings.size();
	int size= theParameters->default_lookback_size;
	vector<Link*> incoming;
	vector<Link*> outgoing;
	//Server* sptr=servers.back(); // Why was this the back? Front makes more sense, since there the standard servers are placed.
	Server* sptr = (*servermap.begin()).second; // safest way, since servermap [0] may not exist (if someone starts numbering their servers at 1 for instance)
	// for all junctions
	for (vector<Junction*>::iterator iter1=junctions.begin();iter1<junctions.end();iter1++)
	{
		cout << " junction id " << (*iter1)->get_id() << endl;
		incoming=(*iter1)->get_incoming();
		cout << " nr incoming links "<< incoming.size() << endl;
			
		outgoing=(*iter1)->get_outgoing();
		cout << " nr outgoing links "<< outgoing.size() << endl;
		// for all incoming links
		for (vector<Link*>::iterator iter2=incoming.begin();iter2<incoming.end();iter2++)
		{
			cout << "incoming link id "<< (*iter2)->get_id() << endl;
			//for all outgoing links
			for (vector<Link*>::iterator iter3=outgoing.begin();iter3<outgoing.end();iter3++)
			{
				cout << "outcoming link id "<< (*iter3)->get_id() << endl;
				cout << "turning id "<< tid << endl;
			
				assert  ( (find_if (turnings.begin(),turnings.end(), compare <Turning> (tid))) == turnings.end() );  // there exists no turning with tid
  				turnings.insert(turnings.begin(),new Turning(tid, (*iter1), sptr, (*iter2), (*iter3),size));
  				tid++;
			}
		}
	}
}


bool Network::writeturnings(string name)
{
	ofstream out(name.c_str());
	assert(out);
	out << "turnings: " << turnings.size() << endl;
	for (vector<Turning*>::iterator iter=turnings.begin();iter<turnings.end();iter++)
	{
		(*iter)->write(out);
	}
	return true;
}

bool Network::readroutes(istream& in)

{
 string keyword;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="routes:")
 {
  cout << " readroutes: no << routes: >> keyword " << endl;
  return false;
 }
 int nr;
 in >> nr;
 for (int i=0; i<nr;i++)
 {
 		if (!readroute(in))
		{
			cout << " readroutes: readroute returned false for line nr " << (i+1) << endl;
   		return false;
		} 
 }
 for (vector<BoundaryIn*>::iterator iter=boundaryins.begin(); iter < boundaryins.end(); iter++)
 {
  (*iter)->register_routes(&routes);
 }
 return true;
}

bool Network::readroute(istream& in)
{
  char bracket;
  int rid, oid, did, lnr, lid;
  vector<Link*> rlinks;
  map <int,Link*>::iterator link_iter;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readroutes scanner jammed at " << bracket;
  	return false;
  }
   in  >> rid >> oid >> did >> lnr;
#ifndef _UNSAFE
   assert ( (find_if (routes.begin(),routes.end(), compare <Route> (rid))) == routes.end() ); // no route exists  with rid
#endif // _UNSAFE
   // check
   in >> bracket;

  if (bracket != '{')
  {
  	cout << "readfile::readroutes scanner jammed at " << bracket;
  	return false;
  }
  for (int i=0; i<lnr; i++)
  {
   in >> lid;
   /*
   vector <Link*>::iterator iter=links.begin();
   iter= find_if (links.begin(),links.end(), compare <Link> (lid));
   //Link* linkptr=(*iter) ;
	assert ( iter < links.end() ); // the link exists
   rlinks.insert(rlinks.end(),(*iter));
   */
   link_iter = linkmap.find(lid);
   assert (link_iter != linkmap.end());
   Link* linkptr = (*link_iter).second;
	 rlinks.insert(rlinks.end(),linkptr);
#ifdef _DEBUG_NETWORK
   cout << " inserted link " << lid << " into route " << rid << endl;
#endif //_DEBUG_NETWORK

  }
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readroutes scanner jammed at " << bracket;
  	return false;
  }
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readroutes scanner jammed at " << bracket;
  	return false;
  }
  // find the origin & dest  pointers
 /*
 vector <Origin*>::iterator o_iter=origins.begin();
  o_iter =  find_if (origins.begin(),origins.end(), compare <Origin> (oid));
  //Origin* optr=(*o_iter) ;
	assert ( o_iter < origins.end() ); // the origin exists
	*/
	map <int, Origin*>::iterator o_iter; 
	o_iter = originmap.find(oid);
  assert (o_iter != originmap.end());
	Origin* optr = o_iter->second;
 /*
  vector <Destination*>::iterator d_iter=destinations.begin();
  d_iter= find_if (destinations.begin(),destinations.end(), compare <Destination> (did));
  //Destination* dptr=(*d_iter) ;
	assert ( d_iter < destinations.end() );  // the destination exists
	*/
	map <int, Destination*>::iterator d_iter; 
	d_iter = destinationmap.find(did);
  assert (d_iter != destinationmap.end());
	Destination* dptr = d_iter->second;
#ifdef _DEBUG_NETWORK
  cout << "found o&d for route" << endl;
#endif //_DEBUG_NETWORK
  Route* rptr = new Route(rid, optr, dptr, rlinks);
	routes.insert(routes.end(), rptr);
	routemap [rid] = rptr;
#ifdef _DEBUG_NETWORK
  cout << " read a route"<<endl;
#endif //_DEBUG_NETWORK
  return true;
}

// read BUS routes
bool Network::readbusroutes(string name) // reads the busroutes, similar to readroutes
{
	 ifstream in(name.c_str());
	 assert (in);
	 string keyword;
	 in >> keyword;
	#ifdef _DEBUG_NETWORK
	 cout << keyword << endl;
	#endif //_DEBUG_NETWORK
	 if (keyword!="routes:")
	 {
	  cout << " readBusroutes: no << routes: >> keyword " << endl;
	  return false;
	 }
	 int nr;
	 in >> nr;
	 for (int i=0; i<nr;i++)
	 {
 		if (!readbusroute(in))
	  {
		cout << " readbusroutes: readbusroute returned false for line nr " << (i+1) << endl;
   		return false;
	  } 
	 }
	 for (vector<BoundaryIn*>::iterator iter=boundaryins.begin(); iter < boundaryins.end(); iter++)
	 {
	  (*iter)->register_busroutes(&busroutes);
	 }
	 return true;

}


bool Network::readbusroute(istream& in)
{
  char bracket;
  int rid, oid, did, lnr, lid;
  vector<Link*> rlinks;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readbusroutes scanner jammed at " << bracket;
  	return false;
  }
   in  >> rid >> oid >> did >> lnr;
#ifndef _UNSAFE
   assert ( (find_if (busroutes.begin(),busroutes.end(), compare <Busroute> (rid))) == routes.end() ); // no route exists  with rid
#endif // _UNSAFE
   // check
   in >> bracket;

  if (bracket != '{')
  {
  	cout << "readfile::readbusroutes scanner jammed at " << bracket;
  	return false;
  }
  for (int i=0; i<lnr; i++)
  {
   in >> lid;
   vector <Link*>::iterator iter=links.begin();
   iter= find_if (links.begin(),links.end(), compare <Link> (lid));
   //Link* linkptr=(*iter) ;
	assert ( iter < links.end() ); // the link exists
   rlinks.insert(rlinks.end(),(*iter));
#ifdef _DEBUG_NETWORK
   cout << " inserted link " << lid << " into busroute " << rid << endl;
#endif //_DEBUG_NETWORK

  }
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readbusroutes scanner jammed at " << bracket;
  	return false;
  }
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readbusroutes scanner jammed at " << bracket;
  	return false;
  }
  // find the origin & dest  pointers
  vector <Origin*>::iterator o_iter=origins.begin();
  o_iter =  find_if (origins.begin(),origins.end(), compare <Origin> (oid));
  //Origin* optr=(*o_iter) ;

  assert ( o_iter < origins.end() ); // the origin exists
  vector <Destination*>::iterator d_iter=destinations.begin();
  d_iter= find_if (destinations.begin(),destinations.end(), compare <Destination> (did));
  //Destination* dptr=(*d_iter) ;
	assert ( d_iter < destinations.end() );  // the destination exists
#ifdef _DEBUG_NETWORK
  cout << "found o&d for route" << endl;
#endif //_DEBUG_NETWORK
  busroutes.insert(busroutes.end(),new Busroute(rid, *o_iter, *d_iter, rlinks));
#ifdef _DEBUG_NETWORK
  cout << " read a route"<<endl;
#endif //_DEBUG_NETWORK
  return true;
}

bool Network::readbuslines(string name) // reads the busstops, buslines, and trips
{
	ifstream in(name.c_str());
	assert (in);
	string keyword;
	// First read the busstops
	in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="busstops:")
	{
		cout << " readbuslines: no << stops: >> keyword " << endl;
		return false;
	}
	int nr= 0;
	in >> nr;
	int i=0;
	for (i; i<nr;i++)
	{
 		if (!readbusstop(in))
		{
			cout << " readbuslines: readbusstop returned false for line nr " << (i+1) << endl;
   			return false;
		} 
	}
	// Second read the buslines
	in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="buslines:")
	{
		cout << " readbuslines: no << buslines: >> keyword " << endl;
		return false;
	}
	in >> nr;
	int limit = i + nr;
	for (i; i<limit;i++)
	{
 		if (!readbusline(in))
		{
			cout << " readbuslines: readbusline returned false for line nr " << (i+1) << endl;
   			return false;
		} 
	}
	// Third read the trips
	in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="bustrips:")
	{
		cout << " readbuslines: no << bustrips: >> keyword " << endl;
		return false;
	}
	in >> nr;
	limit = i + nr;
	for (i; i<limit;i++)
	{
 		if (!readbustrip(in))
		{
			cout << " readbuslines: readbustrip returned false for line nr " << (i+1) << endl;
   			return false;
		} 
	}

	return true;
}

bool Network::readbusstop (istream& in) // reads a busstop
{

//{ stop_id	link_id	length	has_bay	dwelltime }
  char bracket;
  int stop_id, link_id;
  double length, dwelltime;
  bool has_bay ;
  bool ok= true;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbusstop scanner jammed at " << bracket;
  	return false;
  }
  in >> stop_id >> link_id >> length >> has_bay >> dwelltime;
  Busstop* st= new Busstop (stop_id, link_id, length, has_bay, dwelltime);
  in >> bracket;
  if (bracket != '}')
  {
    cout << "readfile::readbusstop scanner jammed at " << bracket;
    return false;
  }
  busstops.push_back (st);
#ifdef _DEBUG_NETWORK
  cout << " read busstop"<< stop_id <<endl;
#endif //_DEBUG_NETWORK
  return ok;
}


bool Network::readbusline(istream& in) // reads a busline
{
//  buslines: nr_buslines
//{	id	name	origin	destination	route_id	vehtype	
//	nr_stops	{	stop_id1  stop_id2  stop_id3  ...	stop_idn	}
//}
	
  char bracket;
  int busline_id, ori_id, dest_id, route_id, vehtype, nr_stops, stop_id;
  string name;
  vector <Busstop*> stops;
  Busstop* stop;
  bool ok= true;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbusline scanner jammed at " << bracket;
  	return false;
  }
  in >> busline_id >> name >> ori_id >> dest_id >> route_id >> vehtype >> nr_stops;
  in >> bracket;
  if (bracket != '{')
  {
	cout << "readfile::readsbusline scanner jammed at " << bracket;
	return false;
  }

  for (int i=0; i < nr_stops; i++)
  {
	  
	  in >> stop_id;
	  stop = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (stop_id) ))); // find the stop 
	  stops.push_back(stop); // and add it
  }
  in >> bracket;
  if (bracket != '}')
  {
	cout << "readfile::readsbusline scanner jammed at " << bracket;
	return false;
  }
  
  
  // find OD pair, route, vehicle type
  odval odid (ori_id, dest_id);
  ODpair* odptr=(*(find_if (odpairs.begin(),odpairs.end(), compareod (odid) )));
  Busroute* br=(*(find_if(busroutes.begin(), busroutes.end(), compare <Route> (route_id) )));
  Vtype* vt= (*(find_if(vehtypes.vtypes.begin(), vehtypes.vtypes.end(), compare <Vtype> (vehtype) )));
  Busline* bl= new Busline (busline_id, name,br,vt,odptr );
  bl->add_stops(stops);

  in >> bracket;
  if (bracket != '}')
  {
    cout << "readfile::readbusstop scanner jammed at " << bracket;
    return false;
  }
  // add to buslines vector
  buslines.push_back (bl);
#ifdef _DEBUG_NETWORK
  cout << " read busline"<< stop_id <<endl;
#endif //_DEBUG_NETWORK
  return ok;
}

bool Network::readbustrip(istream& in) // reads a trip
{
  char bracket;
  int trip_id, busline_id, nr_stops, stop_id;
  double start_time, pass_time;
  vector <Visit_stop*> stops;
  bool ok= true;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbustrip scanner jammed at " << bracket;
  	return false;
  }
  in >> trip_id >> busline_id >> start_time >> nr_stops;
  for (int i=0; i < nr_stops; i++)
  {
	  in >> bracket;
	  if (bracket != '{')
	  {
  		cout << "readfile::readsbustrip scanner jammed at " << bracket;
  		return false;
	  }
	  in >> stop_id >> pass_time;
	  // create the Visit_stop
	  // find the stop in the list
	  Busstop* bs = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (stop_id) )));
	  Visit_stop* vs = new Visit_stop (bs, pass_time);
	  stops.push_back(vs);
	  in >> bracket;
	  if (bracket != '}')
	  {
  		cout << "readfile::readsbustrip scanner jammed at " << bracket;
  		return false;
	  }
  }
   
  // find busline
  Busline* bl=(*(find_if(buslines.begin(), buslines.end(), compare <Busline> (busline_id) )));
  Bustrip* trip= new Bustrip (trip_id, start_time );
  trip->add_stops(stops);
  bl->add_trip(trip,start_time);
  in >> bracket;
  if (bracket != '}')
  {
    cout << "readfile::readbusstop scanner jammed at " << bracket;
    return false;
  }
  // add to buslines vector
  //buslines.push_back (bl);
#ifdef _DEBUG_NETWORK
  cout << " read busstop"<< stop_id <<endl;
#endif //_DEBUG_NETWORK
  return ok;	
	return true;
}







 
// read traffic control
bool Network::readsignalcontrols(string name)
{
  ifstream inputfile(name.c_str());
  assert (inputfile);
  string keyword;
  inputfile >> keyword;
#ifdef _DEBUG_NETWORK
  cout << keyword << endl;
#endif //_DEBUG_NETWORK
  if (keyword!="controls:")
 	return false;
  int nr;
  inputfile >> nr;
  for (int i=0; i<nr;i++)
  {
 	if (!readsignalcontrol(inputfile))
 		return false;
  }
 return true;
}

 
bool Network::readsignalcontrol(istream & in)
{
  char bracket;
  int controlid, nr_plans;
  bool ok= true;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsignalcontrol scanner jammed at " << bracket;
  	return false;
  }
  in >> controlid >> nr_plans;
  SignalControl* sc = new SignalControl(controlid);

  for (int i=0; i < nr_plans; i++)
  {
	  ok = ok && (readsignalplan (in, sc));
  }
  in >> bracket;
   if (bracket != '}')
   {
       cout << "readfile::readsignalcontrol scanner jammed at " << bracket;
  	   return false;
   }
   signalcontrols.push_back(sc);
#ifdef _DEBUG_NETWORK
  cout << " read signalcontrol"<< controlid <<endl;
#endif //_DEBUG_NETWORK
  return ok;
}


bool Network::readsignalplan(istream& in, SignalControl* sc)
{
  char bracket;
  int planid, nr_stages;
  double offset, cycletime, start, stop;
  bool ok= true;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsignalplans scanner jammed at " << bracket;
  	return false;
  }
  in >> planid >> start >> stop >> offset >> cycletime >> nr_stages;
  assert ((start >= 0.0) && (stop > 0.0) && (cycletime > 0.0)  && (nr_stages > 0));
 // make a new signalplan
  SignalPlan* sp = new SignalPlan(planid, start, stop, offset, cycletime);
 // read & make all stages
  for (int i=0; i<nr_stages; i++)
  {
	  ok = ok && (readstage(in, sp)); // same as &= but not sure if &= is bitwise or not...
  }
  in >> bracket;
   if (bracket != '}')  
   {
       cout << "readfile::readsignalplans scanner jammed at " << bracket;
  	   return false;
   }
   sc->add_signal_plan(sp);
   signalplans.push_back(sp);
#ifdef _DEBUG_NETWORK
  cout << " read signalplan "<< planid <<endl;
#endif //_DEBUG_NETWORK
  return ok;
}


bool Network::readstage(istream& in, SignalPlan* sp)
{
  char bracket;
  int stageid, nr_turnings, turnid;
  double start, duration;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readstages scanner jammed at " << bracket;
  	return false;
  }
   in  >> stageid >> start >> duration >> nr_turnings;
   assert ((start >= 0.0) && (duration > 0.0));
   
// make a new stage
   Stage* stageptr = new Stage(stageid, start, duration);
// read all turnings find each turning in the turnings list and add it to the stage
   in >> bracket;
   if (bracket != '{')
   {
  	  cout << "readfile::readsignals scanner jammed at " << bracket;
  	  return false;
   }
   for (int i=0; i<nr_turnings; i++)
   {
	   in >> turnid;
	   // find turning in turnings list
	   vector <Turning*>::iterator turn = find_if(turnings.begin(), turnings.end(), compare<Turning>(turnid));
	   assert (turn != turnings.end());
	   // add to stage
	   stageptr->add_turning(*turn);
   }
   in >> bracket;
   if (bracket != '}') // once for the turnings
   {
       cout << "readfile::readstages scanner jammed at " << bracket;
  	   return false;
   }
   in >> bracket; // once for the stage
   if (bracket != '}')
  {
  	cout << "readfile::readstages scanner jammed at " << bracket;
  	return false;
  }
// add stage to stages list
   stages.push_back(stageptr);
// add stage to signal plan
   sp->add_stage(stageptr);
#ifdef _DEBUG_NETWORK
  cout << " read stage "<< stageid <<endl;
#endif //_DEBUG_NETWORK
  return true;
}



bool Network::readnetwork(string name)
{
  ifstream inputfile(name.c_str());
  assert  (inputfile);
  if (readservers(inputfile) && readnodes(inputfile) && readsdfuncs(inputfile) && readlinks (inputfile) )		
  {
	  	return true;
  }
  else
     return false;
}

bool Network::register_links()
{
	// register all the incoming links @ the destinations
	for (vector<Destination*>::iterator iter=destinations.begin(); iter<destinations.end();iter++)
	{
		(*iter)->register_links(links);
	}
	//register all the outgoing links at the origins
	for (vector<Origin*>::iterator iter1=origins.begin(); iter1<origins.end();iter1++)
	{
		(*iter1)->register_links(links);
	}
	//register all the incoming & outgoing links at the junctions
	for (vector<Junction*>::iterator iter2=junctions.begin(); iter2<junctions.end();iter2++)
	{
		(*iter2)->register_links(links);
	}
	return true;
}

bool Network::readods(istream& in)
{
 string keyword;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="od_pairs:")
 {
     cout << "stuck at " << keyword << " instead of : od_pairs: " << endl;
     return false;
 }
 int nr;
 in >> nr;
 in >> keyword;
 if (keyword!="scale:")
 {
     cout << "stuck at " << keyword << " instead of : scale: " << endl;
     return false;
 }
 double scale;
 in >> scale;

 for (int i=0; i<nr;i++)
 {
 	if (!readod(in,scale))
 	{
     cout << "stuck at od pair " << i << " of " << nr << endl;
     return false;
     }
 }
 for (vector<BoundaryIn*>::iterator iter=boundaryins.begin();iter<boundaryins.end();iter++)
 {
  	(*iter)->register_ods(&odpairs);
 }
 return true;
}



bool Network::readod(istream& in, double scale)
{
  char bracket;
  int oid, did, rate;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readinput::readod scanner jammed at " << bracket;
  	return false;
  }
   in  >> oid >> did >> rate;
   // check oid, did, rate;
   // scale up/down the rate
   rate=static_cast<int> (rate*scale);
   //assert (rate > 0);
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readinput::readod scanner jammed at " << bracket;
  	return false;
  }
  // find oid, did
  vector <Origin*>::iterator o_iter=origins.begin();
  o_iter= find_if (origins.begin(),origins.end(), compare <Origin> (oid));
//  Origin* optr=(*o_iter) ;
	assert ( o_iter < origins.end() ); // origin exists
   vector <Destination*>::iterator d_iter=destinations.begin();
   d_iter=  find_if (destinations.begin(),destinations.end(), compare <Destination> (did));
  //Destination* dptr=(*d_iter) ;
	assert ( d_iter < destinations.end() ); //destination exists
#ifdef _DEBUG_NETWORK
  cout << "found o and d " << oid << "," << did << endl;
#endif //_DEBUG_NETWORK
  // create odpair

  ODpair* odpair=new ODpair (*o_iter, *d_iter, rate,&vehtypes); // later the vehtypes can be defined PER OD
  
  //comment out the addroutes and do it in the executemaster() proc instead, so that only routes are generated
  //for existing OD pairs...
  //addroutes (oid,did,odpair);
  
  //add odpair to origin and general list
  odpairs.insert(odpairs.begin(),odpair);
  (*o_iter)->add_odpair(odpair);
  // set od list
#ifdef _DEBUG_NETWORK
  cout << " read an od"<<endl;
#endif //_DEBUG_NETWORK
  return true;
}

bool Network::add_od_routes()	
// adds routes to OD pairs, sorts out unnecessary routes
{
	int nr_deleted =0;
	vector <int> deleted_routes;
	vector <int> new_del_routes;
	for (vector<ODpair*>::iterator iter=odpairs.begin(); iter < odpairs.end(); iter++)
	{	
		addroutes ((*iter)->get_origin()->get_id(), (*iter)->get_destination()->get_id(), *iter );
		if (theParameters->delete_bad_routes)
		{
			new_del_routes = (*iter)->delete_spurious_routes(runtime/4);
			deleted_routes.insert(deleted_routes.begin(), new_del_routes.begin(), new_del_routes.end());
			//nr_deleted += (*iter)->delete_spurious_routes(runtime/4).size(); // deletes all the spurious routes in the route set.
		}
	}
	nr_deleted = deleted_routes.size();
	cout << nr_deleted << " routes deleted" << endl;
// find routes and delete
	
	vector <Route*>::iterator route;
	vector <int>::iterator del;
	bool next_route = true;
	for (route=routes.begin(); route < routes.end(); )
	{
		next_route=true;
		for (del=deleted_routes.begin(); del < deleted_routes.end(); del++)
		{
			
			if ((*route)->get_id() == *del)
			{
				delete (*route);
				route=routes.erase(route);
				del = deleted_routes.end();
				next_route= false;
			}
		}
		if (next_route)
			route++;
		else
			next_route=true;
	}
// write the new routes file.


	return true;
}

bool Network::addroutes (int oid, int did, ODpair* odpair)
{

  //find routes from o to d and add
  vector<Route*>::iterator iter=routes.begin();
  odval odvalue(oid, did);
  while (iter!=routes.end())
  {
  		iter=(find_if (iter,routes.end(), compareroute(odvalue) )) ;
  	   if (iter!=routes.end())
  	   {
  	   		//Route* rptr=*iter;
  	   		odpair->add_route(*iter);
  	   	#ifdef _DEBUG_NETWORK	
  	   		cout << "added route " << ((*iter)->get_id())<< endl;
  	   #endif //_DEBUG_NETWORK		
  	   	   	iter++;
		}
  }
  return true;
}


ODRate Network::readrate(istream& in, double scale)
{
#ifdef _DEBUG_NETWORK
	cout << "read a rate" << endl;
#endif //_DEBUG_NETWORK	
	ODRate odrate;
	odrate.odid=odval(0,0);
	odrate.rate=-1;
char bracket;
  int oid, did, rate;
  in >> bracket;

  if (bracket != '{')
  {
  	cout << "readinput::readrate scanner jammed at " << bracket;
  	return odrate;
  }
   in  >> oid >> did >> rate;
   // check oid, did, rate;
   // scale up/down the rate
   rate=static_cast<int> (rate*scale);
//   assert (rate > 0);
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readinput::readrate scanner jammed at " << bracket;
  	return odrate;
  }
	odrate.odid=odval(oid,did);
	// find and check od pair)
	odrate.rate=rate;
	return odrate;
}


bool Network::readrates(istream& in)
{
	ODSlice* odslice=new ODSlice();
  string keyword;
  in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="od_pairs:")
 	{
     cout << "stuck at " << keyword << " instead of : od_pairs: " << endl;
     return false;
 }
 int nr;
 in >> nr;
 in >> keyword;
 #ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="scale:")
 {
     cout << "stuck at " << keyword << " instead of : scale: " << endl;
     return false;
 }
 double scale;
 in >> scale;
 in >> keyword;
 #ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="loadtime:")
 {
     cout << "stuck at " << keyword << " instead of : loadtime: " << endl;
     return false;
 }
 double loadtime;
 in >> loadtime;
 for (int i=0; i<nr;i++)
 {
 	ODRate odrate=readrate(in,scale);
 	if (odrate.rate==-1)
 	{
     cout << "stuck at od readrates. load time : "<< loadtime << " od nr " << i << " of " <<nr << endl;

     return false;
    }
 	else
 		odslice->rates.insert(odslice->rates.end(),odrate);	
 }
 odmatrix.add_slice(loadtime,odslice);
  #ifdef _DEBUG_NETWORK
 cout << " added a slice " << endl;
 #endif //_DEBUG_NETWORK
 // MatrixAction* mptr adds itself into the eventlist, and will be cleared up when executed.
 // it is *not* a dangling pointer 
 MatrixAction* mptr=new MatrixAction(eventlist, loadtime, odslice, &odpairs);
 assert (mptr != NULL);
 #ifdef _DEBUG_NETWORK

  cout << " added a matrixaction " << endl;
  #endif //_DEBUG_NETWORK
 return true;	
}

bool Network::readserverrates(string name)
{
  ifstream in(name.c_str());
	assert (in);
  string keyword;
  in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="rates:")
 	return false;
 int nr;
 in >> nr;
 for (int i=0; i<nr;i++)
 {
 	if (!readserverrate(in))
 		return false;
 }
  return true;	
}

bool Network::readserverrate(istream& in)
{
#ifdef _DEBUG_NETWORK
	cout << "read a rate" << endl;
#endif //_DEBUG_NETWORK	
  char bracket;
  int sid;
  double time, mu, sd;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readinput::readserverrate scanner jammed at " << bracket;
  	return  false;
  }
   in  >> sid >> time >> mu >> sd;
   //assert  ( (find_if (servers.begin(),servers.end(), compare <Server> (sid))) != servers.end() );   // server with sid exists
   assert (servermap.count(sid)); // make sure exists
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readinput::readserverrate scanner jammed at " << bracket;
  	return false;
  }
   //Server* sptr=*(find_if (servers.begin(),servers.end(), compare <Server> (sid)));
   Server* sptr = servermap[sid];
   ChangeRateAction* cptr=new ChangeRateAction(eventlist,time,sptr,mu,sd);
   assert (cptr != NULL);
	return true;
}

bool Network::readinput(string name)
{

	ifstream inputfile(name.c_str());
	assert (inputfile);
	if (readods(inputfile))
	 {		
		string keyword;
		inputfile >> keyword;
		#ifdef _DEBUG_NETWORK
 		cout << keyword << endl;
		#endif //_DEBUG_NETWORK
 		if (keyword!="slices:")
 				return false;
		int nr;
		inputfile >> nr;
		for (int i=0; i<nr; i++)
		{
			if (!readrates(inputfile))
				return false;				
		}
		return true;
	}	
	else
		return false;	
}

bool Network::readvtype (istream & in)
{
  char bracket;
  int id;
  string label;
  double prob, length;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readvtypes scanner jammed at " << bracket;
  	return false;
  }
   in  >> id >> label >> prob >> length;

	assert ( (prob >= 0.0) && (prob<=1.0) && (length>0.0) ); // 0.0 means unused in the vehicle míx
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readvtypes scanner jammed at " << bracket;
  	return false;
  }
  vehtypes.vtypes.insert(vehtypes.vtypes.end(), new Vtype (id,label,prob,length));
  return true;
}


bool Network::readvtypes (string name)
{
	ifstream in(name.c_str());
	assert (in);

	string keyword;

	in >> keyword;
#ifdef _DEBUG_NETWORK
 	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="vtypes:")
 		return false;
 	int nr;
 	in >> nr;
 	for (int i=0; i<nr;i++)
 	{
 		if (!readvtype(in))
 			return false;
 	}
	vehtypes.initialize();
	return true;
}



bool Network::writeoutput(string name)
// !!!!!!! changed to write the OD output!!!!!!!!!
{
	ofstream out(name.c_str());
	assert(out);
	bool ok=true;
	vector<ODpair*>::iterator iter=odpairs.begin();
	(*iter)->writefieldnames(out);
	for (iter;iter<odpairs.end();iter++)
	{
		ok=ok && (*iter)->write(out);
	}
	return ok;
}

bool Network::writesummary(string name)
// !!!!!!! writes the OD pair output!!!!!!!!!
{
	ofstream out(name.c_str());
	assert(out);
	bool ok=true;
	out << "Origin\tDestination\tNrRoutes\tNrArrived\tTotalTravelTime(s)\tTotalMileage(m)" << endl;
	for (vector<ODpair*>::iterator iter=odpairs.begin();iter<odpairs.end();iter++)
	{
		ok=ok && (*iter)->writesummary(out);
	}
	return ok;
}

bool Network::writeheadways(string name)
// writes the time headways for the virtual links. to compare with the arrival process in Mitsim
{
  ofstream out(name.c_str());
	assert(out);
	for (vector<VirtualLink*>::iterator iter=virtuallinks.begin();iter<virtuallinks.end();iter++)
	{
    out << (*iter)->get_id()<< endl;
		(*iter)->write_in_headways(out);
	}
	return true;


}


bool Network::setlinktimes()
{
	for (vector<Link*>::iterator iter=links.begin();iter<links.end();iter++)
		(*iter)->set_hist_time((*iter)->get_freeflow_time());
	return true;
}

bool Network::writelinktimes(string name)
{
	ofstream out(name.c_str());
	assert(out);
	out << "links:\t" << links.size() << endl;
	out << "periods:\t" << nrperiods << endl;
	out << "periodlength:\t" << periodlength << endl;
  for (vector<Link*>::iterator iter=links.begin();iter<links.end();iter++)
	{
		(*iter)->write_time(out);
	}
	return true;
}

bool Network::readlinktimes(string name)
{
	ifstream inputfile(name.c_str());
	assert (inputfile);
	if (readtimes(inputfile))
		return true;
	else
		return false;	
}

bool Network::readtimes(istream& in)
{
 string keyword;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="links:")
  return false;
 int nr;
 in >> nr;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="periods:")
 	return false;
 in >> nrperiods;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="periodlength:")
 	return false;
 in >> periodlength;
 for (int i=0; i<nr;i++)

 {
 	if (!readtime(in))
  {
   cout << " readtimes for link : " << i << " failed " << endl;
   return false;

  } 
 }

 if (nr == 0) // create the histtimes from freeflow times
 {
	 cout << " creating linktimes from freeflow times " << endl;
#ifdef _DEBUG_NETWORK
  cout << " creating linktimes from freeflow times " << endl;
  cout << " links.size() " << links.size() << endl;
  cout << " virtuallinks.size() " << virtuallinks.size() << endl;
#endif _DEBUG_NETWORK
   for (vector<Link*>::iterator iter=links.begin();iter<links.end();iter++)
   {
     double linktime= (*iter)->get_freeflow_time();
   	 LinkTime* ltime=new LinkTime();
		 ltime->periodlength=periodlength;
 	 	 ltime->nrperiods=nrperiods;
   	 ltime->id=(*iter)->get_id();
     for (int i=0;i < nrperiods;i++)
        (ltime->times).push_back(linktime);
     (*iter)->set_hist_time(linktime);
	   (*iter)->set_histtimes(ltime);
  	 //linkinfo->times.push_back(ltime);   
	   linkinfo->times.insert(pair <int,LinkTime*> ((*iter)->get_id(),ltime )); 
   }
	} 
 
 return true;
}

bool Network::readtime(istream& in)

{
  char bracket;
  int lid;
  double linktime;
  LinkTime* ltime=new LinkTime();
  ltime->periodlength=periodlength;
  ltime->nrperiods=nrperiods;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readtimes scanner jammed at " << bracket;
  	return false;
  }
   in  >> lid ;
   ltime->id=lid;
   for (int i=0;i<nrperiods;i++)
   {
    in >> linktime;
    (ltime->times).push_back(linktime);
    }
    vector <Link*>::iterator l_iter=links.begin();
    l_iter =  find_if (links.begin(),links.end(), compare <Link> (lid));
   assert  ( l_iter < links.end() );     // lid exists
	assert ( linktime >= 0.0 );
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readtimes scanner jammed at " << bracket;
  	return false;
  }
  //Link* lptr= (*l_iter );
  (*l_iter)->set_hist_time(linktime);
  (*l_iter)->set_histtimes(ltime);
//  linkinfo->times.push_back(ltime);
   linkinfo->times.insert(pair<int,LinkTime*> (lid,ltime));
#ifdef _DEBUG_NETWORK
  cout << " read a linktime"<<endl;
#endif //_DEBUG_NETWORK
  return true;
}

bool Network::readincident (istream & in)
{
  // OPTIMIZE
  char bracket;
  int lid, sid;
  bool blocked;
  double penalty, start, stop, info_start, info_stop;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readincident scanner jammed at " << bracket;
  	return false;

  }
   in  >> lid  >> sid >> penalty >> start >> stop >> info_start >> info_stop >> blocked;
    assert  ( (find_if (links.begin(),links.end(), compare <Link> (lid))) < links.end() );     // lid exists
    assert  ( (find_if (sdfuncs.begin(),sdfuncs.end(), compare <Sdfunc> (sid))) < sdfuncs.end() );     // sid exists
	assert ( (penalty >= 0.0 ) && (start>0.0) && (stop>start) );
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readincident scanner jammed at " << bracket;
  	return false;

  }
	 Incident* incident = new Incident(lid, sid, start,stop,info_start,info_stop,eventlist,this, blocked);
  incidents.insert(incidents.begin(), incident); // makes the incident and initialises its start in the eventlist
#ifdef _DEBUG_NETWORK
 cout <<"incident from " << start << " to " << stop << " on link nr " << lid << endl;
#endif //_DEBUG_NETWORK
  return find_alternatives_all(lid,penalty,incident);    // make the alternatives
}


bool Network::readincidents (istream & in)
{
 string keyword;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="incidents:")
 	return false;
 int nr;
 in >> nr;
 for (int i=0; i<nr;i++)
 {
 	if (!readincident(in))
 		return false;
 }
 return true;
}

bool Network::readincidentparams (istream &in)
{
 string keyword;
 in >> keyword;

#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="parameters:")
 	return false;
 int nr;
 in >> nr;
 for (int i=0; i<nr;i++)
 {
 	if (!readincidentparam(in))
 		return false;

 }
 return true;
}


bool Network::readincidentparam (istream &in)
{
  char bracket;
  double mu, sd;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readincidentparam scanner jammed at " << bracket;
  	return false;
  }
   in  >> mu  >> sd ;
   in >> bracket;
   if (bracket != '}')
  {
  	cout << "readfile::readincidentparam scanner jammed at " << bracket;
  	return false;
  }
  // SOMEWHERE I NEED TO STORE THEM SO THEY CAN BE ACCESSED!!! DON'T KNOW YET WHERE

    incident_parameters.push_back(mu);
   //  cout << "checking: mu " << mu << " first of list " << incident_parameters[0] << endl;
    incident_parameters.push_back(sd);
    return true;
}

bool Network::readx1 (istream &in)
{
 string keyword;
 char bracket;
 in >> keyword;
#ifdef _DEBUG_NETWORK
 cout << keyword << endl;
#endif //_DEBUG_NETWORK
 if (keyword!="X1:")
 	return false;
 double mu,sd;
 in >> bracket >> mu >> sd >> bracket;
// STORE THE MU AND SD SOMEWHERE!


	incident_parameters.push_back(mu);

    incident_parameters.push_back(sd);
 return true;


}


bool Network::readincidentfile(string name)
{
	ifstream inputfile(name.c_str());
	assert (inputfile);
	if (readsdfuncs (inputfile) && readincidents(inputfile) &&readincidentparams(inputfile) && readx1(inputfile))
		return true;
	else
		return false;		

}

bool Network::readpathfile(string name)
{
	ifstream inputfile(name.c_str());
	assert (inputfile);
	if (readroutes(inputfile))
	{
    return true;
  }
	else
	{
   	 return false;
  }
}

bool Network::writepathfile(string name)
{
	ofstream out(name.c_str());
	assert(out);
	out << "routes: " << routes.size() << endl;
	for (vector<Route*>::iterator iter=routes.begin();iter<routes.end();iter++)
	{
		(*iter)->write(out);
	}
	return true;
}


bool Network::readassignmentlinksfile(string name)
{
	ifstream in(name.c_str());
	assert(in);
	string keyword,temp;
	in >> keyword;

	#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
	#endif //_DEBUG_NETWORK
	if (keyword!="no_obs_links:")
	 	return false;
	int nr,lid;
	in >> nr;
	in >> temp;
	assert (temp=="{");
	vector<Link*>::iterator iter;
	for (int i=0; i<nr;i++)
	{
	  in >> lid;
	  iter = find_if (links.begin(),links.end(), compare <Link> (lid)) ;  
	  assert (iter!=links.end()); // assert it exists
	  (*iter)->set_use_ass_matrix(true);
	  no_ass_links++;
	}
	in >> temp;
	assert (temp=="}");
	return true;
}


bool Network::readparameters(string name)
{
	ifstream inputfile(name.c_str());
	assert (inputfile);
	if (theParameters->read_parameters(inputfile))
		return true;
	else
		return false;	

}

bool Network::init_shortest_path()
/* Initialises the shortest path graph with the link costs
*/
{
	int lid,in,out,routenr;
	routenr=routes.size();
	double cost, mu, sd;
	random=new (Random);
		/**********test***/
	if (randseed != 0)
	   random->seed(randseed);
	else
		random->randomize();

	 #ifdef _DEBUG_SP
	cout << "network::init_shortest path, routes.size  " << routenr << ", links.size " << links.size() << ", nodes.size " << nodes.size() << endl;
	#endif //_DEBUG_SP
  // CREATE THE GRAPH
   #ifndef _USE_VAR_TIMES
       graph=new Graph<double, GraphNoInfo<double> > (nodes.size() /* 50000*/, links.size(), 9999999.0);
   #else
       graph=new Graph<double, LinkTimeInfo > (/*nodes.size()*/ 50000, links.size()*10, 9999999.0);
   #endif
   // ADD THE LINKS AND NODES

  for (vector<Link*>::iterator iter=links.begin(); iter<links.end();iter++) // create the link graph for shortest path
	{
		lid=(*iter)->get_id();
		in=(*iter)->get_in_node_id();
		out=(*iter)->get_out_node_id();
		mu=(*iter)->get_hist_time();
		sd=disturbance*mu;
		cost=mu;
		 #ifdef _DEBUG_SP
		cout << " graph->addlink: link " << lid << ", innode " << in << ", outnode " << out << ", cost " << cost << endl;
    	#endif //_DEBUG_SP
    	graph->addLink(lid,in,out,cost);
	}
  // ADD THE TURNPENALTIES;

  // first set all the indices
  graph->set_downlink_indices();
  
  for(vector <TurnPenalty*>::iterator iter1=turnpenalties.begin();iter1<turnpenalties.end();iter1++)
  {

     //graph->penalty((*iter1)->from_link, (*iter1)->to_link,(*iter1)->cost);
     graph->set_turning_prohibitor((*iter1)->from_link, (*iter1)->to_link);
  }
	
  theParameters->shortest_paths_initialised= true;

	return true;
}


vector<Link*> Network::get_path(int destid)  // get the path from
{
	 #ifdef _DEBUG_SP
	cout << "shortest path to " << destid << endl << " with " ;
	#endif //_DEBUG_SP
	//cout << "...calling  shortest_path_vector...." << endl;
	vector <int> linkids=graph->shortest_path_vector(destid);  // get out the shortest path current root link to Destination (*iter3)
	//cout << "...exited shortest path vector call" << endl;
	 #ifdef _DEBUG_SP
	cout << linkids.size() << " links " << endl << "  : " ;
	#endif //_DEBUG_SP

	if (linkids.empty())
	{
   	cout << "Shortest path: get_path : PROBLEM OBTAINING links in path to  dest " << destid << endl;
	vector <Link*> rlinks;
	return rlinks;
	}	
	vector <Link*> rlinks;
	//linkids.insert(linkids.begin(),(*iter2)->get_id()); // add the root link to the path
	for (vector<int>::iterator iter4=linkids.begin();iter4<linkids.end();iter4++) // find all the links
	{
					int lid=(*iter4);
					#ifdef _DEBUG_SP					
					cout << lid << " , ";
          #endif //_DEBUG_SP
					Link* linkptr=(*(find_if (links.begin(),links.end(), compare <Link> (lid)))) ;
					assert ( (find_if (links.begin(),links.end(), compare <Link> (lid))) < links.end() ); // the link exists
				   rlinks.insert(rlinks.end(),linkptr);
	}
  assert (rlinks.size() > 0);
  return rlinks;
}	

/* OLD WAY***
bool Network::shortest_paths_all()	
//calculate the shortest paths for each link emanating from each origin to each destination;
// and saving them if  there is a new path found (i.e. it's not in the routes vector already)
{		
  double entrytime=0.0;    // entry time for time-variant shortest path search
  int nr_reruns=static_cast<int> (runtime/update_interval_routes); // determines the number of reruns of the shortest path alg.
  int routenr=routes.size();
  for (int i=0; i<nr_reruns; i++)
  {
        entrytime= i*update_interval_routes;
		for (vector<Origin*>::iterator iter1=origins.begin(); iter1<origins.end();iter1++)
		{
			vector<Link*> outgoing=(*iter1)->get_links();
			for (vector<Link*>::iterator iter2=outgoing.begin();iter2<outgoing.end();iter2++)
		    {
				
			//	 #ifdef _DEBUG_SP
				cout << "shortest_paths_all: starting label correcting from root " << (*iter2)->get_id() << endl;
			//	#endif //_DEBUG_SP
              #ifndef _USE_VAR_TIMES
                  graph->labelCorrecting((*iter2)->get_id());  // find the shortest path from Link (*iter2) to ALL nodes
              #else
                  if (linkinfo) // if there are link info times
                      graph->labelCorrecting((*iter2)->get_id(),entrytime,linkinfo);  // find the shortest path from Link (*iter2) to ALL nodes
                  else
                       graph->labelCorrecting((*iter2)->get_id());  // find the shortest path from Link (*iter2) to ALL nodes NO LINKINFO
              #endif // _USE_VAR_TIMES
         #ifdef _DEBUG_SP
				cout << "finished label correcting for root link "<<(*iter2)->get_id() << endl;
        #endif //_DEBUG_SP
				for (vector<Destination*>::iterator iter3=destinations.begin();iter3<destinations.end();iter3++)
				{
         
				
           #ifdef _DEBUG_SP
					cout << " see if we can reach destination " << (*iter3)->get_id()<< endl;
          #endif //_DEBUG_SP
					if (graph->reachable((*iter3)->get_id())) // if the destination is reachable from this link...
					{
             #ifdef _DEBUG_SP
						cout << " it's reachable.." << endl;
            #endif //_DEBUG_SP
						vector<Link*> rlinks=get_path((*iter3)->get_id());
             #ifdef _DEBUG_SP
						cout << " gotten path" << endl;
            #endif //_DEBUG_SP
						if (rlinks.size() > 0)
						{
							int frontid=(rlinks.front())->get_id();
            #ifdef _DEBUG_SP
							cout << " gotten front " << endl;
            #endif //_DEBUG_SP
							if (frontid!=(*iter2)->get_id())
	       						rlinks.insert(rlinks.begin(),(*iter2)); // add the root link to the path
							routenr++;
            #ifdef _DEBUG_SP
							cout << " checking if the routenr does not already exist " << endl;
            #endif //_DEBUG_SP
							assert ( (find_if (routes.begin(),routes.end(), compare <Route> (routenr))) == routes.end() ); // No route with routenr exists
            #ifdef _DEBUG_SP
							cout << " making route " << endl;
            #endif //_DEBUG_SP
							Route* rptr=new  Route(routenr, (*iter1), (*iter3), rlinks);
							bool not_exists=false;
							if (rptr)
								not_exists= ( (find_if (routes.begin(),routes.end(), equalmembers <Route> (*rptr))) == routes.end() ); // find if there's a route with the same links
							if (not_exists)
							{	
								routes.insert(routes.end(),rptr); // add the newly found route
					 		}
					 		else
					 			routenr--;  	
					 
						}
					}		
   		   		 }
			}                	
		}
    }
	return true;
}
*/
// NEW Try
bool Network::shortest_paths_all()	
//calculate the shortest paths for each link emanating from each origin to each destination;
// and saving them if  there is a new path found (i.e. it's not in the routes vector already)
{		
	double entrytime=0.0;    // entry time for time-variant shortest path search
	int nr_reruns=static_cast<int> (runtime/theParameters->update_interval_routes)-1; // except for last period
	// determines the number of reruns of the shortest path alg.
	int routenr=routes.size();
	for (int i=0; i<nr_reruns; i++)
	{
		entrytime= i*theParameters->update_interval_routes;
		int lastorigin = -1;
		for (vector<ODpair*>::iterator iter1=odpairs.begin(); iter1<odpairs.end();)
		{
			// OD pairs are sorted by origin, destination
			// For each origin in OD pairs, find the destinations that need another route
			Origin* ori = (*iter1)->get_origin();
			lastorigin = ori->get_id();
			cout << "last origin: " << lastorigin << endl;
			vector <Destination*> dests;
			bool exitloop = false;
			while  (!exitloop)
			{	
				double od_rate= (*iter1)->get_rate();
				double nr_routes= (*iter1)->get_nr_routes();
				if ( ((od_rate > small_od_rate) && ( (od_rate/small_od_rate) < nr_routes)) || (nr_routes < 1) )
					// if the od pair has not too many routes for its size
					dests.push_back((*iter1)->get_destination());
				iter1++;
				if (iter1 == odpairs.end())
					exitloop = true;
				else
					if (((*iter1)->get_origin()->get_id()) != lastorigin )
						exitloop = true;
			}
			cout << " dests size is: " << dests.size() << endl;
			vector<Link*> outgoing=ori->get_links();
			for (vector<Link*>::iterator iter2=outgoing.begin();iter2<outgoing.end();iter2++)
		    {
				
			//	 #ifdef _DEBUG_SP
				cout << "shortest_paths_all: starting label correcting from root " << (*iter2)->get_id() << endl;
			//	#endif //_DEBUG_SP
		    #ifndef _USE_VAR_TIMES
			    graph->labelCorrecting((*iter2)->get_id());  // find the shortest path from Link (*iter2) to ALL nodes
			#else
				if (linkinfo) // if there are link info times
					graph->labelCorrecting((*iter2)->get_id(),entrytime,linkinfo);  // find the shortest path from Link (*iter2) to ALL nodes
				else
					graph->labelCorrecting((*iter2)->get_id());  // find the shortest path from Link (*iter2) to ALL nodes NO LINKINFO
	        #endif // _USE_VAR_TIMES
			#ifdef _DEBUG_SP
    			cout << "finished label correcting for root link "<<(*iter2)->get_id() << endl;
			#endif //_DEBUG_SP
				for (vector<Destination*>::iterator iter3=dests.begin();iter3<dests.end();iter3++)
				{				
				#ifdef _DEBUG_SP
					cout << " see if we can reach destination " << (*iter3)->get_id()<< endl;
				#endif //_DEBUG_SP
					if (graph->reachable((*iter3)->get_id())) // if the destination is reachable from this link...
					{
					#ifdef _DEBUG_SP
						cout << " it's reachable.." << endl;
					#endif //_DEBUG_SP
						vector<Link*> rlinks=get_path((*iter3)->get_id());
					#ifdef _DEBUG_SP
						cout << " gotten path" << endl;
					#endif //_DEBUG_SP
						if (rlinks.size() > 0)
						{
							int frontid=(rlinks.front())->get_id();
						#ifdef _DEBUG_SP
							cout << " gotten front " << endl;
						#endif //_DEBUG_SP
							if (frontid!=(*iter2)->get_id())
	       						rlinks.insert(rlinks.begin(),(*iter2)); // add the root link to the path
							routenr++;
						#ifdef _DEBUG_SP
							cout << " checking if the routenr does not already exist " << endl;
						#endif //_DEBUG_SP
							assert ( (find_if (routes.begin(),routes.end(), compare <Route> (routenr))) == routes.end() ); // No route with routenr exists
						#ifdef _DEBUG_SP
							cout << " making route " << endl;
						#endif //_DEBUG_SP
							Route* rptr=new  Route(routenr, ori, (*iter3), rlinks);
							bool not_exists=false;
							if (rptr)
								not_exists= ( (find_if (routes.begin(),routes.end(), equalmembers <Route> (*rptr))) == routes.end() ); // find if there's a route with the same links
							if (not_exists)
							{	
								routes.insert(routes.end(),rptr); // add the newly found route
					 		}
					 		else
					 			routenr--;  	
					 
						}
					}		
   		   		}
			}                	
		}
    }
	return true;
}


/* Old ************
bool Network::shortest_alternatives_all (int lid, double penalty)
{
	double cost=(graph->linkCost (lid)) + penalty;
	graph->linkCost(lid, cost);
	for (vector<Link*>::iterator iter1=links.begin();iter1<links.end();iter1++)
	{	
		graph->labelCorrecting((*iter1)->get_id());
		for (vector<Destination*>::iterator iter2=destinations.begin(); iter2<destinations.end();iter2++)
		{	
		 	if (graph->reachable ((*iter2)->get_id()))
		 	{
				
				vector<Link*> rlinks=get_path((*iter2)->get_id());
				#ifdef _DEBUG_SP
					cout << " network::shortest_alternatives from link " << (*iter1)->get_id() << " to destination " <<(*iter2)->get_id() << endl;
			 		graph->printPathToNode((*iter2)->get_id());
				#endif //_DEBUG_SP
				//save the found remainder in the link table
				int frontid=(rlinks.front())->get_id();
			//	if (frontid==(*iter1)->get_id())
	       //				rlinks.erase(rlinks.begin()); // remove the root link from the path
	        // let's makes sure the current link is in the path
	       	if (frontid!=(*iter1)->get_id())
	       				rlinks.insert(rlinks.begin(), (*iter1)); // a the rddoot link from the path
				(*iter1)->add_alternative((*iter2)->get_id(),rlinks);
			}
		}
	}
	return true;
}
***/

bool Network::find_alternatives_all (int lid, double penalty, Incident* incident)
// Makes sure that each affected link has an alternative
{
	map <int, map <int,Link*>> affected_links_per_dest; // indexed by destination, each destination will have a nr of affected links
	map <int, Origin*> affected_origins; // Simple map of affected origins
	map <int, Link*> affected_links; // simple map of affected links
	map <int, map <int,Link*>> affected_links_without_alternative;// indexed by destination
	map <int, set <int>> aff_dests_per_link; // easier to do shortest path search on: map <linkid, vector <destid>>
	// Find all the affected links
	Link* incident_link=linkmap[lid];
	multimap <int, Route*> routemap = incident_link->get_routes();// get all routes through incident link
	unsigned int nr_affected_routes = routemap.size();
	multimap <int, Route*>::iterator rmiter=routemap.begin();
	// get all affected links from each route, and store the destinations as well
	for (rmiter;rmiter != routemap.end(); rmiter++)
	{
		Route* r = (*rmiter).second;
		int dest =(*rmiter).first;
		vector <Link*> affectedlinks = r->get_upstream_links(lid);
		vector<Link*>::iterator l_iter;
		for (l_iter=affectedlinks.begin();l_iter!=affectedlinks.end();l_iter++)
		{
			Link* link = (*l_iter);
			int link_id =link->get_id();
			affected_links_per_dest [dest] [link_id] = link;  // stores all affected links, per destination
			affected_links [link_id] = link; // stores all affected links, once
		}
		Origin* ori = r->get_origin();
		int oid = ori->get_id();
		affected_origins [oid] = ori;
	}
// per destination, for all affected links, find out if they have an alternative that does not go through incident link
	map<int, map <int,Link*>>::iterator lm_iter=affected_links_per_dest.begin();
	for (lm_iter; lm_iter!=affected_links_per_dest.end(); lm_iter++)
	{
		int dest = (*lm_iter).first;
		map <int,Link*> thelinks = (*lm_iter).second;
		map <int,Link*>::iterator linkiter=thelinks.begin();
		for (linkiter; linkiter != thelinks.end(); linkiter++)
		{
			 Link* link = linkiter->second;
			 link->set_selected(true); // set the affected link icon to 'selected colour'
			 int linkid=link->get_id();
			 int nr_alternatives = link->nr_alternative_routes(dest,lid );
			 if (nr_alternatives == 0 )
			 {
				 affected_links_without_alternative[dest] [linkid] = link; 
				 // maybe do something smarter here, store by link_id (will be root in shortest path search), and then a list of destinations.
				 aff_dests_per_link [linkid].insert(dest);

#ifndef _NO_GUI  
				 link->set_selected_color(Qt::red); // set red colour for Affected links without alternatives
#endif
			}
		}
	}	



	// Add the affected links & origins to the Incident
	incident->set_affected_links(affected_links);
	incident->set_affected_origins(affected_origins);


	// IF affected_links_without_alternative is NOT empty
	if (!(affected_links_without_alternative.empty()))
	{
		// DO a shortest path init, and a shortest path search wih penalty for incident link to create alternatives for each link.
		bool initok=false;
		if (!theParameters->shortest_paths_initialised)
			initok = init_shortest_path();
		if (initok)
	 {
		 map <int, set <int >>::iterator mi = aff_dests_per_link.begin();
		 for (mi; mi != aff_dests_per_link.end();mi++)
		 {
			 // get shortest path and add.
			 double cost=(graph->linkCost (lid)) + penalty;
			 int root = mi->first;
			 Link* rootlink=linkmap[root];
			 set <int> dests = mi->second;
			 graph->linkCost(lid, cost);
			 graph->labelCorrecting(root);
			 for (set<int>::iterator di = dests.begin(); di != dests.end(); di++)
			 {	
				 if (graph->reachable (*di))
				 {

					 vector<Link*> rlinks=get_path((*di));
#ifdef _DEBUG_SP
					 cout << " network::shortest_alternatives from link " << root << " to destination " << (*di) << endl;
					 graph->printPathToNode((*di));
#endif //_DEBUG_SP
					 //save the found remainder in the link table
					 int frontid=(rlinks.front())->get_id();
					 // let's makes sure the current link is in the path
					 if (frontid!=root)
						 rlinks.insert(rlinks.begin(), rootlink); // add the rddoot link from the path
					 rootlink->add_alternative((*di),rlinks);
				 }
			 }
		 }

	 }
	}
	//Now select all links that are affected:
	cout << " nr of routes affected by incident " << routemap.size() << endl;
	cout << " nr of links without alternatives " << affected_links_without_alternative.size() << endl;
	return true;
}

void Network::delete_spurious_routes()
{
/******** OLD
	 // CHECK THAT ONLY ROUTES FOR EXISTING ODPAIRS ARE GENERATED
	 for (vector<Route*>::iterator iter1=routes.begin();iter1<routes.end();)
	 {
      bool odpair_exists=false;
      int dest=(*iter1)->get_oid_did().first;
      int org=(*iter1)->get_oid_did().second;
      cout << "testing  org " << org <<",dest " << dest << endl;
      for (vector <ODpair*>::iterator iter2=odpairs.begin();iter2 < odpairs.end();iter2++)
      {
        if (((*iter2)->odids().first == org) && ((*iter2)->odids().second == dest))
          		odpair_exists = true;
			}
    	if (odpair_exists == false) // delete found routes that have no OD pair
     	{
       	delete (*iter1); // calls automatically destructor
				iter1=routes.erase(iter1);	
			}
			else
				iter1++;
		}
*/	
}

void Network::renum_routes ()
{
	vector<Route*>::iterator route;
	int counter=0;
	for (route=routes.begin(); route < routes.end(); route++, counter++)
	{
		(*route)->set_id(counter);
		
	}
	
}


bool Network::readmaster(string name)
{
	string temp;
	ifstream inputfile(name.c_str());
	assert (inputfile);
	inputfile >> temp;
	if (temp!="#input_files")
		return false;
	inputfile >> temp;
	if (temp!="network=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #0
	inputfile >> temp;
	if (temp!="turnings=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #1

	// INSERT SIGNALS HERE 
	inputfile >> temp;
	if (temp!="signals=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #2

	inputfile >> temp;
	if (temp!="histtimes=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #3
	inputfile >> temp;
	if (temp!="routes=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #4
	inputfile >> temp;
	if (temp!="demand=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); //  #5
	inputfile >> temp;
	if (temp!="incident=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #6
	inputfile >> temp;
	if (temp!="vehicletypes=")
		return false;
	inputfile >> temp;	
	filenames.push_back(workingdir+temp); // #7	
	inputfile >> temp;
	if (temp!="virtuallinks=")
		return false;
	inputfile >> temp;	
	filenames.push_back(workingdir+temp); // #8		
	inputfile >> temp;
	if (temp!="serverrates=")
		return false;
	inputfile >> temp;	
	filenames.push_back(workingdir+temp); // #9			
	inputfile >> temp;
	if (temp!="#output_files")
		return false;
	inputfile >> temp;
	if (temp!="linktimes=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #10
	inputfile >> temp;
	if (temp!="output=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #11
	inputfile >> temp;
	if (temp!="summary=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); //  #12

	inputfile >> temp;
	if (temp!="speeds=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #13

	inputfile >> temp;
	if (temp!="inflows=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #14
	inputfile >> temp;
	if (temp!="outflows=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); //  #15
	inputfile >> temp;
	if (temp!="queuelengths=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #16
	inputfile >> temp;
	if (temp!="densities=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #17


	inputfile >> temp;
	if (temp!="#scenario")
		return false;
	// insert Start time here
	inputfile >> temp;
	if (temp!="starttime=")
		return false;
	inputfile >> starttime;

	inputfile >> temp;
	if (temp!="stoptime=") // stoptime==runtime
		return false;
	inputfile >> runtime;
	inputfile >> temp;
	if (temp!="calc_paths=")
	{
		calc_paths=false;
		return true;
	}
	else
	{
		inputfile >> calc_paths;
	}
	inputfile >> temp;
	if (temp!="traveltime_alpha=")
		return false;
	inputfile >> time_alpha;

	inputfile >> temp;
	if (temp!="parameters=")
		return false;
	inputfile >> temp;
	filenames.push_back(workingdir+temp);   //  #18 Parameters

#ifdef _VISSIMCOM	
	inputfile >> temp;
	if (temp!="vissimfile=")
	{
		//cout << "No vissimfile specified in masterfile" << endl;
		return true;
	}
	else
	{	
		// NOTE: here the full path is specified! (since it may be somewhere else)
		inputfile >> vissimfile; // read in separate var, because this part is only used with VISSIMCOM
	}
#endif //_VISSIMCOM
	inputfile >> temp;
	if (temp!="background=")
	{
		//cout << "No background specified in masterfile" << endl;
		return true;

	}
	if (inputfile >> temp)
		filenames.push_back(workingdir+temp); //  #19	
	return true;
}

#ifndef _NO_GUI
double Network::executemaster(QPixmap * pm_,QMatrix * wm_)
{
	pm=pm_;
	wm=wm_;
	time=0.0;
	if (!readparameters(filenames [18]))
		cout << "Problem reading parameters: " << filenames [18] << endl; // read parameters first

	if (!readvtypes(filenames[7]))
		cout << "Problem reading vtypes: " << filenames [6] << endl; // read the vehicle types first
	if (!readnetwork(filenames[0]))
		cout << "Problem reading network: " << filenames [0] << endl; // read the network configuration
	if(!readvirtuallinks(filenames[8]))
		cout << "Problem reading virtuallinks: " << filenames [7] << endl;	//read the virtual links
	if(!readserverrates(filenames[9]))
		cout << "Problem reading serverrates: " << filenames [8] << endl;	//read the virtual links		
	if (!register_links())
		cout << "Problem reading registering links at nodes "<< endl; // register the links at the destinations, junctions and origins
	if (!(readturnings(filenames[1])))
	{
		cout << "no turnings read, making new ones...." << endl;
		create_turnings(); // creates the turning movements for all junctions    if not read by file
		writeturnings(filenames[1]); // writes the new turnings
	}

	if (!(readlinktimes(filenames[3])))
	{
		cout << "no linktimes read, taking freeflow times... " << endl;
		setlinktimes();  //read the historical link times if exist, otherwise set them to freeflow link times.
	}

// 2005-11-28 put the reading of OD matrix before the paths...
   if (!readinput(filenames[5]))
		cout << "Problem reading OD matrix " << filenames [5] << endl; // generate the odpairs.
  //Sort the ODpairs
  sort (odpairs.begin(), odpairs.end(), od_less_than);


   if (!(readpathfile(filenames[4]))) // read the known paths
   {
		cout << "no routes read from the pathfile" << endl;
     	calc_paths=true; // so that new ones are calculated.
   }
   if (calc_paths)
   {
		if (!init_shortest_path())
	 		cout << "Problem starting init shortest path " << endl; // init the shortest paths
		if (!shortest_paths_all())
			cout << "Problem calculating shortest paths for all OD pairs " << endl; // see if there are new routes based on shortest path
   }
  // Sort the routes by OD pair
  sort(routes.begin(), routes.end(), route_less_than);
  // add the routes to the OD pairs
  add_od_routes();
  // temporary
  renum_routes (); // renum the routes
  writepathfile(filenames[4]); // write back the routes.
  // end temporary
  if (calc_paths)
	{
		delete_spurious_routes();
		writepathfile(filenames[4]); // write back the routes.
	}

	this->readsignalcontrols(filenames[2]);
// NEW 2007_03_08
#ifdef _BUSES
	// read the busroutes & Lines
	this->readbusroutes (workingdir + "busroutes.dat"); //FIX IN THE MAIN READ & WRITE
	this->readbuslines (workingdir + "buslines.dat"); //FIX IN THE MAIN READ & WRITE
#endif // _BUSES
	if (!init())
		cout << "Problem initialising " << endl;
	if (!readincidentfile(filenames[6]))
		cout << "Problem reading incident file " << filenames [5] << endl; // reads the incident file   and makes all the alternative routes at all  links
	if (filenames.size() >= 20)
		drawing->set_background(filenames[19].c_str());	
	if (theParameters->use_ass_matrix) 
	{
		this->readassignmentlinksfile (workingdir + "assign_links.dat"); //FIX IN THE MAIN READ & WRITE
	}
   
	return runtime;
}
#endif // _NO_GUI	

double Network::executemaster()
{
	time=0.0;
	if (!readparameters(filenames [18]))
		cout << "Problem reading parameters: " << filenames [18] << endl; // read parameters first

	if (!readvtypes(filenames[7]))
		cout << "Problem reading vtypes: " << filenames [6] << endl; // read the vehicle types
	if (!readnetwork(filenames[0]))
		cout << "Problem reading network: " << filenames [0] << endl; // read the network configuration
	if(!readvirtuallinks(filenames[8]))
		cout << "Problem reading virtuallinks: " << filenames [7] << endl;	//read the virtual links
	if(!readserverrates(filenames[9]))
		cout << "Problem reading serverrates: " << filenames [8] << endl;	//read the virtual links
	if (!register_links())
		cout << "Problem reading registering links at nodes "<< endl; // register the links at the destinations, junctions and origins
	if (!(readturnings(filenames[1])))
	{
		cout << "no turnings read, making new ones...." << endl;
		create_turnings(); // creates the turning movements for all junctions    if not read by file
		writeturnings(filenames[1]); // writes the new turnings
	}
	if (!(readlinktimes(filenames[3])))
	{
		cout << "no linktimes read, taking freeflow times... " << endl;
		setlinktimes();  //read the historical link times if exist, otherwise set them to freeflow link times.
	}
// New 2005-11-28 put the reading of OD matrix before the paths...
   if (!readinput(filenames[5]))
		cout << "Problem reading OD matrix " << filenames [4] << endl; // generate the odpairs.
  //Sort the ODpairs
  sort (odpairs.begin(), odpairs.end(), od_less_than);


   if (!(readpathfile(filenames[4]))) // read the known paths
   {
		cout << "no routes read from the pathfile" << endl;
     	calc_paths=true; // so that new ones are calculated.
   }   
   if (calc_paths)
   {
		if (!init_shortest_path())
	 		cout << "Problem starting init shortest path " << endl; // init the shortest paths
		if (!shortest_paths_all())
			cout << "Problem calculating shortest paths for all OD pairs " << endl; // see if there are new routes based on shortest path
   }
  // Sort the routes by OD pair
  sort(routes.begin(), routes.end(), route_less_than);
  // add the routes to the OD pairs
  add_od_routes();
  renum_routes ();
  // temporary
  writepathfile(filenames[4]); // write back the routes.
  // end temporary
  if (calc_paths)
	{
		delete_spurious_routes();
		writepathfile(filenames[4]); // write back the routes.
	}

	readsignalcontrols(filenames[2]);
#ifdef _BUSES
	// NEW 2007_03_08
	// read the busroutes & Lines
	this->readbusroutes (workingdir +"busroutes.dat");//FIX IN THE MAIN READ & WRITE
	this->readbuslines (workingdir +"buslines.dat");//FIX IN THE MAIN READ & WRITE
#endif //_BUSES

	if (!init())
		cout << "Problem initialising " << endl;
	if (!readincidentfile(filenames[6]))
		cout << "Problem reading incident file " << filenames [5] << endl; // reads the incident file   and makes all the alternative routes at all  links
	if (theParameters->use_ass_matrix) 
	{
		this->readassignmentlinksfile (workingdir + "assign_links.dat"); // !!! WE NEED TO FIX THIS INTO THE MAIN READ& WRITE
	}
	
	return runtime;
}


bool Network::writeall()
{
	writelinktimes(filenames[10]);
	// NEW: Write also the non-smoothed times
	string cleantimes=filenames[10]+".clean";
	time_alpha=1.0;
	writelinktimes(cleantimes);
	////////
	writesummary(filenames[12]); // write the summary first because	
	writeoutput(filenames[11]);  // here the detailed output is written and then deleted from memory
	writemoes();
	writeallmoes("allmoes.dat");
    writeheadways("timestamps.dat");
	writeassmatrices("assign.dat");
	write_v_queues("v_queues.dat");
	 return true;
}

bool Network::writeallmoes(string name)
{
   //cout << "going to write to this file: " << name << endl;
   ofstream out(name.c_str());
	assert(out);
	
	out << "CONVERGENCE" << endl;
	
	out << "SumDiff_InputOutputLinkTimes : " << calc_diff_input_output_linktimes () << endl << endl;
	out << "SumSquare_InputOutputLinkTimes : " << calc_sumsq_input_output_linktimes () << endl << endl;
	out << "MOES" << endl;
/****** TEMPORARY TO CUT OUT THE ALL MOES THAT ARE NOT USED NOW
	int maxindex=0;
	
	for (vector<Link*>::iterator iter=links.begin();iter<links.end();iter++)
	{
		out << (*iter)->get_id() << "\t\t\t\t\t";
		maxindex=_MAX (maxindex, (*iter)-> max_moe_size());
	}
   out << endl;
   for (vector<Link*>::iterator iter1=links.begin();iter1<links.end();iter1++)
	{
		out << "speed"<< "\t" << "density" << "\t" << "inflow" <<"\t" << "outflow" <<"\t" << "queue" << "\t";
	}
	out << endl;
   for (int index=0;index <maxindex; index++)
   {
   		for (vector<Link*>::iterator iter=links.begin();iter<links.end();iter++)
		{
			(*iter)->write_speed(out,index);
		 	(*iter)->write_density(out,index);
		  	(*iter)->write_inflow(out,index);
		  	(*iter)->write_outflow(out,index);
		  	(*iter)->write_queue(out,index);
		}
		out << endl;
   }
   */
   return true;
}

double Network::calc_diff_input_output_linktimes ()
{
	double total =0.0;
	for (vector<Link*>::iterator iter1=links.begin();iter1<links.end();iter1++)
	{	
		if ((*iter1)->get_nr_passed() > 0 )
			total+=(*iter1)->calc_diff_input_output_linktimes();
	}
	return total;
}

double Network::calc_sumsq_input_output_linktimes ()
{
	double total =0.0;
	for (vector<Link*>::iterator iter1=links.begin();iter1<links.end();iter1++)
	{
		if ((*iter1)->get_nr_passed() > 0 )
			total+=(*iter1)->calc_sumsq_input_output_linktimes();
	}
	return total;
}

bool Network::writemoes()
{
	string name=filenames[13]; // speeds
	ofstream out(name.c_str());
	assert(out);
	
	for (vector<Link*>::iterator iter=links.begin();iter<links.end();iter++)
	{

		(*iter)->write_speeds(out);
	}
	out.close();
	name=filenames[14]; // inflows
	out.open(name.c_str());
	assert(out);
	
	for (vector<Link*>::iterator iter1=links.begin();iter1<links.end();iter1++)
	{
		(*iter1)->write_inflows(out);
	}
	out.close();
	name=filenames[15]; // outflows
	out.open(name.c_str());
	assert(out);
	
	for (vector<Link*>::iterator iter2=links.begin();iter2<links.end();iter2++)
	{
		(*iter2)->write_outflows(out);
	}
	out.close();
	name=filenames[16]; // queues
	out.open(name.c_str());
	assert(out);
	
	for (vector<Link*>::iterator iter3=links.begin();iter3<links.end();iter3++)
	{
		(*iter3)->write_queues(out);
	}
	out.close();
	name=filenames[17]; // densities
	out.open(name.c_str());
	assert(out);
	
	for (vector<Link*>::iterator iter4=links.begin();iter4<links.end();iter4++)
	{
		(*iter4)->write_densities(out);
	}
	out.close();
	return true;
}


bool Network::write_v_queues(string name)
{
	ofstream out(name.c_str());
	assert(out);
	for (vector<Origin*>::iterator iter = origins.begin();iter != origins.end(); iter++)
	{
		(*iter)->write_v_queues(out);
	}
	return true;
}


bool Network::writeassmatrices(string name)
{
	ofstream out(name.c_str());
	assert(out);
	out << "no_obs_links: " << no_ass_links << endl;
	int nr_periods = static_cast<int>(runtime / theParameters->ass_link_period);
	out << "no_link_pers: " << nr_periods << endl;
	for (int i=0; i < nr_periods; i++)
	{
		out << "link_period: " << i << endl;
		for (vector<Link*>::iterator iter1=links.begin();iter1<links.end(); iter1++)
		{
			(*iter1)->write_ass_matrix(out,i);
		}
		//out << endl;
	}
	return true;
	
}


bool Network::init()

{
	// initialise the turining events
	double initvalue =0.1;
	for(vector<Turning*>::iterator iter=turnings.begin(); iter<turnings.end(); iter++)
	{
	 	(*iter)->init(eventlist,initvalue);
		initvalue += 0.00001;
	}
    // initialise the signal controllers (they will init the plans and stages)
    for (vector <SignalControl*>::iterator iter1=signalcontrols.begin(); iter1<signalcontrols.end(); iter1++)
	{
		(*iter1)->execute(eventlist,initvalue);
		initvalue += 0.000001;
	//	cout << "Signal control initialised " << endl;
	}
#ifdef _BUSES
	// Initialise the buslines
	for (vector <Busline*>::iterator iter3=buslines.begin(); iter3 < buslines.end(); iter3++)
	{
		(*iter3)->execute(eventlist,initvalue);
		initvalue+=0.00001;
	}
#endif //_BUSES
#ifdef _DEBUG_NETWORK	
	cout << "turnings initialised" << endl;
#endif //_DEBUG_NETWORK	
	// initialise the od pairs and their events
	for(vector<ODpair*>::iterator iter0=odpairs.begin(); iter0<odpairs.end();)
	{
		if ((*iter0)->get_nr_routes() == 0) //chuck out the OD pairs without paths
		{
	//#ifdef _DEBUG_NETWORK
			cout << "OD pair " << (*iter0)->get_origin()->get_id() << " - " <<
				(*iter0)->get_destination()->get_id() << " does not have any route connecting them. deleting..." << endl;
	//#endif //_DEBUG_NETWORK
		delete *iter0;
			iter0=odpairs.erase(iter0);
		}
		else // otherwise initialise them
		{
			(*iter0)->execute(eventlist,initvalue);
			initvalue += 0.00001;
			iter0++;
		}
	}

/* // OLD INIT, redundant, put it together with the cleaning up of non-connected OD pairs.
	for(vector<ODpair*>::iterator iter1=odpairs.begin(); iter1<odpairs.end(); iter1++)
	{
	 	(*iter1)->execute(eventlist,initvalue);
		initvalue += 0.00001;
	}
	*/
#ifdef _DEBUG_NETWORK	
	cout << "odpairs initialised" << endl;
	cout << "number of destinations " <<destinations.size() <<endl;
#endif //_DEBUG_NETWORK	
	// initialise the destination events
//	register_links();
	for (vector<Destination*>::iterator iter2=destinations.begin(); iter2<destinations.end();iter2++)
	{
		(*iter2)->execute(eventlist,initvalue);
		initvalue += 0.00001;
	}
	// initialise origin events
	// 2004-01-10: take out the origin actions
	// 2005-11-28: If we put back the origin actions, we need to put back the route generation before the input reading.
  /*
	for (vector<Origin*>::iterator iter=origins.begin(); iter<origins.end();iter++)
	{
		(*iter)->initialise();
		(*iter)->execute(eventlist,0.1);
	}
	*/

	// Initialise the communication
#ifdef _MIME
	if (communicator->is_connected())
	{
#ifdef _PVM
		communicator->initsend();
#endif //_PVM
#ifdef _VISSIMCOM
		communicator->register_virtuallinks(&virtuallinks); // register the virtual links
		communicator->init(vissimfile,runtime);
		
#endif // _VISSIMCOM
		communicator->register_boundaryouts(&boundaryouts);          // register the boundary nodes that send messages
		communicator->register_boundaryins(&boundaryins);          // register the boundary nodes that receive messages
		
		communicator->execute(eventlist,initvalue); // book yourself for the first time.
		initvalue += 0.000001;
	}
#endif // _MIME
   #ifndef _NO_GUI
      recenter_image();
   #endif // _NO_GUI    
	return true;
}

bool Network::run(int period)
{
 	// This part will be transferred to the GUI
 	
 	double t0=timestamp();
 	double tc;
 	double next_an_update=t0+an_step;
   #ifndef _NO_GUI
 	drawing->draw(pm,wm);
  #endif //_NO_GUI
 	//eventhandler->startup();
 	double time=0.0;
 	while ((time>-1.0) && (time<period))       // the big loop
 	{
 	  time=eventlist->next();
 	  if (time > (next_an_update-t0)*speedup)  // if the sim has come far enough
 	  {
 	  	tc=timestamp();
 	  	while (tc < next_an_update)  // wait till the next animation update
 	    {
 	      	tc=timestamp();
 	  	 }
 	//	eventhandler->startup();     //update animation
   #ifndef _NO_GUI
   drawing->draw(pm,wm);
   #endif // _NO_GUI
 	  	next_an_update+=an_step;    // update next step.
 	   }
 	}
 	double tstop=timestamp();
	
  	//cout << "running time " << (tstop-t0) << endl;
   return 0;
 	
 }


 double Network::step(double timestep)
 // same as run, but more stripped down. Called every timestep by the GUI
 {
   double t0=timestamp();
#ifndef _NO_GUI
   double tc; // current time
#endif //_NO_GUI  
   double next_an_update=t0+timestep;   // when to exit
 	while ((time>-1.0) && (time<runtime))       // the big loop
 	{
 	  time=eventlist->next();
#ifndef _NO_GUI
 	  tc=timestamp();
 	  if (tc > next_an_update)  // the time has come for the next animation update
 	    {
 	    	drawing->draw(pm,wm);
 	      	return time;
 	  	 } 	
#endif //_NO_GUI  


 	}
 	return time;
 }


 #ifndef _NO_GUI
 // Graphical funcitons

void Network::recenter_image()
{
   wm->reset(); // resets the worldmatrix to unary matrix (neutral zoom and pan)
   // position the drawing in the center of the pixmap
   vector <int> boundaries = drawing->get_boundaries();

   width_x = (double)(boundaries[2]-boundaries[0]);
   height_y = (double)(boundaries[3]-boundaries[1]);
   double scale_x = (pm->width()) / width_x;
   double scale_y = (pm->height()) / height_y;

   scale = _MIN (scale_x,scale_y);
   // cout << "scales. x: " << scale_x << " y: " << scale_y <<" scale: " << scale <<  endl;
   wm->translate(boundaries[0],boundaries[1]); // so that (minx,miny)=(0,0)
   
   // center the image
   if (scale_x > scale_y)// if the Y dimension determines the scale
   {	
	   double move_x = (pm->width() - (width_x*scale_y))/2; //
	   wm->translate (move_x,0);
   }
   else
   {
	   double move_y = (pm->height() - (height_y*scale_x))/2; //
	   wm->translate (0,move_y);	
   }
   wm->scale(scale,scale);   
}

/**
*	initialize to fit the network boundaries into 
*   the standard graphic view:  
*	a) transform from model coordinate (O')  
*      to standar view coordinate (O) 
*		X=sxy*(X'-Xmin')
*		Y=sxy*(Y'-Ymin')
*	b) move the overscaled dimension to centre
*   IMPORTANT: the order of operation is
*	step 1: translate (-Xmin', -Ymin')
*   step 2: scale the coordinate with a factor=min{view_X/model_X', view_Y/model_Y'}
*   step 3: translate the overscaled dimension
*   PROOF BY M=M1*M2*M3
*/
QWMatrix Network::netgraphview_init()
{	
   // make sure initial worldmatrix is a unit matrix 
   initview_wm.reset(); 
   
   vector <int> boundaries = drawing->get_boundaries();
   //initview_wm.translate(boundaries[0],boundaries[1]); 
   
   //scale the image and then centralize the image along
   //the overscaled dimension 
   width_x = (double)(boundaries[2]-boundaries[0]);
   height_y = (double)(boundaries[3]-boundaries[1]);
   double scale_x = (pm->width())/width_x;
   double scale_y = (pm->height())/height_y;
 
   if (scale_x > scale_y){	
	   scale=scale_y;	
	   // the x dimension is overscaled
	   double x_adjust =pm->width()/2-width_x*scale/2;
	   initview_wm.translate(x_adjust,0);
	   initview_wm.scale(scale,scale);
   }
   else{
	   scale=scale_x;
	   double y_adjust = pm->height()/2-height_y*scale/2;
	   initview_wm.translate(0,y_adjust);
	   initview_wm.scale(scale,scale);
   }
   initview_wm.translate(-boundaries[0],-boundaries[1]);
   // make a copy to "wm"
   (*wm)=initview_wm;
   // return the information to the canvas 
   return initview_wm;
}


void Network::redraw() // redraws the image
{
	drawing->draw(pm,wm);
}

#endif // _NO_GUI
 
 // INCIDENT FUNCTIONS

void Network::set_incident(int lid, int sid, bool blocked)
 {
	 //cout << "incident start on link " << lid << endl;
 	//Link* lptr=(*(find_if (links.begin(),links.end(), compare <Link> (lid) ))) ;
	 Link* lptr = linkmap [lid];
 	//Sdfunc* sdptr=(*(find_if (sdfuncs.begin(),sdfuncs.end(), compare <Sdfunc> (sid) ))) ;
 	Sdfunc* sdptr = sdfuncmap [sid];
	lptr->set_incident (sdptr, blocked);
 }

void Network::unset_incident(int lid)
 {
 	//cout << "end of incident on link  "<< lid << endl;
	//Link* lptr=(*(find_if (links.begin(),links.end(), compare <Link> (lid) ))) ;
	Link* lptr = linkmap [lid];
	lptr->unset_incident ();
 }

void Network::broadcast_incident_start(int lid)
{
 // for all links inform and if received, apply switch algorithm
	//cout << "BROADCAST incident on link  "<< lid << endl;
 	for (vector <Link*>::iterator iter=links.begin();iter<links.end();iter++)
 	{
 	 (*iter)->broadcast_incident_start(lid,incident_parameters);  	
	}
	// for all origins : start the automatic switiching algorithm
	for (vector <Origin*>::iterator iter1=origins.begin();iter1<origins.end();iter1++)
 	{
 	 (*iter1)->broadcast_incident_start(lid,incident_parameters);  	
	}

}


void Network::broadcast_incident_stop(int lid)
{	//cout << "BROADCAST END of incident on link  "<< lid << endl;
	// for all origins: stop the automatic switching stuff
   for (vector <Origin*>::iterator iter=origins.begin();iter<origins.end();iter++)
 	{
 	 (*iter)->broadcast_incident_stop(lid);  	
	}
}

void Network::removeRoute(Route* theroute)
{
	for(vector<Route*>::iterator it=routes.begin(); it!=routes.end(); it++)
	{
		if((*it)==theroute)
		{
			routes.erase(it);
			break;
		}
	}
}

Incident::Incident (int lid_, int sid_, double start_, double stop_, double info_start_,double info_stop_, Eventlist* eventlist, Network* network_, bool blocked_):start(start_), stop(stop_),
	info_start(info_start_), info_stop(info_stop_), lid(lid_), sid(sid_),network(network_), blocked (blocked_)
{
	eventlist->add_event(start,this);
}

bool Incident::execute(Eventlist* eventlist, double time)
{
	//cout << "incident_execute time: " << time << endl;

	// In case no information is Broadcasted:
	if ((info_start < 0.0) || (info_stop<0.0)) // there is no information broadcast
	{
		// #1: Start the incident on the link
	 	if (time < stop)
	 	{
	     	network->set_incident(lid, sid, blocked); // replace sdfunc with incident sd function
 	 		eventlist->add_event(stop,this);
	 	}
		// #2: End the incident on the link
	    else
	    {
	      network->unset_incident(lid);
	    }
	}
	
	else
	{
		// #1: Start the incident on the link
		if (time < info_start)
		{
 	 		network->set_incident(lid, sid, blocked); // replace sdfunc with incident sd function
 	 		eventlist->add_event(info_start,this);
 	 	}
		// #2: Start the broadcast of Information
 		else if (time < stop)
 		{	
 			network->broadcast_incident_start(lid);
			// TO DO: Change the broadcast to only include only the affected links and origins
			eventlist->add_event(stop,this);
		}
		// #3: End the incident on the link
		else if (time < info_stop)
    	{

    	 	network->unset_incident(lid);
     		eventlist->add_event(info_stop,this);
    	}
		// #4: End the broadcast of Information
    	else
    	{
     		network->broadcast_incident_stop(lid);
    	}
    }
    return true;
}

// ODMATRIX CLASSES

ODMatrix::ODMatrix (){}

void ODMatrix::add_slice(double time, ODSlice* slice)
{
  slices.insert(slices.end(), (pair <double,ODSlice*> (time,slice)) );
}

// MATRIXACTION CLASSES

MatrixAction::MatrixAction(Eventlist* eventlist, double time, ODSlice* slice_, vector<ODpair*> *ods_)
{
 slice=slice_;
 ods=ods_;
 eventlist->add_event(time, this);
}

bool MatrixAction::execute(Eventlist* eventlist, double time)
{
  assert (eventlist != NULL);
	//cout << time << " : MATRIXACTION:: set new rates "<< endl;
// for all odpairs in slice

 	for (vector <ODRate>::iterator iter=slice->rates.begin();iter<slice->rates.end();iter++)
 	{
	// find odpair
		ODpair* odptr=(*(find_if (ods->begin(),ods->end(), compareod (iter->odid) )));
		//init new rate
		odptr->set_rate(iter->rate);
	}
 return true;
}