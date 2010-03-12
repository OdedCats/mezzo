#include "pass_route.h"

Pass_path:: Pass_path ()
{
}
Pass_path:: Pass_path (int path_id, vector<vector<Busline*>> alt_lines_)
{
	p_id = path_id;
	alt_lines = alt_lines_;
	number_of_transfers = find_number_of_transfers();
}
Pass_path:: Pass_path (int path_id, vector<vector<Busline*>> alt_lines_, vector <vector <Busstop*>> alt_transfer_stops_)
{
	p_id = path_id;
	alt_lines = alt_lines_;
	alt_transfer_stops = alt_transfer_stops_;
	number_of_transfers = find_number_of_transfers();
	random = new (Random);
	if (randseed != 0)
		{
		random->seed(randseed);
		}
	else
	{
		random->randomize();
	}
}

Pass_path::Pass_path (int path_id, vector<vector<Busline*>> alt_lines_, vector <vector <Busstop*>> alt_transfer_stops_, vector<double> walking_distances_)
{
	p_id = path_id;
	alt_lines = alt_lines_;
	alt_transfer_stops = alt_transfer_stops_;
	walking_distances = walking_distances_;
	number_of_transfers = find_number_of_transfers();
	random = new (Random);
	if (randseed != 0)
		{
		random->seed(randseed);
		}
	else
	{
		random->randomize();
	}
}

Pass_path::~Pass_path()
{
}

void Pass_path::reset()
{
	alt_lines.clear();
	alt_transfer_stops.clear();
	number_of_transfers = 0;
	scheduled_in_vehicle_time = 0;
	scheduled_headway = 0;
	arriving_bus_rellevant = 0;
}

int Pass_path::find_number_of_transfers ()
{
	int nr_trans = ((alt_transfer_stops.size()-2)/2)-1;
	if (nr_trans < 0)
	{
		nr_trans = 0; // in case it is only a walking alternative (no buslines included)
	}
	return (nr_trans); // omitting origin and destination stops
}

double Pass_path::calc_total_scheduled_in_vehicle_time (ODstops* odstops)
{
	double sum_in_vehicle_time = 0.0;
	vector<vector <Busstop*>>::iterator iter_alt_transfer_stops = alt_transfer_stops.begin();
	iter_alt_transfer_stops++; // starting from the second stop
	for (vector<vector <Busline*>>::iterator iter_alt_lines = alt_lines.begin(); iter_alt_lines < alt_lines.end(); iter_alt_lines++)
	{
		sum_in_vehicle_time += (*iter_alt_lines).front()->calc_curr_line_ivt((*iter_alt_transfer_stops).front(),(*(iter_alt_transfer_stops+1)).front());
		iter_alt_transfer_stops++;
		iter_alt_transfer_stops++; 
	}
	return (sum_in_vehicle_time/60); // minutes
}

double Pass_path::calc_total_walking_distance()
{
	double sum_walking_distance = 0.0;
	for (vector <double>::iterator iter_walking = walking_distances.begin(); iter_walking < walking_distances.end(); iter_walking++)
	{
		sum_walking_distance += (*iter_walking);
	}
	return (sum_walking_distance); // minutes
}

double Pass_path::calc_total_scheduled_headway ()
{
	double sum_headway_time = 0.0;
	for (vector<vector <Busline*>>::iterator iter_alt_lines = alt_lines.begin(); iter_alt_lines < alt_lines.end(); iter_alt_lines++)
	{
		sum_headway_time += calc_curr_leg_headway(*iter_alt_lines);
	}
	return sum_headway_time; // minutes
}

double Pass_path::calc_curr_leg_headway (vector<Busline*> leg_lines)
{
	double accumlated_frequency = 0.0;
	for (vector<Busline*>::iterator iter_leg_lines = leg_lines.begin(); iter_leg_lines < leg_lines.end(); iter_leg_lines++)
	{
		accumlated_frequency += 3600.0 / ((*iter_leg_lines)->calc_curr_line_headway ());
	}
	return (60/accumlated_frequency); // minutes
}
double Pass_path::calc_arriving_utility (ODstops* odstop)
// this function currently assumes direct paths (has to include waiting times at transfers)
{
	return (random->nrandom(theParameters->transfer_coefficient, theParameters->transfer_coefficient / 4) * number_of_transfers + random->nrandom(theParameters->in_vehicle_time_coefficient, theParameters->in_vehicle_time_coefficient / 4 ) * calc_total_scheduled_in_vehicle_time(odstop));
}

double Pass_path::calc_waiting_utility (ODstops* odstop)
{
	return (random->nrandom(theParameters->transfer_coefficient, theParameters->transfer_coefficient / 4) * number_of_transfers + random->nrandom(theParameters->in_vehicle_time_coefficient, theParameters->in_vehicle_time_coefficient / 4 ) * calc_total_scheduled_in_vehicle_time(odstop) + random->nrandom(theParameters->waiting_time_coefficient, theParameters->waiting_time_coefficient / 4) * calc_estimated_waiting_time());
}

double Pass_path::calc_estimated_waiting_time ()
{	
	return (calc_total_scheduled_headway()/2);
}