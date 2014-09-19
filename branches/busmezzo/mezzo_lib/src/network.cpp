
#include "gettime.h"
#include <assert.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <set>
#include <math.h>
#include <windows.h>
//#include <strstream> // OLD include for gcc2


#include "network.h"
#include "od.h"

//using namespace std;

// initialise the global variables and objects

long int randseed=0;
int vid=0;
int pid=0;
long pathid=0;
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
	//eventhandle=new Eventhandle(*drawing);
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
	delete linkinfo;
	delete eventlist;
	
#ifdef _MIME
	delete communicator;
#endif //_MIME
	for (map <int, Origin*>::iterator iter=originmap.begin();iter!=originmap.end();)
	{			
		delete (iter->second); // calls automatically destructor
		iter=originmap.erase(iter);	
	}
	for (map <int, Destination*>::iterator iter1=destinationmap.begin();iter1!=destinationmap.end();)
	{			
		delete (iter1->second); // calls automatically destructor
		iter1=destinationmap.erase(iter1);	
	}
	for (map <int, Junction*>::iterator iter2=junctionmap.begin();iter2!=junctionmap.end();)
	{			
		delete (iter2->second); // calls automatically destructor
		iter2=junctionmap.erase(iter2);	
	}
	for (map <int,Node*>::iterator iter3=nodemap.begin();iter3!=nodemap.end();)
	{			
		iter3=nodemap.erase(iter3);	
	}
	for (map <int,Link*>::iterator iter4=linkmap.begin();iter4!=linkmap.end();)
	{			
		delete (iter4->second); // calls automatically destructor
		iter4=linkmap.erase(iter4);	
	}
	for (vector <Incident*>::iterator iter5=incidents.begin();iter5<incidents.end();)
	{			
		delete (*iter5); // calls automatically destructor
		iter5=incidents.erase(iter5);	
	}	

	// for now keep OD pairs in vector
	for (vector <ODpair*>::iterator iter6=odpairs.begin();iter6!=odpairs.end();)
	{
		delete (*iter6);
		iter6=odpairs.erase(iter6);
	}
	for (multimap <odval, Route*>::iterator iter7=routemap.begin();iter7!=routemap.end();)
	{			
		delete (iter7->second); // calls automatically destructor
		iter7=routemap.erase(iter7);	
	}
	for (map <int, Sdfunc*>::iterator iter8=sdfuncmap.begin();iter8!=sdfuncmap.end();)
	{			
		delete (iter8->second); // calls automatically destructor
		iter8=sdfuncmap.erase(iter8);	
	}
	for (map <int, Server*>::iterator iter9=servermap.begin();iter9!=servermap.end();)
	{			
		delete (iter9->second); // calls automatically destructor
		iter9=servermap.erase(iter9);	
	}
	for (map <int, Turning*>::iterator iter10=turningmap.begin();iter10!=turningmap.end();)
	{			
		delete (iter10->second); // calls automatically destructor
		iter10=turningmap.erase(iter10);	
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

int Network::reset()
{	
	time=0.0;
	vid = 0;
	// reset eventlist
	eventlist->reset();

	// reset all the network objects
	//Origins
	for (map <int, Origin*>::iterator iter1=originmap.begin();iter1!=originmap.end();iter1++)
	{			
		(iter1->second)->reset();	
	}
	//Destinations
	for (map <int, Destination*>::iterator iter2=destinationmap.begin();iter2!=destinationmap.end();iter2++)
	{			
		(iter2->second)->reset();
	}
	//Junctions
	for (map <int,Junction*>::iterator iter3=junctionmap.begin();iter3!=junctionmap.end();iter3++)
	{			
		(iter3->second)->reset();
	}
	//Links
	for (map <int, Link*>::iterator iter4=linkmap.begin();iter4!=linkmap.end();iter4++)
	{			
		(iter4->second)->reset();
	}
	//Routes
	for (multimap <odval, Route*>::iterator iter5=routemap.begin();iter5!=routemap.end();iter5++)
	{			
		(iter5->second)->reset();
	}
	//OD pairs
	for (vector <ODpair*>::iterator iter6=odpairs.begin();iter6!=odpairs.end();iter6++)
	{			
		(*iter6)->reset();
	}
	// OD Matrix rates : re-book all MatrixActions
	odmatrix.reset(eventlist,&odpairs);
	// turnings
	for (map<int,Turning*>::iterator iter7=turningmap.begin(); iter7!=turningmap.end(); iter7++)
	{
		(iter7->second)->reset();
	}

	for (map <int, Server*>::iterator sv_iter=servermap.begin(); sv_iter!=servermap.end(); sv_iter++)
	{
		(*sv_iter).second->reset();
	}
	for (vector <ChangeRateAction*>::iterator cr_iter=changerateactions.begin(); cr_iter != changerateactions.end(); cr_iter++)
	{
		(*cr_iter)->reset();
	}	
	//traffic signals
	for (vector <SignalControl*>::iterator sc_iter = signalcontrols.begin(); sc_iter != signalcontrols.end(); sc_iter++)
	{
		(*sc_iter)->reset();
	}

	// vehicle types
	vehtypes.initialize();

	// buslines
	for (vector<Busline*>::iterator lines_iter = buslines.begin(); lines_iter < buslines.end(); lines_iter++)
	{
		(*lines_iter)->reset();
	}

	// bustrips
	for (vector<Bustrip*>::iterator trips_iter = bustrips.begin(); trips_iter < bustrips.end(); trips_iter++)
	{
		(*trips_iter)->reset();
	}

	// busstops
	for (vector<Busstop*>::iterator stops_iter = busstops.begin(); stops_iter < busstops.end(); stops_iter++)
	{
		(*stops_iter)->reset();
	}

	// busroutes
	for (vector<Busroute*>::iterator route_iter = busroutes.begin(); route_iter < busroutes.end(); route_iter++)
	{
		(*route_iter)->reset();
	}

	// ODstops, passengers, paths
	for (vector<ODstops*>::iterator odstops_iter = odstops.begin(); odstops_iter < odstops.end(); odstops_iter++)
	{		
		(*odstops_iter)->reset();
	}

	// busvechiles
	for (vector<Bus*>::iterator bus_iter = busvehicles.begin(); bus_iter < busvehicles.end(); bus_iter++)
	{
		(*bus_iter)->reset();
	}

	//TO DO
	
	// incidents

	// all the Hybrid functions: BoundaryIn, BoundaryOut etc.

	// AND FINALLY Init the next run
	bool ok = init();

	return runtime;
}

void Network::end_of_simulation(double time)
{
	for (map<int,Link*>::iterator iter=linkmap.begin();iter != linkmap.end();iter++)
	{
		(*iter).second->end_of_simulation();
	}
}


multimap<odval, Route*>::iterator Network::find_route (int id, odval val)
{
	multimap<odval, Route*>::iterator upper, lower, r_iter;
	lower = routemap.lower_bound(val);
	upper = routemap.upper_bound(val);
	for (r_iter=lower; r_iter!=upper; r_iter++)
	{
		if ((*r_iter).second->get_id() == id)
			return r_iter;
	}
	return routemap.end(); // in case none found
}

bool Network::exists_route (int id, odval val)
{
	return find_route(id,val) != routemap.end();
}

bool Network::exists_same_route (Route* route)
{
	multimap<odval, Route*>::iterator upper, lower, r_iter;
	odval val = route->get_oid_did();
	lower = routemap.lower_bound(val);
	upper = routemap.upper_bound(val);
	for (r_iter=lower; r_iter!=upper; r_iter++)
	{
		if ((*r_iter).second->equals(*route))
			return true;
	}
	return false;
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
		boundaryins.insert(boundaryins.begin(),biptr);
#ifdef _DEBUG_NETWORK  	
		cout << " boundaryin "  << nid;
#endif //_DEBUG_NETWORK  	
	}

	if (type==5) // BOUNDARY OUT
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
		boundaryouts.insert(boundaryouts.begin(),jptr);
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
	int sdid=0, type=0; 
	double vmax=0, vmin=0, romax=0, romin=0;
	double alpha=0.0, beta=0.0;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsdfuncs scanner jammed at " << bracket;
		return false;
	}
	in  >> sdid >> type >> vmax >> vmin >> romax >> romin;
	if ((type==1) ||(type ==2))
		in >> alpha >> beta;
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
		sdptr = new Sdfunc(sdid,vmax,vmin,romax, romin);
	}	else if ((type==1)	|| (type == 2))
	{
		sdptr = new DynamitSdfunc(sdid,vmax,vmin,romax,romin,alpha,beta);
	}
	assert (sdptr);
	sdfuncmap [sdid] = sdptr;

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
	int innode, outnode, length, nrlanes, sdid, lid;
	string name;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readlinks scanner jammed at " << bracket;
		return false;
	}
	in  >> lid >> innode >> outnode >> length >> nrlanes >> sdid >> name;

	if (name == "}")
	{
		bracket = '}';
		name="";
	}
	else
	{
		in >> bracket;
	}
	if (bracket != '}')
	{
		cout << "readfile::readlinks scanner jammed at " << bracket;
		return false;
	}
	// find the nodes and sdfunc pointers

	assert ( (length>0) && (nrlanes > 0) );           // check that the length and nrlanes are positive
#ifndef _UNSAFE
	assert (!linkmap.count(lid));
#endif // _UNSAFE  
	
	assert (nodemap.count(innode));
	Node* inptr = nodemap [innode];
	
	assert (nodemap.count(outnode));
	Node* outptr = nodemap [outnode];
	
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
	link->set_name(name);
	// register the icon in the link
#ifndef _NO_GUI
	link->set_icon(icon);
	icon->set_link(link);
#endif //_NO_GUI 
	linkmap [lid] = link;
	//links.insert(links.end(),link);
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
	{
		in.close();
		return false;
	}
	int nr;
	in >> nr;
	for (int i=0; i<nr;i++)
	{
		if (!readvirtuallink(in))
		{
			in.close();
			return false;
		}
	}
	in.close();
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
	assert (!virtuallinkmap.count(lid));
	assert (nodemap.count(innode));
	Node* inptr = nodemap [innode];
	assert (nodemap.count(outnode));
	Node* outptr = nodemap [outnode];
	assert (boundaryinmap.count(outnode));
	BoundaryIn* biptr = boundaryinmap [outnode];
	assert (boundaryoutmap.count(innode));
	BoundaryOut* boptr = boundaryoutmap [innode];
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
	in  >> sid >> stype;
	
	assert (!servermap.count(sid));
	assert ( (stype > -1) && (stype<5));
	in >> mu >> sd >> delay;
	assert ((mu>=0.0) && (sd>=0.0) && (delay>=0.0)); // to be updated when more server types are added
	// check id, vmax, vmin, romax;
	// type 0= dummy server: Const server
	// type 1=standard N(mu,sd) sever
	// type 2=deterministic (mu) server
    // type 3=stochastic delay server: min_time(mu) + LN(delay, std_delay)
	// type 4=stochastic delay server: LogLogistic(alpha/scale=mu, beta/shape=sd)
	// type -1 (internal) = OD server
	// type -2 (internal)= Destination server
	Server* sptr;
	if (stype==0)
		sptr = new ConstServer(sid,stype,mu,sd,delay);
	if (stype==1)
		sptr = new Server(sid,stype,mu,sd,delay);
	if (stype==2)
		sptr = new DetServer(sid,stype,mu,sd,delay);
	if (stype==3)
	{
		sptr = new LogNormalDelayServer (sid,stype,mu,sd*theParameters->sd_server_scale,delay);
	}
	if (stype==4)
	{
		sptr = new LogLogisticDelayServer (sid,stype,mu,sd,delay);
	}
	assert (sptr);
	servermap [sid] = sptr;

	in >> bracket;
    if (bracket != '}')
    {
  		cout << "readfile::readservers scanner jammed at " << bracket;
  		return false;
	}
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
	{
		inputfile.close();
		return false;
	}
	int nr;
	inputfile >> nr;
	for (int i=0; i<nr;i++)
	{
		if (!readturning(inputfile))
		{
			inputfile.close();
			return false;
		}
	}
	readgiveways(inputfile);
	inputfile.close();
	return true;
}


bool Network::readturning(istream& in)
{

	char bracket;
	int tid, nid, sid, size, inlink, outlink;
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
	map <int, Node*>::iterator node_iter;
	node_iter=nodemap.find(nid);
	assert (node_iter != nodemap.end());
	Node* nptr = (*node_iter).second;
	map <int, Link*>::iterator link_iter;
	link_iter=linkmap.find(inlink);
	assert (link_iter != linkmap.end());
	Link* inlinkptr = (*link_iter).second;
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
	assert (servermap.count(sid));
	Server* sptr = servermap[sid];
#ifndef _UNSAFE
	assert (!turningmap.count(tid));
#endif // _UNSAFE
	Turning* tptr = new Turning(tid, nptr, sptr, inlinkptr, outlinkptr,size);
	turningmap [tid] = tptr;
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
	int tid=turningmap.size();
	int size= theParameters->default_lookback_size;
	vector<Link*> incoming;
	vector<Link*> outgoing;
	Server* sptr = (*servermap.begin()).second; // safest way, since servermap [0] may not exist (if someone starts numbering their servers at 1 for instance)
	// for all junctions
	for (map <int, Junction*>::iterator iter1=junctionmap.begin();iter1!=junctionmap.end();iter1++)
	{
		cout << " junction id " << (*iter1).second->get_id() << endl;
		incoming=(*iter1).second->get_incoming();
		cout << " nr incoming links "<< incoming.size() << endl;

		outgoing=(*iter1).second->get_outgoing();
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

				map<int,Turning*>::iterator t_iter;
				t_iter=	turningmap.find(tid);
				assert (t_iter != turningmap.end());
				Turning* t_ptr= new Turning(tid, (*iter1).second, sptr, (*iter2), (*iter3),size);
				turningmap [tid]=t_ptr;
				tid++;
			}
		}
	}
}


bool Network::writeturnings(string name)
{
	ofstream out(name.c_str());
	assert(out);
	out << "turnings: " << turningmap.size() << endl;
	for (map<int,Turning*>::iterator iter=turningmap.begin();iter!=turningmap.end();iter++)
	{
		(*iter).second->write(out);
	}
	return true;
}

bool Network::readgiveway(istream& in)
{
	char bracket;
	int nid, tin, tcontr; // node id, turn in, controlling turning
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readgiveway scanner jammed at " << bracket;
		return false;
	}

	in  >>  nid >> tin >> tcontr;
	// check
	assert (nodemap.count(nid));
	Node* node = nodemap [nid];
	assert (turningmap.count(tin));
	Turning * t_in = turningmap [tin];
	assert (turningmap.count(tcontr));
	Turning * t_contr = turningmap [tcontr];

	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readgiveway scanner jammed at " << bracket;
		return false;
	}

	t_in->register_controlling_turn(t_contr);
	return true;
}
bool Network::readgiveways(istream& in)
{
	string keyword;
	in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="giveways:")
	{
		cout << " readgiveways: no << giveways: >> keyword " << endl;
		return false;
	}
	int nr;
	in >> nr;
	for (int i=0; i<nr;i++)
	{
		if (!readgiveway(in))
		{
			cout << " readgiveways: readgiveway returned false for line nr " << (i+1) << endl;
			return false;
		} 
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

	// TO DO: Check out why ALL routes are registered at the boundaryIn nodes!
	for (vector<BoundaryIn*>::iterator iter=boundaryins.begin(); iter < boundaryins.end(); iter++)
	{
		(*iter)->register_routes(&routemap);
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
	assert (!exists_route(rid,odval(oid,did)));
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

	map <int, Origin*>::iterator o_iter; 
	o_iter = originmap.find(oid);
	assert (o_iter != originmap.end());
	Origin* optr = o_iter->second;
	
	map <int, Destination*>::iterator d_iter; 
	d_iter = destinationmap.find(did);
	assert (d_iter != destinationmap.end());
	Destination* dptr = d_iter->second;
#ifdef _DEBUG_NETWORK
	cout << "found o&d for route" << endl;
#endif //_DEBUG_NETWORK
	Route* rptr = new Route(rid, optr, dptr, rlinks);
	routemap.insert(pair <odval, Route*> (odval(oid,did),rptr));
#ifdef _DEBUG_NETWORK
	cout << " read a route"<<endl;
#endif //_DEBUG_NETWORK
	return true;
}

// read BUS routes
bool Network::readtransitroutes(string name) // reads the busroutes, similar to readroutes
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
		in.close();
		return false;
	}
	int nr;
	in >> nr;
	for (int i=0; i<nr;i++)
	{
		if (!readbusroute(in))
		{
			cout << " readbusroutes: readbusroute returned false for line nr " << (i+1) << endl;
			in.close();
			return false;
		} 
	}
	for (vector<BoundaryIn*>::iterator iter=boundaryins.begin(); iter < boundaryins.end(); iter++)
	{
		(*iter)->register_busroutes(&busroutes);
	}
	in.close();
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
	assert ( (find_if (busroutes.begin(),busroutes.end(), compare <Busroute> (rid))) == busroutes.end() ); // no route exists  with rid
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

		map <int, Link*>::iterator l_iter; 
		l_iter = linkmap.find(lid);
		assert (l_iter != linkmap.end());

		rlinks.insert(rlinks.end(),l_iter->second);
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
	map <int, Origin*>::iterator o_iter; 
	o_iter = originmap.find(oid);
	assert (o_iter != originmap.end());

	map <int, Destination*>::iterator d_iter; 
	d_iter = destinationmap.find(did);
	assert (d_iter != destinationmap.end());

#ifdef _DEBUG_NETWORK
	cout << "found o&d for route" << endl;
#endif //_DEBUG_NETWORK
	busroutes.insert(busroutes.end(),new Busroute(rid, o_iter->second, d_iter->second, rlinks));
#ifdef _DEBUG_NETWORK
	cout << " read a route"<<endl;
#endif //_DEBUG_NETWORK
	return true;
}

bool Network::readtransitnetwork(string name) //!< reads the stops, distances between stops, lines, trips and travel disruptions
{
	ifstream in(name.c_str()); // open input file
	assert (in);
	string keyword;
	int format;
	// First read the busstops
	in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="stops:")
	{
		cout << " readbuslines: no << stops: >> keyword " << endl;
		in.close();
		return false;
	}
	int nr= 0;
	in >> nr;
	int i=0;
	int limit;
	for (i; i<nr;i++)
	{
		if (!readbusstop(in))
		{
			cout << " readbuslines: readbusstop returned false for line nr " << (i+1) << endl;
			in.close();
			return false;
		} 
	}
	// in case that demand format is an OD matrix at the zone level
	if (theParameters->demand_format == 4)
	{
		in >> keyword;
		if (keyword!="zones:")
		{
			cout << " readbuslines: no << zones: >> keyword " << endl;
			in.close();
			return false;
		}	
		in >> nr;
		limit = i + nr;
		for (i; i<limit;i++)
		{
			if (!readtransitzones(in))
			{
				cout << " readbuslines: readbusstop returned false for line nr " << (i+1) << endl;
				in.close();
				return false;
			} 	
		}
	}
	// in case of passenger route choice - read walking distances between stops
	if (theParameters->demand_format == 3 || theParameters->demand_format == 4)
	{
		in >> keyword;
		if (keyword!="stops_distances:")
		{
			cout << " readbuslines: no << busstops_distances: >> keyword " << endl;
			in.close();
			return false;
		}
		in >> nr;
		limit = i + nr;
		in >> keyword;
		if (keyword!="format:")
		{
			cout << " readbusstops_distances: no << format: >> keyword " << endl;
			return false;
		}
		in >> format; // Give an indication for time-table format
		for (i; i<limit;i++)
		{
			if (format == 1 )
			{
				if (!readbusstops_distances_format1(in))
				{
					cout << " readbuslines: readbusstops_distances returned false for line nr " << (i+1) << endl;
					in.close();
					return false;
				}
			}
			if (format == 2 )
			{
				if (!readbusstops_distances_format2(in))
				{
					cout << " readbuslines: readbusstops_distances returned false for line nr " << (i+1) << endl;
					in.close();
					return false;
				}
			}
			// set distnaces between busstops to stops
		}
	}

	// Second read the buslines
	in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="lines:")
	{
		cout << " readbuslines: no << buslines: >> keyword " << endl;
		in.close();
		return false;
	}
	in >> nr;
	limit = i + nr;
	for (i; i<limit;i++)
	{
		if (!readbusline(in))
		{
			cout << " readbuslines: readbusline returned false for line nr " << (i+1) << endl;
			in.close();
			return false;
		} 
	}
	/*
	for (vector<Busline*>::iterator line_iter = buslines.begin(); line_iter < buslines.end(); line_iter++)
	{
		(*line_iter)->set_opposite_line(*(find_if(buslines.begin(), buslines.end(), compare <Busline> ((*line_iter)->get_opposite_id()) )));
	}
	*/
	// Third read the trips
in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="trips:")
	{
		cout << " readbuslines: no << bustrips: >> keyword " << endl;
		in.close();
		return false;
	}
	in >> nr;
	limit = i + nr;
	in >> keyword;
	if (keyword!="format:")
	{
		cout << " readbuslines: no << format: >> keyword " << endl;
		return false;
	}
	in >> format; // Give an indication for time-table format
	for (i; i<limit;i++)
	{
		if (format == 1 )
		{
			if (!readbustrip_format1(in))
			{
				cout << " readbuslines: readbustrip returned false for line nr " << (i+1) << endl;
				in.close();
				return false;
			} 
		}
		if (format == 2 )
		{
			if (!readbustrip_format2(in))
			{
				cout << " readbuslines: readbustrip returned false for line nr " << (i+1) << endl;
				in.close();
				return false;
			} 
		}
		if (format == 3 )
		{
			if (!readbustrip_format3(in))
			{
				cout << " readbuslines: readbustrip returned false for line nr " << (i+1) << endl;
				in.close();
				return false;
			} 
		}
		// set busline to trip
	}
	in >> keyword;
	if (keyword!="travel_time_disruptions:")
	{
		cout << " readbuslines: no << travel_time_disruptions: >> keyword " << endl;
		in.close();
		return false;
	}
	in >> nr;
	limit = i + nr;
	for (i; i<limit;i++)
	{
		if (!read_travel_time_disruptions(in))
		{
			cout << " readbuslines: read_travel_time_disruptions returned false for line nr " << (i+1) << endl;
			in.close();
			return false;
		}	
	}
return true;
}

bool Network::readbusstop (istream& in) // reads a busstop
{
  char bracket;
  int stop_id, link_id, can_disregard, RTI_stop;
  double position, length, min_DT;
  string name;
	bool has_bay, can_overtake;
	bool ok= true;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusstop scanner jammed at " << bracket;
		return false;
	}
  in >> stop_id >> name >> link_id >> position >> length >> has_bay >> can_overtake >> min_DT >> RTI_stop;

  if (linkmap.find(link_id) == linkmap.end())
  {
	  cout << "readfile::readsbusstop error at stop " << stop_id << ". Link " << link_id << " does not exist." << endl;
  }

  Busstop* st= new Busstop (stop_id, name, link_id, position, length, has_bay, can_overtake, min_DT, RTI_stop);
  st->add_distance_between_stops(st,0.0);
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

bool Network::readtransitzones(istream &in) // reads a transit travel zone
{
  char bracket;
  Busstop* stop;
  int zone_id, nr_stops, stop_id;
  double mean_distance, sd_distance;
	bool ok= true;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusstop scanner jammed at " << bracket;
		return false;
	}
  in >> zone_id >> nr_stops;
  ODzone* od_zone = new ODzone(zone_id); 
  for (int i=0; i< nr_stops; i++)
  {
		in >> bracket;
		if (bracket != '{')
		{
			cout << "readfile::readsbusstop scanner jammed at " << bracket;
			return false;
		}
		in >> stop_id >> mean_distance >> sd_distance;
		stop = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (stop_id) ))); // find the stop 
		od_zone->add_stop_distance(stop, mean_distance, sd_distance);
		in >> bracket;
		if (bracket != '}')
		{
			cout << "readfile::readsbusstop scanner jammed at " << bracket;
			return false;
		}
  }
  odzones.push_back(od_zone);
  in >> bracket;
  if (bracket != '}')
   {
		cout << "readfile::readsbusstop scanner jammed at " << bracket;
		return false;
	}
  return ok;
}

