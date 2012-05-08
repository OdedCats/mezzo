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
	int nr_trans = 0; 
	if (alt_lines.empty() == true)
	{
		return -1;
	}
	for (vector<vector<Busline*>>::iterator iter_count = alt_lines.begin(); iter_count < alt_lines.end(); iter_count++)
	{
		nr_trans++;
	}	
	return nr_trans-1; // omitting origin and destination stops
}

double Pass_path::calc_total_scheduled_in_vehicle_time_CSGM ()
{
	IVT.clear();
	double sum_in_vehicle_time = 0.0;
	vector<vector <Busstop*>>::iterator iter_alt_transfer_stops = alt_transfer_stops.begin();
	iter_alt_transfer_stops++; // starting from the second stop
	for (vector<vector <Busline*>>::iterator iter_alt_lines = alt_lines.begin(); iter_alt_lines < alt_lines.end(); iter_alt_lines++)
	{
		IVT.push_back((*iter_alt_lines).front()->calc_curr_line_ivt_CSGM((*iter_alt_transfer_stops).front(),(*(iter_alt_transfer_stops+1)).front(),alt_transfer_stops.front().front()->get_rti()));
		sum_in_vehicle_time += IVT.back();
		iter_alt_transfer_stops++;
		iter_alt_transfer_stops++; 
	}
	return (sum_in_vehicle_time/60); // minutes
}

