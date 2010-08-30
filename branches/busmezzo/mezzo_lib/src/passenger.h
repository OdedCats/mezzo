#ifndef _PASSENGER
#define _PASSENGER

#include "parameters.h"
#include "busline.h"
#include "od_stops.h"
#include "Random.h"

class Bustrip;
class Busline;
class Busstop;
class ODstops;
class ODzone;

class Passenger : public Action
{
public:
	Passenger ();
	~Passenger ();
	void init (int pass_id, double start_time_, ODstops* OD_stop_);
	void init_zone (int pass_id, double start_time_, ODzone* origin_, ODzone* destination_);
	void reset();
	
	// Gets and sets:
	int get_id () {return passenger_id;}
	double get_start_time () {return start_time;}
	void set_end_time (double end_time_) {end_time = end_time_;}
	double get_end_time () {return end_time;}
	ODstops* get_OD_stop () {return OD_stop;}
	ODzone* get_o_zone () {return o_zone;}
	ODzone* get_d_zone () {return d_zone;}
	void set_origin_walking_distances (map<Busstop*,double> origin_walking_distances_) {origin_walking_distances = origin_walking_distances_;}
	void set_destination_walking_distances (map<Busstop*,double> destination_walking_distances_) {destination_walking_distances = destination_walking_distances_;}
	double get_origin_walking_distance (Busstop* stop) {return origin_walking_distances[stop];}
	double get_destination_walking_distance (Busstop* stop) {return destination_walking_distances[stop];}
	Busstop* get_original_origin () {return original_origin;}
	int get_nr_boardings () {return nr_boardings;}
	vector <Busstop*> get_chosen_path_stops () {return selected_path_stops;}
	void set_ODstop (ODstops* ODstop_) {OD_stop = ODstop_;}
	void add_to_selected_path_stop (Busstop* stop) {selected_path_stops.push_back(stop);}
	// bool get_already_walked () {return already_walked;}
	// void set_already_walked (bool already_walked_) {already_walked = already_walked_;}

	bool execute(Eventlist* eventlist, double time); // called every time passengers choose to walk to another stop (origin/transfer), puts the passenger at the waiting list at the right timing

	// Passenger decision processes
	bool make_boarding_decision (Bustrip* arriving_bus, double time); // boarding decision making 
	Busstop* make_alighting_decision (Bustrip* boarding_bus, double time); // alighting decision making 
	Busstop* make_connection_decision (double time); // connection link decision (walking between stops)
	Busstop* make_first_stop_decision (double time); // deciding at which stop to initiate the trip
	

	// Demand in terms of zones
	map<Busstop*,double> sample_walking_distances (ODzone* zone);
	double calc_boarding_probability (Busline* arriving_bus, Busstop* o_stop, double time);

	// output-related 
	void write_selected_path(ostream& out);

protected:
	int passenger_id;
	double start_time;
	double end_time;
	Busstop* original_origin;
	ODstops* OD_stop;
	bool boarding_decision;
	Random* random;
	bool already_walked;
	int nr_boardings; // counts the number of times pass boarded a vehicle
	vector <Busstop*> selected_path_stops;

	// relevant only for OD in terms od zones
	ODzone* o_zone;
	ODzone* d_zone;
	map<Busstop*,double> origin_walking_distances;
	map<Busstop*,double> destination_walking_distances;
};

class PassengerRecycler
{
 public:
 	~PassengerRecycler();
	Passenger* newPassenger() {	 	if (pass_recycled.empty())
     								return new Passenger();
     								else
     								{
     									Passenger* pass=pass_recycled.front();
     									pass_recycled.pop_front();
     									return pass;
     								}	
     							}
							
     void addPassenger(Passenger* pass){pass_recycled.push_back(pass);}
 
private:
	list <Passenger*> pass_recycled;
};
//static PassengerRecycler recycler;
extern PassengerRecycler pass_recycler;

#endif //_Passenger