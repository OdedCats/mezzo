#ifndef _BUSLINE
#define _BUSLINE


#include "parameters.h" 
#include "route.h"
#include "Random.h"
#include <algorithm>
#include "vehicle.h"
#include <fstream>
#include <string>
#include <vector>
#include "link.h"
#include "sdfunc.h"
#include "q.h"
#include "passenger.h"
#include "MMath.h" 
#include "od_stops.h"


class Busroute;
class Busstop;
class Bustrip;
class ODpair;
class Bus;
class Bustype;
class Passenger;
class ODstops;

typedef pair<Bustrip*,double> Start_trip;
typedef vector <Passenger*> passengers;

class Busline: public Action
	
	
{
public:
	
	
	Busline (); //!< simple constructor
	Busline (int id_, string name_, Busroute* busroute_, vector <Busstop*> stops_, Vtype* vtype_, ODpair* odpair_, int average_headway_, float ratio_headway_holding_, int holding_strategy_); //!< Initialising constructor
	virtual ~Busline(); //!< destructor

	// Gets and sets
	int get_id () {return id;} //!< returns id, used in the compare  functions for find and find_if algorithms
	Busroute* get_busroute() {return busroute;} //!< returns Busroute
	Vtype* get_vtype() {return vtype;} //!< returns Vtype
	ODpair* get_odpair() {return odpair;} //!< returns ODpair
	vector <Start_trip>::iterator get_curr_trip() {return curr_trip;} 
	int get_average_headway() {return average_headway;} //!< returns avg headway to previous bus
	float get_ratio_headway_holding() {return ratio_headway_holding;} //!< returns ratio_headway_holding
	int get_holding_strategy() {return holding_strategy;} //!< returns the holding strategy
	void set_curr_trip(vector <Start_trip>::iterator curr_trip_) {curr_trip = curr_trip_;}

	// initialization
	void add_timepoints (vector <Busstop*> tp) {line_timepoint = tp;}
	void add_trip(Bustrip* trip, double starttime){trips.push_back(Start_trip(trip,starttime));}
	
	// checks
	bool is_line_timepoint (Busstop* stop); //!< returns true if stops is a time point for this busline, otherwise it returns false
	bool check_first_stop (Busstop* stop); // returns true if the stop is the first stop on the bus line, otherwise it returns false 
	bool check_first_trip (Bustrip* trip); // returns true if the trip is the first trip on the bus line, otherwise it returns false  
	
	bool execute(Eventlist* eventlist, double time); //!< re-implemented from virtual function in Action this function does the real work. It initiates the current Bustrip and books the next one
	
	// calc attributes (for pass_paths)
	double calc_curr_line_headway ();
	double calc_curr_line_ivt (Busstop* start_stop, Busstop* end_stop);
	
	
	vector <Busstop*>  stops; //!< contains all the stops on this line

protected:
	int id; //!< line ID
	string name; //!< name of the busline "46 Sofia"
//	int vtype; //!< vehicle type. There are usually multiple types of Busses

	vector <Busstop*> line_timepoint;
	vector <Start_trip> trips; //!< the trips that are to be made
	Busroute* busroute; //!< the route (in terms of links) that the busses follow
	Vtype* vtype; //!< the type of vehicle for the buses to be generated.
	ODpair* odpair; 
	int average_headway;
	float ratio_headway_holding;
	int holding_strategy; 
	bool active; //!< is true when the busline has started generating trips
	vector <Start_trip>::iterator curr_trip; //!< indicates the next trip
};

typedef pair<Busstop*,double> Visit_stop;