bool Network::readbusline(istream& in) // reads a busline
{
  char bracket;
  int busline_id, opposite_busline_id, ori_id, dest_id, route_id, vehtype, holding_strategy, nr_stops, stop_id, nr_tp, tp_id, nr_stops_init_occup;
  float ratio_headway_holding;
  double init_occup_per_stop;
  string name;
  vector <Busstop*> stops, line_timepoint;
  Busstop* stop;
  Busstop* tp;
	bool ok= true;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusline scanner jammed at " << bracket << ", expected {";
		return false;
	}
	in >> busline_id >> opposite_busline_id >> name >> ori_id >> dest_id >> route_id >> vehtype >> holding_strategy >> ratio_headway_holding >> nr_stops_init_occup >>  init_occup_per_stop >> nr_stops;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusline scanner jammed when reading stop points at " << bracket << ", expected {";
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
		cout << "readfile::readsbusline scanner jammed when reading stop point " << stop_id << " at " << bracket << ", expected }";
		return false;
	}

	// find OD pair, route, vehicle type
	odval odid (ori_id, dest_id);
	ODpair* odptr=(*(find_if (odpairs.begin(),odpairs.end(), compareod (odid) )));
	Busroute* br=(*(find_if(busroutes.begin(), busroutes.end(), compare <Route> (route_id) )));
	Vtype* vt= (*(find_if(vehtypes.vtypes.begin(), vehtypes.vtypes.end(), compare <Vtype> (vehtype) )));
	Busline* bl= new Busline (busline_id,opposite_busline_id,name,br,stops,vt,odptr,holding_strategy,ratio_headway_holding,nr_stops_init_occup,init_occup_per_stop);
	
	for (vector<Busstop*>::iterator stop_iter = bl->stops.begin(); stop_iter < bl->stops.end(); stop_iter++)
	{
		(*stop_iter)->add_lines(bl);
		(*stop_iter)->add_line_nr_waiting(bl, 0);
		(*stop_iter)->add_line_nr_boarding(bl, 0);
		(*stop_iter)->add_line_nr_alighting(bl, 0);
		(*stop_iter)->set_had_been_visited(bl, false);
		if (theParameters->real_time_info == 0)
		{
			(*stop_iter)->add_real_time_info(bl,0);
		}
		else
		{
			(*stop_iter)->add_real_time_info(bl,1);
		}
	}

// reading time point stops
  in >> nr_tp;
  in >> bracket;
  if (bracket != '{')
  {
	cout << "readfile::readsbusline scanner jammed when reading time point stops at " << bracket << ", expected {";
	return false;
  }
  for (int i=0; i < nr_tp; i++)
  {
	  in >> tp_id;
	  tp = (*(find_if(stops.begin(), stops.end(), compare <Busstop> (tp_id) ))); 
	  // search for it in the stops route of the line - 'line_timepoint' is a subset of 'stops' 
//	  assert (tp != *(stops.end())); // assure tp exists
	  line_timepoint.push_back(tp); // and add it
  }
  in >> bracket;
  if (bracket != '}')
  {
	  cout << "readfile::readsbusline scanner jammed when reading time point stops at " << bracket << ", expected }";
	return false;
  }
  bl->add_timepoints(line_timepoint);
  in >> bracket;
  if (bracket != '}')
  {
	  cout << "readfile::readbusline scanner jammed at " << bracket << ", expected }";
		return false;
	}
	// add to buslines vector
	buslines.push_back (bl);
#ifdef _DEBUG_NETWORK
	cout << " read busline"<< stop_id <<endl;
#endif //_DEBUG_NETWORK
	return ok;
}

bool Network::readbustrip_format1(istream& in) // reads a trip
{
	char bracket;
	int trip_id, busline_id, nr_stops, stop_id;
	double start_time, pass_time;
	vector <Visit_stop*> stops;
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
	Bustrip* trip= new Bustrip (trip_id, start_time,bl);
	trip->add_stops(stops);
	bl->add_trip(trip,start_time);
	bl->reset_curr_trip();

  	trip->convert_stops_vector_to_map();
	// add to bustrips vector
	bustrips.push_back (trip);

	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readbusstop scanner jammed at " << bracket;
		return false;
	}
	return true;
}

bool Network::readbustrip_format2(istream& in) // reads a trip
{
	char bracket;
	int busline_id, nr_stops, nr_trips;
	double arrival_time_at_stop, dispatching_time;
	vector <Visit_stop*> delta_at_stops;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbustrip scanner jammed at " << bracket;
		return false;
	}
	in >> busline_id >>  nr_stops;
	Busline* bl=(*(find_if(buslines.begin(), buslines.end(), compare <Busline> (busline_id) )));
	vector <Busstop*>::iterator stops_iter = bl->stops.begin();
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbustrip scanner jammed at " << bracket;
		return false;
	}
	for (int i=0; i < nr_stops; i++)
	{
		in >> arrival_time_at_stop;
		// create the Visit_stop
		// find the stop in the list
		Visit_stop* vs = new Visit_stop ((*stops_iter), arrival_time_at_stop);
		delta_at_stops.push_back(vs);
		stops_iter++;
	}
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readsbustrip scanner jammed at " << bracket;
		return false;
	}
	
	in >> nr_trips;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbustrip scanner jammed at " << bracket;
		return false;
	}
	for (int i=1; i < nr_trips+1; i++)
	{
		in >> dispatching_time;
		vector <Visit_stop*> curr_trip;
		double acc_time_table = dispatching_time;
		for (vector <Visit_stop*>::iterator iter = delta_at_stops.begin(); iter < delta_at_stops.end(); iter++)
		{
			acc_time_table += (*iter)->second;
			Visit_stop* vs_ct = new Visit_stop ((*iter)->first, acc_time_table);
			curr_trip.push_back(vs_ct);
		}
		Bustrip* trip= new Bustrip (busline_id*100 + i, dispatching_time, bl); // e.g. line 2, 3rd trip: trip_id = 23
		trip->add_stops(curr_trip);
		bl->add_trip(trip,dispatching_time);
		bl->reset_curr_trip();
		trip->convert_stops_vector_to_map();
		bustrips.push_back (trip); // add to bustrips vector
	}
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readsbustrip scanner jammed at " << bracket;
		return false;
	}
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readbusstop scanner jammed at " << bracket;
		return false;
	}
	return true;
}

bool Network::readbustrip_format3(istream& in) // reads a trip
{
	char bracket;
	int busline_id, nr_stops, nr_trips;
	double arrival_time_at_stop, initial_dispatching_time, headway;
	vector <Visit_stop*> delta_at_stops;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbustrip scanner jammed at " << bracket;
		return false;
	}
	in >> busline_id >>  nr_stops;
	Busline* bl=(*(find_if(buslines.begin(), buslines.end(), compare <Busline> (busline_id) )));
	vector <Busstop*>::iterator stops_iter = bl->stops.begin();
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbustrip scanner jammed at " << bracket;
		return false;
	}
	for (int i=0; i < nr_stops; i++)
	{
		in >> arrival_time_at_stop;
		// create the Visit_stop
		// find the stop in the list
		Visit_stop* vs = new Visit_stop ((*stops_iter), arrival_time_at_stop);
		delta_at_stops.push_back(vs);
		stops_iter++;
	}
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readsbustrip scanner jammed at " << bracket;
		return false;
	}
	in >>  initial_dispatching_time >> headway >> nr_trips ;
	for (int i=1; i < nr_trips+1; i++)
	{
		double trip_acc_time = initial_dispatching_time;
		vector <Visit_stop*> curr_trip;
		for (vector <Visit_stop*>::iterator iter = delta_at_stops.begin(); iter < delta_at_stops.end(); iter++)
		{
			trip_acc_time = trip_acc_time + (*iter)->second;
			Visit_stop* vs_ct = new Visit_stop ((*iter)->first, trip_acc_time);
			curr_trip.push_back(vs_ct);
		}
		Bustrip* trip= new Bustrip (busline_id*100 + i, initial_dispatching_time,bl); // e.g. line 2, 3rd trip: trip_id = 23
		trip->add_stops(curr_trip);
		bl->add_trip(trip,curr_trip.front()->second);
		bl->reset_curr_trip();
		trip->convert_stops_vector_to_map();
		trip->set_last_stop_visited(trip->stops.front()->first);
		bustrips.push_back (trip); // add to bustrips vector
		initial_dispatching_time = initial_dispatching_time + headway;
	}
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readbusstop scanner jammed at " << bracket;
		return false;
	}
	return true;
}

bool Network::readtransitdemand (string name)
{
	ifstream in(name.c_str()); // open input file
	assert (in);
	string keyword;
	int format, limit;
	int nr= 0;
	int i=0;
in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="passenger_rates:")
	{
		cout << " readbuslines: no << passenger_rates: >> keyword " << endl;
		return false;
	}
	in >> nr;
	limit = i + nr;
	
	in >> keyword;
	if (keyword!="format:")
	{
		cout << " readbuslines: no << format: >> keyword " << endl;
		return false;
	}
	in >> format; // Give an indication for demand matrix format
	/*
	if (format == 3)
	{
		generate_stop_ods();
	}
	*/
	for (i; i<limit;i++)
	{
 		if (format == 1) 
		{
			if (!read_passenger_rates_format1(in))
			{
				cout << " readbuslines: read_passenger_rates returned false for line nr " << (i+1) << endl;
   				return false;
			} 
		}
		if (format == 10) 
		{
			if (!read_passenger_rates_format1_TD_basic(in, nr))
			{
				cout << " readbuslines: read_passenger_rates returned false" << endl;
   				return false;
			} 
			i = limit;
		}
		if (format == 2)
		{
			if (!read_passenger_rates_format2(in))
			{
				cout << " readbuslines: read_passenger_rates returned false for line nr " << (i+1) << endl;
   				return false;
			} 
		}
		if (format == 3)
		{
			if (!read_passenger_rates_format3(in))
			{
				cout << " readbuslines: read_passenger_rates returned false for line nr " << (i+1) << endl;
   				return false;
			} 
		}
		if (format == 4)
		{
			if (!read_passenger_rates_format4(in))
			{
				cout << " readbuslines: read_passenger_rates returned false for line nr " << (i+1) << endl;
   				return false;
			} 
		}
		if (format!=1 && format!=2 && format!=3 && format!=4 && format!=10)
		{
			cout << " readbuslines: read_passenger_rates returned false for wrong format coding " << (i+1) << endl;
   			return false;
		}
	}
	if (format == 3 || format == 4)
	{
		//generate_stop_ods();
		if (theParameters->choice_set_indicator == 0)
		{
			generate_consecutive_stops();
			if (theParameters->od_pairs_for_generation == true)
			{
				read_od_pairs_for_generation (workingdir + "ODpairs_pathset.dat");
				find_all_paths_with_OD_for_generation();
			}
			else
			{
				find_all_paths();
			}
		}
	}
	return true;
}

bool Network::read_od_pairs_for_generation (string name)
{
	ifstream in(name.c_str()); // open input file
	assert (in);
	string keyword;
	int format, limit;
	int nr= 0;
	in >> keyword;
	if (keyword!="ODpairs:")
	{
		cout << " readbuslines: no << ODpairs: >> keyword " << endl;
		return false;
	}
	in >> nr;
	for (int i=0; i<nr; i++)
	{
		int origin_id, destination_id;
		in >> origin_id >> destination_id; 
		Busstop* bs_o=(*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (origin_id) ))); 
		Busstop* bs_d=(*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (destination_id) ))); 
		pair<Busstop*,Busstop*> odpair;
		odpair.first = bs_o;
		odpair.second = bs_d;
		od_pairs_for_generation.push_back(odpair);
	}
	return true;
}

bool Network::read_passenger_rates_format1 (istream& in) // reads the passenger rates in the format of arrival rate and alighting fraction per line and stop combination
{
  char bracket;
  int origin_stop_id, busline_id;
  double arrival_rate, alighting_fraction;
  bool ok= true;
  
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbusstop scanner jammed at " << bracket;
  	return false;
  }

	in >> origin_stop_id >> busline_id >> arrival_rate >> alighting_fraction ;
	Busstop* bs_o=(*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (origin_stop_id) ))); 
	Busline* bl=(*(find_if(buslines.begin(), buslines.end(), compare <Busline> (busline_id) )));
	bs_o->add_line_nr_boarding (bl, arrival_rate * theParameters->demand_scale);
	bs_o->add_line_nr_alighting (bl, alighting_fraction);

  in >> bracket;
  if (bracket != '}')
  {
    cout << "readfile::readbusstop scanner jammed at " << bracket;
    return false;
  }

#ifdef _DEBUG_NETWORK
#endif //_DEBUG_NETWORK
  return ok;
}

bool Network::read_passenger_rates_format1_TD_basic (istream& in, int nr_rates) // reads the passenger rates in the format of arrival rate and alighting fraction per line and stop combination
{
  char bracket;
  int slices, origin_stop_id, busline_id;
  double scale, arrival_rate, alighting_fraction;
  bool ok= true;
  string keyword;
	in >> keyword;
	if (keyword!="scale:")
	{
		cout << " readPassengerDemand: no << scale: >> keyword " << endl;
		return false;
	}
	in >> scale;
	for (int i=0; i < nr_rates; i++)
	{
		in >> bracket;
		if (bracket != '{')
		{
  			cout << "readfile::readsbusstop scanner jammed at " << bracket;
  		return false;
		}
		in >> origin_stop_id >> busline_id >> arrival_rate >> alighting_fraction;
		Busstop* bs_o=(*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (origin_stop_id) ))); 
		Busline* bl=(*(find_if(buslines.begin(), buslines.end(), compare <Busline> (busline_id) )));
		bs_o->add_line_nr_boarding (bl, arrival_rate * theParameters->demand_scale);
		bs_o->add_line_nr_alighting (bl, alighting_fraction);
		in >> bracket;
		if (bracket != '}')
		{
			cout << "readfile::readbusstop scanner jammed at " << bracket;
			return false;
		}
	}
	in >> keyword;
	if (keyword!="slices:")
	{
		cout << " readPassengerDemand: no << slices: >> keyword " << endl;
		return false;
	}
	in >> slices;
	for (int i=0; i < slices; i++)
	{
		read_passenger_rates_format1_TD_slices (in);
	}
#ifdef _DEBUG_NETWORK
#endif //_DEBUG_NETWORK
  return ok;
}

bool Network::read_passenger_rates_format1_TD_slices (istream& in) // reads the passenger rates in the format of arrival rate and alighting fraction per line and stop combination
{
  char bracket;
  int nr_rates, origin_stop_id, busline_id;
  double scale, loadtime, arrival_rate, alighting_fraction;
  bool ok= true;
  string keyword;
	in >> keyword;
	if (keyword!="passenger_rates:")
	{
		cout << " readPassengerDemand: no << passenger_rates: >> keyword " << endl;
		return false;
	}
	in >> nr_rates;
	in >> keyword;
	if (keyword!="scale:")
	{
		cout << " readPassengerDemand: no << scale: >> keyword " << endl;
		return false;
	}
	in >> scale;
	in >> keyword;
	if (keyword!="loadtime:")
	{
		cout << " readPassengerDemand: no << loadtime: >> keyword " << endl;
		return false;
	}
	in >> loadtime;
	Change_arrival_rate* car = new Change_arrival_rate (loadtime);
	car->book_update_arrival_rates (eventlist, loadtime);
	for (int i=0; i < nr_rates; i++)
	{
		in >> bracket;
		if (bracket != '{')
		{
  			cout << "readfile::readsbusstop scanner jammed at " << bracket;
  		return false;
		}
		in >> origin_stop_id >> busline_id >> arrival_rate >> alighting_fraction;
		Busstop* bs_o=(*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (origin_stop_id) ))); 
		Busline* bl=(*(find_if(buslines.begin(), buslines.end(), compare <Busline> (busline_id) )));
		car->add_line_nr_boarding_TD (bs_o, bl, arrival_rate * theParameters->demand_scale);
		car->add_line_nr_alighting_TD (bs_o, bl, alighting_fraction);
		bs_o->add_line_update_rate_time(bl, loadtime);
		in >> bracket;
		if (bracket != '}')
		{
			cout << "readfile::readbusstop scanner jammed at " << bracket;
			return false;
		}
	}
#ifdef _DEBUG_NETWORK
#endif //_DEBUG_NETWORK
  return ok;
}

bool Network::read_passenger_rates_format2 (istream& in) // reads the passenger rates in the format of arrival rate per line, origin stop and destination stop combination
{
	char bracket;
	int origin_stop_id, destination_stop_id, busline_id, nr_stops_info;
	double arrival_rate;
	bool ok= true;

	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusstop scanner jammed at " << bracket << ", expected {";
		return false;
	}

	in >> busline_id; 
	vector<Busline*>::iterator bl_it=find_if(buslines.begin(), buslines.end(), compare <Busline> (busline_id) );
	if (bl_it == buslines.end())
	{
		cout << "Bus line " << busline_id << " not found.";
			return false;
	}
	Busline* bl = *bl_it;

	for (int i=0; i < bl->stops.size()-1; i++)
	{
		stop_rate stop_rate_d;
		stops_rate stops_rate_d;
		multi_rate multi_rate_d;
		in >> bracket;
		if (bracket != '{')
		{
			cout << "readfile::readsbustrip scanner jammed at " << bracket << ", expected {";
			return false;
		}
		in >> origin_stop_id >> nr_stops_info;
		vector<Busstop*>::iterator bs_o_it = find_if(busstops.begin(), busstops.end(), compare <Busstop> (origin_stop_id) );
		if (bs_o_it == busstops.end())
		{
			cout << "Bus stop " << origin_stop_id << " not found.";
			return false;
		}
		Busstop* bs_o = *bs_o_it; 
		for (int j=0; j< nr_stops_info; j++)
		{
			in >> bracket;
			if (bracket != '{')
			{
				cout << "readfile::readsbustrip scanner jammed at " << bracket << ", expected {";
				return false;
			}
			in >> destination_stop_id >> arrival_rate;
			vector<Busstop*>::iterator bs_d_it = find_if(busstops.begin(), busstops.end(), compare <Busstop> (destination_stop_id) );
			if (bs_d_it == busstops.end())
			{
				cout << "Bus stop " << destination_stop_id << " not found.";
				return false;
			}
			Busstop* bs_d = *bs_d_it;
			stop_rate_d.first = bs_d;
			stop_rate_d.second = arrival_rate* theParameters->demand_scale;
			stops_rate_d.insert(stop_rate_d);
			multi_rate_d.first = bl;
			multi_rate_d.second = stops_rate_d;
			in >> bracket;
			if (bracket != '}')
			{
				cout << "readfile::readsbustrip scanner jammed at " << bracket << ", expected }, (at origin " << origin_stop_id << ", destination " << destination_stop_id << ")";
				return false;
			}
		}
		in >> bracket;
		bs_o->multi_arrival_rates.insert(multi_rate_d);
		if (bracket != '}')
		{
			cout << "readfile::readsbustrip scanner jammed at " << bracket << ", expected }, (at origin " << origin_stop_id << ")";
			return false;
		}
	}
	in >> bracket;
	if (bracket != '}')
	{
	cout << "readfile::readbusstop scanner jammed at " << bracket << ", expected }";
	return false;
	}

#ifdef _DEBUG_NETWORK
#endif //_DEBUG_NETWORK
  return ok;
}

bool Network::read_passenger_rates_format3 (istream& in) // reads the passenger rates in the format of arrival rate per OD in terms of stops (no path is pre-determined)
{
  bool ok= true;
  char bracket;
  int origin_stop_id, destination_stop_id;
  double arrival_rate;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbusstop scanner jammed at " << bracket;
  	return false;
  }
  in >> origin_stop_id >> destination_stop_id >> arrival_rate; // 
  Busstop* bs_o=(*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (origin_stop_id) ))); 
  Busstop* bs_d=(*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (destination_stop_id) ))); 
  
  ODstops* od_stop = new ODstops (bs_o, bs_d, arrival_rate);
  //ODstops* od_stop = bs_o->get_stop_od_as_origin_per_stop(bs_d);
  od_stop->set_arrival_rate(arrival_rate* theParameters->demand_scale);
  //odstops.push_back(od_stop);
  odstops_demand.push_back(od_stop);
  bs_o->add_odstops_as_origin(bs_d, od_stop);
  bs_d->add_odstops_as_destination(bs_o, od_stop);
  in >> bracket;
  if (bracket != '}')
  {
    cout << "readfile::readbusstop scanner jammed at " << bracket;
    return false;
  }

#ifdef _DEBUG_NETWORK
#endif //_DEBUG_NETWORK
  return ok;
}

bool Network::read_passenger_rates_format4 (istream& in)
{
  bool ok= true;
  char bracket;
  int origin_taz_id, destination_taz_id;
  double arrival_rate;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbusstop scanner jammed at " << bracket;
  	return false;
  }
  in >> origin_taz_id >> destination_taz_id >> arrival_rate; 
  ODzone* taz_o=(*(find_if(odzones.begin(), odzones.end(), compare <ODzone> (origin_taz_id) ))); 
  ODzone* taz_d=(*(find_if(odzones.begin(), odzones.end(), compare <ODzone> (destination_taz_id) ))); 
  taz_o->add_arrival_rates(taz_d,arrival_rate); 
  map<Busstop*,pair<double,double>> o_walk_dis = taz_o->get_stop_distances();
  map<Busstop*,pair<double,double>> d_walk_dis = taz_d->get_stop_distances();
  for (map<Busstop*,pair<double,double>>::iterator o_stops_iter = o_walk_dis.begin(); o_stops_iter != o_walk_dis.end(); o_stops_iter++)
  {
	  for (map <Busstop*,pair<double,double>>::iterator d_stops_iter = d_walk_dis.begin(); d_stops_iter != d_walk_dis.end(); d_stops_iter++)
	  {
		Busstop* bs_o = (*o_stops_iter).first;
		Busstop* bs_d = (*d_stops_iter).first;
		ODstops* od_stop = new ODstops (bs_o, bs_d);
		odstops.push_back(od_stop);
		odstops_demand.push_back(od_stop);
		bs_o->add_odstops_as_origin(bs_d, od_stop);
		bs_d->add_odstops_as_destination(bs_o, od_stop);
	  }
   }
  in >> bracket;
  if (bracket != '}')
  {
  	cout << "readfile::readsbusstop scanner jammed at " << bracket;
  	return false;
  }
  return ok;
}

bool Network::readbusstops_distances_format1 (istream& in)
{
	char bracket;
	int from_stop_id, to_stop_id, nr_stops;
	double distance;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
		return false;
	}
	in >> from_stop_id >> nr_stops;
	vector<Busstop*>::iterator from_bs_it = find_if(busstops.begin(), busstops.end(), compare <Busstop> (from_stop_id) );
	if (from_bs_it == busstops.end())
	{
		cout << "Bus stop " << from_stop_id << " not found.";
		return false;
	}
	Busstop* from_bs = *from_bs_it;
	for (int i=0; i < nr_stops; i++)
	{
		in >> bracket;
		if (bracket != '{')
		{
			cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
			return false;
		}
		in >> to_stop_id >> distance;
		// create the Visit_stop
		// find the stop in the list
		vector<Busstop*>::iterator to_bs_it = find_if(busstops.begin(), busstops.end(), compare <Busstop> (to_stop_id) );
		if (to_bs_it == busstops.end())
		{
			cout << "Bus stop " << to_stop_id << " not found.";
			return false;
		}
		Busstop* to_bs = *to_bs_it;
		from_bs->add_distance_between_stops(to_bs,distance);
		to_bs->add_distance_between_stops(from_bs,distance);
		in >> bracket;
		if (bracket != '}')
		{
			cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
			return false;
		}
	}
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
		return false;
	}
	return true;
}

