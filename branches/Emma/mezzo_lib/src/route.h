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

#include "c:\develop\mezzo_Emma\getp\codegen\lib\getP\getp.h"
//#include "getp.h" 

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
	Route(const int id_, Origin* const origin_, Destination* const destination_,  vector <Link*> & links_ );//!< Standard constructor, requires Origin*, Destination* and Link* vector.
	Route(const int id, Route* const route, const vector<Link*> & links_); //!< copy constructor that copies route and overwrites remaining part starting from links_.front()
	void reset(); //!< resets all variables to initial state
	//EMMAROUTE
	virtual Link* const nextlink(Link* const currentlink) const; //!< returns next_link for a vehicle, given currentlink.
	virtual vector <Link*>::const_iterator nextlink_iter(Link* const currentlink) ; //!< returns const_iterator to the next link of the route, given currentlink.
	virtual void generate_nextlink(Link* const currentlink); //!< for the EmmaRoute

	Link* const firstlink() const {	return (links.front());} //!< returns the first link of the route.
	vector <Link*>::const_iterator firstlink_iter() {return links.begin();} //!< returns const_iterator to the first link of the route
	vector <Link*>::const_iterator lastlink_iter() {return --(links.end());} //!< returns const_iterator to the last link of the route
	
	const int get_id () const {return id;} //!< get route id
	void set_id(const int id_) {id=id_;} //!< set route id
	Origin* const get_origin() const {return origin;} //!< get origin
	Destination* const get_destination() const {return destination;} //!< get destination
	const ODVal get_oid_did() const; //!< get ids for origin and destination as ODVal pair.
	void set_selected(const bool selected); //!< sets the links' selected attribute (to show routes in GUI).
#ifndef _NO_GUI
	void set_selected_color(const QColor & selcolor); //!< sets the 'selected' colour of links (to show routes in GUI)
#endif
	const bool check (const int oid, const int did) const ; //!< Checks if this route connects the supplied origin (id) and destination (id).
	const bool less_than(const Route* const  route) const ; //!< returns true if origin_id of this route is less than origin_id of the "route" parameter, or if the origins are equal, if the destination_id is less than that of the "route" parameter provided
	const double cost(const double time=0.0) ; //!< returns the cost of this route, given the entry time. Calculates by summing the link costs, taking into account the time it takes to get to each subsequent link.
	const bool equals (const Route& route) const ; //!< returns true if same route, checks if the current route has the same links as the route supplied. 
	
	const vector<Link*> & get_links() const {return links;}	//!< returns the links of the route
	const vector<Link*> get_upstream_links(const int link_id) const ;//!< returns all links upstream of link_id. NOTE: no reference (&) as the vector needs to be copied unfortunately.
	const vector<Link*> get_downstream_links(const  int link_id) const ;  //!< returns all links downstream of link_id, including Link(link_id)
	const bool has_link(const int lid) const ; //!< returns true if the route contains Link (lid)
	const bool has_link_after(const int lid, const int curr_lid) const ; //!< returns true if the route contains Link (lid), after Link (curr_lid)
	void write(ostream& out) const; //!< writes route description to out.
	void write_routeflows (ostream& out) const; //!< writes the route flows for each time period to out.
	const double utility (const double time)  ; //!< calculates the routes' utility for supplied entry time.
	const int computeRouteLength() const ; //!< calculates the total route length, sum of link lengths.

	void register_veh_departure(const double time); //!< registers the departure of a vehicle on this route (for routeflows)
	const vector <int> get_routeflows() const { return routeflows;} //!< returns the routeflow vector.
	const int get_od_period(const double time) const; //! returns the od time period that belongs to supplied entry time.
	const int get_abs_diff_routeflows() const; //!< returns the sum of absolute differences between the previous and current route flows.
	const int get_sum_prev_routeflows() const; //!< returns the sum of the previous route flows.
	const int get_sum_routeflows()const ; //!< returns the sum of the current route flows.
  protected:
	int id;
	Origin* origin;
	Destination* destination;
	vector <Link*> links; //!< ordered sequence of the links in the route
	//map <int, Link*> linkmap; // in addition to the 'links' vector, to enable fast lookup. Taken out to reduce size of Route objects
	//map <int, Vehicle*> departures; // ttaken out to reduce size of Route objects
	vector <int> routeflows; //!<  for each OD time period the nr of departures.
	vector <int> prev_routeflows; //!<  routeflows previous iteration.
	double sumcost; //!< the cached route cost.
	double last_calc_time; //!< last time the sumcost was updated
};


struct compare_route //!< Checks if two routes are the same.
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
	Busroute(const int id_, Origin*const  origin_, Destination* const destination_,  vector <Link*> & links_) :
		Route (id_, origin_, destination_, links_) {}

};

class EmmaRoute: public Route
{
public:
	EmmaRoute(const int id_, Origin*const  origin_, Destination* const destination_, vector <Link*> & links_) ;
		

		//reimplement
		Link* const nextlink(Link* const currentlink) const; //!< returns next_link for a vehicle, given currentlink.
		vector <Link*>::const_iterator nextlink_iter(Link* const currentlink) ; //!< returns const_iterator to the next link of the route, given currentlink.

		void generate_nextlink(Link* const currentlink);
private:
	//Link* firstlink;

		/*************** OLD stuff *****************
	void get_probs () { 
		getP_initialize();
		//real_T probs[196];
		getP(probs);
	}

private:
	real_T probs[196];

	**********************************************/
};


#endif
