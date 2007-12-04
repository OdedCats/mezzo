#ifndef _BUSLINE
#define _BUSLINE

#include "parameters.h" 
#include "route.h"
#include "Random.h"
#include <algorithm>
#include "vehicle.h"
#include <fstream>
#include <string>

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

	// GETS & SETS
	int get_id () {return id;} // returns id, used in the compare <..> functions for find and find_if algorithms
	Busroute* get_busroute() {return busroute;} 
	Vtype* get_vtype() {return vtype;}
	ODpair* get_odpair() {return odpair;}

	bool execute(Eventlist* eventlist, double time); // re-implemented from virtual function in Action
													 // this function does the real work. It initiates 
													//	the current Bustrip and books the next one
	void add_stops (vector <Busstop*>  st) {stops = st;}
	void add_timepoints (vector <Busstop*> tp) {line_timepoint = tp;}
	void add_trip(Bustrip* trip, double starttime){trips.push_back(Start_trip(trip,starttime));}
	bool is_line_timepoint (Busstop* stop); // returns true if stops is a time point for this busline, otherwise it returns false

protected:
	int id; // line ID
	string name; // name of the busline "46 Sofia"
//	int vtype; // vehicle type. There are usually multiple types of Busses
	vector <Busstop*>  stops; // contains all the stops on this line
	vector <Busstop*> line_timepoint;
	vector <Start_trip> trips; // the trips that are to be made
	Busroute* busroute; // the route (in terms of links) that the busses follow
	Vtype* vtype; // the type of vehicle for the buses to be generated.
	ODpair* odpair; 
	bool active; // is true when the busline has started generating trips
	vector <Start_trip>::iterator curr_trip; // indicates the next trip
	
};

typedef pair<Busstop*,double> Visit_stop;

class Bustrip 
{
public:
	Bustrip ();
	~Bustrip ();
	Bustrip (int id_, double start_time_);

	// GETS & SETS
	int get_id () {return id;} // returns id, used in the compare <..> functions for find and find_if algorithms
	Bus* get_busv () {return busv;}
	void set_busv (Bus* busv_) {busv = busv_;}
	Bus* get_previous_bus () {return previous_busv;}
	void set_previous_bus (Bus* previous_busv_) {previous_busv = previous_busv_;}
	void set_line (Busline* line_) {line = line_;}
	Busline* get_line () {return line;}
	double get_starttime () {return starttime;}
	vector <Visit_stop*> :: iterator& get_next_stop() {return next_stop;} // returns pointer to next stop
//	bool is_trip_timepoint(Busstop* stop); // returns 1 if true, 0 if false, -1 if busstop not found
	bool advance_next_stop (double time, Eventlist* eventlist_); // advances the pointer to the next stop (checking bounds)
	void add_stops (vector <Visit_stop*>  st) {stops = st; next_stop = stops.begin();}
	bool activate (double time, Route* route, Vtype* vehtype, ODpair* odpair, Eventlist* eventlist_); // activates the trip. Generates the bus and inserts in net.
	double scheduled_arrival_time (Busstop* stop); // finds the scheduled arrival time for a given bus stop
	void book_stop_visit (double time, Bus* bus); // books a visit to the stop
	bool check_end_trip (); // returns 1 if true, 0 if false
	vector <Visit_stop*> stops; // contains all the busstops and the times that they are supposed to be served.
								// NOTE: this can be a subset of the total nr of stops in the Busline (according to the schedule input file)
	int get_avaliable_bus () {return avaliable_bus;}
	void set_avaliable_bus (bool avaliable_bus_) {avaliable_bus=avaliable_bus_;}

protected:
	
	int id; // course nr
	Bus* busv; // pointer to the bus vehicle
	Bus* previous_busv;
	Busline* line; // pointer to the line it serves
	int init_occupancy; // initial occupancy, usually 0
	double starttime; // when the trip is starting from the origin
	vector <Visit_stop*> :: iterator next_stop;
	bool avaliable_bus; // is true if the assigned vehicle is avaliable (done with its previous trip) and false if not 

//	map <Busstop*,bool> trips_timepoint; // will be relevant only when time points are trip-specific. binary map with time point indicatons for stops on route only (according to the schedule input file)  
	Eventlist* eventlist; // for use by busstops etc to book themselves.
};

