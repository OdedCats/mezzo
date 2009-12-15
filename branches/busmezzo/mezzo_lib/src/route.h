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
typedef pair <int,int> odval;

class Route
{
  public:
	Route(int id_, Origin* origin_, Destination* destination_, vector <Link*> links_);
	Route(int id, Route* route, vector<Link*> links_); // copy constructor that copies route and overwrites remaining part starting from links_.front()
	void reset();
	Link* nextlink(Link* currentlink);
	Link* firstlink() {	return (links.front());}
	const int get_id () {return id;}
	void set_id(int id_) {id=id_;}
	Origin* get_origin() {return origin;}
	Destination* get_destination() {return destination;}
	odval get_oid_did();
	void set_selected(bool selected); // sets the links' selected attribute
#ifndef _NO_GUI
	void set_selected_color(QColor selcolor);
#endif
	bool check (int oid, int did);
	bool less_than(Route* route);
	double cost(double time=0.0);
	bool equals (Route& route); // returns true if same route {return ( (route.get_links())==(get_links()) );}
	vector<Link*> get_links() {return links;}	
	vector<Link*> get_upstream_links(int link_id) ;// returns all links upstream of link_id
	vector<Link*> get_downstream_links(int link_id);  // returns all links downstream of link_id, including Link(link_id)
	bool has_link(int lid);
	bool has_link_after(int lid, int curr_lid);
	void write(ostream& out);
	double utility (double time);
	int computeRouteLength();
  protected:
	int id;
	Origin* origin;
	Destination* destination;
	vector <Link*> links; // ordered sequence of the links in the route
	map <int, Link*> linkmap; // in addition to the 'links' vector, to enable fast lookup
	double sumcost; // the cached route cost.
	double last_calc_time; // last time the route cost was updated
};


struct compare_route
{
 compare_route(odval odvalue_):odvalue(odvalue_) {}
 bool operator () (Route* route)
 	{
 	 return (route->check(odvalue.first, odvalue.second)==true);
 	}
 odval odvalue;
};

class Busroute: public Route
{
public:
	Busroute(int id_, Origin* origin_, Destination* destination_, vector <Link*> links_) :
		Route (id_, origin_, destination_, links_) {}
};

#endif
