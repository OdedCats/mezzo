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
#define _DEBUG_OD_ROUTES

class Route;
class Origin;
class Destination;
class Network;


typedef pair <int,int> ODVal;
typedef pair <Route*,double> rateval; // pair of linkid and rate

class ODpair;


class ODaction: public Action
	// ODactions generate the vehicles with the routes using OD servers.
	// There is an OD action for each Vehicle class
	//

{
public:
	//ODaction(ODpair* odpair_); // old constructor without vclasses
	ODaction::ODaction(ODpair* odpair_, Vclass* vclass_=NULL);
	virtual ~ODaction();
	void reset (double rate_);
	const unsigned int get_total_nr_veh() const {return total_nr_veh;}  
	const bool execute(Eventlist* eventlist, const double time);
	void set_rate(double rate){server->set_rate((3600/rate),theParameters->odserver_sigma);}
	void set_active(const bool val) {active=val;}
	const bool get_active() {return active;}
	const bool move_event(Eventlist* eventlist,  double new_time);
	void book_later(Eventlist* eventlist, double time);
	const double get_booked_time() const {return booked_time;}
	const double get_last_gen_time() const {return last_gen_time;}

	
private:
	ODpair* odpair;
	ODServer* server;
	bool active; // indicates if an odpair is active
	unsigned int total_nr_veh;
	double booked_time; // currently booked time in the eventlist
	double last_gen_time; // last time a vehicle was generated
	Vclass* vclass; // the vehicle class for this ODaction
};

class ODpair

{
public:
	ODpair();
	ODpair(Origin* origin_, Destination* destination_, const double & rate_, Vtypes* vtypes_); // OLD
	ODpair(Origin* origin_, Destination* destination_);
	void init_vclass(Vclass* vclass_); // creates the ODaction for this Vclass
	~ODpair();
	void reset();
// GETS
	ODVal  odids();
	const long get_nr_routes() {return routes.size();}
	Route* get_route(int id);
	Origin* get_origin();
	Destination* get_destination();
	const double get_rate();
	const double get_rate(const int vclass);
	vector <rateval> get_route_rates(); // UNUSED
	vector<Route*>& get_allroutes(){return routes;} // TODO: routes per class
	Vtypes* vehtypes() {return vtypes;} // TODO: still used here?
	Random* get_random(){return random;}  // TODO: still used?
	Grid* get_grid() {return grid;} // TODO add the vclass to the grid
	Grid* get_oldgrid() {return grid;}
	double get_diff_odtimes(){ return (grid->sum(6) - oldgrid->sum(6));}
	double get_mean_odtimes() {return (grid->sum(6))/(grid->size());}
	double get_mean_old_odtimes() {if (oldgrid->size() ==0)
										return 0.0;
									else
										return (oldgrid->sum(6))/(oldgrid->size());}
	double get_nr_arrived() {return grid->size();}
	
//SETS
	void add_route(Route* route); // TODO: per vclass
	void set_rate(double newrate_, double time) ; // TODO: replace with class based one
	void set_rate(const int vclass_, const double newrate_,const double time_); // TODO: implement
	
//OTHER	
	bool execute(Eventlist* eventlist, double time); // TODO: make vclass based
	Route* select_route(double time); // pretrip route choice  TODO: move to recycler?
	void report (list <double>   collector); // TODO: make vclass specific
	bool write (ostream& out) {return grid->write_empty(out);}
	bool writesummary(ostream& out);
	bool writefieldnames(ostream& out);
	bool less_than(ODpair* od); 
	vector <Route*> delete_spurious_routes(double time=0.0); // deletes spurious routes (with unrealistic costs) and returns ids of routes deleted
	Route* filteredRoute(int index);
	void delete_routes(){routes.clear();} // removes all routes, used before adding the new route set. DOES NOT DELETE THE Route* pointed to!
private:
	int id;  // for later use
	ODaction* odaction;
	map <int, ODaction*> odactions;
	Origin* origin;
	Destination* destination;
	double rate;
	double start_rate; // original OD rate, to be used when OD pair is reset. // OLD
	map <int,double> start_rates;// rates per vehicle class
	map <int,double> rates; // rates per vehicle class
	//double rate;
	vector <Route*> routes;
	vector <Route*> filtered_routes_;
	vector <double> utilities;
	Random* random;
	Grid* grid;
	Grid* oldgrid;
	Vtypes* vtypes;
	double totalU;    // total utility  = sum of all utilities
	double subU;    //subsum of utilities
	vector <Route*>::iterator shortest_route; // stores the shortest_known_route
	Eventlist* eventlist_; // to keep track of the eventlist pointer locally
};

struct ODRate
{
public:
	ODRate() {}
	ODRate(const ODVal& odid_, ODpair* const  odpair_,const double rate_): odid(odid_),odpair(odpair_),rate(rate_) {} 
	ODVal odid;
	ODpair* odpair;
	double rate;
};


struct ODSlice
{
public:
	const bool remove_rate(const ODVal& odid); //!< removes od_rate for given od_id.
	vector <ODRate> rates;	
};


class  ODMatrix
{
public:
	ODMatrix ();
	ODMatrix (const int vclass_, const map <int,double> & loadtimes_,const double scale_, Network* network_);
	~ODMatrix ();
	bool read_from_stream(istream& in,const int& nr_odpairs,bool create_odpairs,Vclass* vclass_);
	void ODMatrix::create_events(Eventlist* eventlist);
	void add_slice(const double time, ODSlice* slice);
	void reset(Eventlist* eventlist,vector <ODpair*> * odpairs); //!< rebooks all the MatrixActions
	const vector <double> old_get_loadtimes(); //!< TODO: replace !  OLD:returns the loadtimes for each OD slice, slice 0 at t=0 has index [0], etc.
	map <int, double> get_loadtimes() {return loadtimes;}
	const bool remove_rate(const ODVal& odid); //!< removes od_rates for given od_id for all slices
private:
	int vclass;
	double scale;
	map <int, double> loadtimes;
	vector < pair <double,ODSlice*> > slices;	
	Network* network;
};

class MatrixAction: public Action
{
public:
	MatrixAction(Eventlist* eventlist, double time, ODSlice* slice_, vector<ODpair*> *ods_); //OLD
	MatrixAction(Eventlist* eventlist, double time, ODSlice* slice_,int vclass_id_);
	const bool execute(Eventlist* eventlist, const double time);
private:
	ODSlice* slice;
	vector <ODpair*> * ods;
	int vclass_id;
};

#endif