bool Network::readbusstops_distances_format2 (istream& in)
{
	char bracket;
	int from_stop_id, to_stop_id, nr_stops;
	double distance;
	string name;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
		return false;
	}
	in >> name >> nr_stops;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
		return false;
	}
	vector <Busstop*> stops;
	for (int i=0; i < nr_stops; i++)
	{
		in >> from_stop_id;
		Busstop* from_bs = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (from_stop_id) )));
		stops.push_back(from_bs);
	}
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
		return false;
	}
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
		return false;
	}
	for (vector <Busstop*>::iterator from_stop_iter = stops.begin(); from_stop_iter < stops.end(); from_stop_iter++)
	{
		
		for (vector <Busstop*>::iterator to_stop_iter = stops.begin(); to_stop_iter < stops.end(); to_stop_iter++)
		{
			in >> distance;
			(*from_stop_iter)->add_distance_between_stops((*to_stop_iter),distance);
			(*to_stop_iter)->add_distance_between_stops((*from_stop_iter),distance);
		}
	}
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
		return false;
	}
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
		return false;
	}
	return true;
}

bool Network::read_travel_time_disruptions (istream& in)
{
	char bracket;
	int line, from_stop, to_stop;
	double start_time, end_time;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
		return false;
	}
	in >> line >> from_stop >> to_stop >> start_time >> end_time;
	Busline* d_line = (*(find_if(buslines.begin(), buslines.end(), compare <Busline> (line) )));
	Busstop* from_bs = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (from_stop) )));
	Busstop* to_bs = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (to_stop) )));
	d_line->add_disruptions(from_bs, to_bs, start_time, end_time);
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readsbusstop_distances scanner jammed at " << bracket;
		return false;
	}
	return true;
}

/////////////// Transit path-set generation functions: start

void Network::generate_consecutive_stops()
{
	for (vector<Busstop*>::iterator iter_stop = busstops.begin(); iter_stop < busstops.end(); iter_stop++)
	{
		vector<Busline*> lines = (*iter_stop)->get_lines();
		for (vector<Busline*>::iterator iter_line = lines.begin(); iter_line < lines.end(); iter_line++)
		{
			bool prior_this_stop = true;
			for (vector<Busstop*>::iterator iter_consecutive_stops = (*iter_line)->stops.begin(); iter_consecutive_stops < (*iter_line)->stops.end(); iter_consecutive_stops++)
			{
				if (prior_this_stop == false)
				{
					// int origin_id = (*iter_stop)->get_id();
					// int destination_id = (*iter_consecutive_stops)->get_id();
					// map <int, vector<Busline*>> origin_map = direct_lines[origin_id];
					// origin_map[destination_id].push_back(*iter_line);
					// direct_lines[origin_id] = origin_map;
					consecutive_stops[(*iter_stop)].push_back((*iter_consecutive_stops));
				}
				if ((*iter_consecutive_stops)->get_id() == (*iter_stop)->get_id())
				{
					prior_this_stop = false;
				}
			}
		}
	}
	// omitting all repetitions (caused by several direct lines connecting two stops)
	for (vector<Busstop*>::iterator iter_stop = busstops.begin(); iter_stop < busstops.end(); iter_stop++)
	{
		vector<Busstop*> updated_cons;
		map<Busstop*,bool> already_exist;
		vector<Busstop*> cons_stops = consecutive_stops[(*iter_stop)];
		for (vector<Busstop*>::iterator cons_stops_iter = cons_stops.begin(); cons_stops_iter < cons_stops.end(); cons_stops_iter++)
		{
			  already_exist[(*cons_stops_iter)] = false;	
		}
		for (vector<Busstop*>::iterator cons_stops_iter = cons_stops.begin(); cons_stops_iter < cons_stops.end(); cons_stops_iter++)
		{
			if (already_exist[(*cons_stops_iter)] == false)
			{
				updated_cons.push_back((*cons_stops_iter));
				already_exist[(*cons_stops_iter)] = true;
			}
		}
		consecutive_stops[(*iter_stop)].swap(updated_cons);
		/*
		for (vector <vector<Busstop*>::iterator>::iterator iter_drop = drop_stops.begin(); iter_drop < drop_stops.end(); iter_drop++)
		{
			consecutive_stops[(*iter_stop)].erase((*iter_drop));	
		}
		*/
	}
}

bool Network::check_consecutive (Busstop* first, Busstop* second)
{
	for (vector<Busstop*>::iterator stop_iter = consecutive_stops[first].begin(); stop_iter < consecutive_stops[first].end(); stop_iter++)
	{
		if ((*stop_iter)->get_id() == second->get_id())
		{
			return true;
		}
	}
	return false;
}

bool Network::find_direct_paths (Busstop* bs_origin, Busstop* bs_destination) 
// finds if there is a direct path between a given pair of stops, generate new direct paths
{
  vector <Busline*> lines_o = bs_origin->get_lines();
  for (vector <Busline*>::iterator bl_o = lines_o.begin(); bl_o < lines_o.end(); bl_o++)
  {
	vector <Busline*> lines_d = bs_destination->get_lines();
	for (vector <Busline*>::iterator bl_d = lines_d.begin(); bl_d < lines_d.end(); bl_d++)
	{
		if ((*bl_o)->get_id() == (*bl_d)->get_id())
		{
			for (vector <Busstop*>::iterator stop = (*bl_o)->stops.begin(); stop < (*bl_o)->stops.end(); stop++)
			{
				if ((*stop) == (bs_origin)) // if this condition is met first - it means that this is a possible path for this OD (origin preceeds destination)
				{	
					bool original_path = true;
					vector<Busline*> last_line;
					vector<vector<Busline*>> lines_sequence;
					vector<vector<Busstop*>> stops_sequence;
					vector<double> walking_distances_sequence;
					// in any case - add this specific direct path
						vector<Busstop*> o_stop;
						o_stop.push_back(bs_origin);
						stops_sequence.push_back(o_stop);
						stops_sequence.push_back(o_stop);
						vector<Busstop*> d_stop;
						d_stop.push_back(bs_destination);
						stops_sequence.push_back(d_stop);
						stops_sequence.push_back(d_stop);
						last_line.push_back(*bl_o);
						lines_sequence.push_back(last_line);
						walking_distances_sequence.push_back(0);
						walking_distances_sequence.push_back(0);
						Pass_path* direct_path = new Pass_path(pathid, lines_sequence, stops_sequence, walking_distances_sequence);
						pathid++;
						ODstops* od_stop = new ODstops (bs_origin,bs_destination); 
						bs_origin->add_odstops_as_origin(bs_destination, od_stop);
						bs_destination->add_odstops_as_destination(bs_origin, od_stop);
						vector <Pass_path*> current_path_set = od_stop->get_path_set();
						for (vector <Pass_path*>::iterator iter = current_path_set.begin(); iter != current_path_set.end(); iter++)
						{
							if (compare_same_lines_paths(direct_path,(*iter)) == true)
							{
								original_path = false;
							}
						}
						// add the direct path if it is an original one and it fulfills the constraints
						if (original_path == true && check_constraints_paths(direct_path) == true)
						{
							bs_origin->get_stop_od_as_origin_per_stop(bs_destination)->add_paths(direct_path);
							od_direct_lines[bs_origin->get_stop_od_as_origin_per_stop(bs_destination)].push_back(*bl_o);
							if (direct_path->get_number_of_transfers() < bs_origin->get_stop_od_as_origin_per_stop(bs_destination)->get_min_transfers())
							// update the no. of min transfers required if decreased
							{
								bs_origin->get_stop_od_as_origin_per_stop(bs_destination)->set_min_transfers(direct_path->get_number_of_transfers());
							}
						}
						else
						{
							bs_origin->clear_odstops_as_origin(bs_destination);
							bs_destination->clear_odstops_as_destination(bs_origin);
						}

				}
				if ((*stop) == bs_destination) // if this condition is met first - it means that this is not a possible path (destination preceeds origin)
				{
					break;
				}
			}
		}
	}
  }
  //vector<Busline*> dir_lines = get_direct_lines(bs_origin->get_stop_od_as_origin_per_stop(bs_destination));
  //if  (dir_lines.size() != 0)
  //{
  //	return true;
  //}
#ifdef _DEBUG_NETWORK
#endif //_DEBUG_NETWORK
return false;
}

void Network::generate_indirect_paths()
{
	vector<Pass_path*> updated_paths;
	vector<Pass_path*> paths;
	vector<vector<Busstop*>> stops_sequence = compose_stop_sequence(); // compose the list of intermediate stops
	
	/* count how many combinations of paths you can make (multiply number of direct lines on segments) - don't need it
	int nr_path_combinations = 1;
	for (vector<Busstop*>::iterator im_iter = collect_im_stops.begin(); im_iter < (collect_im_stops.end()-1); im_iter++)
	{
		map <Busstop*, ODstops*> od_as_origin = (*im_iter)->get_stop_as_origin();
		int origin_id = (*im_iter)->get_id();
		int destination_id = (*(im_iter+1))->get_id();
		map <int, vector<Busline*>> origin_map = direct_lines[origin_id];
		nr_path_combinations = nr_path_combinations * origin_map[destination_id].size();
	}
	*/
	vector<vector<Busline*>> lines_sequence = compose_line_sequence(collect_im_stops.back());
	ODstops* od = collect_im_stops.front()->get_stop_od_as_origin_per_stop(collect_im_stops.back());
	/*
	if (totaly_dominancy_rule(od,lines_sequence, stops_sequence) == true)
	// in case it is dominated by an existing path - don't generate the path
	{
		return;
	}
	*/
	Pass_path* indirect_path = new Pass_path(pathid, lines_sequence, stops_sequence, collect_walking_distances);
	pathid++;
	paths.push_back(indirect_path);
	/*
	int stop_leg = 0;
	vector<vector<Busline*>>::iterator lines_sequence_iter = lines_sequence.begin();
	for (vector<Busstop*>::iterator im_iter = collect_im_stops.begin()+1; im_iter < collect_im_stops.end()-1; im_iter = im_iter+2)
	{
		stop_leg++;
		vector <Busline*> d1_lines = get_direct_lines((*im_iter)->get_stop_od_as_origin_per_stop((*(im_iter+1))));
		// generating all the permutations from the possible direct lines between consecutive transfer stops
		for (vector<Busline*>::iterator d_lines = d1_lines.begin() ; d_lines < d1_lines.end(); d_lines++)
		{
			vector <Busline*> p_lines;
			p_lines.push_back((*d_lines));
			for (vector<Pass_path*>::iterator existing_paths = paths.begin(); existing_paths < paths.end(); existing_paths++)
			{
				vector<vector<Busline*>> permute_path_lines;
				vector<vector<Busline*>> path_lines = (*existing_paths)->get_alt_lines();
				int line_leg = 0;
				for (vector<vector<Busline*>>::iterator path_lines_iter = path_lines.begin(); path_lines_iter < path_lines.end(); path_lines_iter++)
				{
					line_leg++;
					if (stop_leg != line_leg)
					{
						permute_path_lines.push_back((*path_lines_iter));
					}
					else
					{
						permute_path_lines.push_back(p_lines);
					}
				}
				Pass_path* test_path = new Pass_path(pathid, permute_path_lines , stops_sequence, collect_walking_distances);	
				pathid++;
				updated_paths.push_back(test_path);
			}
			for (vector<Pass_path*>::iterator additional_paths = updated_paths.begin(); additional_paths < updated_paths.end(); additional_paths++)
			{
				paths.push_back((*additional_paths));
			}
			updated_paths.clear();
		}
	}
	*/
	for (vector<Pass_path*>::iterator paths_iter = paths.begin(); paths_iter < paths.end(); paths_iter++)
	{
		// check if this path was already generated		
		bool original_path = true;
		vector <Pass_path*> current_path_set = collect_im_stops.front()->get_stop_od_as_origin_per_stop(collect_im_stops.back())->get_path_set();
		for (vector <Pass_path*>::iterator iter = current_path_set.begin(); iter != current_path_set.end(); iter++)
		{
			if (compare_same_lines_paths((*paths_iter),(*iter)) == true && compare_same_stops_paths((*paths_iter),(*iter)) == true)
			{
				delete *paths_iter;
				original_path = false;
				break;
			}
		}
		// add the indirect path if it is an original one and it fulfills the constraints
		if (original_path == true && check_constraints_paths ((*paths_iter)) == true)
		{
			collect_im_stops.front()->get_stop_od_as_origin_per_stop(collect_im_stops.back())->add_paths((*paths_iter));
			if ((*paths_iter)->get_number_of_transfers() < collect_im_stops.front()->get_stop_od_as_origin_per_stop(collect_im_stops.back())->get_min_transfers())
			// update the no. of min transfers required if decreased
			{
				collect_im_stops.front()->get_stop_od_as_origin_per_stop(collect_im_stops.back())->set_min_transfers((*paths_iter)->get_number_of_transfers());
			}
		}
	}
}

vector<vector<Busline*>> Network::compose_line_sequence (Busstop* destination)
{
// compose the list of direct lines between each pair of intermediate stops
	vector<vector<Busline*>> lines_sequence;
	for (vector<Busstop*>::iterator im_iter = collect_im_stops.begin()+1; im_iter < collect_im_stops.end()-1; im_iter = im_iter+2)
	{
		vector<Busline*> d_lines = get_direct_lines((*im_iter)->get_stop_od_as_origin_per_stop((*(im_iter+1))));
		if ((*(im_iter+1))->get_id() != destination->get_id())
		{
			map<Busline*,bool> delete_lines;
			for (vector<Busline*>::iterator line_iter = d_lines.begin(); line_iter < d_lines.end(); line_iter++)
			{
				delete_lines[(*line_iter)] = false;
			}
			if ((*im_iter)->check_destination_stop(destination) == true)
			{
				vector<Busline*> des_lines = get_direct_lines ((*im_iter)->get_stop_od_as_origin_per_stop(destination));
				for (vector<Busline*>::iterator line_iter = d_lines.begin(); line_iter < d_lines.end(); line_iter++)
				{
					for (vector<Busline*>::iterator line_iter1 = des_lines.begin(); line_iter1 < des_lines.end(); line_iter1++)
					{
						if ((*line_iter)->get_id() == (*line_iter1)->get_id()) // if there is a direct line to the destination in the set
						{
							delete_lines[(*line_iter)] = true;
						}
					}
				}
				for (map<Busline*,bool>::iterator iter = delete_lines.begin(); iter != delete_lines.end(); iter++)
				// delete those lines that reach directly the destination and therefore should not be included in the transfer alternative
				{
					vector <Busline*>::iterator line_to_delete;
					for (vector<Busline*>::iterator iter1 = d_lines.begin(); iter1 < d_lines.end(); iter1++)
					{
						if ((*iter).first->get_id() == (*iter1)->get_id())
						{
							line_to_delete = iter1;
							break;
						}
					}
					if ((*iter).second == true)
					{
						d_lines.erase(line_to_delete);
					}	
				}
			}
		}
		lines_sequence.push_back(d_lines);	
	}
	return lines_sequence;
}

vector<vector<Busstop*>> Network::compose_stop_sequence ()
{
	vector<vector<Busstop*>> stop_seq;
	vector<Busstop*> stop_vec;
	for (vector<Busstop*>::iterator stop_iter = collect_im_stops.begin(); stop_iter < collect_im_stops.end(); stop_iter++)
	{	
		stop_vec.clear();
		stop_vec.push_back((*stop_iter));
		stop_seq.push_back(stop_vec);
	}
	return stop_seq;
}

void Network::generate_stop_ods()
{
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{
		for (vector <Busstop*>::iterator basic_destination = busstops.begin(); basic_destination < busstops.end(); basic_destination++)
		{
			/*
			bool od_with_demand = false;
			for (vector <ODstops*>::iterator od = odstops_demand.begin(); od< odstops_demand.end(); od++)
			{
				if ((*od)->get_origin()->get_id() == (*basic_origin)->get_id() && (*od)->get_destination()->get_id() == (*basic_destination)->get_id())
				{
					od_with_demand = true;
					break;
				}
			}
			*/
			//if (od_with_demand == false) // if it is not an od with a non-zero demand
			//{
			//	if (check_stops_opposing_directions((*basic_origin),(*basic_destination)) == false)
			//	{
					ODstops* od_stop = new ODstops ((*basic_origin),(*basic_destination));
					//odstops_map [(*basic_origin)].push_back(od_stop);
					//odstops.push_back(od_stop);
					(*basic_origin)->add_odstops_as_origin((*basic_destination), od_stop);
					(*basic_destination)->add_odstops_as_destination((*basic_origin), od_stop);
			//	}
			//}
		}
	}
}

bool Network::check_stops_opposing_directions (Busstop* origin, Busstop* destination)
{
	vector<Busline*> lines_at_origin = origin->get_lines();
	vector<Busline*> lines_at_destination = destination->get_lines();
	if (lines_at_origin.size() == lines_at_destination.size()) // if there is a diff. in number of passing lines then there are some non-opposing lines
	{
		vector<bool> opposite;
		for (vector<Busline*>::iterator line_iter_o = lines_at_origin.begin(); line_iter_o < lines_at_origin.end(); line_iter_o++)
		{
			for (vector<Busline*>::iterator line_iter_d = lines_at_destination.begin(); line_iter_d < lines_at_destination.end(); line_iter_d++)
			{
				if((*line_iter_o)->get_opposite_id() == (*line_iter_d)->get_id())
				{
					opposite.push_back(true);
				}
			}
		}
		if (opposite.size() == lines_at_origin.size()) // if all the lines are opposing directions
		{
			return true;
		}
	}
	return false;
}

void Network::find_all_paths () 
// goes over all OD pairs for a given origin to generate their path choice set
{
	generate_consecutive_stops();
	// first - generate all direct paths
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{
		for (vector <Busstop*>::iterator basic_destination = busstops.begin(); basic_destination < busstops.end(); basic_destination++)
		{
			if ((*basic_origin)->get_id() != (*basic_destination)->get_id())
			{
				find_direct_paths ((*basic_origin), (*basic_destination));
			}
		}
	}

	// second - generate indirect paths with walking distances
	cout << "Generating indirect paths:" << endl;
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{
		for (vector <Busstop*>::iterator basic_destination = busstops.begin(); basic_destination < busstops.end(); basic_destination++)
		{
			cout << (*basic_origin)->get_name() << " - " << (*basic_destination)->get_name() << endl;
			if ((*basic_origin)->check_destination_stop(*basic_destination) == false)
			{
				ODstops* od_stop = new ODstops ((*basic_origin),(*basic_destination)); 
				(*basic_origin)->add_odstops_as_origin((*basic_destination), od_stop);
				(*basic_destination)->add_odstops_as_destination((*basic_origin), od_stop);
			}
			if ((*basic_origin)->get_id() != (*basic_destination)->get_id() && (*basic_origin)->check_destination_stop(*basic_destination) == true)
			{
				collect_im_stops.push_back((*basic_origin));
				find_recursive_connection_with_walking (((*basic_origin)), (*basic_destination));
				collect_im_stops.clear();
				collect_walking_distances.clear();
			}
			merge_paths_by_stops((*basic_origin),(*basic_destination));
			merge_paths_by_common_lines((*basic_origin),(*basic_destination));
		}
		//write_path_set_per_stop (workingdir + "path_set_generation.dat", (*basic_origin));
	}
	// apply static filtering rules
	cout << "Filtering paths..." << endl;
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{
		static_filtering_rules(*basic_origin);
	}
	// apply dominancy rules
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{	
		dominancy_rules(*basic_origin);
	}
	
	// report generated choice-sets 
	cout << "Saving paths..." << endl;
	
	write_path_set (workingdir + "path_set_generation.dat");
	cout << "Path generation finished!" << endl;
}

void Network::find_all_paths_with_OD_for_generation () 
// goes over all OD pairs for a given origin to generate their path choice set
{
	generate_consecutive_stops();
	// first - generate all direct paths
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{
		for (vector <Busstop*>::iterator basic_destination = busstops.begin(); basic_destination < busstops.end(); basic_destination++)
		{
			if ((*basic_origin)->get_id() != (*basic_destination)->get_id())
			{
				find_direct_paths ((*basic_origin), (*basic_destination));
			}
		}
	}

	// second - generate indirect paths with walking distances
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{
		for (vector <Busstop*>::iterator basic_destination = busstops.begin(); basic_destination < busstops.end(); basic_destination++)
		{
			bool relevant_od = false;
			for (vector<pair<Busstop*,Busstop*>>::iterator iter = od_pairs_for_generation.begin(); iter < od_pairs_for_generation.end(); iter ++)
			{
				if ((*basic_origin)->get_id() == (*iter).first->get_id() && (*basic_destination)->get_id() == (*iter).second->get_id())
				{
					relevant_od = true;
					break;
				}
			}
			if (relevant_od == true)
			{
			if ((*basic_origin)->check_destination_stop(*basic_destination) == false)
			{
				ODstops* od_stop = new ODstops ((*basic_origin),(*basic_destination)); 
				(*basic_origin)->add_odstops_as_origin((*basic_destination), od_stop);
				(*basic_destination)->add_odstops_as_destination((*basic_origin), od_stop);
			}
			if ((*basic_origin)->get_id() != (*basic_destination)->get_id() && (*basic_origin)->check_destination_stop(*basic_destination) == true)
			{
				collect_im_stops.push_back((*basic_origin));
				find_recursive_connection_with_walking (((*basic_origin)), (*basic_destination));
				collect_im_stops.clear();
				collect_walking_distances.clear();
			}
			merge_paths_by_stops((*basic_origin),(*basic_destination));
			merge_paths_by_common_lines((*basic_origin),(*basic_destination));
			}
		}
		write_path_set_per_stop (workingdir + "path_set_generation.dat", (*basic_origin));
	}
	// apply static filtering rules
	/*
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{
		static_filtering_rules(*basic_origin);
	}
	// apply dominancy rules
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{	
		dominancy_rules(*basic_origin);
	}
	*/
	// report generated choice-sets 
	//write_path_set (workingdir + "path_set_generation.dat");
}

/*
void Network:: find_recursive_connection (Busstop* origin, Busstop* destination)
// search recursively for a path (forward - from origin to destination) - without walking links
{
	map <Busstop*, ODstops*> od_as_origin = origin->get_stop_as_origin();
	map <Busstop*, ODstops*> original_od_as_origin = collect_im_stops.front()->get_stop_as_origin();
	vector <Busstop*> cons_stops = consecutive_stops[origin];
	if (cons_stops.size() > 0)
	{
		if ((collect_im_stops.size() - 1) <= original_od_as_origin[destination]->get_min_transfers() + theParameters->max_nr_extra_transfers)
		{
			//for (vector<Busstop*>::iterator stop = cons_stops.begin(); stop < cons_stops.end(); stop++)
			for (map<Busstop*, ODstops*>::iterator iter = od_as_origin.begin(); iter != od_as_origin.end(); iter++)
			{
				//Busstop* intermediate_destination = (*stop);
				Busstop* intermediate_destination = (*iter).first;
				
				int intermediate_id = intermediate_destination->get_id();
				int previous_stop_id = collect_im_stops.back()->get_id();
				map <int, vector<Busline*>> origin_map = direct_lines[previous_stop_id];
				if (origin_map[intermediate_id].size() != 0)
				{
					collect_im_stops.push_back(intermediate_destination);
						if (intermediate_destination->get_id() != destination->get_id())
						{
							int destination_id = destination->get_id();
							map <int, vector<Busline*>> origin_map = direct_lines[intermediate_id];
							if (origin_map[destination_id].size() != 0)
							{			
								collect_im_stops.push_back(destination);
								generate_indirect_paths();
								collect_im_stops.pop_back();
							}
							find_recursive_connection (intermediate_destination, destination);	
						}
					collect_im_stops.pop_back();
				}
			}
		}
	}
	return;
}
*/