double Pass_path::calc_total_scheduled_in_vehicle_time (Passenger* pass)
{
	IVT.clear();
	double sum_in_vehicle_time = 0.0;
	vector<vector <Busstop*>>::iterator iter_alt_transfer_stops = alt_transfer_stops.begin();
	iter_alt_transfer_stops++; // starting from the second stop
	for (vector<vector <Busline*>>::iterator iter_alt_lines = alt_lines.begin(); iter_alt_lines < alt_lines.end(); iter_alt_lines++)
	{
		IVT.push_back((*iter_alt_lines).front()->calc_curr_line_ivt(pass,(*iter_alt_transfer_stops).front(),(*(iter_alt_transfer_stops+1)).front(),alt_transfer_stops.front().front()->get_rti()));
		sum_in_vehicle_time += IVT.back();
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
	return (sum_walking_distance); // meters
}

double Pass_path::calc_total_waiting_time (Passenger* pass, double time, bool without_first_waiting, bool alighting_decision, double avg_walking_speed, bool has_network_rti)
{
	double sum_waiting_time = 0.0;
	bool first_line = true;
	vector <vector <Busstop*>>::iterator alt_transfer_stops_iter = alt_transfer_stops.begin() + 1;
	vector<Busstop*> first_stops = alt_transfer_stops.front();
	vector<Busstop*> second_stops = (*alt_transfer_stops_iter);
	vector<vector <Busline*>>::iterator iter_alt_lines = alt_lines.begin();
	if (without_first_waiting == true) // if it is calculated for an arriving vehicle, don't include waiting time for the first leg in the calculations
	{
		alt_transfer_stops_iter++;
		alt_transfer_stops_iter++;
		iter_alt_lines++;
		first_line = false;
	}
	bool first_entrance = true;
	if (alighting_decision == true) //  besides in case the calculation is for an alighting decision
	{
		first_line = false;
	}
	vector<double>::iterator iter_IVT = IVT.begin();
	vector<double>::iterator iter_walk = walking_distances.begin();
	double pass_arrival_time_at_next_stop;
	double sum_IVT = 0.0;
	double sum_walking_times = 0.0;
	for (; iter_alt_lines < alt_lines.end(); iter_alt_lines++)
	{	
		if (first_entrance == false) // in all cases beside the first entrance
		{
			first_line = false;
			alt_transfer_stops_iter++;
			alt_transfer_stops_iter++;
			sum_IVT += (*iter_IVT);
			iter_IVT++;
			iter_walk++;
		}
		sum_walking_times += (((*iter_walk) / avg_walking_speed) * 60); // in seconds
		pass_arrival_time_at_next_stop = time + (sum_waiting_time*60) + sum_walking_times + sum_IVT;
		first_entrance = false;
		int level_of_rti = first_stops.front()->get_rti();
		if (alighting_decision == true)
		{
			level_of_rti = 0;
		}
		if (has_network_rti == 1)
		{
			level_of_rti = 3;
		}
		switch (level_of_rti) 
		{
			case 0:
				// all legs are calculated based on headway or time-table
				sum_waiting_time += (calc_curr_leg_headway(pass,(*iter_alt_lines), alt_transfer_stops_iter, pass_arrival_time_at_next_stop, has_network_rti) / 2);
				break;
			case 1:
				// first leg is calculated based on real-time if it is an alternative of staying at the same stop (stop1==stop2),
				//otherwise (involves connection) - based on headway or time-table, while downstream legs are estimated based on headway or time-table	
				if (first_line == true)
				{
					if (second_stops.size() == 1 && first_stops.front() == second_stops.front()) // staying at the same stop
					{
						sum_waiting_time += calc_curr_leg_waiting_RTI(pass, (*iter_alt_lines), alt_transfer_stops_iter, pass_arrival_time_at_next_stop, has_network_rti);
					}
					else // using a connected stop
					{
						sum_waiting_time += (calc_curr_leg_headway(pass, (*iter_alt_lines), alt_transfer_stops_iter, pass_arrival_time_at_next_stop, has_network_rti) / 2);
					}
				}
				else
				{
					sum_waiting_time += (calc_curr_leg_headway(pass, (*iter_alt_lines), alt_transfer_stops_iter, pass_arrival_time_at_next_stop, has_network_rti) / 2);
					break;
				}
			case 2:
				// first leg is calculated based on real-time, while other legs are estimated based on headway or time-table
				if (first_line == true)
				{
					sum_waiting_time += calc_curr_leg_waiting_RTI(pass, (*iter_alt_lines), alt_transfer_stops_iter, pass_arrival_time_at_next_stop, has_network_rti);
					break; 
				}
				else
				{
					sum_waiting_time += (calc_curr_leg_headway(pass, (*iter_alt_lines), alt_transfer_stops_iter, pass_arrival_time_at_next_stop, has_network_rti) / 2);
					break;
				}
			case 3:
				// all legs are estimated based on real-time info
				sum_waiting_time += calc_curr_leg_waiting_RTI(pass, (*iter_alt_lines), alt_transfer_stops_iter, pass_arrival_time_at_next_stop, has_network_rti);
				break;
		}
	}
	return sum_waiting_time; // minutes
}

/*
double Pass_path::calc_total_scheduled_waiting_time (double time, bool without_first_waiting)
{
	double sum_waiting_time = 0.0;
	vector <vector <Busstop*>>::iterator alt_transfer_stops_iter = alt_transfer_stops.begin() + 1;
	vector<vector <Busline*>>::iterator iter_alt_lines = alt_lines.begin();
	if (without_first_waiting == true) // if it is calculated for an arriving vehicle, don't include waiting time for the first leg in the calculations
	{
		alt_transfer_stops_iter++;
		alt_transfer_stops_iter++;
		iter_alt_lines++;
	}
	for (; iter_alt_lines < alt_lines.end(); iter_alt_lines++)
	{
		// for each leg the relevant time is the time when the passenger is expected to arrive at the stop and start waiting
		sum_waiting_time += calc_curr_leg_waiting_schedule((*iter_alt_lines), alt_transfer_stops_iter, time);
		alt_transfer_stops_iter++;
		alt_transfer_stops_iter++;
		Bustrip* next_trip = (*iter_alt_lines).front()->find_next_scheduled_trip_at_stop((*alt_transfer_stops_iter).front(), time);
		// update the time for calculating the waiting time according to the expected arrival time at the next stop
		time = next_trip->stops_map[(*alt_transfer_stops_iter).front()]; 
	}
	return sum_waiting_time; // minutes
}
*/

double Pass_path::calc_curr_leg_headway (Passenger* pass, vector<Busline*> leg_lines, vector <vector <Busstop*>>::iterator stop_iter, double time, bool has_network_rti)
{
	double accumulated_frequency = 0.0;
	// map<Busline*, bool> worth_to_wait = check_maybe_worthwhile_to_wait(pass, leg_lines, stop_iter, 1, has_network_rti);
	for (vector<Busline*>::iterator iter_leg_lines = leg_lines.begin(); iter_leg_lines < leg_lines.end(); iter_leg_lines++)
	{
		//double time_till_next_trip = (*iter_leg_lines)->find_time_till_next_scheduled_trip_at_stop((*stop_iter).front(), time);
		//if (time_till_next_trip < theParameters->max_waiting_time && worth_to_wait[(*iter_leg_lines)] == true) 
			// dynamic filtering rules - consider only if it is available in a pre-defined time frame and it is maybe worthwhile to wait for it
		if(pass->any_waiting_time((*stop_iter).front(),(*iter_leg_lines))==false)	
		{
			accumulated_frequency += 3600.0 / 2*(pass->strategy_waiting_time((*stop_iter).front(),(*iter_leg_lines)));
		}
		else
		{
			accumulated_frequency += 3600.0 / ((*iter_leg_lines)->calc_curr_line_headway ());
		}
	}
	if (accumulated_frequency == 0.0) // in case no line is avaliable
	{
		return 0.0;
	}
	return (60/accumulated_frequency); // minutes
}

/*
double Pass_path::calc_curr_leg_waiting_schedule (vector<Busline*> leg_lines, vector <vector <Busstop*>>::iterator stop_iter, double arriving_time)
{
	Bustrip* next_trip = leg_lines.front()->find_next_scheduled_trip_at_stop((*stop_iter).front(), arriving_time);
	double min_waiting_time = next_trip->stops_map[(*stop_iter).front()];
	map<Busline*, bool> worth_to_wait = check_maybe_worthwhile_to_wait(leg_lines, stop_iter, 1);
	for (vector<Busline*>::iterator iter_leg_lines = leg_lines.begin()+1; iter_leg_lines < leg_lines.end(); iter_leg_lines++)
	{
		if (worth_to_wait[(*iter_leg_lines)] == true) 
		// dynamic filtering rules - consider only if it is maybe worthwhile to wait for it
		{
			next_trip = (*iter_leg_lines)->find_next_scheduled_trip_at_stop((*stop_iter).front(), arriving_time);	
			min_waiting_time = min(min_waiting_time, next_trip->stops_map[(*stop_iter).front()] - arriving_time);
		}
	}
	return (min_waiting_time/60); // minutes
}
*/

double Pass_path::calc_curr_leg_waiting_RTI (Passenger* pass, vector<Busline*> leg_lines, vector <vector <Busstop*>>::iterator stop_iter, double arriving_time, bool has_network_rti)
{ 
	double min_waiting_time;
	bool first_time = true;
	//map<Busline*, bool> worth_to_wait = check_maybe_worthwhile_to_wait(pass, leg_lines, stop_iter, 1, has_network_rti);
	for (vector<Busline*>::iterator iter_leg_lines = leg_lines.begin(); iter_leg_lines < leg_lines.end(); iter_leg_lines++)
	{
		//if (worth_to_wait[(*iter_leg_lines)] == true) 
		// dynamic filtering rules - consider only if it is maybe worthwhile to wait for it
		//{
			if (first_time == true)
			{
				first_time = false;
				min_waiting_time = (*iter_leg_lines)->time_till_next_arrival_at_stop_after_time((*stop_iter).front(),arriving_time);
			}
			else
			{
				min_waiting_time = min(min_waiting_time, (*iter_leg_lines)->time_till_next_arrival_at_stop_after_time((*stop_iter).front(),arriving_time));
			}
		//}
	}
	return (min_waiting_time/60); // minutes
}

double Pass_path::calc_arriving_utility (Passenger* pass, double time, bool has_network_rti)
// taking into account: transfer penalty + future waiting times + in-vehicle time + walking times
{ 
	double avg_walking_speed = random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4);
	return (random->nrandom(theParameters->transfer_coefficient, theParameters->transfer_coefficient / 4) * number_of_transfers + random->nrandom(theParameters->in_vehicle_time_coefficient, theParameters->in_vehicle_time_coefficient / 4 ) * calc_total_scheduled_in_vehicle_time(pass) + random->nrandom(theParameters->waiting_time_coefficient, theParameters->waiting_time_coefficient / 4) * calc_total_waiting_time(pass, time, true, false, avg_walking_speed, has_network_rti) + random->nrandom(theParameters->walking_time_coefficient, theParameters->walking_time_coefficient/4) * (calc_total_walking_distance() / avg_walking_speed));
}