class Bustrip 
{
public:
	Bustrip ();
	~Bustrip ();
	Bustrip (int id_, double start_time_);

// GETS & SETS
	int get_id () {return id;} //!< returns id, used in the compare <..> functions for find and find_if algorithms
	Bus* get_busv () {return busv;}
	void set_busv (Bus* busv_) {busv = busv_;}
	void set_bustype (Bustype* btype_) {btype = btype_;}
	Bustype* get_bustype () {return btype;}
	void set_line (Busline* line_) {line = line_;}
	Busline* get_line () {return line;}
	double get_starttime () {return starttime;}
	int get_init_occupancy () {return init_occupancy;}
	vector <Visit_stop*> :: iterator& get_next_stop() {return next_stop;} //!< returns pointer to next stop
	void set_pass (Passenger* passenger) {pass = passenger;}
	Passenger* get_pass () {return pass;}
	void set_enter_time (double enter_time_) {enter_time = enter_time_;}
	double get_enter_time () {return enter_time;}

// other functions:	
//	bool is_trip_timepoint(Busstop* stop); //!< returns 1 if true, 0 if false, -1 if busstop not found
	bool activate (double time, Route* route, ODpair* odpair, Eventlist* eventlist_); //!< activates the trip. Generates the bus and inserts in net.
	bool advance_next_stop (double time, Eventlist* eventlist_); //!< advances the pointer to the next stop (checking bounds)
	void add_stops (vector <Visit_stop*>  st) {stops = st; next_stop = stops.begin();}
	void add_trips (vector <Start_trip*>  st) {driving_roster = st;} 
	double scheduled_arrival_time (Busstop* stop); //!< finds the scheduled arrival time for a given bus stop
	void book_stop_visit (double time, Bus* bus); //!< books a visit to the stop
	bool check_end_trip (); //!< returns 1 if true, 0 if false
	double calc_departure_time (double time); //!< calculates departure time from origin according to arrival time and schedule (including layover effect)
	

// public vectors
	vector <Visit_stop*> stops; //!< contains all the busstops and the times that they are supposed to be served. NOTE: this can be a subset of the total nr of stops in the Busline (according to the schedule input file)
	vector <Start_trip*> driving_roster; //!< trips assignment for each bus vehicle.
	map <Busstop*, passengers> passengers_on_board; // passenger on-board storaged by their alighting stop (format 3)
	map <Busstop*, int> nr_expected_alighting; //!< number of passengers expected to alight at the busline's stops (format 2)
	
protected:
	int id; //!< course nr
	Bus* busv; //!< pointer to the bus vehicle
	Bustype* btype;
	Busline* line; //!< pointer to the line it serves
	int init_occupancy; //!< initial occupancy, usually 0
	double starttime; //!< when the trip is schedule to departure from the origin
	vector <Visit_stop*> :: iterator next_stop; 
	Random* random;
	double enter_time; // the time it entered the most recently bus stop
//	map <Busstop*,bool> trips_timepoint; //!< will be relevant only when time points are trip-specific. binary map with time point indicatons for stops on route only (according to the schedule input file)  
	Eventlist* eventlist; //!< for use by busstops etc to book themselves.
	Passenger* pass;
};

typedef pair<Busstop*, double> stop_rate;
typedef map <Busstop*, double> stops_rate;
typedef pair <Busline*, stops_rate> multi_rate;
typedef map <Busline*, stops_rate> multi_rates;
typedef map <Busstop*, ODstops*> ODs_for_stop;

class Busstop_Visit // container object holding output data for stop visits
{
public:
	Busstop_Visit (int line_id_,	int trip_id_,	int vehicle_id_,	 int stop_id_, double entering_time_,	double sched_arr_time_,	double dwell_time_,	double lateness_,	
							double exit_time_,double time_since_arr_, double time_since_dep_,int nr_alighting_,	int nr_boarding_,	int occupancy_,	int nr_waiting_,	double holding_time_):
							line_id(line_id_),trip_id(trip_id_),vehicle_id(vehicle_id_), stop_id(stop_id_),entering_time(entering_time_),sched_arr_time(sched_arr_time_),dwell_time(dwell_time_),
							lateness(lateness_), exit_time (exit_time_),time_since_arr(time_since_arr_),time_since_dep(time_since_dep_),nr_alighting(nr_alighting_),nr_boarding(nr_boarding_),occupancy(occupancy_),nr_waiting(nr_waiting_),
							holding_time(holding_time_) {}
	void write (ostream& out) { out << line_id << '\t'<< trip_id << '\t'<< vehicle_id << '\t'<< stop_id<< '\t'<<entering_time << '\t'<< sched_arr_time << '\t'
		<< dwell_time << '\t'<< lateness << '\t'<< exit_time <<'\t'<< time_since_arr << '\t'<< time_since_dep << '\t'<< nr_alighting << '\t'<< nr_boarding 
		<< '\t'<< occupancy << '\t'<< nr_waiting << '\t'<< holding_time << '\t'	<< endl; }
	int line_id;
	int trip_id;
	int vehicle_id;
	int stop_id;
	double entering_time;
	double sched_arr_time;
	double dwell_time;
	double lateness;
	double exit_time;
	double time_since_arr;
	double time_since_dep;
	int nr_alighting;
	int nr_boarding;
	int occupancy;
	int nr_waiting;
	double holding_time;
};