void Network:: find_recursive_connection_with_walking (Busstop* origin, Busstop* destination)
// search recursively for a path (forward - from origin to destination) with walking links
{
	map <Busstop*, double> possible_origins = origin->get_walking_distances();
	vector <Busstop*> cons_stops = get_cons_stops(origin);
//	if (cons_stops.size() > 0)
//	{
		// find the number of expected transfers in this path search	
		int nr_im_stop_elements = 0;
		for (vector<Busstop*>::iterator iter_count = collect_im_stops.begin(); iter_count < collect_im_stops.end(); iter_count++)
		{
			nr_im_stop_elements++;
		}	
		if (collect_im_stops.front()->check_destination_stop(destination) == true)
		{
		if (((nr_im_stop_elements-1)/2) <= collect_im_stops.front()->get_stop_od_as_origin_per_stop(destination)->get_min_transfers() + theParameters->max_nr_extra_transfers || collect_im_stops.back()->check_walkable_stop(destination) == true)
		{
			for (map <Busstop*, double>::iterator poss_origin = possible_origins.begin(); poss_origin != possible_origins.end(); poss_origin++)
			{
				if ((*poss_origin).second < 10000)
				{
				collect_walking_distances.push_back((*poss_origin).second);
				collect_im_stops.push_back((*poss_origin).first);
				bool already_visited = false;
				for (vector<Busstop*>::iterator collected_stops = collect_im_stops.begin(); collected_stops < collect_im_stops.end()-2; collected_stops++)
				{
					if ((*collected_stops)->get_id() == (*poss_origin).first->get_id())
					{
						already_visited = true;
						break;
					}
				}
				if (already_visited == false)
				{
					if (collect_im_stops.back()->get_id() == destination->get_id())
					{	
						generate_indirect_paths();
						break;
					}
					//???	if ((check_consecutive((*iter),destination) == false && check_consecutive(destination,(*iter)) == true) == false) // in case destination preceeds origin on a given line but not vice versa - then do not continue this branch (heuristic)
										
					cons_stops = get_cons_stops(collect_im_stops.back());
					// applying dominancy rules of transfers and max transfers constraint
					if (collect_im_stops.front()->get_stop_od_as_origin_per_stop(destination)->get_min_transfers() + theParameters->max_nr_extra_transfers >= ((collect_im_stops.size()-2)/2))
					{
						for (vector<Busstop*>::iterator iter = cons_stops.begin(); iter != cons_stops.end(); iter++)
						{					
							Busstop* intermediate_destination = (*iter);
							Busstop* previous_stop = collect_im_stops.back();
							collect_im_stops.push_back(intermediate_destination);
							// check path constraints and apply dominancy rules of IVT 
							if (check_path_constraints(destination))
							{
								if (check_consecutive(previous_stop,intermediate_destination) == true)
								{
									if (destination->check_walkable_stop(intermediate_destination) == true)
									{
										collect_im_stops.push_back(destination);
										collect_walking_distances.push_back(destination->get_walking_distance_stop(intermediate_destination));
										generate_indirect_paths();
										collect_im_stops.pop_back();
										collect_walking_distances.pop_back();
									}
									else
									{
										// decide if there is a point in searching further deep
										if (theParameters->absolute_max_transfers >= ((nr_im_stop_elements+1)/2) || intermediate_destination->check_walkable_stop(destination) == true) 
										// only if this alternative doesn'tincludes too many transfers already or it can be completed without an extra transit leg
										// note that nr_elements is calculated at the beginging of the function and since then there are two extra stops
										{
											find_recursive_connection_with_walking (intermediate_destination, destination);
										}
									}	
								}
							}
							collect_im_stops.pop_back();
						}
					}
				}
				collect_im_stops.pop_back();
				collect_walking_distances.pop_back();
			}
			}
		}
		}
//	}
}

bool Network::check_path_constraints(Busstop* destination)
{
	vector<vector<Busline*>> lines = compose_line_sequence(destination);
	for (vector<vector<Busline*>>::iterator lines_iter = lines.begin(); lines_iter < lines.end(); lines_iter++)
	{
		if ((*lines_iter).empty() == true)
		{
			return false;
		}
	}
	vector<vector<Busstop*>> stops = compose_stop_sequence();
	if (check_sequence_no_repeating_stops(collect_im_stops) == true)
	{
		if (check_path_no_repeating_lines(lines,stops) == true)
		{
			if (check_path_no_opposing_lines(lines) == true)
			{
				return true;
			}
		}
	}
	return false;
}

void Network::merge_paths_by_stops (Busstop* origin_stop, Busstop* destination_stop) // merge paths with same lines for all legs (only different transfer stops) 
	// only if the route between this set of possible transfer stops is identical for the two leg sets
{
	//map <Busstop*, ODstops*> od_as_origin = stop->get_stop_as_origin();
	//for (map <Busstop*, ODstops*>::iterator odpairs = od_as_origin.begin(); odpairs != od_as_origin.end(); odpairs++)
		if (origin_stop->get_id() != destination_stop->get_id() && origin_stop->check_destination_stop(destination_stop) == true)
		{
			ODstops* odpairs = origin_stop->get_stop_od_as_origin_per_stop(destination_stop);
			vector <Pass_path*> path_set = odpairs->get_path_set();
			// go over all the OD pairs of stops that have more than a single alternative
			if (path_set.size() > 1)
			{
				map <Pass_path*,bool> paths_to_be_deleted;
				vector <Pass_path*> merged_paths_to_be_added;
				map <Pass_path*,bool> flagged_paths;
				for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
				{
					flagged_paths[(*path_iter)] = false;
					paths_to_be_deleted[(*path_iter)] = false;
				}
				for (vector <Pass_path*>::iterator path1 = path_set.begin(); path1 < path_set.end()-1; path1++)
				{
					bool perform_merge = false;
					vector <vector <Busstop*>> stops1 = (*path1)->get_alt_transfer_stops();	
					if (flagged_paths[(*path1)] == false)
					{
						for (vector <Pass_path*>::iterator path2 = path1 + 1; path2 < path_set.end(); path2++)
						{
							bool fulfilled_conditions = compare_same_lines_paths ((*path1), (*path2));
							if (fulfilled_conditions == true)
							{
							// both have exactly the same lines for all legs 
								vector<vector<Busline*>> path2_lines = (*path2)->get_alt_lines();
								vector<vector<Busline*>>::iterator path2_line = path2_lines.begin();
								vector<vector<Busline*>> path1_lines = (*path1)->get_alt_lines();
								vector<vector<Busstop*>> path1_set_stops = (*path1)->get_alt_transfer_stops();
								vector<vector<Busstop*>>::iterator start_stops = path1_set_stops.begin()+1;
								for(vector<vector<Busline*>>::iterator path1_line = path1_lines.begin(); path1_line < path1_lines.end(); path1_line++)
								{
									vector<Busline*> line1 = (*path1_line);
									vector<Busline*> line2 = (*path2_line);
									vector<Busstop*> start_stop = (*start_stops);
									vector<Busstop*> end_stop = (*(start_stops+1));
									// do they have the same routes between stops?
									if (compare_common_partial_routes (line1.front(),line2.front(),start_stop.front(),end_stop.front()) == false)
									{
										fulfilled_conditions = false;
										break;
									}
									start_stops = start_stops + 2;
									path2_line++;
								}
								if (fulfilled_conditions == false)
								{
									break;
								}
							}
							if (fulfilled_conditions == true )
							// both have exactly the same lines for all legs AND the same route for lines on leg 1 and lines on leg 2 between stops
							{			
								vector <vector <Busstop*>> stops2 = (*path2)->get_alt_transfer_stops();
								vector <vector <Busstop*>>::iterator stops2_iter = stops2.begin();
								for (vector <vector <Busstop*>>::iterator stops1_iter = stops1.begin(); stops1_iter < stops1.end(); stops1_iter++)
								{
									for (vector<Busstop*>::iterator stops2_leg = (*stops2_iter).begin(); stops2_leg < (*stops2_iter).end(); stops2_leg++)
									{
										// search for the stops that you want to copy (union of two sets)
										bool no_identical = true;
										for (vector<Busstop*>::iterator stops1_leg = (*stops1_iter).begin(); stops1_leg < (*stops1_iter).end(); stops1_leg++)
										{
											if ((*stops1_leg)->get_id() == (*stops2_leg)->get_id())
											{
												no_identical = false;
												break;
											}
										}
										if (no_identical == true)
										// add those non-shared stops to the stops set
										{
											(*stops1_iter).push_back((*stops2_leg));
											flagged_paths[(*path2)] = true;
											perform_merge = true;
										}
									}
									stops2_iter++;
								}
								paths_to_be_deleted[*path1] = true;					
								paths_to_be_deleted[*path2] = true;
							}
						}
					}
					if (perform_merge == true)
					// generate a new path with the joined stops set
					{
						Pass_path* merged_path = new Pass_path(pathid, (*path1)->get_alt_lines(), stops1, (*path1)->get_walking_distances());
						pathid++;
						merged_paths_to_be_added.push_back(merged_path);
					}
				}
				for (map <Pass_path*,bool>::iterator delete_iter = paths_to_be_deleted.begin(); delete_iter != paths_to_be_deleted.end(); delete_iter++)
				// delete all the paths that were used as source for the merged paths
				{
					vector<Pass_path*>::iterator path_to_delete;
					for (vector<Pass_path*>::iterator paths_iter = path_set.begin(); paths_iter < path_set.end(); paths_iter++)
					{
						if ((*delete_iter).first->get_id() == (*paths_iter)->get_id() && (*delete_iter).second == true)
						{
							path_to_delete = paths_iter;
							break;
						}
					}
					if ((*delete_iter).second == true)
					{
						delete *path_to_delete;
						path_set.erase(path_to_delete);
					}
				}
				for (vector<Pass_path*>::iterator adding_iter = merged_paths_to_be_added.begin(); adding_iter < merged_paths_to_be_added.end(); adding_iter++)
				// add the new merged paths to the path set
				{
					path_set.push_back((*adding_iter));
				}
			}
			odpairs->set_path_set(path_set);
		}
}

void Network::merge_paths_by_common_lines (Busstop* origin_stop, Busstop* destination_stop)  // merge paths with lines that have identical route between consecutive stops
{
	if (origin_stop->get_id() != destination_stop->get_id() && origin_stop->check_destination_stop(destination_stop) == true)
	{
		ODstops* odpairs = origin_stop->get_stop_od_as_origin_per_stop(destination_stop);
		vector <Pass_path*> path_set = odpairs->get_path_set();
		if (path_set.size() > 1)
		{
			// go over all the OD pairs of stops that have more than a single alternative
			map <Pass_path*,bool> paths_to_be_deleted;
			vector <Pass_path*> merged_paths_to_be_added;
			map <Pass_path*,bool> flagged_paths;
			for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
			{
				flagged_paths[(*path_iter)] = false;
				paths_to_be_deleted[(*path_iter)] = false;
			}
			for (vector <Pass_path*>::iterator path1 = path_set.begin(); path1 < path_set.end()-1; path1++)
			{
				bool perform_merge = false;
				vector<vector<Busline*>> transfer_lines1_collect = (*path1)->get_alt_lines();	
				if (flagged_paths[(*path1)] == false)
				{
					for (vector <Pass_path*>::iterator path2 = path1 + 1; path2 < path_set.end(); path2++)
					{
						vector<vector<Busline*>> transfer_lines1_ = (*path1)->get_alt_lines();						
						vector<vector<Busline*>>::iterator transfer_lines1 = transfer_lines1_.begin();
						vector<vector<Busline*>>::iterator transfer_lines1_collect_iter = transfer_lines1_collect.begin();
						vector<vector<Busstop*>> transfer_stops1_ = (*path1)->get_alt_transfer_stops();
						if (transfer_lines1_.size() == 0) // a walking-only path - there is no need in merging
						{
							break;
						}
						vector<vector<Busstop*>> transfer_stops2_ = (*path2)->get_alt_transfer_stops();
						vector<vector<Busline*>> transfer_lines2_ = (*path2)->get_alt_lines();
						vector<vector<Busstop*>>::iterator transfer_stops2 = transfer_stops2_.begin()+1;
						vector<vector<Busline*>>::iterator transfer_lines2 = transfer_lines2_.begin();
						if (transfer_lines2_.size() == 0) // a walking-only path - there is no need in merging
						{
							break;
						}
						for (vector<vector<Busstop*>>::iterator transfer_stops1 = transfer_stops1_.begin()+1; transfer_stops1 < transfer_stops1_.end()-1; transfer_stops1 = transfer_stops1 + 2)
						{
							int counter_shared_current = 0;
							int coutner_shared_next = 0;
							// check if the two paths share stops in two consecutive boarding-alighting stops
							for (vector<Busstop*>::iterator transfer_stop1 = (*transfer_stops1).begin(); transfer_stop1 < (*(transfer_stops1)).end(); transfer_stop1++)
							{
								for (vector<Busstop*>::iterator transfer_stop2 = (*transfer_stops2).begin(); transfer_stop2 < (*(transfer_stops2)).end(); transfer_stop2++)
								{
									if ((*transfer_stop1)->get_id() == (*transfer_stop2)->get_id())
									{
										counter_shared_current++;
										break;
									}
								}
							}
							for (vector<Busstop*>::iterator transfer_stop1 = (*(transfer_stops1+1)).begin(); transfer_stop1 < (*(transfer_stops1+1)).end(); transfer_stop1++)
							{
								for (vector<Busstop*>::iterator transfer_stop2 = (*(transfer_stops2+1)).begin(); transfer_stop2 < (*(transfer_stops2+1)).end(); transfer_stop2++)
								{
									if ((*transfer_stop1)->get_id() == (*transfer_stop2)->get_id())
									{
										coutner_shared_next++;
										break;
									}
								}
							}
							// if ALL these stops are shared
							if (counter_shared_current == (*transfer_stops1).size() && counter_shared_current == (*transfer_stops2).size() && coutner_shared_next == (*(transfer_stops1+1)).size() && coutner_shared_next == (*(transfer_stops2+1)).size()) // there are two consecutive identical sets of transfer stops
							{
								if ((*(transfer_lines1)).front()->get_id() == (*(transfer_lines2)).front()->get_id() || compare_common_partial_routes((*(transfer_lines1)).front(), (*(transfer_lines2)).front(), (*transfer_stops1).front(), (*(transfer_stops1 + 1)).front()) == true )
								{
									// the two lines have the same route between the stops - merge 
									for (vector<Busline*>::iterator transfer_line2 = (*(transfer_lines2)).begin(); transfer_line2 < (*(transfer_lines2)).end(); transfer_line2++)
									{
										bool identical_line = false;
										for (vector<Busline*>::iterator transfer_line1 = (*(transfer_lines1)).begin(); transfer_line1 < (*(transfer_lines1)).end(); transfer_line1++)
										{
											// check if the identical route is because it is really the same line...
											if ((*transfer_line1)->get_id() == (*transfer_line2)->get_id())
											{
												identical_line = true;
											}
										}
										if (identical_line == false)
										{
											bool line_included = false;
											for (vector<Busline*>::iterator line_iter = (*transfer_lines1_collect_iter).begin(); line_iter < (*transfer_lines1_collect_iter).end(); line_iter++)
											{
												if ((*line_iter)->get_id() == (*transfer_line2)->get_id())
												{
													line_included = true;
													break;
												}
											}
											if (line_included == false)
											{
												(*transfer_lines1_collect_iter).push_back(*transfer_line2);
											}
											perform_merge = true;
											flagged_paths[(*path2)] = true;
										}
										paths_to_be_deleted[*path1] = true;
										paths_to_be_deleted[*path2] = true;
									}
								}
							}
							// progress all the iterators
							transfer_stops2++;
							transfer_stops2++;
							if (transfer_stops2 >= transfer_stops2_.end()-1 || transfer_lines1 >= transfer_lines1_.end() || transfer_lines2 == transfer_lines2_.end())
							{
								break;
							}
							if (transfer_stops1 != transfer_stops1_.end())
							{
								transfer_lines1++;
								transfer_lines1_collect_iter++;
								transfer_lines2++;
							}
						}
						if (perform_merge == true && path2 == path_set.end()-1)
						// only after all the required merging took place
						{
							Pass_path* merged_path = new Pass_path(pathid, transfer_lines1_collect, (*path1)->get_alt_transfer_stops(), (*path1)->get_walking_distances()); 
							pathid++;
							merged_paths_to_be_added.push_back(merged_path);
						}
					}
				}
			}
			for (map <Pass_path*,bool>::iterator delete_iter = paths_to_be_deleted.begin(); delete_iter != paths_to_be_deleted.end(); delete_iter++)
			// delete all the paths that were used as source for the merged paths
			{
				vector<Pass_path*>::iterator path_to_delete;
				for (vector<Pass_path*>::iterator paths_iter = path_set.begin(); paths_iter < path_set.end(); paths_iter++)
				{
					if ((*delete_iter).first->get_id() == (*paths_iter)->get_id() && (*delete_iter).second == true)
					{
						path_to_delete = paths_iter;
						break;
					}
				}
				if ((*delete_iter).second == true)
				{
					delete *path_to_delete;
					path_set.erase(path_to_delete);
				}
			}
			for (vector<Pass_path*>::iterator adding_iter = merged_paths_to_be_added.begin(); adding_iter < merged_paths_to_be_added.end(); adding_iter++)
			// add the new merged paths to the path set
			{
				path_set.push_back((*adding_iter));
			}
		}
		odpairs->set_path_set(path_set);
		}
}

void Network::static_filtering_rules (Busstop* stop)
{
	// includes the following filtering rules: (1) max walking distance; (2) max IVT ratio; (3) maybe worthwhile to way.
	for (vector <Busstop*>::iterator basic_destination = busstops.begin(); basic_destination < busstops.end(); basic_destination++)
	{
		if (stop->get_id() != (*basic_destination)->get_id() && stop->check_destination_stop(*basic_destination) == true)
		{	
		ODstops* odpairs = stop->get_stop_od_as_origin_per_stop(*basic_destination);
		vector <Pass_path*> path_set = odpairs->get_path_set();
		if (path_set.empty() == false)
		{
			map <Pass_path*,bool> paths_to_be_deleted;
			// calculate the minimum total IVT for this OD pair
			double min_total_scheduled_in_vehicle_time = path_set.front()->calc_total_scheduled_in_vehicle_time(0.0);
			for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
			{
				min_total_scheduled_in_vehicle_time = min (min_total_scheduled_in_vehicle_time, (*path_iter)->calc_total_scheduled_in_vehicle_time(0.0));
				paths_to_be_deleted[(*path_iter)] = false;
			}
			for (vector <Pass_path*>::iterator path = path_set.begin(); path < path_set.end(); path++)
			{
				// more than max. transfers
				if ((*path)->get_number_of_transfers() > odpairs->get_min_transfers() + theParameters->max_nr_extra_transfers)
				{
					paths_to_be_deleted[(*path)] = true;
				}
				// max walking distance
				else if ((*path)->calc_total_walking_distance() > theParameters->max_walking_distance)
				{
					paths_to_be_deleted[(*path)] = true;
				}
				// max IVT ratio
				else if ((*path)->calc_total_scheduled_in_vehicle_time(0.0) > min_total_scheduled_in_vehicle_time * theParameters->max_in_vehicle_time_ratio)
				{
					paths_to_be_deleted[(*path)] = true;
				}
				// eliminating dominated stops by upstream transfer stops (avoiding further downstream transfer stops
				// downstream_dominancy_rule(*path);
				// maybe worthwhile to wait (with worst case headway)
				vector<vector<Busline*>> alt_lines = (*path)->get_alt_lines();
				for (vector<vector<Busline*>>::iterator line_set_iter = alt_lines.begin(); line_set_iter < alt_lines.end(); line_set_iter++)
				{
					map <Busline*,bool> lines_to_be_deleted;
					for (vector <Busline*>::iterator line_iter = (*line_set_iter).begin(); line_iter < (*line_set_iter).end(); line_iter++)
					{
						lines_to_be_deleted[(*line_iter)] = false;
					}
					vector<vector<Busstop*>> alt_stops = (*path)->get_alt_transfer_stops();
					map<Busline*, bool> lines_to_include = (*path)->check_maybe_worthwhile_to_wait((*path)->get_alt_lines().front(), alt_stops.begin(), 0);
					for (map<Busline*, bool>::iterator lines_to_include_iter = lines_to_include.begin(); lines_to_include_iter != lines_to_include.end(); lines_to_include_iter++)
					{
						if ((*lines_to_include_iter).second == false)
						// delete the lines that are not worthwhile to wait for
						{
							lines_to_be_deleted[(*lines_to_include_iter).first] = true;
						}
					}
					for (map <Busline*,bool>::iterator delete_iter = lines_to_be_deleted.begin(); delete_iter != lines_to_be_deleted.end(); delete_iter++)	
					// delete all the lines that did not fulfill the maybe worthwhile to wait rule
					{
						vector<Busline*>::iterator line_to_delete;
						for (vector<Busline*>::iterator lines_iter = (*line_set_iter).begin(); lines_iter < (*line_set_iter).end(); lines_iter++)
						{
							if ((*delete_iter).first->get_id() == (*lines_iter)->get_id() && (*delete_iter).second == true)
							{
								line_to_delete = lines_iter;
								break;
							}
						}
						if ((*delete_iter).second == true)
						{
							(*line_set_iter).erase(line_to_delete);
						}
					}
				}
			}
				/*
				// apply dominancy rule at stop level
				vector<vector<Busstop*>> alt_stops = (*path)->get_alt_transfer__stops();
				for (vector<vector<Busstop*>>::iterator stop_set_iter = alt_stops.begin()+1; stop_set_iter < alt_stops.end(); stop_set_iter+2) 
					// goes over connected stops (even locations - 2,4,...)
				{
					map <Busstop*,bool> stops_to_be_deleted;
					for (vector <Busstop*>::iterator stop_iter = (*stop_set_iter).begin(); stop_iter < (*stop_set_iter).end(); stop_iter++)
					{
						stops_to_be_deleted[(*stop_iter)] = false;
					}
					for (vector <Busstop*>::iterator stop_iter = (*stop_set_iter).begin(); stop_iter < (*stop_set_iter).end()-1; stop_iter++)
					{
						for (vector <Busstop*>::iterator stop_iter1 = stop_iter + 1; stop_iter < (*stop_set_iter).end(); stop_iter1++)
						{
							if 
						}
					}
					for (map<Busline*, bool>::iterator lines_to_include_iter = lines_to_include.begin(); lines_to_include_iter != lines_to_include.end(); lines_to_include_iter++)
					{
						if ((*lines_to_include_iter).second == false)
						// delete the stop that is been dominated
						{
							stops_to_be_deleted[(*stops_to_include_iter).first] = true;
						}
					}
					for (map <Busstop*,bool>::iterator delete_iter = stops_to_be_deleted.begin(); delete_iter != stops_to_be_deleted.end(); delete_iter++)	
					// delete all the stops that are been dominated
					{
						vector<Busstop*>::iterator stop_to_delete;
						for (vector<Busstop*>::iterator stops_iter = (*stop_set_iter).begin(); stops_iter < (*stop_set_iter).end(); stops_iter++)
						{
							if ((*delete_iter).first->get_id() == (*stops_iter)->get_id() && (*delete_iter).second == true)
							{
								stop_to_delete = stops_iter;
								break;
							}
						}
						if ((*delete_iter).second == true)
						{
							(*stop_set_iter).erase(stop_to_delete);
						}
					}
				}
				*/
			for (map <Pass_path*,bool>::iterator delete_iter = paths_to_be_deleted.begin(); delete_iter != paths_to_be_deleted.end(); delete_iter++)	
			// delete all the paths that did not fulfill the filtering rules
			{
				vector<Pass_path*>::iterator path_to_delete;
				for (vector<Pass_path*>::iterator paths_iter = path_set.begin(); paths_iter < path_set.end(); paths_iter++)
				{
					if ((*delete_iter).first->get_id() == (*paths_iter)->get_id() && (*delete_iter).second == true)
					{
						path_to_delete = paths_iter;
						break;
					}
				}
				if ((*delete_iter).second == true)
				{
					delete *path_to_delete;
					path_set.erase(path_to_delete);
				}
			}
		}
		odpairs->set_path_set(path_set);
		}
	}
}

