#include "pass_route.h"

Pass_path:: Pass_path ()
{
}
Pass_path:: Pass_path (vector<vector<Busline*>> alt_lines_)
{
	alt_lines = alt_lines_;
	number_of_transfers = find_number_of_transfers();
}
Pass_path:: Pass_path (vector<vector<Busline*>> alt_lines_, vector <vector <Busstop*>> alt_transfer_stops_)
{
	alt_lines = alt_lines_;
	alt_transfer_stops = alt_transfer_stops_;
	number_of_transfers = find_number_of_transfers();
}

Pass_path::~Pass_path()
{
}

int Pass_path::find_number_of_transfers ()
{
	return (alt_transfer_stops.size()-2); // omitting origin and destination stops
}

double Pass_path::calc_total_scheduled_in_vehicle_time (ODstops* odstops)
{
	double sum_in_vehicle_time = 0.0;
	vector<vector <Busstop*>>::iterator iter_alt_transfer_stops = alt_transfer_stops.begin();
	for (vector<vector <Busline*>>::iterator iter_alt_lines = alt_lines.begin(); iter_alt_lines < alt_lines.end(); iter_alt_lines++)
	{
		sum_in_vehicle_time += (*iter_alt_lines).front()->calc_curr_line_ivt((*iter_alt_transfer_stops).front(),(*(iter_alt_transfer_stops+1)).front());
		iter_alt_transfer_stops++;
	}
		/*
		vector <Start_trip>::iterator iter_curr_trip = (*iter_alt_lines).front()->get_curr_trip();
		Bustrip* line_first_trip = (*iter_alt_lines).front()->get_trips().front().first;
		if ((*iter_curr_trip).first != line_first_trip)
		{
			iter_curr_trip--;
		}
		
		vector <Visit_stop*>::iterator board_stop,alight_stop; 
		for (vector <Visit_stop*>::iterator stop = (*(iter_curr_trip)).first->stops.begin(); stop < (*(iter_curr_trip)).first->stops.end(); stop++)
		{
			if ((*stop)->first->get_id() == boarding_stop->get_id())
			{
				board_stop = stop;
			}
			if ((*stop)->first->get_id() == alighting_stop->get_id())
			{
				alight_stop = stop;
				break;
			}
		}
		sum_in_vehicle_time += (*alight_stop)->second - (*board_stop)->second;
		*/
	return sum_in_vehicle_time;
}

double Pass_path::calc_total_scheduled_headway ()
{
	double sum_headway_time = 0.0;
	for (vector<vector <Busline*>>::iterator iter_alt_lines = alt_lines.begin(); iter_alt_lines < alt_lines.end(); iter_alt_lines++)
	{
		sum_headway_time += calc_curr_leg_headway(*iter_alt_lines);
	}
	return sum_headway_time;
}

double Pass_path::calc_curr_leg_headway (vector<Busline*> leg_lines)
{
	double accumlated_frequency = 0.0;
	for (vector<Busline*>::iterator iter_leg_lines = leg_lines.begin(); iter_leg_lines < leg_lines.end(); iter_leg_lines++)
	{
		accumlated_frequency += 60.0 / ((*iter_leg_lines)->calc_curr_line_headway ());
	}
	return (60/accumlated_frequency);
}
double Pass_path::calc_arriving_utility (ODstops* odstop)
{
	return (theParameters->transfer_coefficient * number_of_transfers + theParameters->in_vehicle_time_coefficient * calc_total_scheduled_in_vehicle_time(odstop));
}

double Pass_path::calc_waiting_utility (ODstops* odstop)
{
	return (theParameters->transfer_coefficient * number_of_transfers + theParameters->in_vehicle_time_coefficient * calc_total_scheduled_in_vehicle_time(odstop) + theParameters->waiting_time_coefficient * calc_estimated_waiting_time());
}

double Pass_path::calc_estimated_waiting_time ()
{
	return (calc_total_scheduled_headway()/2);
}