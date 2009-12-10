#ifndef _ODSTOPS
#define _ODSTOPS

#include "parameters.h"
#include "od.h"
#include "busline.h"
#include "pass_route.h"
#include "passenger.h"
#include "Random.h"
#include "MMath.h" 

class Busline;
class Busstop;
class Bustrip;
class Passenger;
class Pass_path;

typedef vector <Passenger*> passengers;

class Pass_boarding_decision // container object holding output data for passenger boarding decision
{
public:
		Pass_boarding_decision (int pass_id_, int original_origin_, int destination_stop_, int line_id_, int trip_id_, int stop_id_, double time_,	double generation_time_,
							bool boarding_,double u_boarding_, double u_staying_):
							pass_id(pass_id_),original_origin(original_origin_),destination_stop(destination_stop_),line_id(line_id_),trip_id(trip_id_), stop_id(stop_id_),time(time_),generation_time(generation_time_),boarding(boarding_),
							u_boarding(u_boarding_),u_staying(u_staying_) {}
	void write (ostream& out) { out << pass_id << '\t' << original_origin << '\t' << destination_stop << '\t' << line_id << '\t'<< trip_id << '\t'<< stop_id<< '\t'<< time << '\t'<< generation_time << '\t'
		<< boarding << '\t'<< u_boarding << '\t'<< u_staying <<'\t'<< endl; }
	int pass_id;
	int original_origin;
	int destination_stop;
	int line_id;
	int trip_id;
	int stop_id;
	double time;
	double generation_time;
	bool boarding;
	double u_boarding;
	double u_staying;
};

class ODstops : public Action
{
public:
	ODstops ();
	ODstops (Busstop* origin_stop_, Busstop* destination_stop_, double arrival_rate_);
	~ODstops ();
	
	//Gets and Sets:
	Busstop* get_origin() {return origin_stop;}
	Busstop* get_destination () {return destination_stop;}
	vector <Pass_path*> get_path_set () {return path_set;}
	double get_arrivalrate () {return arrival_rate;}
	passengers get_waiting_passengers() {return waiting_passengers;}
	int get_min_transfers () {return min_transfers;}
	void set_waiting_passengers(passengers passengers_) {waiting_passengers = passengers_;}
	void set_origin (Busstop* origin_stop_) {origin_stop=origin_stop_;}
	void set_destination (Busstop* destination_stop_) {destination_stop=destination_stop_;}
	void set_min_transfers (int min_transfers_) {min_transfers = min_transfers_;}
	
	// Passengers processes
	void book_next_passenger (double curr_time);
	bool execute(Eventlist* eventlist, double time);

	// Path set - not operative yet
	void add_paths (Pass_path* pass_path_) {path_set.push_back(pass_path_);}
	double calc_boarding_probability (Busline* arriving_bus);
	double calc_binary_logit (double utility_i, double utility_j);

	
	// output-related functions
	void record_passenger_boarding_decision (Passenger* pass, Bustrip* trip, double time, bool boarding_decision); //!< creates a log-file for boarding decision related info
	void write_output(ostream & out);

protected:
	Busstop* origin_stop;
	Busstop* destination_stop;
	double arrival_rate; 
	passengers waiting_passengers; // a list of passengers with this OD that wait at the origin
	int min_transfers; // the minimum number of trnasfers possible for getting from O to D
	
	vector <Pass_path*> path_set;
	double boarding_utility;
	double staying_utility;

	// output structures
	list <Pass_boarding_decision> output_pass_decisions; //!< list of output data for buses visiting stops

	Random* random;
	Eventlist* eventlist; //!< to book passenger generation events
	bool active; // indicator for non-initialization call
};
#endif //_OD_stops