void Network::dominancy_rules (Busstop* stop)
{
	// applying static dominancy rules on the transit path choice set
	// relevant aspects: number of transfers, total IVT, total walking distance and further downstream transfer stop
	for (vector <Busstop*>::iterator basic_destination = busstops.begin(); basic_destination < busstops.end(); basic_destination++)
	{
		if (stop->get_id() != (*basic_destination)->get_id() && stop->check_destination_stop(*basic_destination) == true)
		{	
		ODstops* odpairs = stop->get_stop_od_as_origin_per_stop(*basic_destination);
		vector <Pass_path*> path_set = odpairs->get_path_set();
		if (path_set.size() > 1)
		{	
			// go over all the OD pairs of stops that have more than a single alternative
			map <Pass_path*,bool> paths_to_be_deleted;
			for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
			{
				paths_to_be_deleted[(*path_iter)] = false;
			}
			for (vector <Pass_path*>::iterator path1 = path_set.begin(); path1 < path_set.end()-1; path1++)
			{
				for (vector <Pass_path*>::iterator path2 = path1 + 1; path2 < path_set.end(); path2++)
				{
					// check if path1 dominates path2
					if ((*path1)->find_number_of_transfers() < (*path2)->find_number_of_transfers() && (*path1)->calc_total_scheduled_in_vehicle_time(0.0) <= (*path2)->calc_total_scheduled_in_vehicle_time(0.0) && (*path1)->calc_total_walking_distance() <= (*path2)->calc_total_walking_distance())
					{
						paths_to_be_deleted[(*path2)] = true;
						break;
					}
					if ((*path1)->find_number_of_transfers() <= (*path2)->find_number_of_transfers() && (*path1)->calc_total_scheduled_in_vehicle_time(0.0) * (1 + theParameters->dominancy_perception_threshold) < (*path2)->calc_total_scheduled_in_vehicle_time(0.0) && (*path1)->calc_total_walking_distance() * (1 + theParameters->dominancy_perception_threshold) <= (*path2)->calc_total_walking_distance())
					{
						paths_to_be_deleted[(*path2)] = true;
						break;
					}
					if ((*path1)->find_number_of_transfers() <= (*path2)->find_number_of_transfers() && (*path1)->calc_total_scheduled_in_vehicle_time(0.0) * (1 + theParameters->dominancy_perception_threshold) <= (*path2)->calc_total_scheduled_in_vehicle_time(0.0)&& (*path1)->calc_total_walking_distance() * (1 + theParameters->dominancy_perception_threshold) < (*path2)->calc_total_walking_distance())
					{
						paths_to_be_deleted[(*path2)] = true;
						break;
					}
					// check if path2 dominates path1
					if ((*path1)->find_number_of_transfers() > (*path2)->find_number_of_transfers() && (*path1)->calc_total_scheduled_in_vehicle_time(0.0) >= (*path2)->calc_total_scheduled_in_vehicle_time(0.0) && (*path1)->calc_total_walking_distance() >= (*path2)->calc_total_walking_distance()) 
					{
						paths_to_be_deleted[(*path1)] = true;
						break;
					}
					if ((*path1)->find_number_of_transfers() >= (*path2)->find_number_of_transfers() && (*path1)->calc_total_scheduled_in_vehicle_time(0.0) > (*path2)->calc_total_scheduled_in_vehicle_time(0.0) * (1+ theParameters->dominancy_perception_threshold)&& (*path1)->calc_total_walking_distance() >= (*path2)->calc_total_walking_distance() * (1 + theParameters->dominancy_perception_threshold)) 
					{ 
					
						paths_to_be_deleted[(*path1)] = true;
						break;
					}
					if ((*path1)->find_number_of_transfers() >= (*path2)->find_number_of_transfers() && (*path1)->calc_total_scheduled_in_vehicle_time(0.0) >= (*path2)->calc_total_scheduled_in_vehicle_time(0.0) * (1+ theParameters->dominancy_perception_threshold)&& (*path1)->calc_total_walking_distance() > (*path2)->calc_total_walking_distance() * (1 + theParameters->dominancy_perception_threshold)) 
					{
						paths_to_be_deleted[(*path1)] = true;
						break;
					}
				}
			}
			for (map <Pass_path*,bool>::iterator delete_iter = paths_to_be_deleted.begin(); delete_iter != paths_to_be_deleted.end(); delete_iter++)	
			// delete all the dominated paths
			{
				vector<Pass_path*>::iterator path_to_delete;
				for (vector<Pass_path*>::iterator paths_iter = path_set.begin(); paths_iter < path_set.end(); paths_iter++)
				{
					if ((*delete_iter).first->get_id() == (*paths_iter)->get_id() && (*delete_iter).second == true)
					{
						path_to_delete = paths_iter;
						break;
					}
				}
				if ((*delete_iter).second == true)
				{
					delete *path_to_delete;
					path_set.erase(path_to_delete);
				}
			}
		}
		odpairs->set_path_set(path_set);
		}
	}
}

bool Network::totaly_dominancy_rule (ODstops* odstops, vector<vector<Busline*>> lines, vector<vector<Busstop*>> stops) // check if there is already a path with shorter IVT than the potential one
{
	vector <Pass_path*> path_set = odstops->get_path_set();
	if (path_set.size() < 1) // if it has no othe paths yet - then there is nothing to compare
	{
		return false;
	}
	map <Pass_path*,bool> paths_to_be_deleted;
	for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
	{
		paths_to_be_deleted[(*path_iter)] = false;
	}
	for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
	{
		vector<vector<Busline*>> lines1 = (*path_iter)->get_alt_lines();
		vector<vector<Busstop*>> stops1 = (*path_iter)->get_alt_transfer_stops();
		double total_walk = 0.0;
		for (vector<double>::iterator walk_iter = collect_walking_distances.begin(); walk_iter < collect_walking_distances.end(); walk_iter++)
		{
			total_walk += (*walk_iter);
		}
		if (calc_total_in_vechile_time(lines1, stops1) * (1 + theParameters->dominancy_perception_threshold) < calc_total_in_vechile_time(lines,stops))
		{
			if ((*path_iter)->calc_total_walking_distance() <= total_walk)
			{
				if (lines1.size() <= lines.size())
				{
				// the proposed path is dominated - does not fulfill constraints
					return true;
				}
			}
		}
		if (lines1.size() < lines.size())
		{
			if (calc_total_in_vechile_time(lines1, stops1) <= calc_total_in_vechile_time(lines,stops))
			{
				if ((*path_iter)->calc_total_walking_distance() <= total_walk )
				{
					return true;
				}
			}
		}
		if (calc_total_in_vechile_time(lines, stops) * (1 + theParameters->dominancy_perception_threshold) < calc_total_in_vechile_time(lines1,stops1))
		// the proposed path dominates previous paths - delete the dominated path
		{
			if (total_walk <= (*path_iter)->calc_total_walking_distance())
			{
				if (lines.size() <= lines1.size())
				{
					paths_to_be_deleted[(*path_iter)] = true;
				}
			}
		}
		if (lines.size() < lines1.size())
		{
			if (calc_total_in_vechile_time(lines, stops) <= calc_total_in_vechile_time(lines1,stops1))
			{
				if (total_walk  <= (*path_iter)->calc_total_walking_distance())
				{
					paths_to_be_deleted[(*path_iter)] = true;
				}
			}
		}
	}
	for (map <Pass_path*,bool>::iterator delete_iter = paths_to_be_deleted.begin(); delete_iter != paths_to_be_deleted.end(); delete_iter++)	
	// delete all the dominated paths
	{
		vector<Pass_path*>::iterator path_to_delete;
		for (vector<Pass_path*>::iterator paths_iter = path_set.begin(); paths_iter < path_set.end(); paths_iter++)
		{
			if ((*delete_iter).first->get_id() == (*paths_iter)->get_id() && (*delete_iter).second == true)
			{
				path_to_delete = paths_iter;
				break;
			}
		}
		if ((*delete_iter).second == true)
		{
			path_set.erase(path_to_delete);
		}
	}
	odstops->set_path_set(path_set);
	return false;
}

double Network::calc_total_in_vechile_time (vector<vector<Busline*>> lines, vector<vector<Busstop*>> stops)
{
	double sum_in_vehicle_time = 0.0;
	vector<vector <Busstop*>>::iterator iter_stops = stops.begin();
	iter_stops++; // starting from the second stop
	for (vector<vector <Busline*>>::iterator iter_lines = lines.begin(); iter_lines < lines.end(); iter_lines++)
	{
		sum_in_vehicle_time += (*iter_lines).front()->calc_curr_line_ivt((*iter_stops).front(),(*(iter_stops+1)).front(), stops.front().front()->get_rti(), 0.0);
		iter_stops++;
		iter_stops++; 
	}
	return (sum_in_vehicle_time);	
}

/*
bool Network::downstream_dominancy_rule (Pass_path* check_path)
{
	vector<vector<Busstop*>> alt_stops = check_path->get_alt_transfer__stops();
	vector<vector<Busline*>> alt_lines = check_path->get_alt_lines();
	vector<vector<Busline*>>::iterator alt_lines_iter = alt_lines.begin();
	for (vector<vector<Busstop*>>::iterator alt_stops_iter = alt_stops.begin()+2; alt_stops_iter < alt_stops.end(); alt_stops_iter = alt_stops_iter + 2)
	{
		if ((*alt_stops_iter).size() > 1) // no need if there is only one transfer stop
		{
			map <Busstop*,bool> curr_stops_to_be_deleted, next_stops_to_be_deleted;
			// initialize all stops in current and next legs
			for (vector <Busstop*>::iterator stop_iter = (*alt_stops_iter).begin(); stop_iter < (*alt_stops_iter).end(); stop_iter++)
			{
				curr_stops_to_be_deleted[(*stop_iter)] = false;
			}
			for (vector <Busstop*>::iterator stop_iter = (*(alt_stops_iter+1)).begin(); stop_iter < (*(alt_stops_iter+1)).end(); stop_iter++)
			{
				next_stops_to_be_deleted[(*stop_iter)] = false;
			}
	!!!		if (what is the right condition here?)
			{	
				for (vector<Busstop*>::iterator curr_stops = (*alt_stops_iter).begin()+1; curr_stops < (*alt_stops_iter).end(); curr_stops++)
				{
					curr_stops_to_be_deleted[(*curr_stops)] = true;
				}
				for (vector<Busstop*>::iterator next_stops = (*(alt_stops_iter+1)).begin()+1; next_stops < (*(alt_stops_iter+1)).end(); next_stops++)
				{
					next_stops_to_be_deleted[(*next_stops)] = true;
				}		
				for (map <Busstop*,bool>::iterator delete_iter = curr_stops_to_be_deleted.begin(); delete_iter != curr_stops_to_be_deleted.end(); delete_iter++)	
				// delete all the stops that are been dominated by upstream stops
				{
					vector<Busstop*>::iterator stop_to_delete;
					for (vector<Busstop*>::iterator stops_iter = (*alt_stops_iter).begin(); stops_iter < (*alt_stops_iter).end(); stops_iter++)
					{
						if ((*delete_iter).first->get_id() == (*stops_iter)->get_id() && (*delete_iter).second == true)
						{
							stop_to_delete = stops_iter;
							break;
						}
					}
					if ((*delete_iter).second == true)
					{
						(*alt_stops_iter).erase(stop_to_delete);
					}
				}
				for (map <Busstop*,bool>::iterator delete_iter = next_stops_to_be_deleted.begin(); delete_iter != next_stops_to_be_deleted.end(); delete_iter++)	
				// delete all the stops that are been dominated by upstream stops
				{
					vector<Busstop*>::iterator stop_to_delete;
					for (vector<Busstop*>::iterator stops_iter = (*(alt_stops_iter+1)).begin(); stops_iter < (*(alt_stops_iter+1)).end(); stops_iter++)
					{
						if ((*delete_iter).first->get_id() == (*stops_iter)->get_id() && (*delete_iter).second == true)
						{
							stop_to_delete = stops_iter;
							break;
						}
					}
					if ((*delete_iter).second == true)
					{
						(*(alt_stops_iter+1)).erase(stop_to_delete);
					}
				}
			}
		}
		alt_lines_iter++;
	}
	return true;
}
*/
	
bool Network::compare_same_lines_paths (Pass_path* path1, Pass_path* path2)
// checks if two paths are identical in terms of lines
{
	if (path1->get_number_of_transfers() == path2->get_number_of_transfers())
	{
		vector <bool> is_shared;
		vector <vector <Busline*>> lines1 = path1->get_alt_lines();	
		vector <vector <Busline*>> lines2 = path2->get_alt_lines();
		if (lines1.size() > 0 && lines2.size() > 0)
		{
			vector <vector <Busline*>>::iterator lines2_iter = lines2.begin();
			for (vector <vector <Busline*>>::iterator lines1_iter = lines1.begin(); lines1_iter < lines1.end(); lines1_iter++)
			{	
				for (vector <Busline*>::iterator leg_lines1 = (*lines1_iter).begin(); leg_lines1 < (*lines1_iter).end(); leg_lines1++)
				{
					is_shared.push_back(false);
					for (vector <Busline*>::iterator leg_lines2 = (*lines2_iter).begin(); leg_lines2 < (*lines2_iter).end(); leg_lines2++)
					{
						if ((*leg_lines1)->get_id() == (*leg_lines2)->get_id())
						{
							is_shared.pop_back();
							is_shared.push_back(true); // found the corresponding
							break;
						}
					}	
					if (is_shared.back() == false) // this line is not shared - no need to continue
					{
						return false;
					}
				}
				lines2_iter++;
			}
			return true;
		}
		return false; // accounts for walking-only alternatives 
	}
	else
	{
		return false;
	}
}