double Pass_path::calc_waiting_utility (ODstops* odstops, Passenger* pass, vector <vector <Busstop*>>::iterator stop_iter, double time, bool alighting_decision, bool has_network_rti, bool without_first_waiting)
{	
	stop_iter++;
	if (alt_transfer_stops.size() == 2) // in case it is a walking-only path
	{
		return (random->nrandom(theParameters->walking_time_coefficient, theParameters->walking_time_coefficient/4) * calc_total_walking_distance()/ random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4));
	}
	vector<vector <Busline*>>::iterator iter_alt_lines = alt_lines.begin();
	for (vector <Busline*>::iterator iter_lines = (*iter_alt_lines).begin(); iter_lines < (*iter_alt_lines).end(); iter_lines++)
	{
		vector<Start_trip>::iterator next_trip_iter = (*iter_lines)->find_next_expected_trip_at_stop((*stop_iter).front());
		if ((*next_trip_iter).first == NULL) // in case there is no next trip planned
		{
			return -10.0;
		}
		if ((*next_trip_iter).first->stops_map[(*stop_iter).front()] - time < theParameters->max_waiting_time)
		{
		// a dynamic filtering rule - if there is at least one line in the first leg which is available - then this waiting alternative is relevant
			double avg_walking_speed = random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4);
			double total_scheduled_in_vehicle_time = calc_total_scheduled_in_vehicle_time(pass);
			double total_waiting_time = calc_total_waiting_time (pass, time, without_first_waiting, alighting_decision, avg_walking_speed, has_network_rti);
			return (random->nrandom(theParameters->transfer_coefficient, theParameters->transfer_coefficient / 4) * number_of_transfers + random->nrandom(theParameters->in_vehicle_time_coefficient, theParameters->in_vehicle_time_coefficient / 4 ) * total_scheduled_in_vehicle_time + random->nrandom(theParameters->waiting_time_coefficient, theParameters->waiting_time_coefficient / 4) * total_waiting_time + random->nrandom(theParameters->walking_time_coefficient, theParameters->walking_time_coefficient/4) * calc_total_walking_distance()/ avg_walking_speed);
		}
	} 
	// if none of the lines in the first leg is available - then the waiting alternative is irrelevant
	return -10.0;
}

