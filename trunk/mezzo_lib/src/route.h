
#ifndef ROUTE_HH
#define ROUTE_HH
#include "link.h"
#include "node.h"
#include <vector>
#include "linktimes.h"

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
	bool has_link(int lid);
	bool has_link_after(int lid, int curr_lid);
	void write(ostream& out);
	double utility (double time);
  protected:
	int id;
	Origin* origin;
	Destination* destination;
	vector <Link*> links;
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
