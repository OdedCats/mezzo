#ifndef _BUSLINE
#define _BUSLINE

#include "parameters.h" 
#include "route.h"
#include "Random.h"
#include <algorithm>
#include "vehicle.h"

// ODED: what i am doing here is not correct yet. I made everything public and have no constructor, destructor etc.
//		 When the structure of the classes is final, I move all the variables to protected: and make constructors,
//		 destructors, and the get_ and set_ functions.

class Busroute;
class Busstop;
class Bustrip;
class ODpair;
class Bus;

typedef pair<Bustrip*,double> Start_trip;

class Busline: public Action
{
public:
	Busline (); // simple constructor
	Busline (int id_, string name_, Busroute* busroute_, Vtype* vtype_, ODpair* odpair_); // Initialising constructor
	virtual ~Busline(); // destructor
	bool execute(Eventlist* eventlist, double time); // re-implemented from virtual function in Action
													 // this function does the real work. It initiates 
													//	the current Bustrip and books the next one

	int get_id () {return id;} // returns id, used in the compare <..> functions for find and find_if algorithms
	void add_stops (vector <Busstop*>  st) {stops = st;}
	void add_trip(Bustrip* trip, double starttime){trips.push_back(Start_trip(trip,starttime));}
	// variables
	int id; // line ID
	string name; // name of the busline "46 Sofia"
//	int vtype; // vehicle type. There are usually multiple types of Busses
	vector <Busstop*>  stops; // contains all the stops on this line.
	vector <Start_trip> trips; // the trips that are to be made
	Busroute* busroute; // the route (in terms of links) that the busses follow
	Vtype* vtype; // the type of vehicle for the buses to be generated.
	ODpair* odpair; 

	bool active; // is true when the busline has started generating trips
	vector <Start_trip>::iterator next_trip; // indicates the next trip
};

typedef pair<Busstop*,double> Visit_stop;

class Bustrip
{
public:
	Bustrip ();
	~Bustrip ();
	Bustrip (int id_, double start_time_);
	int get_id () {return id;} // returns id, used in the compare <..> functions for find and find_if algorithms
	void add_stops (vector <Visit_stop*>  st)  {stops = st;}
	bool activate (double time, Route* route, Vtype* vehtype, ODpair* odpair); // activates the trip. Generates the bus and inserts in net.
// variables
	int id; // course nr
	Bus* busv; // pointer to the bus vehicle
	Busline* line; // pointer to the line it serves
	int init_occupancy; // initial occupancy, usually 0
	double starttime; // when the trip is starting from the origin
	vector <Visit_stop*> stops; // contains all the busstops and the times that they are supposed to be served.
								// NOTE: this can be a subset of the total nr of stops in the Busline
};

typedef pair<Busline*,double> Busline_arrival;

class Busstop
{
public:
	Busstop ();
	~Busstop ();
	int get_id () {return id;} // returns id, used in the compare <..> functions for find and find_if algorithms

	Busstop (int id_, int link_id_, double length_, bool has_bay_, double dwelltime_);
	double calc_dwelltime (Bustrip* trip, double time); // calculates the dwelltime of each bus serving this stop
							// currently includes: standees, out of stop, vehicle refernce
							// currently excludes: poisson headways, unique boarding and alighting rates									
	bool check_out_of_stop (Bustrip* trip); // returns TRUE if there is NO avaliable space for the bus at the stop (meaning the bus is out of the stop)
	void occupy_length (Bustrip* trip); // update avaliable length when bus arrives
	void free_length (Bustrip* trip); // update avaliable length when bus leaves
	void update_last_arrivals (Bustrip* trip, double time); //everytime a bus EXITS it updates the last_arrivlas vector (only AFTER we claculated the dwell time)
	double get_headway (Bustrip* trip, double time); // calculates the headway (current time minus the last ariival) 
	double get_arrival_rates (Bustrip* trip);
	double get_alighting_rates (Bustrip* trip);
	
	// variables	
	int id; // stop id
	string name; //name of the bus stop "Ziv plaza"
	int link_id; // link it is on, maybe later a pointer to the respective link if needed
	double length; // length of the busstop, determines how many buses can be served at the same time
	double position; // relative position from the upstream node of the link (beteen 0 to 1)
	double avaliable_length; // length of the busstop minus occupied length
	bool has_bay; // TRUE if it has a bay, so that vehicles on same lane can pass.
	int nr_waiting; // number of passengers waiting
	double dwelltime; // standard dwell time
	Random* random;
	vector <Busline_arrival> last_arrivals; // contains the arrival time of the last bus from each line that stops at the stop (can result headways)
	
	vector <Busline_arrival> arrival_rates; // parameter lambda that defines the poission proccess of passengers arriving at the stop
	vector <Busline_arrival> alighting_rates; // parameter that defines the poission process of the alighting passengers (second contains the alighting fraction)
	// Maybe in the future, these three vectors could be integrated into a single matrix ( a map with busline as the key)
};

inline bool operator==(Busline_arrival s1, Busline_arrival s2) { return s1.first == s2.first; }

#endif //_BUSLINE