map<Busline*, bool> Pass_path::check_maybe_worthwhile_to_wait_CSGM (vector<Busline*> leg_lines, vector <vector <Busstop*>>::iterator stop_iter, bool dynamic_indicator, bool has_network_rti)
{
	// based on the complete headway
	map <Busline*,bool> worth_to_wait;
	int level_of_rti = (*stop_iter).front()->get_rti();
	if (has_network_rti == 1)
	{
		level_of_rti = 3;
	}
	for (vector<Busline*>::iterator iter_leg_lines = leg_lines.begin(); iter_leg_lines < leg_lines.end(); iter_leg_lines++)
	{
		worth_to_wait[(*iter_leg_lines)] = true;
	}
	if (leg_lines.size() > 1)
		// only if there is what to compare
	{
		if (dynamic_indicator == 0) // in case it is a static filtering rule
		{
			stop_iter++; // the second stops set on the path	
		}
		for (vector<Busline*>::iterator iter_leg_lines = leg_lines.begin(); iter_leg_lines < leg_lines.end()-1; iter_leg_lines++)
		{
			for (vector<Busline*>::iterator iter1_leg_lines = iter_leg_lines+1; iter1_leg_lines < leg_lines.end(); iter1_leg_lines++)
			{
				if ((*iter_leg_lines)->calc_curr_line_ivt_CSGM((*stop_iter).front(),(*(stop_iter+1)).front(), level_of_rti) + (*iter_leg_lines)->calc_max_headway() < (*iter1_leg_lines)->calc_curr_line_ivt_CSGM((*stop_iter).front(),(*(stop_iter+1)).front(), level_of_rti))		
				{
					// if IVT(1) + Max H (1) < IVT (2) then line 2 is not worthwhile to wait for
					worth_to_wait[(*iter1_leg_lines)] = false;
				}
				// in case it is a dynamic filtering rule
				else 
				{
					if ((*iter_leg_lines)->calc_curr_line_ivt_CSGM((*stop_iter).front(),(*(stop_iter+1)).front(), level_of_rti) + (*iter_leg_lines)->calc_curr_line_headway() < (*iter1_leg_lines)->calc_curr_line_ivt_CSGM((*stop_iter).front(),(*(stop_iter+1)).front(),level_of_rti))
					{
						// if IVT(1) + H (1) < IVT (2) then line 2 is not worthwhile to wait for
						worth_to_wait[(*iter1_leg_lines)] = false;
					}
				}
			}
		}
	}
	return worth_to_wait;
}

