/**
 * modification:
 *   add bounding box to the node object
 *
 * Xiaoliang Ma 
 * last change: 2007-10-20 
 */

#ifndef NODE_HH
#define NODE_HH


#include "link.h"
#include <vector>
#include "turning.h"
#include "q.h"
#include "vehicle.h"
#include "od.h"
#include "eventlist.h"
#ifndef _NO_GUI
	#include "icons.h"
#endif //_NO_GUI
#ifdef _PVM
	#include "pvm.h"
#endif // _NO_PVM

#ifdef _VISSIMCOM
//#include "vissimcom.h"
#endif _VISSIMCOM


//#define _DEBUG_NODE

using namespace std;

class Turning;
class Link;
class VirtualLink;
class InputLink;
class Q;
class Vehicle;
class Signature;
class ODpair;
class Oaction;
class Daction;
class BOaction;
class Busroute;

#ifdef _PVM
class PVM;
#endif //_NO_PVM
typedef pair<Link*,double> linkrate;

class Coord
{
	public:
	Coord();
	Coord(double x_, double y_);
	double x;
	double y;		
};

class Node
{
  public:
   Node(int id_);
   virtual ~Node();
   virtual const int get_id();
   void setxy(double x, double y);
   const Coord getxy();
#ifndef _NO_GUI
   Icon* get_icon(){return icon;}
   void set_icon(NodeIcon* icon_);
   virtual bool inbound(double x, double y, double scale);
#endif // _NO_GUI   
  virtual bool  process_veh(Vehicle* veh, double time);
 protected:
  int id;
  Coord position;
#ifndef _NO_GUI  
  NodeIcon* icon;
#endif // _NO_GUI  
};

class Origin : public Node
{
	public:
	 Origin (int id_);
	 virtual ~Origin();
	virtual bool transfer_veh(Link* link, double time); // transfers the vehicle from InputQueue to one of the
																				//outgoing links
	virtual bool insert_veh(Vehicle* veh, double time); // inserts the vehicle into the InputQueue
	virtual void add_odpair(ODpair* odpair);
	virtual void register_links(vector<Link*> links); // registers the outgoing links
	virtual void initialise(); // initialises the OServers for each outgoing link based on rates from ODpairs
	virtual bool execute(Eventlist* eventlist, double time);
	virtual void get_link_rates(); // gets the link rates from the ODpairs
	virtual vector<Link*> get_links() {return outgoing;}
	
	// Broadcast functions are only there for the DYNAMO test cases
	virtual void broadcast_incident_start(int lid, vector <double> parameters);
	virtual void broadcast_incident_stop(int lid);

	void write_v_queues(ostream& out);


	protected:
	vector <OServer*> servers;
	InputLink* inputqueue;
	vector <Link*>  outgoing;	
	vector <ODpair*> odpairs;
	vector <Oaction*> oactions;
	vector<linkrate> link_rates;
	bool incident; // flag that indicates incident behaviour
	int incident_link;
	vector <double> incident_parameters;
	vector <int> v_queue_lengths; // stores the length of the virtual queue for each ass_od time period.
	int currentperiod;
};

class Destination : public Node
{
	public:
	Destination (int id_, Server* server_);
	 Destination (int id_);
	 virtual ~Destination();
	 virtual void register_links(vector<Link*> links);
	 virtual bool execute(Eventlist* eventlist, double time);
	protected:
		vector <Link*>  incoming;
		vector <Daction*> dactions;
		Server* server;		
};

class Junction : public Node
{
	public:	
	 Junction (int id_);
	 void register_links(vector <Link*> links) ;
   	 vector <Link*> get_incoming() {return incoming;}
	 vector <Link*> get_outgoing() {return outgoing;}
	private:
	vector <Turning*> turnings;
	vector <Link*>  incoming;
	vector <Link*>  outgoing;
};

class BoundaryOut : public Junction
{
 public:
 	BoundaryOut (int id_);
 	~BoundaryOut();
 	void block(int code,double speed); // spread the news to the virtual links (setting full to true/false)
 	void register_virtual(VirtualLink* vl) { vlinks.insert(vlinks.begin(),vl);}
	vector <VirtualLink*> & get_virtual_links() {return vlinks;}
#ifdef _MIME
	vector <Signature*> & get_signatures() ;
	void add_signature(Signature* sig);   
#endif //_MIME

#ifdef _PVM 
	int send_message (PVM* com);   // sends messages. returns nr of sigs that are sent
#endif //_PVM
  
#ifdef _VISSIMCOM
	// put here the equivalent of the above functions

#endif //_VISSIMCOM

	bool blocked() {return blocked_;}
 	bool process_veh(Vehicle* veh, double time);

 protected:
 	vector <VirtualLink*> vlinks;
#ifdef _MIME
 	vector <Signature*> signatures;
#endif //_MIME
	bool blocked_;
} ;

class BoundaryIn : public Origin
/* BoundaryIn is a special Origin, in the sense that vehicles that are generated here are generated from a stream (PVM or other)
	that is connected to a microscopic model
*/
{	
	public:
	BoundaryIn (int id_);
	~BoundaryIn ();
	void register_virtual(VirtualLink* vl) { vlinks.insert(vlinks.begin(),vl);}
	void register_routes(vector<Route*> * routes_){routes=routes_;}
	void register_busroutes (vector<Busroute*> * busroutes_) {busroutes=busroutes_;}
	void register_ods(vector<ODpair*> *ods_){ods=ods_;}
    vector <VirtualLink*> & get_virtual_links() {return vlinks;}
#ifdef _PVM	 // PVM specific functions
	bool receive_message(PVM* com);
	int send_message (PVM* com, double time);   // sends messages. returns nr of sigs that are sent 
#endif//_PVM
#ifdef _VISSIMCOM
	// do something similar as the PVM case
		double get_speed(double time); // sends the speed assigned to the last vehicle that entered
#endif //_VISSIMCOM


#ifdef _MIME // common functions for both the PVM implementation (Mitsim) and the COM implementation  (VISSIM)
	bool newvehicle(Signature* sig);
#endif // _MIME	


	private:
	vector <VirtualLink*> vlinks;
	vector <Route*> * routes;
	vector <Busroute*> * busroutes; 
	vector <ODpair*> * ods;
	Vehicle* lastveh;
};




class Oaction : public Action   // loads the link outward from the Origin with traffic from the origins inputqueue.
{
	public:
		Oaction(Link* link_, Origin* origin_, OServer* server_);
		virtual ~Oaction();
		bool execute(Eventlist* eventlist, double time);
		bool process_veh(double time);
  private:
    Link* link;
    Origin* origin;
    OServer* server;
};


class Daction : public Action
{
 public:
 	Daction(Link* link_, Destination* destination_, Server* server_);
 	virtual ~Daction();
 	virtual bool execute(Eventlist* eventlist, double time);
 	virtual bool process_veh(double time);	
 protected:
  Link* link;
  Destination* destination;
  Server* server;
} ;

/*
class BOaction : public Daction
{
 public:
 	BOaction (Link* link_, BoundaryOut* boundaryout_, Server* server_);
 	~BOaction();
 	bool process_veh(double time);
 protected:
 	BoundaryOut* boundaryout;
 };
 */
#endif