class Busstop : public Action
{
public:
	Busstop ();
	~Busstop ();
	Busstop (int id_, int link_id_, double position_, double length_, bool has_bay_, double dwelltime_);
// GETS & SETS:
	int get_id () {return id;} // returns id, used in the compare <..> functions for find and find_if algorithms
	int get_link_id() {return link_id;}
	double get_arrival_rates (Bustrip* trip) {return arrival_rates[trip->get_line()];}
	double get_alighting_fractions (Bustrip* trip) {return alighting_fractions[trip->get_line()];}
	double get_length () {return length;}
	double get_avaliable_length () {return avaliable_length;}
	void set_avaliable_length (double avaliable_length_) {avaliable_length = avaliable_length_;}
	bool get_bay () {return has_bay;}
	int get_nr_boarding () {return nr_boarding;}
	void set_nr_boarding (int nr_boarding_) {nr_boarding = nr_boarding_;}
	void set_nr_alighting (int nr_alighting_) {nr_alighting = nr_alighting_;}	
	int get_nr_alighting () {return nr_alighting;}
	int get_nr_waiting (Bustrip* trip) {return nr_waiting[trip->get_line()];}
	const double get_position () { return position;}
	void set_position (double position_ ) {position = position_;}
	
// functions aimed to init. lines-specific vectors at the busstop level
	void add_lines (Busline*  line) {lines.push_back(line);}
	void add_line_nr_waiting(Busline* line, int value){nr_waiting[line] = value;}
	void add_line_nr_boarding(Busline* line, double value){arrival_rates[line] = value;}
	void add_line_nr_alighting(Busline* line, double value){alighting_fractions[line]= value;}

//	
	double calc_dwelltime (Bustrip* trip, double time); // calculates the dwelltime of each bus serving this stop
							// currently includes: standees, out of stop, bay/lane,  vehicle refernce, poisson headways, unique boarding and alighting rates									
	double calc_exiting_time (Bustrip* trip, double time); // To be implemented when time-points will work
	bool check_out_of_stop (Bus* bus); // returns TRUE if there is NO avaliable space for the bus at the stop (meaning the bus is out of the stop)
	void occupy_length (Bus* bus); // update avaliable length when bus arrives
	void free_length (Bus* bus); // update avaliable length when bus leaves
	void update_last_departures (Bustrip* trip, double time); //everytime a bus EXITS it updates the last_arrivlas vector (only AFTER we claculated the dwell time)
	double get_headway (Bustrip* trip, double time); // calculates the headway (defined as the differnece in time between sequential departures) 

	void write_busstop_visit (string name, Bustrip* trip, double time); // creates a log-file for stop-related info

// Action for visits to stop
	void book_bus_arrival(Eventlist* eventlist, double time, Bus* bus);  // add to expected arrivals
	bool execute(Eventlist* eventlist, double time); // is executed by the eventlist and means a bus needs to be processed
	

protected:
	int id; // stop id
	string name; //name of the bus stop "Ziv plaza"
	int link_id; // link it is on, maybe later a pointer to the respective link if needed
	bool has_bay; // TRUE if it has a bay, so that vehicles on same lane can pass.
	double length; // length of the busstop, determines how many buses can be served at the same time
	double position; // relative position from the upstream node of the link (beteen 0 to 1)
	double avaliable_length; // length of the busstop minus occupied length
	double exit_time;
	double dwelltime; // standard dwell time
	int nr_boarding;// pass. boarding
	int nr_alighting; // pass alighting
	Random* random;
	
	vector <Busline*> lines;
	map <Busline*, int> nr_waiting; // number of waiting passengers for each of the bus lines that stops at the stop
	map <Busline*, double> arrival_rates; // parameter lambda that defines the poission proccess of passengers arriving at the stop
	map <Busline*, double> alighting_fractions; // parameter that defines the poission process of the alighting passengers (second contains the alighting fraction)
	map <Busline*, double> last_departures; // contains the arrival time of the last bus from each line that stops at the stop (can result headways)
	map <double,Bus*> expected_arrivals; // booked arrivals of buses on the link on their way to the stop
	map <double,Bus*> buses_at_stop; // buses currently visiting stop
	// Maybe in the future, these three vectors could be integrated into a single matrix ( a map with busline as the key)
};

#endif //_BUSLINE