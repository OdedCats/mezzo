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
	double calc_total_scheduled_in_vehicle_time (ODstops* odstops);
	double calc_total_walking_distance ();
	double calc_total_scheduled_headway ();
	double calc_curr_leg_headway (vector<Busline*> leg_lines);
	double calc_estimated_waiting_time ();
	double calc_arriving_utility (ODstops* odstop);
	double calc_waiting_utility (ODstops* odstop);

protected:
	int p_id;
	Random* random;
	vector <vector <Busline*>> alt_lines;
	vector <vector <Busstop*>> alt_transfer_stops;
	vector <double> walking_distances;
	int number_of_transfers;
	double scheduled_in_vehicle_time;
	double scheduled_headway;
	bool arriving_bus_rellevant;
};

#endif