bool Network::compare_same_stops_paths (Pass_path* path1, Pass_path* path2)
// checks if two paths are identical in terms of stops
{
	if (path1->get_number_of_transfers() == path2->get_number_of_transfers())
	{
		vector <bool> is_shared;
		vector <vector <Busstop*>> stops1 = path1->get_alt_transfer_stops();	
		vector <vector <Busstop*>> stops2 = path2->get_alt_transfer_stops();
		vector <vector <Busstop*>>::iterator stops2_iter = stops2.begin();
		for (vector <vector <Busstop*>>::iterator stops1_iter = stops1.begin(); stops1_iter < stops1.end(); stops1_iter++)
		{	
			for (vector <Busstop*>::iterator leg_stops1 = (*stops1_iter).begin(); leg_stops1 < (*stops1_iter).end(); leg_stops1++)
			{
				is_shared.push_back(false);
				for (vector <Busstop*>::iterator leg_stops2 = (*stops2_iter).begin(); leg_stops2 < (*stops2_iter).end(); leg_stops2++)
				{
					if ((*leg_stops1)->get_id() == (*leg_stops2)->get_id())
					{
						is_shared.pop_back();
						is_shared.push_back(true); // found the corresponding
						break;
					}
				}	
				if (is_shared.back() == false) // this stop is not shared - no need to continue
				{
					return false;
				}
			}
			stops2_iter++;
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool Network::compare_common_partial_routes (Busline* line1, Busline* line2, Busstop* start_section, Busstop* end_section) // checks if two lines have the same route between two given stops
{
	vector<Busstop*>::iterator iter_stop1;
	vector<Busstop*>::iterator iter_stop2;
	// found the pointer to starting point on both lines
	int stop_on_route_counter = 0;
	for (iter_stop1 = line1->stops.begin(); iter_stop1 < line1->stops.end(); iter_stop1++)
	{
		if ((*iter_stop1)->get_id() == start_section->get_id())
		{
			stop_on_route_counter++;
			break;
		}
	}
	for (iter_stop2 = line2->stops.begin(); iter_stop2 < line2->stops.end(); iter_stop2++)
	{
		if ((*iter_stop2)->get_id() == start_section->get_id())
		{
			stop_on_route_counter++;
			break;
		}
	}
	if (stop_on_route_counter < 2)
	{
		return false;
	}
	else
	{
		for (iter_stop1; (*iter_stop1)->get_id() != end_section->get_id(); iter_stop1++)
		{
			if ((*iter_stop1)->get_id() != (*iter_stop2)->get_id())
			{
				return false;
			}
			iter_stop2++;
			if (iter_stop2 == line2->stops.end())
			{
				return false;
			}
			if (iter_stop1 == line1->stops.end()-1)
			{
				return false;
			}	
		}
		if ((*(iter_stop2))->get_id() != end_section->get_id())
		{
			return false;
		}
	}
	return true;
}

bool Network::check_constraints_paths (Pass_path* path) // checks if the path meets all the constraints
{
	if (path->get_alt_transfer_stops().size() <= 4)
	{
		return true;
	}
	if (check_path_no_repeating_lines(path->get_alt_lines(), path->get_alt_transfer_stops()) == false)
	{
		return false;
	}
	if (check_path_no_repeating_stops(path) == false)
	{
		return false;
	}
	if (check_path_no_opposing_lines(path->get_alt_lines()) == false)
	{
		return false;
	}
return true;
}

bool Network::check_path_no_repeating_lines (vector<vector<Busline*>> lines, vector<vector<Busstop*>> stops_) // checks if the path does not include going on and off the same bus line at the same stop
{
	if (lines.size() < 2)
	{
		return true;
	}
	vector <vector <Busstop*>>::iterator stops = stops_.begin()+1;
	for (vector <vector <Busline*>>::iterator lines_iter1 = lines.begin(); lines_iter1 < (lines.end()-1); lines_iter1++)
	{
		stops = stops + 2; // start at fourth place and then jumps two every next pair of lines
		for (vector <Busline*>::iterator leg_lines1 = (*lines_iter1).begin(); leg_lines1 < (*lines_iter1).end(); leg_lines1++)
		{
			for (vector <vector <Busline*>>::iterator lines_iter2 = lines_iter1 + 1; lines_iter2 < lines.end(); lines_iter2++)
			{
				int counter_sim = 0; 
				// checking all legs
				for (vector <Busline*>::iterator leg_lines2 = (*lines_iter2).begin(); leg_lines2 < (*lines_iter2).end(); leg_lines2++)
				{
					// checking whether the two lines have the same path on the relevant segment (if so - no reason for transfer)
					if ((*leg_lines1)->stops.size() != (*leg_lines2)->stops.size())
					{
						int i = 0;
					}
					if ((*leg_lines1)->get_id() == (*leg_lines2)->get_id() || compare_common_partial_routes((*leg_lines1),(*leg_lines2),(*stops).front(), (*(stops+1)).front()) == true)
					{
						counter_sim++;
					}
				}
				if (counter_sim == ((*lines_iter2).size()))
				{
					return false;
				}
			}
		}
	}
	return true;
}

/*
bool Network::check_sequence_no_repeating_lines(vector<vector<Busline*>> lines, vector<Busstop*> stops_)
{
	vector <Busstop*>::iterator stops = stops_.begin();
	for (vector <vector <Busline*>>::iterator lines_iter1 = lines.begin(); lines_iter1 < (lines.end()-1); lines_iter1++)
	{
		for (vector <Busline*>::iterator leg_lines1 = (*lines_iter1).begin(); leg_lines1 < (*lines_iter1).end(); leg_lines1++)
		{
			for (vector <vector <Busline*>>::iterator lines_iter2 = lines_iter1 + 1; lines_iter2 < lines.end(); lines_iter2++)
			{
				int counter_sim = 0; 
				// checking all legs
				for (vector <Busline*>::iterator leg_lines2 = (*lines_iter2).begin(); leg_lines2 < (*lines_iter2).end(); leg_lines2++)
				{
					if ((*leg_lines1)->get_id() == (*leg_lines2)->get_id() || compare_common_partial_routes((*leg_lines1),(*leg_lines2),(*stops), (*(stops+1))) == true)
					{
						counter_sim++;
					}
				}
				if (counter_sim == ((*lines_iter2).size()))
				{
					return false;
				}
			}
		}
		stops++;
	}
	return true;	
}
*/

bool Network::check_path_no_repeating_stops (Pass_path* path) // chceks if the path does not include going through the same stop more than once
{
	vector <vector <Busstop*>> stops = path->get_alt_transfer_stops();	
	for (vector <vector <Busstop*>>::iterator stops_iter1 = stops.begin(); stops_iter1 < stops.end(); stops_iter1++)
	{	
		for (vector <Busstop*>::iterator leg_stops1 = (*stops_iter1).begin(); leg_stops1 < (*stops_iter1).end()-1; leg_stops1++)
		{
			for (vector <Busstop*>::iterator leg_stops2 = leg_stops1 + 1; leg_stops1 < (*stops_iter1).end(); leg_stops1++)
			// go over the consecutive stops in the same leg
			{
				if ((*leg_stops1)->get_id() == (*leg_stops2)->get_id())
				{
					return false;
				}
			}
			// go over all the stops in consecutive legs
			for (vector <vector <Busstop*>>::iterator stops_iter2 = stops_iter1+1; stops_iter2 < stops.end(); stops_iter2++)
			{
				for (vector <Busstop*>::iterator leg_stops2 = (*stops_iter2).begin(); leg_stops1 < (*stops_iter1).end()-1; leg_stops1++)
				{
					if ((*leg_stops1)->get_id() == (*leg_stops2)->get_id())
					{
						return false;
					}
				}
			}
		}
	}
return true;
}

bool Network::check_sequence_no_repeating_stops (vector<Busstop*> stops) // chceks if the sequence does not include going through the same stop more than once
{
	for (vector <Busstop*>::iterator stops_iter1 = stops.begin()+1; stops_iter1 < stops.end()-1; stops_iter1 = stops_iter1+2)
	{
		for (vector <Busstop*>::iterator stops_iter2 = stops_iter1+1; stops_iter2 < stops.end(); stops_iter2++)
		{
			if ((*stops_iter1)->get_id() == (*stops_iter2)->get_id())
			{
				return false;
			}
		}
	}
return true;
}

bool Network::check_path_no_opposing_lines (vector<vector<Busline*>> lines)
{
	for (vector<vector<Busline*>>::iterator lines_leg = lines.begin(); lines_leg < lines.end()-1; lines_leg++)
	{
		for (vector<vector<Busline*>>::iterator lines_leg1 = lines_leg + 1; lines_leg1 < lines.end(); lines_leg1++)
		{
			for (vector<Busline*>::iterator line_iter = (*lines_leg).begin(); line_iter < (*lines_leg).end(); line_iter++)
			{
				for (vector<Busline*>::iterator line_iter1 = (*lines_leg1).begin(); line_iter1 < (*lines_leg1).end(); line_iter1++)
				{
					if ((*line_iter)->get_opposite_id() == (*line_iter1)->get_id() || (*line_iter)->get_id() == (*line_iter1)->get_opposite_id())
					{	
						return false;
					}
				}
			}
		}
	}
	return true;
}

bool Network::write_path_set (string name1)
{
	ofstream out1(name1.c_str(),ios_base::app);
	assert (out1);
	int pathcounter = 0;
	out1 << "transit_paths:" << '\t' ;
	for (vector<Busstop*>::iterator stop_iter = busstops.begin(); stop_iter < busstops.end(); stop_iter++)
	{
		ODs_for_stop odstops = (*stop_iter)->get_stop_as_origin();
		for (ODs_for_stop::iterator basic_destination = odstops.begin(); basic_destination != odstops.end(); basic_destination++)
		{
			vector <Pass_path*> path_set = (*basic_destination).second->get_path_set();
			for (vector<Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
			{
				pathcounter++;
				vector<vector<Busstop*>> stops = (*path_iter)->get_alt_transfer_stops();
				vector<vector<Busline*>> lines = (*path_iter)->get_alt_lines();
				vector<double> walking = (*path_iter)->get_walking_distances();
				out1 << (*basic_destination).second->get_origin()->get_id() << '\t'<< (*basic_destination).second->get_destination()->get_id() << '\t' << (*path_iter)->get_id()  << '\t' << stops.size() << '\t' << '{' << '\t';
				for (vector<vector <Busstop*>>::iterator stop_leg = stops.begin(); stop_leg < stops.end(); stop_leg++)
				{
					out1 << (*stop_leg).size() << '\t' << '{' << '\t';
					for (vector<Busstop*>::iterator stop_iter = (*stop_leg).begin(); stop_iter < (*stop_leg).end(); stop_iter++)
					{
						out1 << (*stop_iter)->get_id() << '\t';
					}
					out1 << '}' << '\t' ;
				}	
				out1 << '}' << '\t' << lines.size() << '\t' << '{';
				for (vector<vector <Busline*>>::iterator line_leg = lines.begin(); line_leg < lines.end(); line_leg++)
				{
					out1 << (*line_leg).size() << '{' << '\t';
					for (vector<Busline*>::iterator line_iter = (*line_leg).begin(); line_iter < (*line_leg).end(); line_iter++)
					{
						out1 << (*line_iter)->get_id() << '\t';
					}
					out1 << '}' << '\t' ;
				}
				out1 << '}' << '\t' << walking.size() << '\t' << '{';
				for (vector <double>::iterator walk_iter = walking.begin(); walk_iter < walking.end(); walk_iter++)
				{
					out1 << (*walk_iter) << '\t';
				}
				out1 << '}' << endl;	
			}
		}
	}
	out1 << "nr_paths:" << '\t' << pathcounter << '\t' <<  endl;
	return true;
}

bool Network::write_path_set_per_stop (string name1, Busstop* stop)
{
	ofstream out1(name1.c_str(),ios_base::app);
	assert (out1);
	ODs_for_stop odstops = stop->get_stop_as_origin();
	for (ODs_for_stop::iterator basic_destination = odstops.begin(); basic_destination != odstops.end(); basic_destination++)
	{
		vector <Pass_path*> path_set = (*basic_destination).second->get_path_set();
		for (vector<Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
		{
			vector<vector<Busstop*>> stops = (*path_iter)->get_alt_transfer_stops();
			vector<vector<Busline*>> lines = (*path_iter)->get_alt_lines();
			vector<double> walking = (*path_iter)->get_walking_distances();
			out1 << (*basic_destination).second->get_origin()->get_id() << '\t'<< (*basic_destination).second->get_destination()->get_id() << '\t' << (*path_iter)->get_id()  << '\t' << stops.size() << '\t' << '{' << '\t';
			for (vector<vector <Busstop*>>::iterator stop_leg = stops.begin(); stop_leg < stops.end(); stop_leg++)
			{
				out1 << (*stop_leg).size() << '\t' << '{' << '\t';
				for (vector<Busstop*>::iterator stop_iter = (*stop_leg).begin(); stop_iter < (*stop_leg).end(); stop_iter++)
				{
					out1 << (*stop_iter)->get_id() << '\t';
				}
				out1 << '}' << '\t' ;
			}	
			out1 << '}' << '\t' << lines.size() << '\t' << '{';
			for (vector<vector <Busline*>>::iterator line_leg = lines.begin(); line_leg < lines.end(); line_leg++)
			{
				out1 << (*line_leg).size() << '{' << '\t';
				for (vector<Busline*>::iterator line_iter = (*line_leg).begin(); line_iter < (*line_leg).end(); line_iter++)
				{
					out1 << (*line_iter)->get_id() << '\t';
				}
				out1 << '}' << '\t' ;
			}
			out1 << '}' << '\t' << walking.size() << '\t' << '{';
			for (vector <double>::iterator walk_iter = walking.begin(); walk_iter < walking.end(); walk_iter++)
			{
				out1 << (*walk_iter) << '\t';
			}
			out1 << '}' << endl;	
		}
	}
	return true;
}

bool Network::write_path_set_per_od (string name1, Busstop* origin_stop, Busstop* destination_stop)
{
	ofstream out1(name1.c_str(),ios_base::app);
	assert (out1);
	ODstops* od = origin_stop->get_stop_od_as_origin_per_stop(destination_stop);
	vector<Pass_path*> path_set = od->get_path_set();
	for (vector<Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
	{
		vector<vector<Busstop*>> stops = (*path_iter)->get_alt_transfer_stops();
		vector<vector<Busline*>> lines = (*path_iter)->get_alt_lines();
		vector<double> walking = (*path_iter)->get_walking_distances();
		out1 << od->get_origin()->get_id() << '\t'<< od->get_destination()->get_id() << '\t' << (*path_iter)->get_id()  << '\t' << stops.size() << '\t' << '{' << '\t';
		for (vector<vector <Busstop*>>::iterator stop_leg = stops.begin(); stop_leg < stops.end(); stop_leg++)
		{
			out1 << (*stop_leg).size() << '\t' << '{' << '\t';
			for (vector<Busstop*>::iterator stop_iter = (*stop_leg).begin(); stop_iter < (*stop_leg).end(); stop_iter++)
			{
				out1 << (*stop_iter)->get_id() << '\t';
			}
			out1 << '}' << '\t' ;
		}	
		out1 << '}' << '\t' << lines.size() << '\t' << '{';
		for (vector<vector <Busline*>>::iterator line_leg = lines.begin(); line_leg < lines.end(); line_leg++)
		{
			out1 << (*line_leg).size() << '{' << '\t';
			for (vector<Busline*>::iterator line_iter = (*line_leg).begin(); line_iter < (*line_leg).end(); line_iter++)
			{
				out1 << (*line_iter)->get_id() << '\t';
			}
			out1 << '}' << '\t' ;
		}
		out1 << '}' << '\t' << walking.size() << '\t' << '{';
		for (vector <double>::iterator walk_iter = walking.begin(); walk_iter < walking.end(); walk_iter++)
		{
			out1 << (*walk_iter) << '\t';
		}
		out1 << '}' << endl;	
	}
	return true;
}

bool Network::read_transit_path_sets(string name)
{
	ifstream in(name.c_str()); // open input file
	assert (in);
	string keyword;
	in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="transit_paths:")
	{
		cout << " read_transit_path_sets: no << transit_paths: >> keyword " << endl;
		in.close();
		return false;
	}
	int nr= 0;
	in >> nr;
	int i=0;
	for (i; i<nr;i++)
	{
		if (!read_transit_path(in))
		{
			cout << " read_transit_path_sets: read_transit_path_sets returned false for line nr " << (i+1) << endl;
			in.close();
			return false;
		} 
	}
	cout << "Reading path set completed" << endl;
	return true;
}

bool Network::read_transit_path(istream& in)
{
	char bracket;
	int origin_id, destination_id, nr_stop_legs, nr_line_legs, nr_stops, nr_lines, nr_walks, stop_id, line_id;
	double walk_dis;
	string path_id;
	vector<vector<Busstop*>> alt_stops;
	vector<vector<Busline*>> alt_lines;
	vector<double> alt_walking;
	in >> origin_id >> destination_id >> path_id >> nr_stop_legs;

	Busstop* bs_o = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (origin_id) )));	
	Busstop* bs_d = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (destination_id) )));
	// reading all the stops that compose the path
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::read_transit_path scanner jammed at " << bracket;
		return false;
	}
	for (int i=0; i<nr_stop_legs; i++)
	{
		in >> nr_stops;
		in >> bracket;
		if (bracket != '{')
		{
			cout << "readfile::read_transit_path scanner jammed at " << bracket;
			return false;
		}
		vector<Busstop*> stops_leg;
		for (int j=0; j<nr_stops; j++)
		{
			in >> stop_id;
			Busstop* bs = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (stop_id) )));
			stops_leg.push_back(bs);
		}
		in >> bracket;		
		if (bracket != '}')
		{
			cout << "readfile::read_transit_path scanner jammed at " << bracket;
			return false;
		}
		alt_stops.push_back(stops_leg);
	}
	in >> bracket;		
	if (bracket != '}')
	{
		cout << "readfile::read_transit_path scanner jammed at " << bracket;
		return false;
	}

	// reading all the lines that compose the path
	in >> nr_line_legs >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::read_transit_path scanner jammed at " << bracket;
		return false;
	}
	for (int i=0; i<nr_line_legs; i++)
	{
		in >> nr_lines;
		if (nr_lines < 1)
		{
			alt_lines.clear();
		}
		in >> bracket;
		if (bracket != '{')
		{
			cout << "readfile::read_transit_path scanner jammed at " << bracket;
			return false;
		}
		vector<Busline*> lines_leg;
		for (int j=0; j<nr_lines; j++)
		{
			in >> line_id;
			Busline* bl = (*(find_if(buslines.begin(), buslines.end(), compare <Busline> (line_id) )));
			lines_leg.push_back(bl);
		}
		in >> bracket;		
		if (bracket != '}')
		{
			cout << "readfile::read_transit_path scanner jammed at " << bracket;
			return false;
		}
		alt_lines.push_back(lines_leg);
	}
	in >> bracket;		
	if (bracket != '}')
	{
		cout << "readfile::read_transit_path scanner jammed at " << bracket;
		return false;
	}

	// reading the walking distances that compose the path
	in >> nr_walks;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::read_transit_path scanner jammed at " << bracket;
		return false;
	}
	for (int j=0; j<nr_walks; j++)
	{
		in >> walk_dis;
		alt_walking.push_back(walk_dis);
	}
	in >> bracket;		
	if (bracket != '}')
	{
		cout << "readfile::read_transit_path scanner jammed at " << bracket;
		return false;
	}

	Pass_path* path = new Pass_path(pathid, alt_lines , alt_stops, alt_walking); 
	pathid++;
	if (bs_o->check_stop_od_as_origin_per_stop(bs_d) == false)
	{
		ODstops* od_stop = new ODstops (bs_o,bs_d); 
		bs_o->add_odstops_as_origin(bs_d, od_stop);
		bs_d->add_odstops_as_destination(bs_o, od_stop);
	}
	ODstops* od_pair = bs_o->get_stop_od_as_origin_per_stop(bs_d);
	od_pair->add_paths(path);
	return true;
}

/////////////// Transit path-set generation functions: end

bool Network::read_transitday2day(string name)
{
	ifstream in(name.c_str()); // open input file
	assert (in);
	string keyword;
	in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="ODS:")
	{
		cout << " read_ODs: no << ODs: >> keyword " << endl;
		in.close();
		return false;
	}
	int nr= 0;
	in >> nr;
	int i=0;
	for (i; i<nr;i++)
	{
		if (!read_OD_day2day(in))
		{
			cout << " read_OD_day2day: read_OD_day2day returned false for line nr " << (i+1) << endl;
			in.close();
			return false;
		} 
	}
	return true;
}

bool Network::read_IVTT_day2day(string name)
{
	ifstream in(name.c_str()); // open input file
	assert (in);
	string keyword;
	in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="ODS:")
	{
		cout << " read_ODs: no << ODs: >> keyword " << endl;
		in.close();
		return false;
	}
	int nr= 0;
	in >> nr;
	int i=0;
	for (i; i<nr;i++)
	{
		if (!read_OD_IVTT(in))
		{
			cout << " read_OD_day2day: read_OD_day2day returned false for line nr " << (i+1) << endl;
			in.close();
			return false;
		} 
	}
	return true;
}

bool Network::read_OD_day2day (istream& in) 
{
	char bracket;
    int origin_stop_id, destination_stop_id, stop_id, line_id;
	double anticipated_waiting_time, alpha_RTI, alpha_exp;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::read_transit_path scanner jammed at " << bracket;
		return false;
	}
	in >> origin_stop_id >> destination_stop_id >> stop_id >> line_id >> anticipated_waiting_time >> alpha_RTI >> alpha_exp;
	Busstop* bs_o = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (origin_stop_id) )));	
	Busstop* bs_d = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (destination_stop_id) )));
	Busstop* bs_s = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (stop_id) )));
	Busline* bl = (*(find_if(buslines.begin(), buslines.end(), compare <Busline> (line_id) )));
	ODstops* od_stop = bs_o->get_stop_od_as_origin_per_stop(bs_d);
	od_stop->set_anticipated_waiting_time(bs_s, bl, anticipated_waiting_time);
	od_stop->set_alpha_RTI(bs_s, bl, alpha_RTI);
	od_stop->set_alpha_exp(bs_s, bl, alpha_exp);
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::read_transit_path scanner jammed at " << bracket;
		return false;
	}
	return true;
}

bool Network::read_OD_IVTT (istream& in) 
{
	char bracket;
    int origin_stop_id, destination_stop_id, stop_id, line_id, leg_id;
	double anticipated_in_vehicle_time, alpha_exp;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::read_transit_path scanner jammed at " << bracket;
		return false;
	}
	in >> origin_stop_id >> destination_stop_id >> stop_id >> line_id >> leg_id >> anticipated_in_vehicle_time >> alpha_exp;
	Busstop* bs_o = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (origin_stop_id) )));	
	Busstop* bs_d = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (destination_stop_id) )));
	Busstop* bs_s = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (stop_id) )));
	Busline* bl = (*(find_if(buslines.begin(), buslines.end(), compare <Busline> (line_id) )));
	Busstop* bs_l = (*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (leg_id) )));
	ODstops* od_stop = bs_o->get_stop_od_as_origin_per_stop(bs_d);
	od_stop->set_anticipated_ivtt(bs_s, bl, bs_l, anticipated_in_vehicle_time);
	od_stop->set_ivtt_alpha_exp(bs_s, bl, bs_l, alpha_exp);
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::read_transit_path scanner jammed at " << bracket;
		return false;
	}
	return true;
}

bool Network::readtransitfleet (string name) // !< reads transit vehicle types, vehicle scheduling and dwell time functions
{
	ifstream in(name.c_str()); // open input file
	assert (in);
	string keyword;
	int nr= 0;
	int i=0;
	int limit;
in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="dwell_time_functions:")
	{
		cout << " readbustypes: no << bustypes: >> keyword " << endl;
		return false;
	}
	in >> nr;
	limit = i + nr;
	for (i; i<limit;i++)
	{
 		if (!read_dwell_time_function(in))
		{
			cout << " readbustypes: read_bustypes returned false for line nr " << (i+1) << endl;
   			return false;
		} 
	}
in >> keyword;
	if (keyword!="vehicle_types:")
	{
		cout << " readbustypes: no << bustypes: >> keyword " << endl;
		return false;
	}
	in >> nr;
	limit = i + nr;
	for (i; i<limit;i++)
	{
 		if (!read_bustype(in))
		{
			cout << " readbustypes: read_bustypes returned false for line nr " << (i+1) << endl;
   			return false;
		} 
	}
in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="vehicle_scheduling:")
	{
		cout << " readbusvehicle: no << busvehicles: >> keyword " << endl;
		return false;
	}
	in >> nr;
	limit = i + nr;
	for (i; i<limit;i++)
	{
 		if (!read_busvehicle(in))
		{
			cout << " readbusvehicles: read_busvehicle returned false for line nr " << (i+1) << endl;
   			return false;
		} 
	}
	return true;
}

bool Network::read_dwell_time_function (istream& in)
{
   char bracket;
	int func_id;
// dwell time parameters
   int dwell_time_function_form; 
	// 11 - Linear function of boarding and alighting
    // 12 - Linear function of boarding and alighting + non-linear crowding effect (Weidmann) 
    // 13 - Max (boarding, alighting) + non-linear crowding effect (Weidmann) 
    // 20 - TCRP(max doors with crowding, boarding from x doors, alighting from y doors) + bay + stop capacity
    // 21 - TCRP(max doors with crowding, boarding from front door, alighting from both doors) + bay + stop capacity
   double dwell_constant;
   double boarding_coefficient;
   double alighting_cofficient;
   double dwell_std_error;
   
   // in case of TCRP function form
   double share_alighting_front_door;
   double crowdedness_binary_factor;
   double number_boarding_doors, number_alighting_doors;

   // extra delays
   double bay_coefficient;
   double over_stop_capacity_coefficient;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbusstop scanner jammed at " << bracket;
  	return false;
  }
	in >> func_id >> dwell_time_function_form >> dwell_constant >> boarding_coefficient >> alighting_cofficient >> dwell_std_error >> bay_coefficient >> over_stop_capacity_coefficient;
	if (dwell_time_function_form == 20)
	{
		in >> number_boarding_doors >> number_alighting_doors >> share_alighting_front_door >> crowdedness_binary_factor;
	}
	if (dwell_time_function_form == 21)
	{
		in >> share_alighting_front_door >> crowdedness_binary_factor;
		Dwell_time_function* dt= new Dwell_time_function (func_id,dwell_time_function_form,dwell_constant,boarding_coefficient,alighting_cofficient,dwell_std_error,share_alighting_front_door,crowdedness_binary_factor,bay_coefficient,over_stop_capacity_coefficient);
		dt_functions.push_back (dt);
	}
	if (dwell_time_function_form == 22)
	{
		Dwell_time_function* dt= new Dwell_time_function (func_id,dwell_time_function_form,dwell_constant,boarding_coefficient,alighting_cofficient,dwell_std_error,share_alighting_front_door,number_boarding_doors,number_alighting_doors,crowdedness_binary_factor,bay_coefficient,over_stop_capacity_coefficient);
		dt_functions.push_back (dt);
	}	
	else
	{
		Dwell_time_function* dt= new Dwell_time_function (func_id,dwell_time_function_form,dwell_constant,boarding_coefficient,alighting_cofficient,dwell_std_error,bay_coefficient,over_stop_capacity_coefficient);
		dt_functions.push_back (dt);
	}
  in >> bracket;
  if (bracket != '}')
  {
    cout << "readfile::readbustype scanner jammed at " << bracket;
    return false;
  }
}

bool Network::read_bustype (istream& in) // reads a bustype
{

//{ type_id	length	number_seats	capacity }
  char bracket;
  int type_id, number_seats, capacity, dtf_id;
  double length;
  string bus_type_name;
  bool ok= true;
  vector <Bustype*> types;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbusstop scanner jammed at " << bracket;
  	return false;
  }
  in >> type_id  >> bus_type_name >> length >> number_seats >> capacity >> dtf_id;
  Dwell_time_function* dtf=(*(find_if(dt_functions.begin(), dt_functions.end(), compare <Dwell_time_function> (dtf_id) )));
  Bustype* bt= new Bustype (type_id, bus_type_name, length, number_seats, capacity,dtf);
  in >> bracket;
  if (bracket != '}')
  {
    cout << "readfile::readbustype scanner jammed at " << bracket;
    return false;
  }
  bustypes.push_back (bt);

#ifdef _DEBUG_NETWORK
  cout << " read bustype"<< type_id <<endl;
#endif //_DEBUG_NETWORK
  return ok;
}
 
bool Network::read_busvehicle(istream& in) // reads a bus vehicle
{
  char bracket;
  int bv_id, type_id, nr_trips, trip_id;
  vector <Start_trip*> driving_roster;
  bool ok= true;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbusvehicle scanner jammed at " << bracket;
  	return false;
  }
  in >> bv_id >> type_id >> nr_trips;
	
  // find bus type and create bus vehicle
  Bustype* bty=(*(find_if(bustypes.begin(), bustypes.end(), compare <Bustype> (type_id) )));
  // generate a new bus vehicle
  vid++; // increment the veh id counter, buses are vehicles too
  //Bus* bus=recycler.newBus(); // get a bus vehicle
  //bus->set_bus_id(bv_id);
  //bus->set_bustype_attributes(bty);
  in >> bracket;
  if (bracket != '{')
  {
  		cout << "readfile::readsbusvehicle scanner jammed at " << bracket;
  		return false;
  }
  for (int i=0; i < nr_trips; i++) // for each trip on the chain
  {
	  in >> trip_id;
		vector<Bustrip*>::iterator btr_it = find_if(bustrips.begin(), bustrips.end(), compare <Bustrip> (trip_id) ); // find the trip in the list
		if (btr_it == bustrips.end())
		{
			cout << "Bus trip " << trip_id << " not found.";
			return false;
		}
		Bustrip* btr = *btr_it;

	  //Moved here by Jens 2014-09-05
	  Bus* bus=recycler.newBus(); // get a bus vehicle
	  bus->set_bus_id(bv_id);
	  bus->set_bustype_attributes(bty);
	  btr->set_busv(bus);
	  bus->set_curr_trip(btr);
	  if (i>0) // flag as busy for all trip except the first on the chain
	  {
		  btr->get_busv()->set_on_trip(true);
	  }
	  btr->set_bustype(bty); 
	  Start_trip* st = new Start_trip (btr, btr->get_starttime());
	  driving_roster.push_back(st); // save the driving roster at the vehicle level
	  busvehicles.push_back (bus);
  }
  for (vector<Start_trip*>::iterator trip = driving_roster.begin(); trip < driving_roster.end(); trip++)
  {
	(*trip)->first->add_trips(driving_roster); // save the driving roster at the trip level for each trip on the chain
  }
  
  in >> bracket;
  if (bracket != '}')
  {
  		cout << "readfile::readsbusvehicle scanner jammed at " << bracket;
  		return false;
  }   
  in >> bracket;
  if (bracket != '}')
  {
    cout << "readfile::readbusvehicle scanner jammed at " << bracket;
    return false;
  }
#ifdef _DEBUG_NETWORK
  cout << " read busvehicle"<< bv_id <<endl;
#endif //_DEBUG_NETWORK
return ok;	
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
	{
		inputfile.close();
		return false;
	}
	int nr;
	inputfile >> nr;
	for (int i=0; i<nr;i++)
	{
		if (!readsignalcontrol(inputfile))
		{
			inputfile.close();
			return false;
		}
	}
	inputfile.close();
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
		map <int,Turning*>::iterator t_iter;
		t_iter=turningmap.find(turnid);
		assert (t_iter!=turningmap.end());
		Turning* turn=t_iter->second;
		//vector <Turning*>::iterator turn = find_if(turnings.begin(), turnings.end(), compare<Turning>(turnid));
		//assert (turn != turnings.end());
		// add to stage
		stageptr->add_turning(turn);
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
		inputfile.close();
		return true;
	}
	else
	{
		inputfile.close();
		return false;
	}
}

bool Network::register_links()
{
	// register all the incoming links @ the destinations
	for (map<int,Destination*>::iterator iter=destinationmap.begin(); iter!=destinationmap.end();iter++)
	{
		(*iter).second->register_links(linkmap);
	}
	//register all the outgoing links at the origins
	for (map<int, Origin*>::iterator iter1=originmap.begin(); iter1!=originmap.end();iter1++)
	{
		(*iter1).second->register_links(linkmap);
	}
	//register all the incoming & outgoing links at the junctions
	for (map<int, Junction*>::iterator iter2=junctionmap.begin(); iter2!=junctionmap.end();iter2++)
	{
		(*iter2).second->register_links(linkmap);
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
	int oid, did;
	double rate;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readdemandfile::readod scanner jammed at " << bracket;
		return false;
	}
	in  >> oid >> did >> rate;
	// check oid, did, rate;
	// scale up/down the rate
	rate=rate*scale;
	//assert (rate > 0);
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readdemandfile::readod scanner jammed at " << bracket;
		return false;
	}
	// find oid, did
	// find the origin & dest  pointers
	map <int, Origin*>::iterator o_iter; 
	o_iter = originmap.find(oid);
	assert (o_iter != originmap.end());

	map <int, Destination*>::iterator d_iter; 
	d_iter = destinationmap.find(did);
	assert (d_iter != destinationmap.end());

