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

/*****************************************
* last modified by Xiaoliang Ma
*
* ! add access to all routes in ODpair
* 
* 2007-07-22
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

class Route;
class Origin;
class Destination;


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
	void reset (double rate_);
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
	void reset();
// GETS
	odval  odids();
	const long get_nr_routes() {return routes.size();}
	Route* get_route(int id);
	Origin* get_origin();
	Destination* get_destination();
	const int get_rate();
	vector <rateval> get_route_rates();
	vector<Route*>& get_allroutes(){return routes;}
	Vtypes* vehtypes() {return vtypes;}
	Random* get_random(){return random;} 
	Grid* get_grid() {return grid;}
	Grid* get_oldgrid() {return grid;}
	double get_diff_odtimes(){ return (grid->sum(6) - oldgrid->sum(6));}
	double get_mean_odtimes() {return (grid->sum(6))/(grid->size());}
	double get_mean_old_odtimes() {if (oldgrid->size() ==0)
										return 0.0;
									else
										return (oldgrid->sum(6))/(oldgrid->size());}
	double get_nr_arrived() {return grid->size();}
	
//SETS
	void add_route(Route* route);
	void set_rate(double rate_) {rate=static_cast<int>(rate_); odaction->set_rate(rate);}
	
//OTHER	
	bool execute(Eventlist* eventlist, double time);
	Route* select_route(double time); // pretrip route choice
	void report (list <double>   collector);
	bool write (ostream& out) {return grid->write_empty(out);}
	bool writesummary(ostream& out);
	bool writefieldnames(ostream& out);
	bool less_than(ODpair* od); 
	vector <Route*> delete_spurious_routes(double time=0.0); // deletes spurious routes (with unrealistic costs) and returns ids of routes deleted
	Route* filteredRoute(int index);
private:
	int id;  // for later use
	ODaction* odaction;
	Origin* origin;
	Destination* destination;
	int rate;
	int start_rate; // original OD rate, to be used when OD pair is reset.
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
};


#endif
