
#include "gettime.h"

#include "network.h"
#include <assert.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <set>
#include <math.h>
//#include <strstream> // OLD include for gcc2

#include "od.h"

//using namespace std;

// initialise the global variables and objects

long int randseed=0;
int vid=0;
int pid=0;
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

	//TO DO

	
	// incidents

	// buslines, busstops etc etc etc!

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
	int lid, innode, outnode, length,nrlanes,sdid;
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
  double mu, sd, delay, delay_std;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readservers scanner jammed at " << bracket;
		return false;
	}
	in  >> sid >> stype >> mu >> sd >> delay;
	assert (!servermap.count(sid));
	assert ( (stype > -1) && (stype<4) && (mu>=0.0) && (sd>=0.0) && (delay>=0.0)); // to be updated when more server types are added
	// check id, vmax, vmin, romax;
	// type 0= dummy server: Const server
	// type 1=standard N(mu,sd) sever
	// type 2=deterministic (mu) server
    	// type 3=stochastic delay server LN(delay, std_delay)
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
		in >> delay_std;
		assert (delay_std >=0.0);
		sptr = new StochasticDelayServer (sid,stype,mu,sd,delay,delay_std * theParameters->sd_server_scale);
		//sptr->set_delay_std (delay_std);
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

bool Network::readbuslines(string name) // reads the busstops, buslines, trips, passengers rates, bustypes and busvehicles
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
	if (keyword!="busstops:")
	{
		cout << " readbuslines: no << stops: >> keyword " << endl;
		in.close();
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
			in.close();
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
		in.close();
		return false;
	}
	in >> nr;
	int limit = i + nr;
	for (i; i<limit;i++)
	{
		if (!readbusline(in))
		{
			cout << " readbuslines: readbusline returned false for line nr " << (i+1) << endl;
			in.close();
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
		in.close();
		return false;
	}
	in >> nr;
	limit = i + nr;
	for (i; i<limit;i++)
	{
		if (!readbustrip(in))
		{
			cout << " readbuslines: readbustrip returned false for line nr " << (i+1) << endl;
			in.close();
			return false;
		} 
		// set busline to trip

	}
	// Forth read the passengers rates
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
		else if (format == 2)
		{
			if (!read_passenger_rates_format2(in))
			{
				cout << " readbuslines: read_passenger_rates returned false for line nr " << (i+1) << endl;
   				return false;
			} 
		}
		else if (format == 3)
		{
			if (!read_passenger_rates_format3(in))
			{
				cout << " readbuslines: read_passenger_rates returned false for line nr " << (i+1) << endl;
   				return false;
			} 
		}
		else
		{
			cout << " readbuslines: read_passenger_rates returned false for wrong format coding " << (i+1) << endl;
   			return false;
		}
	}
	if (format == 3)
	{
		find_all_paths ();
	}
	// Fifth read bus types
in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="bustypes:")
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

	// Sixth read the bus vehicles
	in >> keyword;
#ifdef _DEBUG_NETWORK
	cout << keyword << endl;
#endif //_DEBUG_NETWORK
	if (keyword!="busvehicles:")
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

bool Network::readbusstop (istream& in) // reads a busstop
{

	//{ stop_id	link_id	length	has_bay	dwelltime }
//	char bracket;
//	int stop_id, link_id;
//{ stop_id	link_id	length	has_bay	dwelltime }
  char bracket;
  int stop_id, link_id;
  double position, length, dwelltime;
	bool has_bay ;
	bool ok= true;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusstop scanner jammed at " << bracket;
		return false;
	}
  in >> stop_id >> link_id >> position >> length >> has_bay >> dwelltime;
  Busstop* st= new Busstop (stop_id, link_id, position, length, has_bay, dwelltime);
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
	//{	id	name	origin	destination	route_id	vehtype mean_total_driving_time(ex. dwell time) average_headway ratio_headway_holding holding_strategy 	
	//	nr_stops	{	stop_id1  stop_id2  stop_id3  ...	stop_idn	} nr_time_points { stop_id1 ... stop_idn }
	//}
  char bracket;
  int busline_id, ori_id, dest_id, route_id, vehtype, average_headway, holding_strategy, nr_stops, stop_id, nr_tp, tp_id;
  float ratio_headway_holding;
	string name;
  vector <Busstop*> stops, line_timepoint;
	Busstop* stop;
  Busstop* tp;
	bool ok= true;
	in >> bracket;
	if (bracket != '{')
	{
		cout << "readfile::readsbusline scanner jammed at " << bracket;
		return false;
	}
	in >> busline_id >> name >> ori_id >> dest_id >> route_id >> vehtype >> average_headway >> ratio_headway_holding >> holding_strategy >> nr_stops;
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
	Busline* bl= new Busline (busline_id,name,br,stops,vt,odptr,average_headway,ratio_headway_holding,holding_strategy);

	for (vector<Busstop*>::iterator stop_iter = bl->stops.begin(); stop_iter < bl->stops.end(); stop_iter++)
	{
		(*stop_iter)->add_lines(bl);
		(*stop_iter)->add_line_nr_waiting(bl, 0);
		(*stop_iter)->add_line_nr_boarding(bl, 0);
		(*stop_iter)->add_line_nr_alighting(bl, 0);
	}

