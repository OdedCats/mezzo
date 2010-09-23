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

#ifndef ROUTE_HH
#define ROUTE_HH

#include "link.h"
#include "node.h"
#include <vector>
#include "linktimes.h"

#ifndef _NO_GUI
	#include <QColor>
#endif

//#define _DEBUG_ROUTE
//#define _DISTANCE_BASED
class Link;
class Origin;
class Destination;
class Vehicle; 

typedef pair <int,int> ODVal;

class Route
{
  public:
	Route(const int id_, Origin* const origin_, Destination* const destination_,  const vector <Link*> & links_ );
	Route(const int id, Route* const route, const vector<Link*> & links_); // copy constructor that copies route and overwrites remaining part starting from links_.front()
	void reset(); // resets all variables to initial state
	Link* const nextlink(Link* const currentlink) const;
	Link* const firstlink() const {	return (links.front());}
	const int get_id () const {return id;}
	void set_id(const int id_) {id=id_;}
	Origin* const get_origin() const {return origin;}
	Destination* const get_destination() const {return destination;}
	const ODVal get_oid_did() const;
	void set_selected(const bool selected); // sets the links' selected attribute
#ifndef _NO_GUI
	void set_selected_color(const QColor & selcolor);
#endif
	const bool check (const int oid, const int did) const ;
	const bool less_than(const Route* const  route) const ;
	const double cost(const double time=0.0) ;
	const bool equals (const Route& route) const ; // returns true if same route 
	
	const vector<Link*> & get_links() const {return links;}	
	const vector<Link*> get_upstream_links(const int link_id) const ;// returns all links upstream of link_id NOTE: no reference (&) as the vector needs to be copied unfortunately.
	const vector<Link*> get_downstream_links(const  int link_id) const ;  // returns all links downstream of link_id, including Link(link_id)
	const bool has_link(const int lid) const ;
	const bool has_link_after(const int lid, const int curr_lid) const ;
	void write(ostream& out) const;
	void write_routeflows (ostream& out) const;
	const double utility (const double time)  ;
	const int computeRouteLength() const ;

	void register_veh_departure(const double time); 
	const vector <int> get_routeflows() const { return routeflows;}
	const int get_od_period(const double time) const;
	const int get_abs_diff_routeflows() const;
	const int get_sum_prev_routeflows() const;
	const int get_sum_routeflows()const ;
  protected:
	int id;
	Origin* origin;
	Destination* destination;
	vector <Link*> links; // ordered sequence of the links in the route
	map <int, Link*> linkmap; // in addition to the 'links' vector, to enable fast lookup
	//map <int, Vehicle*> departures;
	vector <int> routeflows; //  for each OD time period the nr of departures.
	vector <int> prev_routeflows; //  routeflows previous iteration.
	double sumcost; // the cached route cost.
	double last_calc_time; // last time the route cost was updated
};


struct compare_route
{
 compare_route(const ODVal & ODValue_):ODValue(ODValue_) {}
 const bool operator () (const Route* const route)
 	{
 	 return (route->check(ODValue.first, ODValue.second)==true);
 	}
private:
 ODVal ODValue;
};

class Busroute: public Route
{
public:
	Busroute(const int id_, Origin*const  origin_, Destination* const destination_, const vector <Link*> & links_) :
		Route (id_, origin_, destination_, links_) {}


};

#endif