class Busstop : public Action
{
public:
	Busstop ();
	~Busstop ();
	Busstop (int id_, int link_id_, double position_, double length_, bool has_bay_, double dwelltime_);

// GETS & SETS:
	int get_id () {return id;} //!< returns id, used in the compare <..> functions for find and find_if algorithms
	int get_link_id() {return link_id;}
	double get_arrival_rates (Bustrip* trip) {return arrival_rates[trip->get_line()];}
	double get_alighting_fractions (Bustrip* trip) {return alighting_fractions[trip->get_line()];}
	ODs_for_stop get_stop_as_origin () {return stop_as_origin;}
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
	vector <Busline*> get_lines () {return lines;}
	
// functions aimed to initialize lines-specific vectors at the busstop level
	void add_lines (Busline*  line) {lines.push_back(line);}
	void add_line_nr_waiting(Busline* line, int value){nr_waiting[line] = value;}
	void add_line_nr_boarding(Busline* line, double value){arrival_rates[line] = value;}
	void add_line_nr_alighting(Busline* line, double value){alighting_fractions[line]= value;}
	void add_odstops_as_origin(Busstop* destination_stop, ODstops* od_stop){stop_as_origin[destination_stop]= od_stop; is_origin = true;}
	void add_odstops_as_destination(Busstop* origin_stop, ODstops* od_stop){stop_as_destination[origin_stop]= od_stop; is_destination = true;}

//	Action for visits to stop
	bool execute(Eventlist* eventlist, double time); //!< is executed by the eventlist and means a bus needs to be processed
	double passenger_activity_at_stop (Bustrip* trip, double time); //!< progress passengers at stop: waiting, boarding and alighting
	void book_bus_arrival(Eventlist* eventlist, double time, Bus* bus);  //!< add to expected arrivals
	double calc_exiting_time (Bustrip* trip, double time); //!< To be implemented when time-points will work
	
// dwell-time calculation related functions	
	double calc_dwelltime (Bustrip* trip); //!< calculates the dwelltime of each bus serving this stop. currently includes: passenger service times ,out of stop, bay/lane		
	bool check_out_of_stop (Bus* bus); //!< returns TRUE if there is NO avaliable space for the bus at the stop (meaning the bus is out of the stop)
	void occupy_length (Bus* bus); //!< update avaliable length when bus arrives
	void free_length (Bus* bus); //!< update avaliable length when bus leaves
	void update_last_arrivals (Bustrip* trip, double time); //!< everytime a bus ENTERS it updates the last_arrivals vector 
	void update_last_departures (Bustrip* trip, double time); //!< everytime a bus EXITS it updates the last_departures vector 
	double get_time_since_arrival (Bustrip* trip, double time); //!< calculates the headway (defined as the differnece in time between sequential arrivals) 
	double get_time_since_departure (Bustrip* trip, double time); //!< calculates the headway (defined as the differnece in time between sequential departures) 

	// output-related functions
	void write_output(ostream & out);
	void record_busstop_visit ( Bustrip* trip, double enter_time); //!< creates a log-file for stop-related info

	// relevant only for demand format 2
	multi_rates multi_arrival_rates; //!< parameter lambda that defines the poission proccess of passengers arriving at the stop for each sequential stop

protected:
	int id; //!< stop id
	string name; //!< name of the bus stop "Ziv plaza"
	int link_id; //!< link it is on, maybe later a pointer to the respective link if needed
	bool has_bay; //!< TRUE if it has a bay, so that vehicles on same lane can pass.
	double length; //!< length of the busstop, determines how many buses can be served at the same time
	double position; //!< relative position from the upstream node of the link (beteen 0 to 1)
	double avaliable_length; //!< length of the busstop minus occupied length
	double exit_time;
	double dwelltime; //!< standard dwell time
	int nr_boarding;//!< pass. boarding
	int nr_alighting; //!< pass alighting
	Random* random;
	
	vector <Busline*> lines;
	map <double,Bus*> expected_arrivals; //!< booked arrivals of buses on the link on their way to the stop
	map <double,Bus*> buses_at_stop; //!< buses currently visiting stop
	list <Busstop_Visit> output_stop_visits; //!< list of output data for buses visiting stops
	map <Busline*, double> last_arrivals; //!< contains the arrival time of the last bus from each line that stops at the stop (can result headways)
	map <Busline*, double> last_departures; //!< contains the departure time of the last bus from each line that stops at the stop (can result headways)

	// relevant only for demand format 1
	map <Busline*, double> arrival_rates; //!< parameter lambda that defines the poission proccess of passengers arriving at the stop
	map <Busline*, double> alighting_fractions; //!< parameter that defines the poission process of the alighting passengers 

	// relevant only for demand format 2
	multi_rates multi_nr_waiting; // for demant format is from type 2. 

	
	// relevant for demand formats 1 & 2
	map <Busline*, int> nr_waiting; //!< number of waiting passengers for each of the bus lines that stops at the stop

	// relevant only for demand format 3
	ODs_for_stop stop_as_origin; // a map of all the OD's that this busstop is their origin 
	ODs_for_stop stop_as_destination; // a map of all the OD's that this busstop is their destination
	bool is_origin; // indicates if this busstop serves as an origin for some passenger demand
	bool is_destination; // indicates if this busstop serves as an destination for some passenger demand
};

#endif //_BUSLINE