map<Busline*, bool> Pass_path::check_maybe_worthwhile_to_wait (Passenger* pass, vector<Busline*> leg_lines, vector <vector <Busstop*>>::iterator stop_iter, bool dynamic_indicator, bool has_network_rti)
{
	// based on the complete headway
	map <Busline*,bool> worth_to_wait;
	int level_of_rti = (*stop_iter).front()->get_rti();
	if (has_network_rti == 1)
	{
		level_of_rti = 3;
	}
	for (vector<Busline*>::iterator iter_leg_lines = leg_lines.begin(); iter_leg_lines < leg_lines.end(); iter_leg_lines++)
	{
		worth_to_wait[(*iter_leg_lines)] = true;
	}
	if (leg_lines.size() > 1)
		// only if there is what to compare
	{
		if (dynamic_indicator == 0) // in case it is a static filtering rule
		{
			stop_iter++; // the second stops set on the path	
		}
		for (vector<Busline*>::iterator iter_leg_lines = leg_lines.begin(); iter_leg_lines < leg_lines.end()-1; iter_leg_lines++)
		{
			for (vector<Busline*>::iterator iter1_leg_lines = iter_leg_lines+1; iter1_leg_lines < leg_lines.end(); iter1_leg_lines++)
			{
				if ((*iter_leg_lines)->calc_curr_line_ivt(pass, (*stop_iter).front(),(*(stop_iter+1)).front(), level_of_rti) + (*iter_leg_lines)->calc_max_headway() < (*iter1_leg_lines)->calc_curr_line_ivt(pass, (*stop_iter).front(),(*(stop_iter+1)).front(), level_of_rti))		
				{
					// if IVT(1) + Max H (1) < IVT (2) then line 2 is not worthwhile to wait for
					worth_to_wait[(*iter1_leg_lines)] = false;
				}
				// in case it is a dynamic filtering rule
				else 
				{
					if ((*iter_leg_lines)->calc_curr_line_ivt(pass, (*stop_iter).front(),(*(stop_iter+1)).front(), level_of_rti) + (*iter_leg_lines)->calc_curr_line_headway() < (*iter1_leg_lines)->calc_curr_line_ivt(pass, (*stop_iter).front(),(*(stop_iter+1)).front(),level_of_rti))
					{
						// if IVT(1) + H (1) < IVT (2) then line 2 is not worthwhile to wait for
						worth_to_wait[(*iter1_leg_lines)] = false;
					}
				}
			}
		}
	}
	return worth_to_wait;
}