#ifdef _DEBUG_NETWORK
	cout << "found o and d " << oid << "," << did << endl;
#endif //_DEBUG_NETWORK
	// create odpair

	ODpair* odpair=new ODpair (o_iter->second, d_iter->second, rate,&vehtypes); // later the vehtypes can be defined PER OD

	//add odpair to origin and general list
	odpairs.insert(odpairs.begin(),odpair);
	(o_iter->second)->add_odpair(odpair);
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
	vector <Route*> deleted_routes;
	vector <Route*> new_del_routes;
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
	if (nr_deleted > 0 )
		cout << nr_deleted << " routes deleted" << endl;

	// write the new routes file.
	vector <Route*>::iterator del=deleted_routes.begin();
	multimap<odval,Route*>::iterator route_m, route_l, route_u;
	vector <Route*>::iterator route;
	for (del; del < deleted_routes.end(); del++)
	{
		odval val=(*del)->get_oid_did();
		route_l  = routemap.lower_bound(val);
		route_u  = routemap.upper_bound(val);
		for (route_m=route_l;route_m != route_u; route_m++) // check all routes  for given odval
		{
			if ((*route_m).second->get_id() == (*del)->get_id())
			{
				routemap.erase(route_m);
				delete (*del);
				break;
			}
		}
	}


	return true;
}

bool Network::addroutes (int oid, int did, ODpair* odpair)
{
/*
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
	*/
	
	//vector <ODpair*>::iterator od_iter=odpairs.begin();
	odval od_v(oid, did);
	multimap <odval,Route*>::iterator r_iter, r_lower, r_upper;
	r_lower = routemap.lower_bound(od_v); // get lower boundary of all routes with this OD
	r_upper = routemap.upper_bound(od_v); // get upper boundary
	for (r_iter=r_lower; r_iter != r_upper; r_iter++) // add all routes to OD pair
	{
		odpair->add_route((*r_iter).second);
#ifdef _DEBUG_NETWORK	
		cout << "added route " << ((*r_iter).first)<< endl;
#endif //_DEBUG_NETWORK		
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
		cout << "readdemandfile::readrate scanner jammed at " << bracket;
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
		cout << "readdemandfile::readrate scanner jammed at " << bracket;
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
	{
		in.close();
		return false;
	}
	int nr;
	in >> nr;
	for (int i=0; i<nr;i++)
	{
		if (!readserverrate(in))
		{
			in.close();
			return false;
		}
	}
	in.close();
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
		cout << "readserverrate::readserverrate scanner jammed at " << bracket;
		return  false;
	}
	in  >> sid >> time >> mu >> sd;
	//assert  ( (find_if (servers.begin(),servers.end(), compare <Server> (sid))) != servers.end() );   // server with sid exists
	assert (servermap.count(sid)); // make sure exists
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readserverrate::readserverrate scanner jammed at " << bracket;
		return false;
	}
	//Server* sptr=*(find_if (servers.begin(),servers.end(), compare <Server> (sid)));
	Server* sptr = servermap[sid];
	ChangeRateAction* cptr=new ChangeRateAction(eventlist,time,sptr,mu,sd);
	assert (cptr != NULL);
	changerateactions.push_back(cptr);
	return true;
}

bool Network::readdemandfile(string name)
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
		{
			inputfile.close();
			return false;
		}
		int nr;
		inputfile >> nr;
		for (int i=0; i<nr; i++)
		{
			if (!readrates(inputfile))
			{
				inputfile.close();
				return false;			
			}
		}
		inputfile.close();
		return true;
	}	
	else
	{
		inputfile.close();
		return false;	
	}
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
	{
		in.close();
		return false;
	}
	int nr;
	in >> nr;
	for (int i=0; i<nr;i++)
	{
		if (!readvtype(in))
		{
			in.close();
			return false;
		}
	}
	vehtypes.initialize();
	in.close();
	return true;
}



bool Network::writeoutput(string name)
// !!!!!!! changed to write the OD output!!!!!!!!!
{
	ofstream out(name.c_str());
	//assert(out);
	bool ok=true;
	vector<ODpair*>::iterator iter=odpairs.begin();
	(*iter)->writefieldnames(out);
	for (iter;iter<odpairs.end();iter++)
	{
		ok=ok && (*iter)->write(out);
	}
	out.close();
	return ok;
}

bool Network::writesummary(string name)
// !!!!!!! writes the OD pair output!!!!!!!!!
{
	ofstream out(name.c_str());
	//assert(out);
	bool ok=true;
	out << "Origin\tDestination\tNrRoutes\tNrArrived\tTotalTravelTime(s)\tTotalMileage(m)" << endl;
	for (vector<ODpair*>::iterator iter=odpairs.begin();iter<odpairs.end();iter++)
	{
		ok=ok && (*iter)->writesummary(out);
	}
	out.close();
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
	out.close();
	return true;


}

bool Network::write_busstop_output(string name1, string name2, string name3, string name4, string name5, string name6, string name7, string name8, string name9, string name10, string name11, string name12, string name13, string name14, string name15)
{
	ofstream out1(name1.c_str(),ios_base::app);
	ofstream out2(name2.c_str(),ios_base::app);
	ofstream out3(name3.c_str(),ios_base::app);
	ofstream out4(name4.c_str(),ios_base::app);
	ofstream out5(name5.c_str(),ios_base::app);
	ofstream out6(name6.c_str(),ios_base::app);
	ofstream out7(name7.c_str(),ios_base::app);
	ofstream out8(name8.c_str(),ios_base::app);
	ofstream out9(name9.c_str(),ios_base::app);
	ofstream out10(name10.c_str(),ios_base::app);
	ofstream out11(name11.c_str(),ios_base::app);
	ofstream out12(name12.c_str(),ios_base::app);
	ofstream out13(name13.c_str(),ios_base::app);
	ofstream out14(name14.c_str(),ios_base::app);
	ofstream out15(name15.c_str(),ios_base::app);
	/*
	assert(out1);
	assert(out2);
	assert(out3);
	assert(out4);
	assert(out5);
	assert(out6);
	assert(out7);
	assert(out8);
	assert(out9);
	assert(out10);
	assert(out11);
	assert(out12);
	assert(out13);
	*/
	// writing the crude data and summary outputs for each bus stop
	for (vector<Busstop*>::iterator iter = busstops.begin();iter != busstops.end();iter++)
	{	
		(*iter)->write_output(out1);
		vector<Busline*> stop_lines = (*iter)->get_lines();
		for (vector <Busline*>::iterator lines = stop_lines.begin(); lines != stop_lines.end(); lines++)
		{
			(*iter)->calculate_sum_output_stop_per_line((*lines)->get_id());
			(*iter)->get_output_summary((*lines)->get_id()).write(out2,(*iter)->get_id(),(*lines)->get_id(), (*iter)->get_name());
			// this summary file contains for each stop a record for each line thats use it
		}
	}
	// writing the assignment results in terms of each segment on individual trips
	for (vector<Bustrip*>::iterator iter = bustrips.begin(); iter!= bustrips.end(); iter++)
	{
		(*iter)->write_assign_segments_output(out7);
	}
	// writing the trajectory output for each bus vehicle (by stops)
	for (vector<Bus*>::iterator iter = busvehicles.begin(); iter!= busvehicles.end(); iter++)
	{
		(*iter)->write_output(out4);
	}
	// writing the aggregate summary output for each bus line
	for (vector<Busline*>::iterator iter = buslines.begin();iter!= buslines.end(); iter++)
	{	
		(*iter)->calculate_sum_output_line();
		(*iter)->get_output_summary().write(out3,(*iter)->get_id());
		if (theParameters->demand_format == 3)
		{
			//(*iter)->calc_line_assignment();
			(*iter)->write_assign_output(out9);
		}
		(*iter)->write_ttt_output(out11);
	}
	// writing the decisions of passenger and summary per OD pair
	if (theParameters->demand_format == 3)
	{
		for (vector<ODstops*>::iterator od_iter = odstops_demand.begin(); od_iter < odstops_demand.end(); od_iter++)
		{
			if ((*od_iter)->get_passengers_during_simulation().size() > 0)
			{
				(*od_iter)->write_od_summary(out10);
				(*od_iter)->write_od_summary_without_paths(out12);
			}
			vector<Passenger*> pass_vec = (*od_iter)->get_passengers_during_simulation();
			for (vector<Passenger*>::iterator pass_iter = pass_vec.begin(); pass_iter < pass_vec.end(); pass_iter++) 
			{
				(*pass_iter)->write_selected_path(out8);
			}
		}

		for (vector<Busstop*>::iterator stop_iter = busstops.begin(); stop_iter < busstops.end(); stop_iter++)
		{
			map <Busstop*, ODstops*> stop_as_origin = (*stop_iter)->get_stop_as_origin();
			for (map <Busstop*, ODstops*>::iterator od_iter = stop_as_origin.begin(); od_iter != stop_as_origin.end(); od_iter++)
			{
				/*
				map <Passenger*,list<Pass_boarding_decision>> boarding_decisions = od_iter->second->get_boarding_output();
				for (map<Passenger*,list<Pass_boarding_decision>>::iterator pass_iter1 = boarding_decisions.begin(); pass_iter1 != boarding_decisions.end(); pass_iter1++)
				{
					od_iter->second->write_boarding_output(out5, (*pass_iter1).first);
				}
				*/
				map <Passenger*,list<Pass_alighting_decision>> alighting_decisions = od_iter->second->get_alighting_output();
				for (map<Passenger*,list<Pass_alighting_decision>>::iterator pass_iter2 = alighting_decisions.begin(); pass_iter2 != alighting_decisions.end(); pass_iter2++)
				{
					switch (theParameters->demand_format)
					{
						case 3:
							od_iter->second->write_alighting_output(out6, (*pass_iter2).first);
							break;
						case 4:
							break;
					}
				}
			
				map <Passenger*,list<Pass_connection_decision>> connection_decisions = od_iter->second->get_connection_output();
				for (map<Passenger*,list<Pass_connection_decision>>::iterator pass_iter1 = connection_decisions.begin(); pass_iter1 != connection_decisions.end(); pass_iter1++)
				{
					od_iter->second->write_connection_output(out15, (*pass_iter1).first);
				}

				map <Passenger*,list<Pass_waiting_experience>> waiting_experience = od_iter->second->get_waiting_output();
				for (map<Passenger*,list<Pass_waiting_experience>>::iterator pass_iter1 = waiting_experience.begin(); pass_iter1 != waiting_experience.end(); pass_iter1++)
				{
					od_iter->second->write_waiting_exp_output(out13, (*pass_iter1).first);
				}
				map <Passenger*,list<Pass_onboard_experience>> onboard_experience = od_iter->second->get_onboard_output();
				for (map<Passenger*,list<Pass_onboard_experience>>::iterator pass_iter1 = onboard_experience.begin(); pass_iter1 != onboard_experience.end(); pass_iter1++)
				{
					od_iter->second->write_onboard_exp_output(out14, (*pass_iter1).first);
				}
			}
		}
	}
	/*
	out1.close();
	out2.close();
	out3.close();
	out4.close();
	out5.close();
	out6.close();
	out7.close();
	out8.close();
	out9.close();
	out10.close();
	out11.close();
	out12.close();
	out13.close();
/*
	if (theParameters->demand_format == 4)
	{
		for (vector<ODzone*>::iterator od_iter = odzones.begin(); od_iter < odzones.end(); od_iter++)
		{
			map <Passenger*,list<Pass_boarding_decision_zone>> boarding_decisions = (*od_iter)->get_boarding_output_zone();
			for (map<Passenger*,list<Pass_boarding_decision_zone>>::iterator pass_iter1 = boarding_decisions.begin(); pass_iter1 != boarding_decisions.end(); pass_iter1++)
			{
				(*od_iter)->write_boarding_output_zone(out11, (*pass_iter1).first);
			}	
			map <Passenger*,list<Pass_alighting_decision_zone>> alighting_decisions = (*od_iter)->get_alighting_output_zone();
			for (map<Passenger*,list<Pass_alighting_decision_zone>>::iterator pass_iter1 = alighting_decisions.begin(); pass_iter1 != alighting_decisions.end(); pass_iter1++)
			{
				(*od_iter)->write_alighting_output_zone(out12, (*pass_iter1).first);
			}
		}
*/
	return true;
}


bool Network::set_freeflow_linktimes()
{
	for (map <int,Link*>::iterator iter=linkmap.begin();iter!=linkmap.end();iter++)
		(*iter).second->set_hist_time((*iter).second->get_freeflow_time());
	return true;
}

bool Network::writelinktimes(string name)
{
	ofstream out(name.c_str());
	// assert(out);
	out << "links:\t" << linkmap.size() << endl;
	out << "periods:\t" << nrperiods << endl;
	out << "periodlength:\t" << periodlength << endl;
	for (map <int,Link*>::iterator iter=linkmap.begin();iter!=linkmap.end();iter++)
	{
		(*iter).second->write_time(out);
	}
	out.close();
	return true;
}

bool Network::readlinktimes(string name)
{
	ifstream inputfile(name.c_str());
	assert (inputfile);
	if (readtimes(inputfile))
	{
		inputfile.close();
		return true;
	}
	else
	{
		inputfile.close();
		return false;	
	}
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
		cout << " linkmap.size() " << linkmap.size() << endl;
		cout << " virtuallinks.size() " << virtuallinks.size() << endl;
#endif _DEBUG_NETWORK
		for (map<int,Link*>::iterator iter=linkmap.begin();iter!=linkmap.end();iter++)
		{
			double linktime= (*iter).second->get_freeflow_time();
			LinkTime* ltime=new LinkTime();
			ltime->periodlength=periodlength;
			ltime->nrperiods=nrperiods;
			ltime->id=(*iter).second->get_id();
			for (int i=0;i < nrperiods;i++)
		//		(ltime->times).push_back(linktime);
			(ltime->times) [i] = linktime;
			(*iter).second->set_hist_time(linktime);
			(*iter).second->set_histtimes(ltime);
			linkinfo->times.insert(pair <int,LinkTime*> ((*iter).second->get_id(),ltime )); 
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
		//(ltime->times).push_back(linktime);
		(ltime->times) [i] = linktime;
	}
	map <int,Link*>::iterator l_iter;
	l_iter = linkmap.find(lid);
	//   assert  ( l_iter < links.end() );     // lid exists
	assert (l_iter!=linkmap.end());
	assert ( linktime >= 0.0 );
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readtimes scanner jammed at " << bracket;
		return false;
	}
	(*l_iter).second->set_hist_time(linktime);
	(*l_iter).second->set_histtimes(ltime);
	linkinfo->times.insert(pair<int,LinkTime*> (lid,ltime));
#ifdef _DEBUG_NETWORK
	cout << " read a linktime"<<endl;
#endif //_DEBUG_NETWORK
	return true;
}

bool Network::copy_linktimes_out_in()
{
	bool ok=true;
	map<int,Link*>::iterator l_iter=linkmap.begin();
	for (l_iter;l_iter!=linkmap.end();l_iter++)
	{
		ok = ok  && ((*l_iter).second->copy_linktimes_out_in());
	}
	return ok;
}

bool Network::readincident (istream & in)
{
	// OPTIMIZE LATER
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
	//assert  ( (find_if (links.begin(),links.end(), compare <Link> (lid))) < links.end() );     // lid exists
	assert (linkmap.count(lid)); 
	// assert  ( (find_if (sdfuncs.begin(),sdfuncs.end(), compare <Sdfunc> (sid))) < sdfuncs.end() );     // sid exists
	assert (sdfuncmap.count(sid));
	assert ( (penalty >= 0.0 ) && (start>0.0) && (stop>start) );
	in >> bracket;
	if (bracket != '}')
	{
		cout << "readfile::readincident scanner jammed at " << bracket;
		return false;

	}
	Incident* incident = new Incident(lid, sid, start,stop,info_start,info_stop,eventlist,this, blocked);
#ifndef _NO_GUI
	// Set the icon
	Link* link = linkmap[lid];
	int x = link->get_icon()->get_x ();
	int y = link->get_icon()->get_y();
	IncidentIcon* iptr=new IncidentIcon(x,y);
	incident->set_icon(iptr);
	drawing->add_icon(iptr);

#endif
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
	incident_parameters.push_back(mu);
	incident_parameters.push_back(sd);
	return true;


}


bool Network::readincidentfile(string name)
{
	ifstream inputfile(name.c_str());
	assert (inputfile);
	if (readsdfuncs (inputfile) && readincidents(inputfile) &&readincidentparams(inputfile) && readx1(inputfile))
	{
		for (vector <Incident*>::iterator incident=incidents.begin(); incident != incidents.end(); incident++)
		{
			(*incident)->set_incident_parameters(incident_parameters);
		}
		inputfile.close();
		return true;
	}
	else
	{
		inputfile.close();
		return false;		
	}

}

bool Network::readpathfile(string name)
{
	ifstream inputfile(name.c_str());
	assert (inputfile);
	if (readroutes(inputfile))
	{
		inputfile.close();
		return true;
	}
	else
	{
		inputfile.close();
		return false;
	}
}

bool Network::writepathfile(string name)
{
	ofstream out(name.c_str());
	assert(out);
	out << "routes:" << '\t'<< routemap.size() << endl;
	multimap <odval,Route*>::iterator r_iter = routemap.begin();
	for (r_iter=routemap.begin();r_iter!=routemap.end();r_iter++)
	{
		(*r_iter).second->write(out);
	}
	out.close();
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
	{
		in.close();
		return false;
	}
	int nr,lid;
	in >> nr;
	in >> temp;
	assert (temp=="{");
	map <int, Link*>::iterator iter;
	for (int i=0; i<nr;i++)
	{
		in >> lid;
		iter = linkmap.find(lid);
		assert (iter!=linkmap.end()); // assert it exists
		(*iter).second->set_use_ass_matrix(true);
		no_ass_links++;
	}
	in >> temp;
	assert (temp=="}");
	in.close();
	return true;
}


bool Network::readparameters(string name)
{
	ifstream inputfile(name.c_str());
	assert (inputfile);
	if (theParameters->read_parameters(inputfile))
	{
		if (theParameters->pass_day_to_day_indicator == 1)
			SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
		inputfile.close();
		return true;
	}
	else
	{
		inputfile.close();
		return false;	
	}

}

bool Network::init_shortest_path()
/* Initialises the shortest path graph with the link costs
*/
{
	int lid,in,out,routenr;
	routenr=routemap.size();
	double cost, mu, sd;
	random=new (Random);

	if (randseed != 0)
		random->seed(randseed);
	else
		random->randomize();

#ifdef _DEBUG_SP
	cout << "network::init_shortest path, routes.size  " << routenr << ", linkmap.size " << linkmap.size() << ", nodemap.size " << nodemap.size() << endl;
#endif //_DEBUG_SP
	// CREATE THE GRAPH
#ifndef _USE_VAR_TIMES
	graph=new Graph<double, GraphNoInfo<double> > (nodemap.size() /* 50000*/, linkmap.size(), 9999999.0);
#else
	graph=new Graph<double, LinkTimeInfo > (/*nodemap.size()*/ 50000, linkmap.size()*10, 9999999.0);
#endif
	// ADD THE LINKS AND NODES

	for (map<int,Link*>::iterator iter=linkmap.begin(); iter!=linkmap.end();iter++) // create the link graph for shortest path
	{
		lid=(*iter).second->get_id();
		in=(*iter).second->get_in_node_id();
		out=(*iter).second->get_out_node_id();
		mu=(*iter).second->get_hist_time();
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
	vector <Link*> rlinks;

	if (linkids.empty())
	{
		cout << "Shortest path: get_path : PROBLEM OBTAINING links in path to  dest " << destid << endl;
		return rlinks;
	}	
	for (vector<int>::iterator iter4=linkids.begin();iter4<linkids.end();iter4++) // find all the links
	{
		int lid=(*iter4);
#ifdef _DEBUG_SP					
		cout << lid << " , ";
#endif //_DEBUG_SP
		map <int,Link*>::iterator l_iter;
		l_iter=linkmap.find(lid);
		assert (l_iter!=linkmap.end());
		rlinks.insert(rlinks.end(),(*l_iter).second);
	}
	assert (rlinks.size() > 0);
	return rlinks;
}	


bool Network::shortest_paths_all()	
//calculate the shortest paths for each link emanating from each origin to each destination;
// and saving them if  there is a new path found (i.e. it's not in the routes vector already)
{		
	double entrytime=0.0;    // entry time for time-variant shortest path search
	int nr_reruns=static_cast<int> (runtime/theParameters->update_interval_routes)-1; // except for last period
	// determines the number of reruns of the shortest path alg.
	int routenr=routemap.size();
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
				if ( ((od_rate > theParameters->small_od_rate) && ( (od_rate/theParameters->small_od_rate) < nr_routes)) || (nr_routes < 1) )
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
							odval val = odval(ori->get_id(), (*iter3)->get_id());
							assert (!exists_route(routenr,val)); // Check that no route exists with same routeid, at least for this OD pair
							//assert ( (find_if (routes.begin(),routes.end(), compare <Route> (routenr))) == routes.end() ); // No route with routenr exists
#ifdef _DEBUG_SP
							cout << " making route " << endl;
#endif //_DEBUG_SP
							Route* rptr=new  Route(routenr, ori, (*iter3), rlinks);
							bool exists=true;
							if (rptr)
							{
								//not_exists= ( (find_if (routes.begin(),routes.end(), equalmembers <Route> (*rptr))) == routes.end() ); // find if there's a route with the same links
								exists = exists_same_route(rptr);
								if (!exists)
								{	
									routemap.insert(routemap.end(),pair <odval, Route*> (val,rptr)); // add the newly found route
								}
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



bool Network::find_alternatives_all (int lid, double penalty, Incident* incident)
// Makes sure that each affected link has an alternative
{
	map <int, map <int,Link*>> affected_links_per_dest; // indexed by destination, each destination will have a nr of affected links
	map <int, Origin*> affected_origins; // Simple map of affected origins
	map <int, Link*> affected_links; // simple map of affected links
	map <int, set <int>> links_without_alternative; // all links,dests without a 'ready' alternative. indexed by link_id, dest_id
	// Find all the affected links
	Link* incident_link=linkmap[lid];
	multimap <int, Route*> i_routemap = incident_link->get_routes();// get all routes through incident link
	unsigned int nr_affected_routes = i_routemap.size();
	multimap <int, Route*>::iterator rmiter=i_routemap.begin();
	// get all affected (links,destinations) from each route, and store the origins as well
	for (rmiter;rmiter != i_routemap.end(); rmiter++)
	{
		Route* r = (*rmiter).second;
		int dest =(*rmiter).first;
		vector <Link*> route_affected_links_upstream = r->get_upstream_links(lid);
		vector<Link*>::iterator l_iter;
		for (l_iter=route_affected_links_upstream.begin();l_iter!=route_affected_links_upstream.end();l_iter++)
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
#ifndef _NO_GUI  
			link->set_selected_color(Qt::green);
#endif
			int linkid=link->get_id();
			int nr_alternatives = link->nr_alternative_routes(dest,lid );
			if (nr_alternatives == 0 )
			{
				links_without_alternative [linkid].insert(dest);

#ifndef _NO_GUI  
				link->set_selected_color(Qt::red); // set red colour for Affected links without alternatives
#endif
			}
			else
			{
				// Store the routes at the link (do this already at the counting)

			}

		}
	}	



	// Add the affected links & origins to the Incident
	incident->set_affected_links(affected_links);
	incident->set_affected_origins(affected_origins);
	int found_links = 0;

	// IF affected_links_without_alternative is NOT empty
	if (!(links_without_alternative.empty()))
	{
		// DO a shortest path init, and a shortest path search wih penalty for incident link to create alternatives for each link.
		bool initok=false;
		if (!theParameters->shortest_paths_initialised)
			initok = init_shortest_path();
		if (initok)
		{
			map <int, set <int >>::iterator mi = links_without_alternative.begin();
			for (mi; mi != links_without_alternative.end();mi++)
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
					 found_links++;
				 }
			 }
		 }

		}
	}
	//Now select all links that are affected:
	cout << " nr of routes affected by incident " << i_routemap.size() << endl;
	//	cout << " nr of links without alternatives " << affected_links_without_alternative.size() << endl;
	return true;
}