// reading time point stops
  in >> nr_tp;
  in >> bracket;
  if (bracket != '{')
  {
	cout << "readfile::readsbusline scanner jammed at " << bracket;
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
	cout << "readfile::readsbusline scanner jammed at " << bracket;
	return false;
  }
  bl->add_timepoints(line_timepoint);
  in >> bracket;
  if (bracket != '}')
  {
    cout << "readfile::readbusline scanner jammed at " << bracket;
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
    trip->set_line(bl);
	
  
  // add to bustrips vector
  bustrips.push_back (trip);

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

bool Network::read_passenger_rates_format2 (istream& in) // reads the passenger rates in the format of arrival rate per line, origin stop and destination stop combination
{
  char bracket;
  int origin_stop_id, destination_stop_id, busline_id, nr_stops_info;
  double arrival_rate;
  bool ok= true;

  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbusstop scanner jammed at " << bracket;
  	return false;
  }

  in >> busline_id; 
	Busline* bl=(*(find_if(buslines.begin(), buslines.end(), compare <Busline> (busline_id) )));
	for (int i=0; i < bl->stops.size()-1; i++)
	{
		stop_rate stop_rate_d;
		stops_rate stops_rate_d;
		multi_rate multi_rate_d;
		in >> bracket;
		if (bracket != '{')
		{
			cout << "readfile::readsbustrip scanner jammed at " << bracket;
			return false;
		}
		in >> origin_stop_id >> nr_stops_info;
		Busstop* bs_o=(*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (origin_stop_id) ))); 
		for (int j=0; j< nr_stops_info; j++)
		{
			in >> bracket;
			if (bracket != '{')
			{
				cout << "readfile::readsbustrip scanner jammed at " << bracket;
				return false;
			}
			in >> destination_stop_id >> arrival_rate;
			Busstop* bs_d =(*(find_if(busstops.begin(), busstops.end(), compare <Busstop> (destination_stop_id) )));
			stop_rate_d.first = bs_d;
			stop_rate_d.second = arrival_rate;
			stops_rate_d.insert(stop_rate_d);
			multi_rate_d.first = bl;
			multi_rate_d.second = stops_rate_d;
			
			in >> bracket;
			if (bracket != '}')
			{
				cout << "readfile::readsbustrip scanner jammed at " << bracket;
				return false;
			}
		}
		in >> bracket;
		bs_o->multi_arrival_rates.insert(multi_rate_d);
		if (bracket != '}')
		{
			cout << "readfile::readsbustrip scanner jammed at " << bracket;
			return false;
		}
	}
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
  odstops.push_back(od_stop);
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
					// in any case - add this specific direct path
						vector<Busstop*> o_stop;
						o_stop.push_back(bs_origin);
						stops_sequence.push_back(o_stop);
						vector<Busstop*> d_stop;
						d_stop.push_back(bs_destination);
						stops_sequence.push_back(d_stop);
						last_line.push_back(*bl_o);
						lines_sequence.push_back(last_line);
						Pass_path* direct_path = new Pass_path(lines_sequence, stops_sequence);
						// check if this path was already generated
						map <Busstop*, ODstops*> od_as_origin = bs_origin->get_stop_as_origin();
						vector <Pass_path*> current_path_set = od_as_origin[bs_destination]->get_path_set();
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
							map <Busstop*, ODstops*> od_as_origin = bs_origin->get_stop_as_origin();
							od_as_origin[bs_destination]->add_paths(direct_path);
							direct_lines[od_as_origin[bs_destination]].push_back(*bl_o);
							if (direct_path->get_number_of_transfers() < od_as_origin[bs_destination]->get_min_transfers())
							// update the no. of min transfers required if decreased
							{
								od_as_origin[bs_destination]->set_min_transfers(direct_path->get_number_of_transfers());
							}
							return true;
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
#ifdef _DEBUG_NETWORK
#endif //_DEBUG_NETWORK
return false;
}

