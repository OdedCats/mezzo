
#ifndef OD_HH
#define OD_HH
#include "eventlist.h"
#include "node.h"
#include "route.h"
#include "server.h"
#include "Random.h"
#include "grid.h"
#include "vtypes.h"
#include <string>
#include <list>
#include <iostream>

//#define _DEBUG_OD

class Route;
class Origin;
class Destination;
using namespace std;


typedef pair <int,int> odval;
typedef pair <Route*,double> rateval; // pair of linkid and rate

class ODpair;


class ODaction: public Action
// ODactions generate the vehicles with the routes using OD servers.
//

{
	public:
		ODaction(ODpair* odpair_);
		virtual ~ODaction();
		bool execute(Eventlist* eventlist, double time);
		void set_rate(double rate){server->set_rate((3600/rate),theParameters->odserver_sigma);active=true;}
		void book_later(Eventlist* eventlist, double time);
	private:
       ODpair* odpair;
       ODServer* server;
	   bool active; // indicates if an odpair is active
};

class ODpair

{
  public:
	  ODpair();
	 ODpair(Origin* origin_, Destination* destination_, int rate_, Vtypes* vtypes_);
    ~ODpair();
  	 bool execute(Eventlist* eventlist, double time);
    void add_route(Route* route);
	long get_nr_routes() {return routes.size();}
    Route* select_route(double time); // pretrip route choice
    Route* get_route(int id);
    odval  odids();	
    Origin* get_origin();
	Destination* get_destination();
    const int get_rate();
	vector <rateval> get_route_rates();
	void report (list <double>   collector);
	bool write (ostream& out) {return grid->write_empty(out);}
	bool writesummary(ostream& out);
	bool writefieldnames(ostream& out);
	Vtypes* vehtypes() {return vtypes;}
	void set_rate(double rate_) {rate=static_cast<int>(rate_); odaction->set_rate(rate);}
	bool less_than(ODpair* od); 
	vector <int> delete_spurious_routes(double time=0.0); // deletes spurious routes (with unrealistic costs) and returns ids of routes deleted
	Random* get_random(){return random;} 
  private:
   int id;  // for later use
	ODaction* odaction;
	Origin* origin;
	Destination* destination;
	int rate;
  //double rate;
	vector <Route*> routes;
	vector <double> utilities;
	Random* random;
	Grid* grid;
	Vtypes* vtypes;
	double totalU;    // total utility  = sum of all utilities
	double subU;    //subsum of utilities
	vector <Route*>::iterator shortest_route; // stores the shortest_known_route
};


#endif