/*
void Network::delete_spurious_routes()
{
}
*/
void Network::renum_routes ()
{
	multimap <odval, Route*>::iterator route;
	int counter=0;
	for (route=routemap.begin(); route != routemap.end(); route++, counter++)
	{
		(*route).second->set_id(counter);

	}

}

void Network::reset_link_icons() // reset the links to normal color and hide the incident icon
{
#ifndef _NO_GUI 
	map <int,Link*>::iterator link = linkmap.begin();
	for (link; link!=linkmap.end(); link++)
	{
		link->second->set_selected ( false);

		(link->second)->set_selected_color(theParameters->selectedcolor);
	}
	for (vector <Incident*>::iterator inc=incidents.begin();inc != incidents.end(); inc++)
	{
		(*inc)->set_visible(false);
	}
#endif
}


bool Network::readmaster(string name)
{
	string temp;
	ifstream inputfile(name.c_str());
	//assert (inputfile);
	inputfile >> temp;
	if (temp!="#input_files")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	if (temp!="network=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #0
	inputfile >> temp;
	if (temp!="turnings=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #1
	inputfile >> temp;
	if (temp!="signals=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #2
	inputfile >> temp;
	if (temp!="histtimes=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #3
	inputfile >> temp;
	if (temp!="routes=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #4
	inputfile >> temp;
	if (temp!="demand=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); //  #5
	inputfile >> temp;
	if (temp!="incident=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #6
	inputfile >> temp;
	if (temp!="vehicletypes=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;	
	filenames.push_back(workingdir+temp); // #7	
	inputfile >> temp;
	if (temp!="virtuallinks=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;	
	filenames.push_back(workingdir+temp); // #8		
	inputfile >> temp;
	if (temp!="serverrates=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;	
	filenames.push_back(workingdir+temp); // #9			
	inputfile >> temp;
	if (temp!="#output_files")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	if (temp!="linktimes=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #10
	inputfile >> temp;
	if (temp!="output=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #11
	inputfile >> temp;
	if (temp!="summary=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); //  #12
	inputfile >> temp;
	if (temp!="speeds=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #13
	inputfile >> temp;
	if (temp!="inflows=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #14
	inputfile >> temp;
	if (temp!="outflows=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); //  #15
	inputfile >> temp;
	if (temp!="queuelengths=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #16
	inputfile >> temp;
	if (temp!="densities=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp); // #17
	inputfile >> temp;
	if (temp!="#scenario")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	if (temp!="starttime=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> starttime;
	inputfile >> temp;
	if (temp!="stoptime=") // stoptime==runtime
	{
		inputfile.close();
		return false;
	}
	inputfile >> runtime;
	theParameters->running_time = runtime;
	inputfile >> temp;
	if (temp!="calc_paths=")
	{
		calc_paths=false;
		inputfile.close();
		return true;
	}
	else
	{
		inputfile >> calc_paths;
	}
	inputfile >> temp;
	if (temp!="traveltime_alpha=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> time_alpha;
	inputfile >> temp;
	if (temp!="parameters=")
	{
		inputfile.close();
		return false;
	}
	inputfile >> temp;
	filenames.push_back(workingdir+temp);   //  #18 Parameters

#ifdef _VISSIMCOM	
	inputfile >> temp;
	if (temp!="vissimfile=")
	{
		//cout << "No vissimfile specified in masterfile" << endl;
		inputfile.close();
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
		inputfile.close();
		return true;

	}
	if (inputfile >> temp)
		filenames.push_back(workingdir+temp); //  #19	

	inputfile.close();
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
		set_freeflow_linktimes();  //read the historical link times if exist, otherwise set them to freeflow link times.
	}

	// 2005-11-28 put the reading of OD matrix before the paths...
	if (!readdemandfile(filenames[5]))
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
	// NOTE: Obsolete, routemap is always sorted by OD pair
	// sort(routes.begin(), routes.end(), route_less_than);

	// add the routes to the OD pairs AND delete the 'bad routes'
	add_od_routes(); 
//	renum_routes (); // renum the routes
	writepathfile(filenames[4]); // write back the routes.
	// end temporary
	if (calc_paths)
	{
		//delete_spurious_routes();
		writepathfile(filenames[4]); // write back the routes.
	}

	this->readsignalcontrols(filenames[2]);
	// NEW 2007_03_08
#ifdef _BUSES
	// read the transit system input
	this->readtransitroutes (workingdir + "transit_routes.dat"); //FIX IN THE MAIN READ & WRITE
	this->readtransitnetwork (workingdir + "transit_network.dat"); //FIX IN THE MAIN READ & WRITE
	this->readtransitfleet (workingdir + "transit_fleet.dat");
	this->readtransitdemand (workingdir + "transit_demand.dat");
	if (theParameters->choice_set_indicator == 1)
	{
		this->read_transit_path_sets (workingdir +"path_set_generation.dat");
	}
	if (theParameters->pass_day_to_day_indicator ==1)
	{
		this->read_transitday2day (workingdir +"transit_day2day.dat");
	}
	if (theParameters->in_vehicle_d2d_indicator ==1)
	{
		this->read_IVTT_day2day (workingdir +"transit_day2day_onboard.dat");
	}
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
		set_freeflow_linktimes();  //read the historical link times if exist, otherwise set them to freeflow link times.
	}
	// New 2005-11-28 put the reading of OD matrix before the paths...
	if (!readdemandfile(filenames[5]))
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
	//sort(routes.begin(), routes.end(), route_less_than);
	// add the routes to the OD pairs
	add_od_routes();
//	renum_routes ();
	// temporary
	writepathfile(filenames[4]); // write back the routes.
	// end temporary
	if (calc_paths)
	{
		//delete_spurious_routes();
		writepathfile(filenames[4]); // write back the routes.
	}

	readsignalcontrols(filenames[2]);
#ifdef _BUSES
	// read the transit system input
	this->readtransitroutes (workingdir + "transit_routes.dat"); //FIX IN THE MAIN READ & WRITE
	this->readtransitnetwork (workingdir + "transit_network.dat"); //FIX IN THE MAIN READ & WRITE
	this->readtransitfleet (workingdir + "transit_fleet.dat");
	this->readtransitdemand (workingdir + "transit_demand.dat");
	if (theParameters->choice_set_indicator == 1)
	{
		this->read_transit_path_sets (workingdir +"path_set_generation.dat");
	}
	if (theParameters->pass_day_to_day_indicator ==1)
	{
		this->read_transitday2day (workingdir +"transit_day2day.dat");
	}
	if (theParameters->in_vehicle_d2d_indicator ==1)
	{
		this->read_IVTT_day2day (workingdir +"transit_day2day_onboard.dat");
	}
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


bool Network::writeall(unsigned int repl)
{
	replication=repl;
	string rep="";
	string cleantimes;
	end_of_simulation(runtime);
	string linktimesfile = filenames[10];
	cleantimes=linktimesfile+".clean";
	string summaryfile=filenames[12];
	string vehicleoutputfile=filenames[11];
	string allmoesfile="allmoes.dat";
	string assignmentmatfile="assign.dat";
	string vqueuesfile="v_queues.dat";
	if (replication >0)
	{
		stringstream repstr;
		repstr << "." << replication;
		rep=repstr.str();
		cleantimes=linktimesfile +".clean" +rep;
		linktimesfile += rep ;
		summaryfile += rep ;
		vehicleoutputfile += rep ;
		allmoesfile += rep ;
		assignmentmatfile += rep;
		vqueuesfile += rep;
	}
	writelinktimes(linktimesfile);
	// NEW: Write also the non-smoothed times
	
	time_alpha=1.0;
	writelinktimes(cleantimes);

	writesummary(summaryfile); // write the summary first because	
	writeoutput(vehicleoutputfile);  // here the detailed output is written and then deleted from memory
	writemoes(rep);
	writeallmoes(allmoesfile);
	//writeheadways("timestamps.dat"); // commented out, since no-one uses them 
	writeassmatrices(assignmentmatfile);
	write_v_queues(vqueuesfile);
	this->write_busstop_output(workingdir + "o_buslog_out.dat", workingdir + "o_busstop_sum.dat", workingdir + "o_busline_sum.dat", workingdir + "o_bus_trajectory.dat", workingdir + "o_passenger_boarding.dat", workingdir + "o_passenger_alighting.dat", workingdir + "o_segments_trip_loads.dat", workingdir + "o_selected_paths.dat", workingdir + "o_segments_line_loads.dat", workingdir + "o_od_stops_summary.dat", workingdir + "o_trip_total_travel_time.dat", workingdir + "o_od_stop_summary_without_paths.dat", workingdir + "o_passenger_waiting_experience.dat", workingdir + "o_passenger_onboard_experience.dat", workingdir + "o_passenger_connection.dat");
	return true;
}

bool Network::writeallmoes(string name)
{
	//cout << "going to write to this file: " << name << endl;
	ofstream out(name.c_str());
	//assert(out);

	//out << "CONVERGENCE" << endl;

	//out << "SumDiff_InputOutputLinkTimes : " << calc_diff_input_output_linktimes () << endl << endl;
	//out << "SumSquare_InputOutputLinkTimes : " << calc_sumsq_input_output_linktimes () << endl << endl;
	//out << "Root Mean Square Linktimes : " <<this->calc_rms_input_output_linktimes() << endl;
	//out << "Root Mean Square Normalized Linktimes : " <<this->calc_rmsn_input_output_linktimes() << endl;
	out << "MOES" << endl;
	/****** TEMPORARY TO CUT OUT THE ALL MOES THAT ARE NOT USED NOW
	int maxindex=0;

	for (map<int, Link*>::iterator iter=linkmap.begin();iter!=linkmap.end();iter++)
	{
	out << (*iter).second->get_id() << "\t\t\t\t\t";
	maxindex=_MAX (maxindex, (*iter).second-> max_moe_size());
	}
	out << endl;
	for (map<int,Link*>::iterator iter1=linkmap.begin();iter1!=linkmap.end();iter1++)
	{
	out << "speed"<< "\t" << "density" << "\t" << "inflow" <<"\t" << "outflow" <<"\t" << "queue" << "\t";
	}
	out << endl;
	for (int index=0;index <maxindex; index++)
	{
	for (map<int,Link*>::iterator iter=linkmap.begin();iter!=linkmap.end();iter++)
	{
	(*iter).second->write_speed(out,index);
	(*iter).second->write_density(out,index);
	(*iter).second->write_inflow(out,index);
	(*iter).second->write_outflow(out,index);
	(*iter).second->write_queue(out,index);
	}
	out << endl;
	}
	*/
	out.close();
	return true;
}

double Network::calc_diff_input_output_linktimes ()
{
	double total =0.0;
	for (map <int, Link*>::iterator iter1=linkmap.begin();iter1!=linkmap.end();iter1++)
	{	
		if ((*iter1).second->get_nr_passed() > 0 )
			total+=(*iter1).second->calc_diff_input_output_linktimes();
	}
	return total;
}

double Network::calc_sumsq_input_output_linktimes ()
{
	double total =0.0;
	for (map <int, Link*>::iterator iter1=linkmap.begin();iter1!=linkmap.end();iter1++)
	{
		if ((*iter1).second->get_nr_passed() > 0 )
			total+=(*iter1).second->calc_sumsq_input_output_linktimes();
	}
	return total;
}

double Network::calc_mean_input_linktimes()
{
	return this->linkinfo->mean();

}

double Network::calc_rms_input_output_linktimes()
{
	double n = linkmap.size() * nrperiods;
	double ssq = calc_sumsq_input_output_linktimes();
	double result= sqrt(ssq/n);
	return result;
}

double Network::calc_rmsn_input_output_linktimes()
{
	return (calc_rms_input_output_linktimes() / calc_mean_input_linktimes());
}

double Network::calc_mean_input_odtimes()
{
	double n= odpairs.size();
	double sum = 0.0;
	for (vector<ODpair*>::iterator od_iter=odpairs.begin(); od_iter != odpairs.end(); od_iter++)
	{
		sum+=(*od_iter)->get_mean_old_odtimes();		
	}
	return sum / n;
}

double Network::calc_rms_input_output_odtimes()
{
	double n = odpairs.size();
	double diff= 0.0;
	double ssq = 0.0;
	int nr_passed = 0;
	for (vector<ODpair*>::iterator od_iter=odpairs.begin(); od_iter != odpairs.end(); od_iter++)
	{
		diff=(*od_iter)->get_diff_odtimes();
		ssq += diff*diff;
	}

	return sqrt(ssq/n);
}

double Network::calc_rmsn_input_output_odtimes()
{
	return (calc_rms_input_output_odtimes() / calc_mean_input_odtimes());
}

bool Network::writemoes(string ending)
{
	string name=filenames[13] + ending; // speeds
	ofstream out(name.c_str());
	//assert(out);
	
	for (map<int,Link*>::iterator iter=linkmap.begin();iter!=linkmap.end();iter++)
	{

		(*iter).second->write_speeds(out,nrperiods);
	}
	out.close();
	name=filenames[14] + ending; // inflows
	out.open(name.c_str());
	//assert(out);

	for (map<int,Link*>::iterator iter1=linkmap.begin();iter1!=linkmap.end();iter1++)
	{
		(*iter1).second->write_inflows(out,nrperiods);
	}
	out.close();
	name=filenames[15] + ending; // outflows
	out.open(name.c_str());
	//assert(out);

	for (map<int,Link*>::iterator iter2=linkmap.begin();iter2!=linkmap.end();iter2++)
	{
		(*iter2).second->write_outflows(out,nrperiods);
	}
	out.close();
	name=filenames[16] + ending; // queues
	out.open(name.c_str());
	//assert(out);

	for (map<int,Link*>::iterator iter3=linkmap.begin();iter3!=linkmap.end();iter3++)
	{
		(*iter3).second->write_queues(out,nrperiods);
	}
	out.close();
	name=filenames[17] + ending; // densities
	out.open(name.c_str());
	//assert(out);

	for (map<int,Link*>::iterator iter4=linkmap.begin();iter4!=linkmap.end();iter4++)
	{
		(*iter4).second->write_densities(out,nrperiods);
	}
	out.close();
	return true;
}


bool Network::write_v_queues(string name)
{
	ofstream out(name.c_str());
	//assert(out);
	for (map <int, Origin*>::iterator iter = originmap.begin();iter != originmap.end(); iter++)
	{
		(*iter).second->write_v_queues(out);
	}
	out.close();
	return true;
}


bool Network::writeassmatrices(string name)
{
	ofstream out(name.c_str());
	//assert(out);
	out << "no_obs_links: " << no_ass_links << endl;
	int nr_periods = static_cast<int>(runtime / theParameters->ass_link_period);
	out << "no_link_pers: " << nr_periods << endl;
	for (int i=0; i < nr_periods; i++)
	{
		out << "link_period: " << i << endl;
		for (map <int,Link*>::iterator iter1=linkmap.begin();iter1!=linkmap.end(); iter1++)
		{
			(*iter1).second->write_ass_matrix(out,i);
		}
		//out << endl;
	}
	out.close();
	return true;

}


bool Network::init()

{
	// initialise the turining events
	double initvalue =0.1;
	for(map<int,Turning*>::iterator iter=turningmap.begin(); iter!=turningmap.end(); iter++)
	{
		(*iter).second->init(eventlist,initvalue);
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
	switch (theParameters->demand_format)
	{
		case 3:
			for (vector<ODstops*>::iterator iter_odstops = odstops_demand.begin(); iter_odstops < odstops_demand.end(); iter_odstops++ )
			{
				if ((*iter_odstops)->get_arrivalrate() != 0.0 )
				{
					if ((*iter_odstops)->check_path_set() == true)
					{
						(*iter_odstops)->execute(eventlist,initvalue); // adds an event for the generation time of the first passenger per OD in terms of stops
					}
					initvalue+=0.00001;
				}
			}
			break;
		case 4:
			for (vector<ODzone*>::iterator iter_odzones = odzones.begin(); iter_odzones < odzones.end(); iter_odzones++ )
			{
				(*iter_odzones)->execute(eventlist,initvalue); // adds an event for the generation time of the first passenger per origin zone
				initvalue+=0.00001;
			}
			break;
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

#ifdef _DEBUG_NETWORK	
	cout << "odpairs initialised" << endl;
	cout << "number of destinations " <<destinations.size() <<endl;
#endif //_DEBUG_NETWORK	
	// initialise the destination events
	//	register_links();
	for (map <int, Destination*>::iterator iter2=destinationmap.begin(); iter2!=destinationmap.end();iter2++)
	{
		(*iter2).second->execute(eventlist,initvalue);
		initvalue += 0.00001;
	}

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
	//eventhandle->startup();
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
			//	eventhandle->startup();     //update animation
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
	double timer = 1200;
#ifndef _NO_GUI
	double tc; // current time
#endif //_NO_GUI  
	double next_an_update=t0+timestep;   // when to exit
	while ((time>-1.0) && (time<runtime))       // the big loop
	{
		time=eventlist->next();
		//cout << time << "\t";
#ifndef _NO_GUI
		
		tc=timestamp();
		
		if (tc > next_an_update)  // the time has come for the next animation update
		{
			drawing->draw(pm,wm);
			return time;
		} 
		
#endif //_NO_GUI  

		if (time >= timer) //Jens 2014
		{
			cout << "Time: " << timer << endl;
			timer += 1200;
		}
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
QMatrix Network::netgraphview_init()
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

void Network::set_incident(int lid, int sid, bool blocked, double blocked_until)
{
	//cout << "incident start on link " << lid << endl;
	//Link* lptr=(*(find_if (links.begin(),links.end(), compare <Link> (lid) ))) ;
	Link* lptr = linkmap [lid];
	//Sdfunc* sdptr=(*(find_if (sdfuncs.begin(),sdfuncs.end(), compare <Sdfunc> (sid) ))) ;
	Sdfunc* sdptr = sdfuncmap [sid];
	lptr->set_incident (sdptr, blocked, blocked_until);
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
	for (map <int,Link*>::iterator iter=linkmap.begin();iter!=linkmap.end();iter++)
	{
		(*iter).second->broadcast_incident_start(lid,incident_parameters);  	
	}
	// for all origins : start the automatic switiching algorithm
	for (map <int,Origin*>::iterator iter1=originmap.begin();iter1!=originmap.end();iter1++)
	{
		(*iter1).second->broadcast_incident_start(lid,incident_parameters);  	
	}

}


void Network::broadcast_incident_stop(int lid)
{	//cout << "BROADCAST END of incident on link  "<< lid << endl;
	// for all origins: stop the automatic switching stuff
	for (map <int,Origin*>::iterator iter=originmap.begin();iter!=originmap.end();iter++)
	{
		(*iter).second->broadcast_incident_stop(lid);  	
	}
}

void Network::removeRoute(Route* theroute)
{	
	odval val = theroute->get_oid_did();

	multimap<odval,Route*>::iterator it, lower, upper;
	lower = routemap.lower_bound(val);
	upper = routemap.upper_bound(val);
	for(it=lower; it!=upper; it++)
	{
		if((*it).second==theroute)
		{
			routemap.erase(it);
			return;
		}
	}
}

void Network::set_output_moe_thickness ( unsigned int val ) // sets the output moe for the links
{
	double maxval = 0.0, minval=999999.0;
	pair <double,double> minmax;
	map <int,Link*>::iterator iter = linkmap.begin();
	for (iter;iter != linkmap.end(); iter++)
	{
		minmax = (*iter).second->set_output_moe_thickness(val);
		minval = min (minval, minmax.first);
		maxval = max(maxval, minmax.second);
		
	}
	theParameters->min_thickness_value=minval;
	theParameters->max_thickness_value=maxval;
}

void Network::set_output_moe_colour ( unsigned int val ) // sets the output moe for the links
{
	double maxval = 0.0, minval=999999.0;
	pair <double,double> minmax;
	map <int,Link*>::iterator iter = linkmap.begin();
	for (iter;iter != linkmap.end(); iter++)
	{
		minmax = (*iter).second->set_output_moe_colour(val);
		minval = min (minval, minmax.first);
		maxval = max(maxval, minmax.second);
		
	}
	theParameters->min_colour_value=minval;
	theParameters->max_colour_value=maxval;
	
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
			network->set_incident(lid, sid, blocked, stop); // replace sdfunc with incident sd function
#ifndef _NO_GUI
			icon->set_visible(true);
#endif
			eventlist->add_event(stop,this);
		}
		// #2: End the incident on the link
		else
		{
			network->unset_incident(lid);
#ifndef _NO_GUI
			icon->set_visible(false);
#endif
		}
	}

	else
	{
		// #1: Start the incident on the link
		if (time < info_start)
		{
			network->set_incident(lid, sid, blocked, stop); // replace sdfunc with incident sd function
#ifndef _NO_GUI
			icon->set_visible(true);
#endif
			eventlist->add_event(info_start,this);
		}
		// #2: Start the broadcast of Information
		else if (time < stop)
		{	
			broadcast_incident_start(lid);
			// TO DO: Change the broadcast to only include only the affected links and origins
			eventlist->add_event(stop,this);
		}
		// #3: End the incident on the link
		else if (time < info_stop)
		{
			network->unset_incident(lid);
#ifndef _NO_GUI
			icon->set_visible(false);
#endif
			eventlist->add_event(info_stop,this);
		}
		// #4: End the broadcast of Information
		else
		{
			broadcast_incident_stop(lid);
		}
	}
	return true;
}

void Incident::broadcast_incident_start(int lid)
{
	// for all links
	map <int, Link*>::iterator linkiter = affected_links.begin();
	for (linkiter; linkiter != affected_links.end(); linkiter++)
	{
		(*linkiter).second->broadcast_incident_start (lid,incident_parameters);
	}

	// for all origins
	map <int,Origin*>::iterator oriter= affected_origins.begin();
	for (oriter;oriter != affected_origins.end(); oriter++)
	{
		(*oriter).second->broadcast_incident_start (lid,incident_parameters);
	}
}

void Incident::broadcast_incident_stop(int lid)
{

	map <int,Origin*>::iterator oriter= affected_origins.begin();
	for (oriter;oriter != affected_origins.end(); oriter++)
	{
		(*oriter).second->broadcast_incident_stop (lid);
	}
}


// ODMATRIX CLASSES

ODMatrix::ODMatrix (){}

void ODMatrix::reset(Eventlist* eventlist, vector <ODpair*> * odpairs)
{
	vector < pair <double,ODSlice*> >::iterator s_iter=slices.begin();
	for (s_iter;s_iter != slices.end(); s_iter++)
	{
		//create and book the MatrixAction
		double loadtime = (*s_iter).first;
		ODSlice* odslice = (*s_iter).second;
		MatrixAction* mptr=new MatrixAction(eventlist, loadtime, odslice, odpairs);
		assert (mptr != NULL);

	}
}


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