void Network::generate_indirect_paths()
{
	bool original_path = true;
	vector<vector<Busline*>> lines_sequence;
	vector<vector<Busstop*>> stops_sequence;
	vector<Pass_path*> paths;
	// compose the list of intermediate stops
	for (vector<Busstop*>::iterator im_iter = collect_im_stops.begin(); im_iter < collect_im_stops.end(); im_iter++)
	{
		vector <Busstop*> im_stops;
		im_stops.push_back((*im_iter));
		stops_sequence.push_back(im_stops);
	}
	int nr_path_combinations = 1;
	// count how many combinations of paths you can make (multiply number of direct lines on segments)
	for (vector<Busstop*>::iterator im_iter = collect_im_stops.begin(); im_iter < (collect_im_stops.end()-1); im_iter++)
	{
		map <Busstop*, ODstops*> od_as_origin = (*im_iter)->get_stop_as_origin();
		nr_path_combinations = nr_path_combinations * direct_lines[od_as_origin[*(im_iter+1)]].size();
	}
	// compose the list of direct lines between each pair of intermediate stops
	for (vector<Busstop*>::iterator im_iter = collect_im_stops.begin(); im_iter < collect_im_stops.end()-1; im_iter++)
	{
		map <Busstop*, ODstops*> im_as_origin = (*im_iter)->get_stop_as_origin();
		vector <Busline*> p_lines;
		vector<Busline*> d_lines = direct_lines[im_as_origin[(*(im_iter+1))]];
		p_lines.push_back(d_lines.front()); // assuming a unique direct line for each leg
		lines_sequence.push_back(p_lines);
	}
	Pass_path* indirect_path = new Pass_path(lines_sequence, stops_sequence);
	paths.push_back(indirect_path);
	int stop_leg = 0;
	vector<vector<Busline*>>::iterator lines_sequence_iter = lines_sequence.begin();
	for (vector<Busstop*>::iterator im_iter = collect_im_stops.begin(); im_iter < collect_im_stops.end()-1; im_iter++)
	{
		stop_leg++;
		map <Busstop*, ODstops*> im_as_origin = (*im_iter)->get_stop_as_origin();
		vector <Busline*>::iterator d_lines_begin = direct_lines[im_as_origin[(*(im_iter+1))]].begin();
		for (vector<Busline*>::iterator d_lines = d_lines_begin+1 ; d_lines < direct_lines[im_as_origin[(*(im_iter+1))]].end(); d_lines++)
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
				Pass_path* indirect_path = new Pass_path(permute_path_lines , stops_sequence);	
			}
		}
	}
	for (vector<Pass_path*>::iterator paths_iter = paths.begin(); paths_iter < paths.end(); paths_iter++)
	{
		// check if this path was already generated		
		map <Busstop*, ODstops*> od_as_origin = collect_im_stops.front()->get_stop_as_origin();
		vector <Pass_path*> current_path_set = od_as_origin[collect_im_stops.back()]->get_path_set();
		for (vector <Pass_path*>::iterator iter = current_path_set.begin(); iter != current_path_set.end(); iter++)
		{
			if (compare_same_lines_paths((*paths_iter),(*iter)) == true)
			{
				original_path = false;
				break;
			}
		}
		// add the indirect path if it is an original one and it fulfills the constraints
		if (original_path == true && check_constraints_paths ((*paths_iter)) == true)
		{
			map <Busstop*, ODstops*> od_as_origin = collect_im_stops.front()->get_stop_as_origin(); // refer the original origin
			od_as_origin[collect_im_stops.back()]->add_paths((*paths_iter));
			if ((*paths_iter)->get_number_of_transfers() < od_as_origin[collect_im_stops.back()]->get_min_transfers())
			// update the no. of min transfers required if decreased
			{
				od_as_origin[collect_im_stops.back()]->set_min_transfers((*paths_iter)->get_number_of_transfers());
			}
		}
	}
}
void Network::find_all_paths () 
// goes over all OD stop pairs to generate their path choice set
{
	// first - generate all direct paths
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{
		map <Busstop*, ODstops*> od_as_origin = (*basic_origin)->get_stop_as_origin();
		for (map<Busstop*, ODstops*>::iterator iter = od_as_origin.begin(); iter != od_as_origin.end(); iter++)
		{
			Busstop* basic_destination = (*iter).first;
			find_direct_paths ((*basic_origin), basic_destination);
		}
	}
	// second - generate indirect paths
	for (vector <Busstop*>::iterator basic_origin = busstops.begin(); basic_origin < busstops.end(); basic_origin++)
	{
		map <Busstop*, ODstops*> od_as_origin = (*basic_origin)->get_stop_as_origin();
		for (map<Busstop*, ODstops*>::iterator iter = od_as_origin.begin(); iter != od_as_origin.end(); iter++)
		{
			collect_im_stops.push_back(*basic_origin);
			Busstop* basic_destination = (*iter).first;
			find_recursive_connection ((*basic_origin), basic_destination);
			collect_im_stops.clear();
		}
	}
}

