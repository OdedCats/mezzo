#ifndef PASSENGER_ROUTE
#define PASSENGER_ROUTE

#include "busline.h"
#include "parameters.h" 
#include "od_stops.h"
#include "Random.h"

class Busline;
class Busstop;
class ODstops;

class Pass_path
{
	public:
	Pass_path ();
	Pass_path (int path_id, vector<vector<Busline*>> alt_lines_);
	Pass_path (int path_id, vector<vector<Busline*>> alt_lines_, vector <vector <Busstop*>> alt_transfer_stops_);
	Pass_path (int path_id, vector<vector<Busline*>> alt_lines_, vector <vector <Busstop*>> alt_transfer_stops_, vector<double> walking_distances_);
	~Pass_path ();
	void reset();

	// Gets and sets:
	int get_id () {return p_id;}
	vector <vector <Busline*>> get_alt_lines () {return alt_lines;}
	vector <vector <Busstop*>> get_alt_transfer__stops () {return alt_transfer_stops;}
	vector <double> get_walking_distances () {return walking_distances;};
	int get_number_of_transfers () {return number_of_transfers;}
	void set_alt_transfer_stops (vector <vector <Busstop*>> trans_stops) {alt_transfer_stops = trans_stops;}
	bool get_arriving_bus_rellevant () {return arriving_bus_rellevant;}
	void set_arriving_bus_rellevant (bool arriving_bus_rellevant_) {arriving_bus_rellevant = arriving_bus_rellevant_;}

	// Attributes of path alternative
	int find_number_of_transfers ();
	double calc_total_scheduled_in_vehicle_time ();
	double calc_total_walking_distance ();
	double calc_total_waiting_time (double time, bool without_first_waiting, double avg_walking_speed);
//	double calc_total_scheduled_waiting_time (double time, bool without_first_waiting);
	double calc_curr_leg_headway (vector<Busline*> leg_lines, vector <vector <Busstop*>>::iterator stop_iter, double time);
//	double calc_curr_leg_waiting_schedule (vector<Busline*> leg_lines, vector <vector <Busstop*>>::iterator stop_iter, double arriving_time);
	double calc_curr_leg_waiting_RTI (vector<Busline*> leg_lines, vector <vector <Busstop*>>::iterator stop_iter, double arriving_time);

	double calc_arriving_utility (double time);
	double calc_waiting_utility (vector <vector <Busstop*>>::iterator stop_iter, double time);
	map<Busline*, bool> check_maybe_worthwhile_to_wait (vector<Busline*> leg_lines, vector <vector <Busstop*>>::iterator stop_iter, bool dynamic_indicator); // returns false for lines which are not worthwhile to wait for in any case

protected:
	int p_id;
	Random* random;
	vector <vector <Busline*>> alt_lines;
	vector <double> IVT;
	vector <vector <Busstop*>> alt_transfer_stops;
	vector <double> walking_distances;
	int number_of_transfers;
	double scheduled_in_vehicle_time;
	double scheduled_headway;
	bool arriving_bus_rellevant;
};

#endif