void Network:: find_recursive_connection (Busstop* origin, Busstop* destination)
// search recursively for a path (forward - from origin to destination)
{
	map <Busstop*, ODstops*> od_as_origin = origin->get_stop_as_origin();
	map <Busstop*, ODstops*> original_od_as_origin = collect_im_stops.front()->get_stop_as_origin();
	if (od_as_origin.size() > 0)
	{
		if ((collect_im_stops.size() - 1) <= original_od_as_origin[destination]->get_min_transfers() + theParameters->max_nr_extra_transfers)
		{
			for (map<Busstop*, ODstops*>::iterator iter = od_as_origin.begin(); iter != od_as_origin.end(); iter++)
			{
				Busstop* intermediate_destination = (*iter).first;
				map <Busstop*, ODstops*> last_stop_as_origin = collect_im_stops.back()->get_stop_as_origin();
				if (direct_lines[last_stop_as_origin[intermediate_destination]].size() != 0)
				{
					collect_im_stops.push_back(intermediate_destination);
						if (intermediate_destination->get_id() != destination->get_id())
						{
							map <Busstop*, ODstops*> im_as_origin = intermediate_destination->get_stop_as_origin();
							if (direct_lines[im_as_origin[destination]].size() != 0)
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

void Network ::merge_paths (Busstop* stop) // aimed to merge paths with same lines for all legs (only different transfer stops)
// TO DO: merge for different lines with same route between trnasfer stops?
{
	for (map <Busstop*, ODstops*>::iterator odpairs = stop->get_stop_as_origin().begin(); odpairs != stop->get_stop_as_origin().end(); odpairs++)
	{
		for (vector <Pass_path*>::iterator path1 = odpairs->second->get_path_set().begin(); path1 < odpairs->second->get_path_set().end(); path1++)
		{
				for (vector <Pass_path*>::iterator path2 = odpairs->second->get_path_set().begin(); path2 < odpairs->second->get_path_set().end(); path2++)
				{
					if (compare_same_lines_paths ((*path1), (*path2)) == true) // both have exactly the same lines for all legs 
					{
						vector <vector <Busstop*>>::iterator stops1 = (*path1)->get_alt_transfer__stops().begin();
						vector <vector <Busstop*>>::iterator stops2 = (*path2)->get_alt_transfer__stops().begin();
						for (vector<Busstop*>::iterator stops1_leg = (*stops1).begin(); stops1_leg < (*stops1).end(); stops1_leg++)
						{
							for (vector<Busstop*>::iterator stops2_leg = (*stops2).begin(); stops2_leg < (*stops2).end(); stops2_leg++)
							{
								if ((*stops1_leg)->get_id() != (*stops2_leg)->get_id())
								{
									(*stops1).push_back((*stops2_leg));
								}
							}
						}
						odpairs->second->get_path_set().erase(path2);
					}
				}
		}
	}
}

bool Network::compare_same_lines_paths (Pass_path* path1, Pass_path* path2)
// checks if two paths are identical in terms of lines
{
	if (path1->get_number_of_transfers() == path2->get_number_of_transfers())
	{
		vector <bool> is_shared;
		vector <vector <Busline*>> lines1 = path1->get_alt_lines();	
		vector <vector <Busline*>> lines2 = path2->get_alt_lines();
		for (vector <vector <Busline*>>::iterator lines1_iter = lines1.begin(); lines1_iter < lines1.end(); lines1_iter++)
		{
			for (vector <vector <Busline*>>::iterator lines2_iter = lines2.begin(); lines2_iter < lines2.end(); lines2_iter++)
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
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}
bool Network::check_constraints_paths (Pass_path* path) // checks if the path meets all the constraints
{
	if (path->get_alt_transfer__stops().size() == 2)
	{
		return true;
	}
	if (check_path_no_repeating_lines(path) == false)
	{
		return false;
	}
	if (check_path_no_repeating_stops(path) == false)
	{
		return false;
	}
return true;
}

bool Network::check_path_no_repeating_lines (Pass_path* path) // checks if the path does not include going on and off the same bus line at the same stop
{
	vector <vector <Busline*>> lines = path->get_alt_lines();	
	for (vector <vector <Busline*>>::iterator lines_iter1 = lines.begin(); lines_iter1 < (lines.end()-1); lines_iter1++)
	{
		for (vector <Busline*>::iterator leg_lines1 = (*lines_iter1).begin(); leg_lines1 < (*lines_iter1).end(); leg_lines1++)
		{
			vector <vector <Busline*>>::iterator lines_iter2 = lines_iter1 + 1;
			// checking only for the same line in consecutive legs
			// taking the same line with an intermediate line is allowed (1-2-1)
			for (vector <Busline*>::iterator leg_lines2 = (*lines_iter2).begin(); leg_lines2 < (*lines_iter2).end(); leg_lines2++)
			{
				if ((*leg_lines1)->get_id() == (*leg_lines2)->get_id())
				{
					return false;
				}
			}
		}
	}
return true;
}

bool Network::check_path_no_repeating_stops (Pass_path* path) // chceks if the path deos not include going through the same stop more than once
{
	vector <vector <Busstop*>> stops = path->get_alt_transfer__stops();	
	for (vector <vector <Busstop*>>::iterator stops_iter1 = stops.begin(); stops_iter1 < stops.end(); stops_iter1++)
	{	
		for (vector <Busstop*>::iterator leg_stops1 = (*stops_iter1).begin(); leg_stops1 < (*stops_iter1).end()-1; leg_stops1++)
		{
			Busstop* check_stop = (*leg_stops1);
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

bool Network::read_bustype (istream& in) // reads a bustype
{

//{ type_id	length	number_seats	capacity }
  char bracket;
  int type_id, number_seats, capacity;
  double length;
  bool ok= true;
  vector <Bustype*> types;
  in >> bracket;
  if (bracket != '{')
  {
  	cout << "readfile::readsbusstop scanner jammed at " << bracket;
  	return false;
  }
  in >> type_id  >> length >> number_seats >> capacity;
  Bustype* bt= new Bustype (type_id, length, number_seats, capacity);
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
  Bus* bus=recycler.newBus(); // get a bus vehicle
  bus->set_bus_id(bv_id);
  bus->set_bustype_attributes(bty);
  in >> bracket;
  if (bracket != '{')
  {
  		cout << "readfile::readsbusvehicle scanner jammed at " << bracket;
  		return false;
  }
  for (int i=0; i < nr_trips; i++) // for each trip on the chain
  {
	  in >> trip_id;
	  Bustrip* btr=(*(find_if(bustrips.begin(), bustrips.end(), compare <Bustrip> (trip_id) ))); // find the trip in the list
	  btr->set_busv(bus);
	  if (i>0) // flag as busy for all trip except the first on the chain
	  {
		  btr->get_busv()->set_on_trip(true);
	  }
	  btr->set_bustype(bty); 
	  Start_trip* st = new Start_trip (btr, btr->get_starttime());
	  driving_roster.push_back(st); // save the driving roster at the vehicle level
  }
  for (vector<Start_trip*>::iterator trip = driving_roster.begin(); trip < driving_roster.end(); trip++)
  {
	(*trip)->first->add_trips(driving_roster); // save the driving roster at the trip level for each trip on the chain
  }
  busvehicles.push_back (bus);
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
	assert(out);
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
	assert(out);
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

bool Network::write_busstop_output(string name1, string name2, string name3, string name4)
{
	ofstream out1(name1.c_str());
	ofstream out2(name2.c_str());
	ofstream out3(name3.c_str());
	ofstream out4(name4.c_str());
	assert(out1);
	assert(out2);
	assert(out3);
	assert(out4);
	// writing the crude data and summary outputs for each bus stop
	for (vector<Busstop*>::iterator iter = busstops.begin();iter != busstops.end();iter++)
	{	
		(*iter)->write_output(out1);
		vector<Busline*> stop_lines = (*iter)->get_lines();
		for (vector <Busline*>::iterator lines = stop_lines.begin(); lines != stop_lines.end(); lines++)
		{
			(*iter)->calculate_sum_output_stop_per_line((*lines)->get_id());
			(*iter)->get_output_summary((*lines)->get_id()).write(out2,(*iter)->get_id(),(*lines)->get_id());
			// this summary file contains for each stop a record for each line thats use it
		}
	}
	out1.close();
	out2.close();
	// writing the aggregate summary output for each bus line
	for (vector<Busline*>::iterator iter = buslines.begin();iter!= buslines.end(); iter++)
	{	
		(*iter)->calculate_sum_output_line();
		(*iter)->get_output_summary().write(out3,(*iter)->get_id());
	}
	out3.close();
	for (vector<ODstops*>::iterator od_iter = odstops.begin(); od_iter < odstops.end(); od_iter++)
	{
		(*od_iter)->write_output(out4);
	}
	out4.close();
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
	assert(out);
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
	assert (inputfile);
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


bool Network::writeall(unsigned int repl)
{
	replication=repl;
	string rep="";
	string cleantimes;
	end_of_simulation(runtime);
	string linktimesfile = filenames[10];
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
	this->write_busstop_output(workingdir + "buslog_out.dat", workingdir + "busstop_sum.dat", workingdir + "busline_sum.dat", workingdir + "passengerlog.dat");
	return true;
}

bool Network::writeallmoes(string name)
{
	//cout << "going to write to this file: " << name << endl;
	ofstream out(name.c_str());
	assert(out);

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
	assert(out);
	
	for (map<int,Link*>::iterator iter=linkmap.begin();iter!=linkmap.end();iter++)
	{

		(*iter).second->write_speeds(out,nrperiods);
	}
	out.close();
	name=filenames[14] + ending; // inflows
	out.open(name.c_str());
	assert(out);

	for (map<int,Link*>::iterator iter1=linkmap.begin();iter1!=linkmap.end();iter1++)
	{
		(*iter1).second->write_inflows(out,nrperiods);
	}
	out.close();
	name=filenames[15] + ending; // outflows
	out.open(name.c_str());
	assert(out);

	for (map<int,Link*>::iterator iter2=linkmap.begin();iter2!=linkmap.end();iter2++)
	{
		(*iter2).second->write_outflows(out,nrperiods);
	}
	out.close();
	name=filenames[16] + ending; // queues
	out.open(name.c_str());
	assert(out);

	for (map<int,Link*>::iterator iter3=linkmap.begin();iter3!=linkmap.end();iter3++)
	{
		(*iter3).second->write_queues(out,nrperiods);
	}
	out.close();
	name=filenames[17] + ending; // densities
	out.open(name.c_str());
	assert(out);

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
	assert(out);
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
	assert(out);
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
	for (vector<ODstops*>::iterator iter_odstops = odstops.begin(); iter_odstops < odstops.end(); iter_odstops++ )
	{
		(*iter_odstops)->execute(eventlist,initvalue); // adds an event for the generation time for the first passenger per OD in terms of stops
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